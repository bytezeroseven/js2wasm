# js2wasm
A tool for converting Javascript code to WebAssembly while having access to all browser APIs. Performance might not be that great though. Uses QuickJS.

## Example
```html
<script src="js2wasm.js"></script>
<script>

const code = `

window.console.log('running...');
window.console.log(window);

window.document.onclick = function (event) {
	window.alert('Clicked: ' + event.tagName);
}

window.WebSocket;

`;

Module().then(wasm => {
	const bytes = wasm.getBytecode(code);
	console.log(`>>> BYTECODE >>>\n`, bytes);

	const result = wasm.eval(bytes);
	console.log(`>>> RESULT >>>\n`, result);
});

</script>
```

## Building WASM
1. Install emscripten on your machine.
2. Run `make` to bundle the C files and generate the wasm module. 

## Can I use this in production? 
Here's a web game written from scratch in plain JS and converted to wasm using this: [Sandbox Hornex](https://sandbox.hornex.pro).

## Bundling JS Files
1. You need both emscripten & nodejs for generating independant JS bundles. The bytecode is masked and embedded in the WASM file. Certain functions are also trimmed out of wasm. Bundle is fully minified and mangled to reduced readability.
2. Run `npm install`.
3. Run `npm i -g`.
4. Run `npx js2wasm -i [input_js_file] -o [output_js_file]` to bundle JS files from anywhere.
5. Use `npx js2wasm -h` to see all options.

## Why bundle?
Mainly for security reasons. It makes reverse engineering even harder. Here's everything it does:
1. Bytecode is masked and stored in the wasm. 
2. `getBytecode()` & `eval()` are not exported. Some JS code is also trimmed out. 
3. Internal variable names and strings are obfuscated. 
4. All exported function names are mangled. 
5. QuickJS bytecode OP codes are randomized. Bundle won't be able to execute any bytecode generated from the original QuickJS.

## Credits
Created by Zertalious.