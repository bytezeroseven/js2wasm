addToLibrary({
	host_throw_error: function (msg) {
		throw new Error(UTF8ToString(msg));
	},
	host_finalize: function (id) {
		finalizeHostObject(id);
	}, 
	host_get_window: function (ctx) {
		return toJSValue(ctx, globalThis);
	},

	host_call_func: function (ctx, func, target) {
		const value = hostObjects[func].object.apply(hostObjects[target].object, args);
		args = [];
		return toJSValue(ctx, value);
	},
	host_construct: function (ctx, func) {
		const value = new hostObjects[func].object(...args);
		args = [];
		return toJSValue(ctx, value);
	},
	host_get_prop: function (ctx, id, prop) {
		const value = hostObjects[id].object[UTF8ToString(prop)];
		return toJSValue(ctx, value);
	}, 

	host_set_arg_num: function (value) {
		args.push(value);
	},
	host_set_arg_str: function (value) {
		args.push(UTF8ToString(value));
	},
	host_set_arg_null: function () {
		args.push(null);
	}, 
	host_set_arg_undefined: function () {
		args.push(undefined);
	}, 
	host_set_arg_false: function () {
		args.push(false);
	}, 
	host_set_arg_true: function () {
		args.push(true);
	}, 
	host_set_arg_bigint: function (value) {
		args.push(BigInt(value));
	},
	host_set_arg_arraybuffer: function (ptr, length) {
		args.push(HEAP8.buffer.slice(ptr, ptr + length));
	}, 
	host_set_arg_json: function (json) {
		args.push(JSON.parse(UTF8ToString(json)));
	},
	host_set_arg_hostobject: function (objectId) {
		args.push(hostObjects[objectId].object);
	}, 
	host_set_arg_func: function (ctx, func) {
		args.push(getFunction(ctx, func));
	},
	
	host_set_prop_num: function (id, prop, value) {
		hostObjects[id].object[UTF8ToString(prop)] = value;
	},
	host_set_prop_str: function (id, prop, value) {
		hostObjects[id].object[UTF8ToString(prop)] = UTF8ToString(value);
	},
	host_set_prop_null: function (id, prop) {
		hostObjects[id].object[UTF8ToString(prop)] = null;
	}, 
	host_set_prop_undefined: function (id, prop) {
		hostObjects[id].object[UTF8ToString(prop)] = undefined;
	}, 
	host_set_prop_false: function (id, prop) {
		hostObjects[id].object[UTF8ToString(prop)] = false;
	}, 
	host_set_prop_true: function (id, prop) {
		hostObjects[id].object[UTF8ToString(prop)] = true;
	}, 
	host_set_prop_bigint: function (id, prop, value) {
		hostObjects[id].object[UTF8ToString(prop)] = BigInt(value);
	},
	host_set_prop_arraybuffer: function (id, prop, ptr, length) {
		hostObjects[id].object[UTF8ToString(prop)] = HEAP8.buffer.slice(ptr, ptr + length);
	}, 
	host_set_prop_json: function (id, prop, json) {
		hostObjects[id].object[UTF8ToString(prop)] = JSON.parse(UTF8ToString(json));
	},
	host_set_prop_hostobject: function (id, prop, objectId) {
		hostObjects[id].object[UTF8ToString(prop)] = hostObjects[objectId].object;
	}, 
	host_set_prop_func: function (id, prop, ctx, func) {
		hostObjects[id].object[UTF8ToString(prop)] = getFunction(ctx, func);
	},

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
		returnValue = false
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
});