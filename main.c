#include <string.h>
#include <stdlib.h>
#include "./quickjs/quickjs.h"
#include "./quickjs/quickjs-atom.h"

extern void host_throw_error(char* msg);
extern void host_finalize(int32_t id);
extern JSValue host_get_window(JSContext* ctx);

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

extern void host_set_prop_num(int32_t id, char* prop, double num);
extern void host_set_prop_str(int32_t id, char* prop, char* value);
extern void host_set_prop_null(int32_t id, char* prop);
extern void host_set_prop_undefined(int32_t id, char* prop);
extern void host_set_prop_false(int32_t id, char* prop);
extern void host_set_prop_true(int32_t id, char* prop);
extern void host_set_prop_bigint(int32_t id, char* prop, int64_t num);
extern void host_set_prop_arraybuffer(int32_t id, char* prop, uint8_t* data, size_t length);
extern void host_set_prop_json(int32_t id, char* prop, char* json);
extern void host_set_prop_hostobject(int32_t id, char* prop, uint32_t objectId);
extern void host_set_prop_func(int32_t id, char* prop, JSContext* ctx, JSValueConst* func);

extern void host_set_arg_num(double num);
extern void host_set_arg_str(char* value);
extern void host_set_arg_null();
extern void host_set_arg_undefined();
extern void host_set_arg_false();
extern void host_set_arg_true();
extern void host_set_arg_bigint(int64_t num);
extern void host_set_arg_arraybuffer(uint8_t* data, size_t length);
extern void host_set_arg_json(char* json);
extern void host_set_arg_hostobject(uint32_t objectId);
extern void host_set_arg_func(JSContext* ctx, JSValueConst* func);

extern JSValue host_get_prop(JSContext* ctx, int32_t id, char* prop);
extern JSValue host_call_func(JSContext* ctx, int32_t func, int32_t target);
extern JSValue host_construct(JSContext* ctx, int32_t cls);

extern void host_set_prop_list(uint8_t* json, int32_t length);
extern void host_set_prop_by_id_num(int32_t id, int32_t prop, double num);
extern void host_set_prop_by_id_str(int32_t id, int32_t prop, char* value);
extern void host_set_prop_by_id_null(int32_t id, int32_t prop);
extern void host_set_prop_by_id_undefined(int32_t id, int32_t prop);
extern void host_set_prop_by_id_false(int32_t id, int32_t prop);
extern void host_set_prop_by_id_true(int32_t id, int32_t prop);
extern void host_set_prop_by_id_bigint(int32_t id, int32_t prop, int64_t num);
extern void host_set_prop_by_id_arraybuffer(int32_t id, int32_t prop, uint8_t* data, size_t length);
extern void host_set_prop_by_id_json(int32_t id, int32_t prop, char* json);
extern void host_set_prop_by_id_hostobject(int32_t id, int32_t prop, uint32_t objectId);
extern void host_set_prop_by_id_func(int32_t id, int32_t prop, JSContext* ctx, JSValueConst* func);

extern JSValue host_get_prop_by_id(JSContext* ctx, int32_t objectId, int32_t prop);
extern JSValue host_call_func_by_id(JSContext* ctx, int32_t objectId, int32_t prop);

typedef struct {
	JSValue HostObject;
	JSAtom hostObjectId;
	JSAtom lengthAtom;
} ContextData;

JSValue* jsvalue_to_heap(JSValue value) {
	JSValue* result = malloc(sizeof(JSValue));
	*result = value;
	return result;
}

void handle_exception(JSContext* ctx) {
	JSValue exception = JS_GetException(ctx);
	
	char* str = JS_ToCString(ctx, exception);
	host_throw_error(str);
	JS_FreeCString(ctx, str);

	JS_FreeValue(ctx, exception);
}

JSValue get_prop(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t id;
	JS_ToInt32(ctx, &id, argv[0]);

	char* prop = JS_ToCString(ctx, argv[1]);
	JSValue result = host_get_prop(ctx, id, prop);
	JS_FreeCString(ctx, prop);

	return result;
}

