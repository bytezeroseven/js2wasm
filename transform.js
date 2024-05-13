const parser = require('@babel/parser');
const traverse = require('@babel/traverse').default;
const generator = require('@babel/generator').default;
const t = require('@babel/types');

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

		return id;
	}

	function getName(name) {
		if (nameMap && nameMap[name]) return nameMap[name];
		return name;
	}

	function check(object, prop) {
		if (varRegex && object.type === 'Identifier') return varRegex.test(object.name);
		if (propRegex) return propRegex.test(prop);

		return false;
	}

	traverse(ast, {
		MemberExpression(path) {
			if (!path.node.property.computed) {
				const prop = path.node.property.name;

				if (check(path.node.object, prop)) {
					const args = [
						path.node.object, 
						t.numericLiteral(getPropId(prop))
					];
					path.replaceWith(t.callExpression(t.identifier(getName('get_prop_by_id')), args))
				}
			}
		}, 
		AssignmentExpression(path) {
			if (path.node.left.type === 'MemberExpression' && !path.node.left.computed) {
				const prop = path.node.left.property.name;

				if (check(path.node.left.object, prop)) {
					let right = path.node.right;
					if (path.node.operator.length === 2) {
						right = t.binaryExpression(path.node.operator[0], path.node.left, path.node.right);
					}

					const args = [
						path.node.left.object, 
						t.numericLiteral(getPropId(prop)), 
						right
					];
					path.replaceWith(t.callExpression(t.identifier(getName('set_prop_by_id')), args));
				}
			}
		}, 
		CallExpression(path) {
			if (path.node.callee.type === 'MemberExpression' && !path.node.callee.computed) {
				const prop = path.node.callee.property.name;

				if (check(path.node.callee.object, prop)) {
					const args = [
						path.node.callee.object, 
						t.numericLiteral(getPropId(prop))
					];

					if (path.node.arguments.length > 0) {
						args.push(t.arrayExpression(path.node.arguments));
					}

					path.replaceWith(t.callExpression(t.identifier(getName('call_func_by_id')), args));
				}
			}
		}
	});

	return {
		code: generator(ast).code, 
		props
	};
}

module.exports = transform;

/*const code = `

el.style.border = '1px solid red';

`;

const result = transform(code, null, /style/);

console.log(result.code)*/