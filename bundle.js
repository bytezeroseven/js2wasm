const fs = require('fs');
const { minify } = require('terser');
const exec = require('child_process').exec;
const path = require('path');

const Module = require('./out.js');

const input = process.argv[2];
const output = process.argv[3];

if (!input || !output) {
	console.log(`Missing arguments.\nUsage: node build.js [input_file] [output_file]`);
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
		fs.rmSync('temp-main.c');
		fs.rmSync('temp-post.js');

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
				regex: /\b(?:object|refCount|ref|ptr)\b/, 
				builtins: true
			}
		}
	});

	return result.code;
}

function writeCFile(bytes) {
	const mask = Array.from({ length: 256 }, () => Math.floor(Math.random() * 256));
	const maskedBytes = bytes.map((x, i) => x ^ mask[i % mask.length]);

	let code = fs.readFileSync('./main.c', 'utf8');
	code = code + `

	uint8_t mask[] = {${mask.toString()}};
	size_t maskLength = ${mask.length};

	uint8_t maskedBytes[] = {${maskedBytes.toString()}};
	size_t length = ${bytes.length};

	void run() {
		uint8_t bytes[length];
		for (int i = 0; i < length; i++) {
			bytes[i] = maskedBytes[i] ^ mask[i % maskLength];
		}

		eval(bytes, length);
	}

	`;

	fs.writeFileSync('temp-main.c', code);
}

function writeJsFile() {
	let code = fs.readFileSync('./post.js', 'utf8');
	code = code.replace(/\/\* no_bundle \*\/[\s\S]*?\/\* no_bundle \*\//g, '');

	fs.writeFileSync('temp-post.js', code);
}

function getCmd() {
	const text = fs.readFileSync('Makefile', 'utf8');
	const cmd = text.split('\t')[1];

	const regex = /EXPORTED_FUNCTIONS='([^']+)'/;
	const list = regex.exec(cmd)[1].split(',').map(x => x.trim());
	list.splice(list.indexOf('_eval'), 1);
	list.splice(list.indexOf('_bytecode'), 1);
	list.push('_run');

	return cmd.replace(regex, `EXPORTED_FUNCTIONS='${list.join(', ')}'`)
		.replace('main.c', 'temp-main.c')
		.replace('post.js', 'temp-post.js')
		.replace('out.js', output);
}