JSValue set_prop(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t id;
	JS_ToInt32(ctx, &id, argv[0]);

	char* prop = JS_ToCString(ctx, argv[1]);
	JSValue value = argv[2];

	uint32_t tag = JS_VALUE_GET_NORM_TAG(value);

	switch (tag) {
		case JS_TAG_EXCEPTION: {
			handle_exception(ctx);
		}	break;

		case JS_TAG_STRING: {
			char* str = JS_ToCString(ctx, value);
			host_set_prop_str(id, prop, str);
			JS_FreeCString(ctx, str);
		}	break;

		case JS_TAG_INT:
		case JS_TAG_FLOAT64: {
			double num;
			JS_ToFloat64(ctx, &num, value);
			host_set_prop_num(id, prop, num);
		}	break;

		case JS_TAG_NULL:
			host_set_prop_null(id, prop);
			break;

		case JS_TAG_BOOL:
			if (value == JS_TRUE) {
				host_set_prop_true(id, prop);
			} else {
				host_set_prop_false(id, prop);
			}
			break;

		case JS_TAG_BIG_INT: {
			int64_t num;
			JS_ToInt64Ext(ctx, &num, value);
			host_set_prop_bigint(id, prop, num);
		}	break;

		case JS_TAG_OBJECT: {
			if (JS_IsFunction(ctx, value)) {

				ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);
				JSValue hostIdValue = JS_GetProperty(ctx, value, ctxData->hostObjectId);
				if (JS_IsNumber(hostIdValue)) {
					
					int32_t hostId;
					JS_ToInt32(ctx, &hostId, hostIdValue);
					host_set_prop_hostobject(id, prop, hostId);

				} else {

					host_set_prop_func(id, prop, ctx, jsvalue_to_heap(value));

				}

				JS_FreeValue(ctx, hostIdValue);

			} else {
				size_t length;
				uint8_t* data = JS_GetArrayBuffer(ctx, &length, value);

				if (data) {
					host_set_prop_arraybuffer(id, prop, data, length);
				} else {
					JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
					
					if (JS_IsException(json)) {
						handle_exception(ctx);
					} else {
						char* cjson = JS_ToCString(ctx, json);
						host_set_prop_json(id, prop, cjson);
						JS_FreeCString(ctx, cjson);
					}

					JS_FreeValue(ctx, json);
				}
			}
			break;
		}

		default:
			host_set_prop_undefined(id, prop);
	}

	JS_FreeCString(ctx, prop);

	return JS_TRUE;
}

void host_set_arg(JSContext* ctx, JSValue value) {
	uint32_t tag = JS_VALUE_GET_NORM_TAG(value);

	switch (tag) {
		case JS_TAG_EXCEPTION: {
			handle_exception(ctx);
		}	break;

		case JS_TAG_STRING: {
			char* str = JS_ToCString(ctx, value);
			host_set_arg_str(str);
			JS_FreeCString(ctx, str);
		}	break;

		case JS_TAG_INT:
		case JS_TAG_FLOAT64: {
			double num;
			JS_ToFloat64(ctx, &num, value);
			host_set_arg_num(num);
		}	break;

		case JS_TAG_NULL:
			host_set_arg_null();
			break;

		case JS_TAG_BOOL:
			if (value == JS_TRUE) {
				host_set_arg_true();
			} else {
				host_set_arg_false();
			}
			break;

		case JS_TAG_BIG_INT: {
			int64_t num;
			JS_ToInt64Ext(ctx, &num, value);
			host_set_arg_bigint(num);
		}	break;

		case JS_TAG_OBJECT: {
			if (JS_IsFunction(ctx, value)) {

				ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);
				JSValue hostIdValue = JS_GetProperty(ctx, value, ctxData->hostObjectId);
				if (JS_IsNumber(hostIdValue)) {
					
					int32_t hostId;
					JS_ToInt32(ctx, &hostId, hostIdValue);
					host_set_arg_hostobject(hostId);

				} else {

					host_set_arg_func(ctx, jsvalue_to_heap(value));

				}

				JS_FreeValue(ctx, hostIdValue);

			} else {
				size_t length;
				uint8_t* data = JS_GetArrayBuffer(ctx, &length, value);

				if (data) {
					host_set_arg_arraybuffer(data, length);
				} else {
					JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
					
					if (JS_IsException(json)) {
						handle_exception(ctx);
					} else {
						char* cjson = JS_ToCString(ctx, json);
						host_set_arg_json(cjson);
						JS_FreeCString(ctx, cjson);
					}

					JS_FreeValue(ctx, json);
				}
			}
			break;
		}

		default:
			host_set_arg_undefined();
	}
}

void host_set_args(JSValue* ctx, JSValueConst array) {
	int32_t length;
	JSValue lengthValue = JS_GetProperty(ctx, array, ((ContextData*)JS_GetContextOpaque(ctx))->lengthAtom);
	JS_ToInt32(ctx, &length, lengthValue);
	JS_FreeValue(ctx, lengthValue);

	for (int i = 0; i < length; i++) {
		JSValue value = JS_GetPropertyUint32(ctx, array, i);
		host_set_arg(ctx, value);
		JS_FreeValue(ctx, value);
	}
}

