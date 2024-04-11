const getWasm = require('../out.js');

const code = `

/*const array = new Uint8Array([10, 20, 30]);
window.output = array.buffer;

function getNum() {
	return 69;
}

window.getNum = getNum;

window.console.log('sdfadsf');

window.add = function(a, b) {
	return a + b;
}

window.console.log(window.console.valueOf())
window.console.log(window.console.valueOf() == window.console);
window.console.log(window.console.valueOf() === window.console.valueOf());

window.console.log('real shit:', window.console === window.console)
*/


const c = window.console;
window.test = function (x) {
	return x === c;
}

window.console === window.console;

`;

getWasm().then(wasm => {
	const bytes = wasm.getBytecode(code);

	console.log('>>> BYTECODE >>>\n', bytes);
	console.log('>>> RESULT >>>\n', wasm.eval(bytes));

	console.log('test()', globalThis.test(globalThis.console));

	/*console.log(globalThis.output);
	console.log(globalThis.getNum());
	console.log(globalThis.add(20, 20));*/
}).catch(error => {
	console.log(error);
});