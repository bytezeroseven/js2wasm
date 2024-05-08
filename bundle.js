const fs = require('fs');
const exec = require('child_process').exec;
const path = require('path');

const { minify } = require('terser');
const transform = require('./transform.js');

module.exports = async function bundle(options) {
	let code = fs.readFileSync(options.input, 'utf8');

	const nameMap = {
		call_func_by_id: getRandomName(), 
		set_prop_by_id: getRandomName(), 
		get_prop_by_id: getRandomName()
	};

	let props;
	if (options.varRegex || options.propRegex) {
		const result = transform(code, options.varRegex, options.propRegex, nameMap);
		code = result.code;
		props = result.props;
	}

	if (options.terserConfig) {
		code = await minifyJs(code, options.terserConfig);
	}

	const outputDir = path.dirname(options.output);
	if (!fs.existsSync(outputDir)) {
		fs.mkdirSync(outputDir, {
			recursive: true
		});
	}

	writeQuickJsFile();

	const buildWasmCmd = getRawCmd()
		.replace('quickjs.c', 'temp-quickjs.c')
		.replace('out.js', 'temp-out.js')
		.replaceAll('./', path.join(__dirname, './')) + ' -s SINGLE_FILE=1';

	runCmd(buildWasmCmd, null, async function () {
		const Module = require('./temp-out.js');
		const wasm = await Module();

		deleteFile('temp-out.js');

		const bytes = wasm.getBytecode(code);
		console.log(`bytecode length: ${bytes.length}`);

		writeCFile(wasm, bytes, nameMap, props);
		writeJsFile();

		runCmd(getBuildCmd(options.output), function () {
			deleteFile('temp-main.c');
			deleteFile('temp-post.js');
			deleteFile('quickjs/temp-quickjs.c');
		}, () => afterBuild(options.output));
	});
}

function writeQuickJsFile() {
	let code = readFile('quickjs/quickjs.c');

	const ids = [];
	code = code.replace(/BC_TAG_([\w_=\d]+),/g, match => {
		let id;
		while (true) {
			id = 1 + Math.floor(Math.random() * 254);
			if (ids.indexOf(id) === -1) {
				ids.push(id);
				break;
			}
		}

		return match.slice(0, -1) + ' = ' + id + ',';
	});

	const mask8 = getBitmask(8);
	const mask16 = getBitmask(16);

	code = code.replace(`dbuf_put(&s->dbuf, p->u.str8, p->len);`, `
		for(i = 0; i < p->len; i++) {
			dbuf_putc(&s->dbuf, p->u.str8[i] ^ ${mask8});
		}
	`).replace(`p->u.str8[size] = '\\0';`, `
		for (uint32_t i = 0; i < len; i++) {
			p->u.str8[i] ^= ${mask8};
		}
		p->u.str8[size] = '\\0';
	`);

	code = code.replace(`bc_put_u16(s, p->u.str16[i]);`, `
		bc_put_u16(s, p->u.str16[i] ^ ${mask16});
	`).replace(`if (is_be()) {`, `
		for (uint32_t i = 0; i < len; i++)
		    p->u.str16[i] ^= ${mask16};
		if (is_be()) {
	`);

	writeFile('quickjs/temp-quickjs.c', code);
}

function runCmd(cmd, onFinish, onSuccess) {
	console.log(`running:\n${cmd}`);

	exec(cmd, (error, stderr, stdout) => {
		onFinish && onFinish();

		if (error) throw error;
		if (stderr) throw new Error(`stderr: ${stderr}`);

		console.log(`stdout ${stdout}`);

		onSuccess && onSuccess();
	});
}