JSValue call_func(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t funcId;
	JS_ToInt32(ctx, &funcId, argv[0]);

	int32_t thisId;
	JS_ToInt32(ctx, &thisId, argv[1]);

	host_set_args(ctx, argv[2]);

	return host_call_func(ctx, funcId, thisId);
}

JSValue construct(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t funcId;
	JS_ToInt32(ctx, &funcId, argv[0]);

	host_set_args(ctx, argv[1]);

	return host_construct(ctx, funcId);
}

void host_set_return(JSContext* ctx, JSValue value) {
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

				ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);
				JSValue hostIdValue = JS_GetProperty(ctx, value, ctxData->hostObjectId);
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
}

//

JSValue get_prop_by_id(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);

	JSValue objectId = JS_GetProperty(ctx, argv[0], ctxData->hostObjectId);
	int32_t id;
	JS_ToInt32(ctx, &id, objectId);
	JS_FreeValue(ctx, objectId);

	int32_t prop;
	JS_ToInt32(ctx, &prop, argv[1]);

	return host_get_prop_by_id(ctx, id, prop);
}

JSValue set_prop_by_id(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);

	JSValue objectId = JS_GetProperty(ctx, argv[0], ctxData->hostObjectId);
	int32_t id;
	JS_ToInt32(ctx, &id, objectId);
	JS_FreeValue(ctx, objectId);

	int32_t prop;
	JS_ToInt32(ctx, &prop, argv[1]);
	
	JSValue value = JS_DupValue(ctx, argv[2]);
	uint32_t tag = JS_VALUE_GET_NORM_TAG(value);

	switch (tag) {
		case JS_TAG_EXCEPTION: {
			handle_exception(ctx);
		}	break;

		case JS_TAG_STRING: {
			char* str = JS_ToCString(ctx, value);
			host_set_prop_by_id_str(id, prop, str);
			JS_FreeCString(ctx, str);
		}	break;

		case JS_TAG_INT:
		case JS_TAG_FLOAT64: {
			double num;
			JS_ToFloat64(ctx, &num, value);
			host_set_prop_by_id_num(id, prop, num);
		}	break;

		case JS_TAG_NULL:
			host_set_prop_by_id_null(id, prop);
			break;

		case JS_TAG_BOOL:
			if (value == JS_TRUE) {
				host_set_prop_by_id_true(id, prop);
			} else {
				host_set_prop_by_id_false(id, prop);
			}
			break;

		case JS_TAG_BIG_INT: {
			int64_t num;
			JS_ToInt64Ext(ctx, &num, value);
			host_set_prop_by_id_bigint(id, prop, num);
		}	break;

		case JS_TAG_OBJECT: {
			if (JS_IsFunction(ctx, value)) {

				JSValue hostIdValue = JS_GetProperty(ctx, value, ctxData->hostObjectId);
				if (JS_IsNumber(hostIdValue)) {
					
					int32_t hostId;
					JS_ToInt32(ctx, &hostId, hostIdValue);
					host_set_prop_by_id_hostobject(id, prop, hostId);

				} else {

					host_set_prop_by_id_func(id, prop, ctx, jsvalue_to_heap(value));

				}

				JS_FreeValue(ctx, hostIdValue);

			} else {
				size_t length;
				uint8_t* data = JS_GetArrayBuffer(ctx, &length, value);

				if (data) {
					host_set_prop_by_id_arraybuffer(id, prop, data, length);
				} else {
					JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
					
					if (JS_IsException(json)) {
						handle_exception(ctx);
					} else {
						char* cjson = JS_ToCString(ctx, json);
						host_set_prop_by_id_json(id, prop, cjson);
						JS_FreeCString(ctx, cjson);
					}

					JS_FreeValue(ctx, json);
				}
			}
			break;
		}

		default:
			host_set_prop_by_id_undefined(id, prop);
	}

	return value;
}

JSValue call_func_by_id(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);

	JSValue objectId = JS_GetProperty(ctx, argv[0], ctxData->hostObjectId);
	int32_t id;
	JS_ToInt32(ctx, &id, objectId);
	JS_FreeValue(ctx, objectId);

	int32_t prop;
	JS_ToInt32(ctx, &prop, argv[1]);

	if (argc > 2) {
		for (int i = 2; i < argc; i++) {
			host_set_arg(ctx, argv[i]);
		}
	}

	return host_call_func_by_id(ctx, id, prop);
}

//

