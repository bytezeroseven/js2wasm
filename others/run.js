const Module = require('../out.js');

const code = `

window.console.log('running...');

window.console.log(window.person);
window.console.log(window);
window.console.log(window.console === window.console);

`;

Module().then(wasm => {
	globalThis.person = {
		name: 'Big Chungus'
	};

	const bytes = wasm.getBytecode(code);

	console.log('>>> BYTECODE >>>\n', bytes);
	console.log('>>> RESULT >>>\n', wasm.eval(bytes));
}).catch(error => {
	console.log(error);
});