# js2wasm
A tool for converting Javascript code to WebAssembly while having access to all browser APIs. Performance might not be that great though. Uses QuickJS.

## Building WASM
1. Install emscripten on your machine.
2. Run `make` to bundle the C files and generate the wasm module. 

## Bundling JS Files
1. You need both emscripten & nodejs for generating independant JS bundles. The bytecode is masked and embedded in the WASM file. Certain functions are also trimmed out of wasm. Bundle is fully minified and mangled to reduced readability.
2. Run `npm install`. 
3. Run `node bundle.js [input_js_file] [output_js_file]` to bundle. 
4. Alternatively, you can use also use the NPX command. Run `npm i -g` and then use `npx js2wasm [input_js_file] [output_js_file]` to bundle JS files from anywhere.

Created by Zertalious.