JSValue QJS_GetNull() { return JS_NULL; }
JSValue QJS_GetUndefined() { return JS_UNDEFINED; }
JSValue QJS_GetTrue() { return JS_TRUE; }
JSValue QJS_GetFalse() { return JS_FALSE; }

JSValue QJS_NewNumber(JSContext* ctx, double num) { return JS_NewFloat64(ctx, num); }
JSValue QJS_NewBigInt(JSContext* ctx, int64_t num) { return JS_NewBigInt64(ctx, num); }

JSValue QJS_NewHostObject(JSContext* ctx, int32_t id) {
	JSValue argv[1];
	argv[0] = JS_NewInt32(ctx, id);

	ContextData* ctxData = (ContextData*)JS_GetContextOpaque(ctx);
	JSValue result = JS_Call(ctx, ctxData->HostObject, JS_UNDEFINED, 1, argv);

	JS_FreeValue(ctx, argv[0]);
	return result;
}

JSValue QJS_Null = JS_NULL;
JSValue QJS_Undefined = JS_UNDEFINED;
JSValue QJS_True = JS_TRUE;
JSValue QJS_False = JS_FALSE;

JSValue* QJS_GetNullPtr() { return &QJS_Null; }
JSValue* QJS_GetUndefinedPtr() { return &QJS_Undefined; }
JSValue* QJS_GetTruePtr() { return &QJS_True; }
JSValue* QJS_GetFalsePtr() { return &QJS_False; }

JSValue* QJS_NewNumberPtr(JSContext* ctx, double num) { 
	return jsvalue_to_heap(JS_NewFloat64(ctx, num)); 
}

JSValue* QJS_NewBigIntPtr(JSContext* ctx, int64_t num) { 
	return jsvalue_to_heap(JS_NewBigInt64(ctx, num));
}

JSValue* QJS_NewStringPtr(JSContext* ctx, char* str) { 
	return jsvalue_to_heap(JS_NewString(ctx, str));
}

JSValue* QJS_NewHostObjectPtr(JSContext* ctx, int32_t id) {
	JSValue v = QJS_NewHostObject(ctx, id);

	JSRefCountHeader* p = (JSRefCountHeader*)JS_VALUE_GET_PTR(v);
	p->ref_count--;

	return jsvalue_to_heap(v);
}

JSValue* QJS_NewArrayBufferCopyPtr(JSContext* ctx, uint8_t* bytes, size_t length) {
	return jsvalue_to_heap(JS_NewArrayBufferCopy(ctx, bytes, length));
}

void QJS_Call(JSContext* ctx, JSValueConst* func, JSValue* jsThis, int argc, JSValue** argv_ptrs) {
	JSValueConst argv[argc];
	for (int i = 0; i < argc; i++) {
		argv[i] = JS_DupValue(ctx, *argv_ptrs[i]);
	}
	JSValue jsThisValue = JS_DupValue(ctx, *jsThis);

	JSValue value = JS_Call(ctx, *func, jsThisValue, argc, argv);
	host_set_return(ctx, value);
	JS_FreeValue(ctx, value);

	JS_FreeValue(ctx, jsThisValue);
	for (int i = 0; i < argc; i++) {
		JS_FreeValue(ctx, argv[i]);
	}
	free(argv_ptrs);
}

void QJS_FreeValue(JSContext* ctx, JSValue* value) {
	JS_FreeValue(ctx, *value);
}

JSValue QJS_DupValue(JSContext* ctx, JSValue* value) {
	return JS_DupValue(ctx, *value);
}

typedef struct {
	uint8_t* bytes;
	size_t length;
} Bytecode;

