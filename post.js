const hostObjects = {};
const hostObjectMap = new Map();

function finalizeHostObject(id) {
	const cache = hostObjects[id];
	_free(cache.ptr);
	
	hostObjects[id] = null;
	hostObjectMap.delete(cache.object);
	
	availableIds.push(id);
}

const availableIds = [];
let nextId = 0;

function hostObjectToPtr(ctx, object) {
	let cache = hostObjectMap.get(object);
	if (!cache) {
		const id = availableIds.length > 0 ? availableIds.pop() : nextId++;
		cache = {
			object, 
			ptr: QJS_NewHostObjectPtr(ctx, id)
		};
		hostObjects[id] = cache;
		hostObjectMap.set(object, cache);
	}
	return cache.ptr;
}

function toJSValue(ctx, value) {
	if (value === null) return _QJS_GetNull();

	switch (typeof value) {
		case 'string':
			return JS_NewString(ctx, value);

		case 'number':
			return QJS_NewNumber(ctx, value);

		case 'boolean':
			return value ? _QJS_GetTrue() : _QJS_GetFalse();

		case 'object':
		case 'function':
			return QJS_DupValueOnStack(ctx, hostObjectToPtr(ctx, value));

		case 'bigint':
			return QJS_NewBigInt(ctx, Number(value));
	}

	return _QJS_GetUndefined();
}

function toJSValuePtr(ctx, value) {
	if (value === null) return _QJS_GetNullPtr();

	switch (typeof value) {
		case 'string':
			return QJS_NewStringPtr(ctx, value);

		case 'number':
			return QJS_NewNumberPtr(ctx, value);

		case 'boolean':
			return value ? _QJS_GetTruePtr() : _QJS_GetFalsePtr();

		case 'object':
		case 'function':
			return hostObjectToPtr(ctx, value);

		case 'bigint':
			return QJS_NewBigIntPtr(ctx, Number(value));
	}

	return _QJS_GetUndefinedPtr();
}

const funcMap = new Map();

const registry = new FinalizationRegistry(id => {
	const entry = funcMap.get(id);
	QJS_FreeValue(entry.ctx, entry.ptr);
	funcMap.delete(id);
});

function getFunction(ctx, funcPtr) {
	const func = new BigUint64Array(Module.HEAP8.buffer, funcPtr, 1)[0];

	const cache = funcMap.get(func);
	if (cache) {
		_free(funcPtr);
		return cache.ref.deref();
	}

	const funcDupPtr = QJS_DupValue(ctx, funcPtr);
	
	const invoker = function () {
		const ptrs = [];
		for (let i = 0; i < arguments.length; i++) {
			const ptr = toJSValuePtr(ctx, arguments[i]);
			ptrs.push(ptr);
		}

		const typedArray = new Int32Array(ptrs);
		const numBytes = 4 * ptrs.length;
		const argvPtr = _malloc(numBytes);
		const heapBytes = new Uint8Array(Module.HEAP8.buffer, argvPtr, numBytes);
		heapBytes.set(new Uint8Array(typedArray.buffer));

		QJS_Call(ctx, funcDupPtr, ptrs.length, argvPtr);

		return getReturnValue();
	}

	funcMap.set(func, {
		ref: new WeakRef(invoker), 
		ptr: funcDupPtr, 
		ctx
	});

	registry.register(invoker, func);

	return invoker;
}

let returnValue;
function getReturnValue() {
	const value = returnValue;
	returnValue = null;
	return value;
}

let args = [];

/* no_bundle */
let bytecode;
let eval;
/* no_bundle */

let JS_NewString, 
	QJS_NewBigInt, 
	QJS_NewNumber, 
	QJS_NewHostObject,
	QJS_Call, 
	QJS_FreeValue, 
	QJS_DupValue;

let QJS_NewStringPtr, 
	QJS_NewNumberPtr,
	QJS_NewBigIntPtr,
	QJS_NewHostObjectPtr, 
	QJS_DupValueOnStack;

Module.postRun = Module.postRun || [];
Module.postRun.push(function () {
	/* no_bundle */
	bytecode = cwrap('bytecode', 'number', ['number']);
	eval = cwrap('eval', null, ['array', 'number']);
	/* no_bundle */

	JS_NewString = cwrap('JS_NewString', 'number', ['number', 'string']);
	QJS_NewNumber = cwrap('QJS_NewNumber', 'number', ['number', 'number']);
	QJS_NewBigInt = cwrap('QJS_NewBigInt', 'number', ['number', 'number']);
	QJS_NewHostObject = cwrap('QJS_NewHostObject', 'number', ['number', 'number']);
	QJS_Call = cwrap('QJS_Call', null, ['number', 'number']);
	QJS_FreeValue = cwrap('QJS_FreeValue', null, ['number', 'number']);
	QJS_DupValue = cwrap('QJS_DupValue', 'number', ['number', 'number']);

	QJS_NewStringPtr = cwrap('QJS_NewStringPtr', 'number', ['number', 'string']);
	QJS_NewNumberPtr = cwrap('QJS_NewNumberPtr', 'number', ['number', 'number']);
	QJS_NewBigIntPtr = cwrap('QJS_NewBigIntPtr', 'number', ['number', 'number']);
	QJS_NewHostObjectPtr = cwrap('QJS_NewHostObjectPtr', 'number', ['number', 'number']);

	QJS_DupValueOnStack = cwrap('QJS_DupValueOnStack', 'number', ['number', 'number']);
});

/* no_bundle */
Module.hostObjects = hostObjects;
Module.funcMap = funcMap;

Module.getBytecode = function (code) {
	const codeSize = 1 + lengthBytesUTF8(code);
	const codePtr = _malloc(codeSize);
	stringToUTF8(code, codePtr, codeSize);

	const ptr = bytecode(codePtr);

	_free(codePtr);

	const dataPtr = HEAP32[ptr >> 2];
	const length = HEAP32[(ptr + 4) >> 2];
	const bytes = new Uint8Array(HEAP8.buffer, dataPtr, length);

	Module._free(ptr);
	return bytes;
}

Module.eval = function (bytes) {
	eval(bytes, bytes.length);
	return getReturnValue();
}
/* no_bundle */