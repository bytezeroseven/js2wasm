#! /usr/bin/env node

const { program } = require('commander');
const bundle = require('./bundle.js');

program
	.name('js2wasm')
	.description('A tool for converting JS into WebAssembly while having access to all Web APIs.')
	.requiredOption('-i, --input <file>', 'Specifies the input JS file.')
	.requiredOption('-o, --output <file>', 'Specifies the output JS file.')
	.option('--terser-config <json>', 'Specifies options for minifying code using terser.')
	.option('--var-regex <regex>', 'Defines regex for identifiers to extract properties name from.')
	.option('--prop-regex <regex>', 'Defines regex for properties names to extract.')
	.action(run)
	.parse();

function run(options) {
	parse(options, 'terserConfig', '--terser-config', 'json', JSON.parse);
	parse(options, 'varRegex', '--var-regex', 'regular expression', RegExp);
	parse(options, 'propRegex', '--prop-regex', 'regular expression', RegExp);

	bundle(options);
}

function parse(options, key, name, typeName, parser) {
	if (key in options) {
		try {
			options[key] = parser(options[key]);
		} catch {
			console.log(`error: value of '${name}' is not a valid ${typeName}`);
			process.exit(0);
		}
	}
}