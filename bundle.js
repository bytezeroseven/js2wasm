const fs = require('fs');
const { minify } = require('terser');
const exec = require('child_process').exec;
const path = require('path');

const Module = require('./out.js');

const input = process.argv[2];
const output = process.argv[3];

if (!input || !output) {
	const isNpx = process.argv[1].endsWith('bin.js');
	console.log(`Missing arguments.\nUsage: ${isNpx ? `npx js2wasm` : `node bundle.js`} [input_js_file] [output_js_file]`);
	process.exit(0);
}

const outputDir = path.dirname(output);
if (!fs.existsSync(outputDir)) {
	fs.mkdirSync(outputDir, {
		recursive: true
	});
}

Module().then(async wasm => {
	let code = fs.readFileSync(input, 'utf8');
	code = await minifyJs(code);

	const bytes = wasm.getBytecode(code);

	console.log(`bytecode length: ${bytes.length}`);

	writeCFile(bytes);
	writeJsFile();

	const cmd = getCmd();
	console.log(`running: ${cmd}`);

	exec(cmd, (error, stderr, stdout) => {
		deleteFile('temp-main.c');
		deleteFile('temp-post.js');

		if (error) throw error;
		if (stderr) throw new Error(`stderr: ${stderr}`);

		console.log(`stdout: ${stdout}`);

		afterBuild();
	});
});

async function afterBuild() {
	let code = fs.readFileSync(output, 'utf8');

	const mangled = {};
	let i = 0;

	code = code.replace(/Module\["_([^\"]+)"\]/g, (match, name) => {
		if (mangled[name] === undefined) {
			mangled[name] = i++;
		}
		return `Module["_${mangled[name]}"]`;
	}).replace(/cwrap\("([^\"]+)"/g, (match, name) => {
		return `cwrap(${mangled[name]}`;
	});

	code += `\nModule().then(wasm => wasm["_${mangled.run}"]());`

	code = await minifyJs(code);

	fs.writeFileSync(output, code);
}

async function minifyJs(code) {
	const result = await minify(code, {
		mangle: {
			toplevel: true, 
			properties: {
				regex: /\b(?:object|refCount|ref|ptr|ctx)\b/, 
				builtins: true
			}
		}
	});

	return result.code;
}

function writeCFile(bytes) {
	let code = readFile('main.c');

	const mangled = {};
	let i = 0;

	code = code.replace(/(JS_NewCFunction\([^,]+,[^,]+,\s+)"([^",]+)"/g, function(match, before, name) {
		if (!mangled[name]) {
			mangled[name] = '_0x' + Math.random().toString(16).slice(2, 8);
		}

		return before + `"${mangled[name]}"`;
	});

	for (const name in mangled) {
		code = code.replace(`"${name}"`, `"${mangled[name]}"`);
	}

	code = code.replace(/char\* code\s*=\s*("[\s\S]+");/, function (match, content) {
		let string = content.split('\n').map(x => x.trim().slice(1, -1).replaceAll('\\n', '')).join('\n');
		for (const name in mangled) {
			string = string.replaceAll(' ' + name, ' ' + mangled[name]);
		}

		const bytes = new TextEncoder().encode(string);

		return `

		${getMaskCode(bytes)}

		char* code = (char*)unmaskedBytes;

		`;
	});

	code = code + `

	void run() {
		${getMaskCode(bytes)}

		eval(unmaskedBytes, bytesLength);
	}

	`;

	writeFile('temp-main.c', code);
}

function getMaskCode(bytes) {
	const mask = Array.from({ length: 256 }, () => Math.floor(Math.random() * 256));
	const maskedBytes = bytes.map((x, i) => x ^ mask[i % mask.length]);

	return `

	uint8_t mask[] = {${mask.toString()}};
	size_t maskLength = ${mask.length};

	uint8_t maskedBytes[] = {${maskedBytes.toString()}};
	size_t bytesLength = ${bytes.length};

	uint8_t unmaskedBytes[bytesLength];
	for (int i = 0; i < bytesLength; i++) {
		unmaskedBytes[i] = maskedBytes[i] ^ mask[i % maskLength];
	}

	`;
}

function writeJsFile() {
	let code = readFile('./post.js');
	code = code.replace(/\/\* no_bundle \*\/[\s\S]*?\/\* no_bundle \*\//g, '');

	writeFile('temp-post.js', code);
}

function getCmd() {
	const text = readFile('Makefile');
	const cmd = text.split('\t')[1];

	const regex = /EXPORTED_FUNCTIONS='([^']+)'/;
	const list = regex.exec(cmd)[1].split(',').map(x => x.trim());
	list.splice(list.indexOf('_eval'), 1);
	list.splice(list.indexOf('_bytecode'), 1);
	list.push('_run');

	return cmd.replace(regex, `EXPORTED_FUNCTIONS='${list.join(', ')}'`)
		.replace('main.c', 'temp-main.c')
		.replace('post.js', 'temp-post.js')
		.replace('./out.js', '%out%')
		.replaceAll('./', path.join(__dirname, './'))
		.replace('%out%', output);
}

function readFile(file) {
	return fs.readFileSync(path.join(__dirname, file), 'utf8');
}

function writeFile(file, content) {
	fs.writeFileSync(path.join(__dirname, file), content);
}

function deleteFile(file) {
	fs.rmSync(path.join(__dirname, file));
}