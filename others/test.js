const Module = require('../out.js');

const code = `

const console = window.console;

console.log('running...');

console.log(window.person);
console.log(window.console === window.console);

window.useThis = function () {
	window.console.log(this.console);
}

const bytes = new Uint8Array(window.bytes.buffer);
console.log(bytes[0]);

`;

Module().then(wasm => {
	globalThis.person = {
		name: 'Big Chungus'
	};
	globalThis.bytes = new Uint8Array([69]);

	const bytes = wasm.getBytecode(code);

	console.log('>>> BYTECODE >>>\n', bytes);
	console.log('>>> RESULT >>>\n', wasm.eval(bytes));
}).catch(error => {
	console.log(error);
});