async function afterBuild(output) {
	let code = fs.readFileSync(output, 'utf8');

	const mangled = {};
	let i = 0;

	code = code.replace(/Module\["_([^\"]+)"\]/g, (match, name) => {
		if (!(name in mangled)) {
			mangled[name] = i++;
		}
		return `Module["_${mangled[name]}"]`;
	}).replace(/cwrap\("([^\"]+)"/g, (match, name) => {
		return `cwrap(${mangled[name]}`;
	});

	code += `\nModule().then(wasm => wasm["_${mangled.run}"]());`

	code = await minifyJs(code, {
		mangle: {
			toplevel: true, 
			properties: {
				regex: /\b(?:object|refCount|ref|ptr|ctx)\b/, 
				builtins: true
			}
		}
	});

	fs.writeFileSync(output, code);
}

async function minifyJs(code, options) {
	const result = await minify(code, options);
	return result.code;
}

function writeCFile(wasm, bytes, mangled, props) {
	let code = readFile('main.c');

	code = code.replace(/(JS_NewCFunction\([^,]+,[^,]+,\s+)"([^",]+)"/g, function(match, before, name) {
		if (!(name in mangled)) {
			mangled[name] = getRandomName();
		}
		return before + `"${mangled[name]}"`;
	});

	for (const name in mangled) {
		code = code.replace(`"${name}"`, `"${mangled[name]}"`);
	}

	console.log(mangled);

	code = code.replaceAll('__hostObjectId__', getRandomName())
		.replaceAll('__finalizer__', getRandomName())
		.replaceAll('__HostObject__', getRandomName());

	code = code.replace(/char\* code\s*=\s*("[\s\S]+");/, function (match, content) {
		let string = content.split('\n').map(x => x.trim().slice(1, -1).replaceAll('\\n', '')).join('\n');
		for (const name in mangled) {
			string = string.replaceAll(' ' + name, ' ' + mangled[name]);
		}

		console.log(string);

		const bytes = wasm.getBytecode(string);
		const [mask, maskedBytes] = maskBytes(bytes);

		return `

		uint8_t maskedBytes[] = {${maskedBytes.toString()}};
		uint8_t mask[] = {${mask.toString()}};

		uint8_t unmaskedBytes[${bytes.length}];
		size_t byteLength = ${bytes.length};
		for (int i = 0; i < ${bytes.length}; i++) {
			unmaskedBytes[i] = maskedBytes[i] ^ mask[i % ${mask.length}];
		}

		`;
	});

	code = code.replace(`JSValue value = JS_Eval(ctx, code, strlen(code), "", JS_EVAL_TYPE_GLOBAL);`, `
		JSValue obj = JS_ReadObject(ctx, unmaskedBytes, byteLength, JS_READ_OBJ_BYTECODE);
		JSValue value = JS_EvalFunction(ctx, obj);
	`);

	const [mask, maskedBytes] = maskBytes(bytes);

	let propCode = '';

	if (props) {
		const propBytes = new TextEncoder().encode(JSON.stringify(props));
		for (let i = 0;i < propBytes.length; i++) {
			propBytes[i] ^= mask[i % mask.length];
		}

		code = code + `

		uint8_t maskedPropBytes[] = {${propBytes.toString()}};

		`;

		propCode = `
		uint8_t* unmaskedPropBytes = malloc(sizeof(uint8_t) * ${propBytes.length});
		for (int i = 0; i < ${propBytes.length}; i++) {
			unmaskedPropBytes[i] = maskedPropBytes[i] ^ mask[i % ${mask.length}];
		}

		host_set_prop_list(unmaskedPropBytes);
		free(unmaskedPropBytes);
		`;
	}

	code = code + `

	uint8_t maskedBytes[] = {${maskedBytes.toString()}};
	uint8_t mask[] = {${mask.toString()}};

	void run() {
		${propCode}

		uint8_t* unmaskedBytes = malloc(sizeof(uint8_t) * ${bytes.length});
		for (int i = 0; i < ${bytes.length}; i++) {
			unmaskedBytes[i] = maskedBytes[i] ^ mask[i % ${mask.length}];
		}

		eval(unmaskedBytes, ${bytes.length});
		free(unmaskedBytes);
	}

	`;

	writeFile('temp-main.c', code);
}

function maskBytes(bytes) {
	const mask = Array.from({ length: Math.min(256, bytes.length) }, () => Math.floor(Math.random() * 256));
	const maskedBytes = bytes.map((x, i) => x ^ mask[i % mask.length]);

	return [mask, maskedBytes];
}

function writeJsFile() {
	let code = readFile('./post.js');
	code = code.replace(/\/\* no_bundle \*\/[\s\S]*?\/\* no_bundle \*\//g, '');

	writeFile('temp-post.js', code);
}

function getBuildCmd(output) {
	const cmd = getRawCmd();

	const regex = /EXPORTED_FUNCTIONS="([^"]+)"/;
	const list = regex.exec(cmd)[1].split(',').map(x => x.trim());
	list.splice(list.indexOf('_eval'), 1);
	list.splice(list.indexOf('_bytecode'), 1);
	list.push('_run');

	return cmd.replace(regex, `EXPORTED_FUNCTIONS="${list.join(', ')}"`)
		.replace('main.c', 'temp-main.c')
		.replace('post.js', 'temp-post.js')
		.replace('quickjs.c', 'temp-quickjs.c')
		.replace('./out.js', '%out%')
		.replaceAll('./', path.join(__dirname, './'))
		.replace('%out%', output)
		.replace('-Oz', '-O3') + ' -flto -msimd128';
}

function getRawCmd() {
	const text = readFile('Makefile');
	return text.split('\t')[1];
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

function getRandomName() {
	return '_0x' + Math.random().toString(16).slice(2, 8);
}

function getBitmask(base) {
	return 1 + Math.floor(Math.random() * (Math.pow(2, base) - 2));
}