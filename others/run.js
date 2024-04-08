const getWasm = require('../out.js');

const code = `

const array = new Uint8Array([10, 20, 30]);
window.output = array.buffer;

function getNum() {
	return 69;
}

window.getNum = getNum;

window.console.log('sdfadsf');

window.add = function(a, b) {
	return a + b;
}

`;

getWasm().then(wasm => {
	const bytes = wasm.getBytecode(code);

	console.log('>>> BYTECODE >>>\n', bytes);
	console.log('>>> RESULT >>>\n', wasm.eval(bytes));

	console.log(globalThis.output);
	console.log(globalThis.getNum());
	console.log(globalThis.add(20, 20));
}).catch(error => {
	console.log(error);
});