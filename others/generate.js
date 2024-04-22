const HostObjectCode = `

function HostObject(id) {
	const object = function () {}
	object.__finalizer__ = new_finalizer(id);

	return new Proxy(object, {
		get(target, prop, receiver) {
			if (prop === '__host_object_id__') return id;
			return get_prop(id, prop);
		}, 
		set(target, prop, value) {
			return set_prop(id, prop, value);
		}, 
		apply(target, thisArgs, args) {
			return call_func(id, thisArgs.__host_object_id__, ...args);
		}, 
		construct(target, args) {
			return construct(id, ...args);
		}
	});
}

`;

const LibraryCode = `
host_set_return_num: function (value) {
	returnValue = value;
},
host_set_return_str: function (value) {
	returnValue = UTF8ToString(value);
},
host_set_return_null: function () {
	returnValue = null;
}, 
host_set_return_undefined: function () {
	returnValue = undefined;
}, 
host_set_return_false: function () {
	returnValue = false;
}, 
host_set_return_true: function () {
	returnValue = true;
}, 
host_set_return_bigint: function (value) {
	returnValue = BigInt(value);
},
host_set_return_arraybuffer: function (ptr, length) {
	returnValue = HEAP8.buffer.slice(ptr, ptr + length);
}, 
host_set_return_json: function (json) {
	returnValue = JSON.parse(UTF8ToString(json));
},
host_set_return_hostobject: function (objectId) {
	returnValue = hostObjects[objectId].object;
}, 
host_set_return_func: function (ctx, func) {
	returnValue = getFunction(ctx, func);
}
`;

const ExternCode = `
extern void host_set_return_num(double num);
extern void host_set_return_str(char* value);
extern void host_set_return_null();
extern void host_set_return_undefined();
extern void host_set_return_false();
extern void host_set_return_true();
extern void host_set_return_bigint(int64_t num);
extern void host_set_return_arraybuffer(uint8_t* data, size_t length);
extern void host_set_return_json(char* json);
extern void host_set_return_hostobject(uint32_t objectId);
extern void host_set_return_func(JSContext* ctx, JSValueConst* func);
`;

const TypeCode = `
uint32_t tag = JS_VALUE_GET_NORM_TAG(value);

switch (tag) {
	case JS_TAG_EXCEPTION: {
		handle_exception(ctx);
	}	break;

	case JS_TAG_STRING: {
		char* str = JS_ToCString(ctx, value);
		host_set_return_str(str);
		JS_FreeCString(ctx, str);
	}	break;

	case JS_TAG_INT:
	case JS_TAG_FLOAT64: {
		double num;
		JS_ToFloat64(ctx, &num, value);
		host_set_return_num(num);
	}	break;

	case JS_TAG_NULL:
		host_set_return_null();
		break;

	case JS_TAG_BOOL:
		if (value == JS_TRUE) {
			host_set_return_true();
		} else {
			host_set_return_false();
		}
		break;

	case JS_TAG_BIG_INT: {
		int64_t num;
		JS_ToInt64Ext(ctx, &num, value);
		host_set_return_bigint(num);
	}	break;

	case JS_TAG_OBJECT: {
		if (JS_IsFunction(ctx, value)) {

			JSValue hostIdValue = JS_GetPropertyStr(ctx, value, "__host_object_id__");
			if (JS_IsNumber(hostIdValue)) {
				
				int32_t hostId;
				JS_ToInt32(ctx, &hostId, hostIdValue);
				host_set_return_hostobject(hostId);

			} else {

				host_set_return_func(ctx, jsvalue_to_heap(value));

			}

			JS_FreeValue(ctx, hostIdValue);

		} else {
			size_t length;
			uint8_t* data = JS_GetArrayBuffer(ctx, &length, value);

			if (data) {
				host_set_return_arraybuffer(data, length);
			} else {
				JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
				
				if (JS_IsException(json)) {
					handle_exception(ctx);
				} else {
					char* cjson = JS_ToCString(ctx, json);
					host_set_return_json(cjson);
					JS_FreeCString(ctx, cjson);
				}

				JS_FreeValue(ctx, json);
			}
		}
		break;
	}

	default:
		host_set_return_undefined();
}
`;

function argString(arg, content) {
	const items = [];
	arg && items.push(arg);
	content && items.push(content);
	return `(${items.join(', ')})`;
}

function generateExterns(name, arg = '') {
	return ExternCode.replaceAll('_return_', `_${name}_`)
		.replace(/\(([^\)]+)?\)/g, (match, content) => argString(arg, content));
}

function generateLibrary(name, wrapValue, arg = '') {
	return LibraryCode.replaceAll('_return_', `_${name}_`)
		.replace(/\(([^\)]+)?\) {/g, (match, content) => argString(arg, content) + ' {')
		.replace(/returnValue = ([^;]+);/g, (match, value) => wrapValue(value));
}

function generateType(name, arg = '') {
	return TypeCode.replaceAll('_return_', `_${name}_`)
		.replace(/(host_set_[^\(]+)\(([^\)]+)?\)/g, (match, func, content) => func + argString(arg, content));
}

function toCString(code) {
	return code.split('\n').filter(x=>x.trim().length > 0).map(x => '"' + x + '\\n"').join( '\n' )
}
