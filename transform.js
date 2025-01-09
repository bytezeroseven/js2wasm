const parser = require('@babel/parser');
const traverse = require('@babel/traverse').default;
const generator = require('@babel/generator').default;
const t = require('@babel/types');

const domGlobals = require('./domGlobals.json');
const domGlobalsMap = {};

for (const prop of domGlobals) {
	domGlobalsMap[prop] = true;
}

function transform(code, varRegex, propRegex, nameMap) {
	const ast = parser.parse(code, {
		sourceType: 'module'
	});

	const propMap = {};
	const props = [];

	function getPropId(prop) {
		let id = propMap[prop];
		
		if (id === undefined) {
			id = props.length;
			props.push(prop);
			propMap[prop] = id;
		}

		return t.numericLiteral(id);
	}

	function getName(name) {
		if (nameMap && name in nameMap) return nameMap[name];
		return name;
	}

	function check(object, prop) {
		if (varRegex && t.isIdentifier(object)) return varRegex.test(object.name);
		if (propRegex) return propRegex.test(prop);

		return false;
	}

	function replace(path, name, args) {
		path.replaceWith(t.callExpression(t.identifier(getName(name)), args))
	}

	const undefinedMap = {};

	traverse(ast, {
		Program(path) {
			path.scope.crawl();
		}, 
		Identifier(path) {
			if (!path.isReferencedIdentifier() ||
				path.parentPath.isVariableDeclarator() ||
				path.parentPath.isFunctionDeclaration()) return;

			const name = path.node.name;
			if (!(name in domGlobalsMap)) return;

			const binding = path.scope.getBinding(name);
			if (!binding) {
				undefinedMap[name] = true;
			}
		}, 
		MemberExpression(path) {
			if (!path.node.property.computed) {
				const prop = path.node.property.name;

				if (check(path.node.object, prop)) {
					replace(path, 'get_prop_by_id', [
						path.node.object, 
						getPropId(prop)
					]);
				}
			}
		}, 
		AssignmentExpression(path) {
			if (t.isMemberExpression(path.node.left) && !path.node.left.computed) {
				const prop = path.node.left.property.name;

				if (check(path.node.left.object, prop)) {
					let right = path.node.right;
					if (path.node.operator.length === 2) {
						right = t.binaryExpression(path.node.operator[0], path.node.left, path.node.right);
					}

					replace(path, 'set_prop_by_id', [
						path.node.left.object, 
						getPropId(prop), 
						right
					]);
				}
			}
		}, 
		CallExpression(path) {
			if (t.isMemberExpression(path.node.callee) && !path.node.callee.computed) {
				const prop = path.node.callee.property.name;

				if (check(path.node.callee.object, prop)) {
					const args = [
						path.node.callee.object, 
						getPropId(prop)
					].concat(path.node.arguments);

					replace(path, 'call_func_by_id', args);
				}
			}
		}
	});

	const undefinedList = Object.keys(undefinedMap);
	console.log(undefinedList);
	
	const definerCode = undefinedList.length > 0 ? `${JSON.stringify(undefinedList)}.forEach(prop => globalThis[prop] = window[prop]);\n\n` : '';

	return {
		code: definerCode + generator(ast).code, 
		props
	};
}

module.exports = transform;

/*const code = `

console.log('bro');
new WebSocket();

el.style.margin = '10px';

`;

const result = transform(code, /el/);

console.log(result.code)*/

function getProcessedNode(root, skip = 0, varRegex) {
	let canProcess = false;
	let n = root;

	const nodes = [];

	while (n) {
		nodes.push(n);

		if (t.isIdentifier(n)) {
			if (varRegex.test(n.name)) {
				canProcess = true;
				break;
			}
		} else if (nodes.length > 0 && t.isThisExpression(n.object) && t.isIdentifier(n.property) && varRegex.test(n.property.name)) {
			canProcess = true;
			break;
		}

		n = n.object;
	}

	if (!canProcess) return;

	n = nodes.pop();
	
	for (let i = nodes.length - 1; i >= skip; i--) {
		const node = nodes[i];
		if (!node.computed) {
			n = t.callExpression(t.identifier(getName('get_prop_by_id')), [
				n, 
				getPropId(node.property.name)
			]);
		} else {
			node.object = n;
			n = node;
		}
	}

	return n;
}