Bytecode* bytecode(char* code) {
	JSRuntime* runtime = JS_NewRuntime();
	JSContext* ctx = JS_NewContext(runtime);

	JSValue object = JS_Eval(ctx, code, strlen(code), "", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

	if (JS_IsException(object)) {
		handle_exception(ctx);
	}

	Bytecode* result = malloc(sizeof(Bytecode));
	result->bytes = JS_WriteObject(ctx, &result->length, object, JS_WRITE_OBJ_BYTECODE);

	JS_FreeValue(ctx, object);

	return result;
}

typedef struct {
	JSContext* ctx;
	int32_t id;
} FinalizerOpaque;

JSClassID finalizerClassId;

void on_finalizer_collected(JSRuntime* runtime, JSValue object) {
	FinalizerOpaque* finalizerOpaque = (FinalizerOpaque*)JS_GetOpaque(object, finalizerClassId);
	host_finalize(finalizerOpaque->id);

	free(finalizerOpaque);
}

JSValue new_finalizer(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t id;
	JS_ToInt32(ctx, &id, argv[0]);

	JSValue object = JS_NewObjectClass(ctx, finalizerClassId);
	if (JS_IsException(object)) return object;

	FinalizerOpaque* finalizerOpaque = malloc(sizeof(FinalizerOpaque));
	finalizerOpaque->ctx = ctx;
	finalizerOpaque->id = id;

	JS_SetOpaque(object, finalizerOpaque);

	return object;
}

void eval(uint8_t* bytes, size_t length) {
	JSRuntime* runtime = JS_NewRuntime();
	JSContext* ctx = JS_NewContext(runtime);

	if (finalizerClassId == 0) {
		finalizerClassId = JS_NewClassID(&finalizerClassId);
	}
	
	JSClassDef classDef;
	memset(&classDef, 0, sizeof(JSClassDef));
	classDef.class_name = "Finalizer";
	classDef.finalizer = on_finalizer_collected;

	if (JS_NewClass(runtime, finalizerClassId, &classDef) < 0) {
		host_throw_error("Failed to create Finalizer class!");
		return;
	}

	char* code = 
		"globalThis.__hostObjectId__ = Symbol();\n"
		"const __finalizer__ = Symbol();\n"
		"const config = {\n"
		"	get(target, prop, receiver) {\n"
		"		if (prop === __hostObjectId__) return target[__hostObjectId__];\n"
		"		return get_prop(target[__hostObjectId__], prop);\n"
		"	}, \n"
		"	set(target, prop, value) {\n"
		"		return set_prop(target[__hostObjectId__], prop, value);\n"
		"	}, \n"
		"	apply(target, thisArgs, args) {\n"
		"		return call_func(target[__hostObjectId__], thisArgs ? thisArgs[__hostObjectId__] : 0, args);\n"
		"	}, \n"
		"	construct(target, args) {\n"
		"		return construct(target[__hostObjectId__], args);\n"
		"	}\n"
		"};\n"
		"function __HostObject__(id) {\n"
		"	const object = function () {}\n"
		"	object[__finalizer__] = new_finalizer(id);\n"
		"	object[__hostObjectId__] = id;\n"
		"	return new Proxy(object, config);\n"
		"}\n";

	JSValue value = JS_Eval(ctx, code, strlen(code), "", JS_EVAL_TYPE_GLOBAL);
	if (JS_IsException(value)) {
		handle_exception(ctx);
		return;
	}

	JSValue global = JS_GetGlobalObject(ctx);

	ContextData* ctxData = malloc(sizeof(ContextData));
	ctxData->HostObject = JS_GetPropertyStr(ctx, global, "__HostObject__");
	ctxData->lengthAtom = JS_NewAtom(ctx, "length");

	JSValue hostObjectIdValue = JS_GetPropertyStr(ctx, global, "__hostObjectId__");
	ctxData->hostObjectId = JS_ValueToAtom(ctx, hostObjectIdValue);
	JS_FreeValue(ctx, hostObjectIdValue);

	JS_SetContextOpaque(ctx, ctxData);

	JS_SetPropertyStr(ctx, global, "set_prop_by_id", JS_NewCFunction(ctx, set_prop_by_id, "set_prop_by_id", 0));
	JS_SetPropertyStr(ctx, global, "get_prop_by_id", JS_NewCFunction(ctx, get_prop_by_id, "get_prop_by_id", 0));
	JS_SetPropertyStr(ctx, global, "call_func_by_id", JS_NewCFunction(ctx, call_func_by_id, "call_func_by_id", 0));

	JS_SetPropertyStr(ctx, global, "new_finalizer", JS_NewCFunction(ctx, new_finalizer, "new_finalizer", 0));
	JS_SetPropertyStr(ctx, global, "get_prop", JS_NewCFunction(ctx, get_prop, "get_prop", 0));
	JS_SetPropertyStr(ctx, global, "set_prop", JS_NewCFunction(ctx, set_prop, "set_prop", 0));
	JS_SetPropertyStr(ctx, global, "call_func", JS_NewCFunction(ctx, call_func, "call_func", 0));
	JS_SetPropertyStr(ctx, global, "construct", JS_NewCFunction(ctx, construct, "construct", 0));
	JS_SetPropertyStr(ctx, global, "window", host_get_window(ctx));

	JS_FreeValue(ctx, global);

	JSValue object = JS_ReadObject(ctx, bytes, length, JS_READ_OBJ_BYTECODE);
	JSValue result = JS_EvalFunction(ctx, object);

	host_set_return(ctx, result);

	JS_FreeValue(ctx, result);
}