#include <string.h>
#include <stdlib.h>
#include "./quickjs/quickjs.h"

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

				JSValue hostIdValue = JS_GetPropertyStr(ctx, value, "__host_object_id__");
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

void host_set_args(JSValue* ctx, int offset, int argc, JSValueConst* argv) {
	for (int i = offset; i < argc; i++) {
		JSValue value = argv[i];

		
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

					JSValue hostIdValue = JS_GetPropertyStr(ctx, value, "__host_object_id__");
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
}

JSValue call_func(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t funcId;
	JS_ToInt32(ctx, &funcId, argv[0]);

	int32_t thisId;
	JS_ToInt32(ctx, &thisId, argv[1]);

	host_set_args(ctx, 2, argc, argv);

	return host_call_func(ctx, funcId, thisId);
}

JSValue construct(JSContext* ctx, JSValueConst jsThis, int argc, JSValueConst* argv) {
	int32_t funcId;
	JS_ToInt32(ctx, &funcId, argv[0]);

	host_set_args(ctx, 1, argc, argv);

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
}

JSValue QJS_GetNull() { return JS_NULL; }
JSValue QJS_GetUndefined() { return JS_UNDEFINED; }
JSValue QJS_GetTrue() { return JS_TRUE; }
JSValue QJS_GetFalse() { return JS_FALSE; }

JSValue QJS_NewNumber(JSContext* ctx, double num) { return JS_NewFloat64(ctx, num); }
JSValue QJS_NewBigInt(JSContext* ctx, int64_t num) { return JS_NewBigInt64(ctx, num); }

JSValue QJS_NewHostObject(JSContext* ctx, int32_t id) {
	JSValue argv[1];
	argv[0] = JS_NewInt32(ctx, id);

	JSValue global = JS_GetGlobalObject(ctx);
	JSValue func = JS_GetPropertyStr(ctx, global, "HostObject");
	JSValue result = JS_Call(ctx, func, JS_UNDEFINED, 1, argv);

	JS_FreeValue(ctx, global);
	JS_FreeValue(ctx, func);
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

void QJS_Call(JSContext* ctx, JSValueConst* func, int argc, JSValue** argv_ptrs) {
	JSValueConst argv[argc];
	for (int i = 0; i < argc; i++) {
		argv[i] = JS_DupValue(ctx, *argv_ptrs[i]);
	}

	JSValue value = JS_Call(ctx, *func, JS_UNDEFINED, argc, argv);
	host_set_return(ctx, value);
	JS_FreeValue(ctx, value);

	for (int i = 0; i < argc; i++) {
		JS_FreeValue(ctx, argv[i]);
	}
	free(argv_ptrs);
}

void QJS_FreeValue(JSContext* ctx, JSValue* value) {
	JS_FreeValue(ctx, *value);
}

JSValue* QJS_DupValue(JSContext* ctx, JSValue* value) {
	return jsvalue_to_heap(JS_DupValue(ctx, *value));
}

JSValue QJS_DupValueOnStack(JSContext* ctx, JSValue* value) {
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
		"function HostObject(id) {\n"
		"	const object = function () {}\n"
		"	object.__finalizer__ = new_finalizer(id);\n"
		"	return new Proxy(object, {\n"
		"		get(target, prop, receiver) {\n"
		"			if (prop === '__host_object_id__') return id;\n"
		"			return get_prop(id, prop);\n"
		"		}, \n"
		"		set(target, prop, value) {\n"
		"			return set_prop(id, prop, value);\n"
		"		}, \n"
		"		apply(target, thisArgs, args) {\n"
		"			return call_func(id, thisArgs.__host_object_id__, ...args);\n"
		"		}, \n"
		"		construct(target, args) {\n"
		"			return construct(id, ...args);\n"
		"		}\n"
		"	});\n"
		"}\n";

	JSValue value = JS_Eval(ctx, code, strlen(code), "", JS_EVAL_TYPE_GLOBAL);
	if (JS_IsException(value)) {
		handle_exception(ctx);
		return;
	}

	JSValue global = JS_GetGlobalObject(ctx);
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