#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <Riva/Module.h>
#include <Util/TypedFunction.h>
#include <stdio.h>
#include "duktape.h"
#include "duk_module_node.h"

typedef struct context_t {
	const Std$Type$t *Type;
	duk_context *Handle;
	Std$Object$t *Error;
} context_t;

TYPE(T);

typedef struct object_t {
	const Std$Type$t *Type;
	void *Handle;
	const char *Name;
	duk_context *Context;
} object_t;

extern Std$Type$t ObjectT[];

SYMBOL($undefined, "undefined");

static void *riva_alloc_fn(context_t *Context, duk_size_t Size) {
	return Riva$Memory$alloc(Size);
}

static void *riva_realloc_fn(context_t *Context, void *Ptr, duk_size_t Size) {
	return Riva$Memory$realloc(Ptr, Size);
}

static void riva_free_fn(context_t *Context, void *Ptr) {
}

static void riva_fatal_fn(context_t *Context, const char *Msg) {
	Context->Error = Std$String$copy(Msg);
}

typedef struct path_node_t path_node_t;

struct path_node_t {
	path_node_t *Next;
	const char *Path;
};

static duk_ret_t cb_resolve_module(duk_context *Context) {
	const char *RequestedId = duk_get_string(Context, 0);
	const char *ParentId = duk_get_string(Context, 1);
	const char *ResolvedId;
    /* Arrive at the canonical module ID somehow. */
    duk_push_string(Context, ResolvedId);
    return 1;  /*nrets*/
}

static duk_ret_t cb_load_module(duk_context *ctx) {
    /*
     *  Entry stack: [ resolved_id exports module ]
     */

    /* Arrive at the JS source code for the module somehow. */
    return 1;  /*nrets*/
}

static duk_ret_t riva_proxy_call_fn(duk_context *Context) {
	printf("%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

TYPED_FUNCTION(void, to_duktape, Std$Object$t *Value, duk_context *Context) {
	if (Value == Std$Object$Nil) return duk_push_null(Context);
	duk_push_c_function(Context, riva_proxy_call_fn, 0);
	duk_push_pointer(Context, Value);
	duk_put_prop_literal(Context, -2, "riva");
	duk_get_global_literal(Context, "object_proxy");
	duk_push_proxy(Context, 0);
}

TYPED_INSTANCE(void, to_duktape, Std$Symbol$T, Std$Object$t *Value, duk_context *Context) {
	if (Value == $true) return duk_push_boolean(Context, 1);
	if (Value == $false) return duk_push_boolean(Context, 0);
	if (Value == $undefined) return duk_push_undefined(Context);
	duk_push_undefined(Context);
}

TYPED_INSTANCE(void, to_duktape, Std$Integer$SmallT, Std$Object$t *Value, duk_context *Context) {
	duk_push_int(Context, Std$Integer$get_small(Value));
}

TYPED_INSTANCE(void, to_duktape, Std$Real$T, Std$Object$t *Value, duk_context *Context) {
	duk_push_number(Context, Std$Real$get_value(Value));
}

TYPED_INSTANCE(void, to_duktape, Std$String$T, Std$Object$t *Value, duk_context *Context) {
	duk_push_lstring(Context, Std$String$flatten(Value), Std$String$get_length(Value));
}

TYPED_INSTANCE(void, to_duktape, Agg$List$T, Std$Object$t *Value, duk_context *Context) {
	duk_push_array(Context);
	int Index = 0;
	for (Agg$List$node *Node = Agg$List$tail(Value); Node; Node = Node->Prev) {
		to_duktape(Node->Value, Context);
		duk_put_prop_index(Context, -2, Index);
		++Index;
	}
}

static int add_property(Std$Object$t *Key, Std$Object$t *Value, duk_context *Context) {
	if (Key->Type == Std$String$T) {
		to_duktape(Value, Context);
		duk_put_prop_lstring(Context, -2, Std$String$flatten(Key), Std$String$get_length(Key));
		return 0;
	} else {
		return 1;
	}
}

TYPED_INSTANCE(void, to_duktape, Agg$Table$T, Std$Object$t *Value, duk_context *Context) {
	duk_push_object(Context);
	Agg$Table$foreach(Value, add_property, Context);
}

TYPED_INSTANCE(void, to_duktape, ObjectT, object_t *Object, duk_context *Context) {
	duk_push_heapptr(Context, Object->Handle);
}

extern Riva$Module_t Riva$Symbol[];

static Std$Object$t *to_riva(duk_context *Context, duk_idx_t Index) {
	if (duk_is_boolean(Context, Index)) {
		return duk_get_boolean(Context, Index) ? $true : $false;
	} else if (duk_is_null(Context, Index)) {
		return Std$Object$Nil;
	} else if (duk_is_undefined(Context, Index)) {
		return $undefined;
	} else if (duk_is_symbol(Context, Index)) {
		const char *Name = duk_get_string(Context, Index);
		Std$Object$t *Value;
		int IsRef;
		Riva$Module$import(Riva$Symbol, Name, &IsRef, &Value);
		return Value;
	} else if (duk_is_string(Context, Index)) {
		size_t Length;
		const char *String = duk_get_lstring(Context, Index, &Length);
		return Std$String$new_length(String, Length);
	} else if (duk_is_number(Context, Index)) {
		return Std$Real$new(duk_get_number(Context, Index));
	} else if (duk_is_pointer(Context, Index)) {
		return Std$Address$new(duk_get_pointer(Context, Index));
	} else if (duk_is_object(Context, Index)) {
		object_t *Object = new(object_t);
		Object->Type = ObjectT;
		Object->Handle = duk_get_heapptr(Context, Index);
		Object->Context = Context;
		asprintf(&Object->Name, "<%x>", Object);
		duk_push_global_stash(Context);
		duk_dup(Context, -2);
		duk_put_prop_string(Context, -2, Object->Name);
		duk_pop(Context);
		// TODO: register finalizer
		return (Std$Object$t *)Object;
	} else {
		printf("Unknown value type!");
		return Std$Object$Nil;
	}
}

METHOD("eval", TYP, T, TYP, Std$String$T) {
	context_t *Context = (context_t *)Args[0].Val;
	const char *Source = Std$String$flatten(Args[1].Val);
	int Length = Std$String$get_length(Args[1].Val);
	if (duk_peval_lstring(Context->Handle, Source, Length) != 0) {
		const char *Message = duk_safe_to_string(Context->Handle, -1);
		duk_pop(Context->Handle);
		SEND(Std$String$new(Message));
	} else {
		Std$Object$t *Value = to_riva(Context->Handle, -1);
		duk_pop(Context->Handle);
		RETURN(Value);
	}
}

METHOD("compile", TYP, T, TYP, Std$String$T) {
	context_t *Context = (context_t *)Args[0].Val;
	const char *Source = Std$String$flatten(Args[1].Val);
	int Length = Std$String$get_length(Args[1].Val);
	if (duk_pcompile_lstring(Context->Handle, 0, Source, Length) != 0) {
		const char *Message = duk_safe_to_string(Context->Handle, -1);
		duk_pop(Context->Handle);
		SEND(Std$String$new(Message));
	} else {
		Std$Object$t *Value = to_riva(Context->Handle, -1);
		duk_pop(Context->Handle);
		RETURN(Value);
	}
}

METHOD("global", TYP, T) {
	context_t *Context = (context_t *)Args[0].Val;
	duk_push_global_object(Context->Handle);
	Std$Object$t *Value = to_riva(Context->Handle, -1);
	duk_pop(Context->Handle);
	RETURN(Value);
}

Std$Function$status duktape_invoke(object_t *Function, unsigned long Count, const Std$Function$argument *Args, Std$Function$result *Result) {
	duk_context *Context = Function->Context;
	duk_push_heapptr(Context, Function->Handle);
	for (int I = 0; I < Count; ++I) to_duktape(Args[I].Val, Context);
	if (duk_pcall(Context, Count) != 0) {
		const char *Message = duk_safe_to_string(Context, -1);
		duk_pop(Context);
		SEND(Std$String$new(Message));
	} else {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop(Context);
		RETURN(Value);
	}
}

METHOD("new", TYP, ObjectT) {
	object_t *Function = (object_t *)Args[0].Val;
	duk_context *Context = Function->Context;
	duk_push_heapptr(Context, Function->Handle);
	for (int I = 1; I < Count; ++I) to_duktape(Args[I].Val, Context);
	if (duk_pnew(Context, Count - 1) != 0) {
		const char *Message = duk_safe_to_string(Context, -1);
		duk_pop(Context);
		SEND(Std$String$new(Message));
	} else {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop(Context);
		RETURN(Value);
	}
}

METHOD(".", TYP, ObjectT, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	if (duk_get_prop_lstring(Context, -1, Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop_2(Context);
		RETURN(Value);
	} else {
		duk_pop_2(Context);
		FAIL;
	}
}

METHOD("[]", TYP, ObjectT, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	if (duk_get_prop_lstring(Context, -1, Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop_2(Context);
		RETURN(Value);
	} else {
		duk_pop_2(Context);
		FAIL;
	}
}

METHOD("[]", TYP, ObjectT, TYP, Std$Integer$SmallT) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	if (duk_get_prop_index(Context, -1, Std$Integer$get_small(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop_2(Context);
		RETURN(Value);
	} else {
		duk_pop_2(Context);
		FAIL;
	}
}

METHOD("set", TYP, ObjectT, TYP, Std$String$T, ANY) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	to_duktape(Args[2].Val, Context);
	duk_put_prop_lstring(Context, -2, Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val));
	duk_pop(Context);
	RETURN(Args[2].Val);
}

METHOD("set", TYP, ObjectT, TYP, Std$Integer$SmallT, ANY) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	to_duktape(Args[2].Val, Context);
	duk_put_prop_index(Context, -2, Std$Integer$get_small(Args[1].Val));
	duk_pop(Context);
	RETURN(Args[2].Val);
}

AMETHOD(Std$String$Of, TYP, ObjectT) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object->Handle);
	duk_to_string(Context, -1);
	size_t Length;
	const char *String = duk_get_lstring(Context, -1, &Length);
	duk_pop(Context);
	RETURN(Std$String$new_length(String, Length));
}

static duk_ret_t riva_object_get_fn(duk_context *Context) {
	duk_get_prop_literal(Context, 0, "riva");
	Std$Object$t *Object = duk_get_pointer(Context, -1);
	duk_pop(Context);
	const char *Property = duk_get_string(Context, 1);
	Std$Object$t *Symbol;
	int IsRef;
	Riva$Module$import(Riva$Symbol, Property, &IsRef, &Symbol);
	Std$Function$result Result;
	if (Std$Function$call(Symbol, 1, &Result, Object, 0) < FAILURE) {
		to_duktape(Result.Val, Context);
	} else {
		duk_push_undefined(Context);
	}
	return 1;
}

static duk_ret_t riva_object_set_fn(duk_context *Context) {
	duk_get_prop_literal(Context, 0, "riva");
	Std$Object$t *Object = duk_get_pointer(Context, -1);
	duk_pop(Context);
	const char *Property = duk_get_string(Context, 1);
	Std$Object$t *Symbol;
	int IsRef;
	Riva$Module$import(Riva$Symbol, Property, &IsRef, &Symbol);
	Std$Function$result Result;
	if (Std$Function$call(Symbol, 1, &Result, Object, 0) < FAILURE) {
		if (Result.Ref) Result.Ref[0] = to_riva(Context, 2);
	}
	return 0;
}

static duk_ret_t riva_object_apply_fn(duk_context *Context) {
	duk_get_prop_literal(Context, 0, "riva");
	Std$Object$t *Object = duk_get_pointer(Context, -1);
	duk_pop(Context);
	Std$Object$t *This = to_riva(Context, 1);
	duk_get_prop_literal(Context, 2, "length");
	int Count = duk_to_int(Context, -1);
	duk_pop(Context);
	Std$Function$argument Args[Count];
	for (int I = 0; I < Count; ++I) {
		duk_get_prop_index(Context, 2, I);
		Args[I].Val = to_riva(Context, -1);
		duk_pop(Context);
		Args[I].Ref = 0;
	}
	Std$Function$result Result;
	if (Std$Function$invoke(Object, Count, &Result, Args) < FAILURE) {
		to_duktape(Result.Val, Context);
	} else {
		duk_push_undefined(Context);
	}
	return 1;
}

GLOBAL_FUNCTION(New, 0) {
	context_t *Context = new(context_t);
	Context->Type = T;
	Context->Handle = duk_create_heap(riva_alloc_fn, riva_realloc_fn, riva_free_fn, Context, riva_fatal_fn);
	duk_push_object(Context->Handle);
	duk_push_c_function(Context->Handle, riva_object_get_fn, 3);
	duk_put_prop_literal(Context->Handle, -2, "get");
	duk_push_c_function(Context->Handle, riva_object_set_fn, 4);
	duk_put_prop_literal(Context->Handle, -2, "set");
	duk_push_c_function(Context->Handle, riva_object_apply_fn, 3);
	duk_put_prop_literal(Context->Handle, -2, "apply");
	duk_push_c_function(Context->Handle, riva_object_apply_fn, 3);
	duk_put_prop_literal(Context->Handle, -2, "call");
	duk_put_global_literal(Context->Handle, "object_proxy");
	/*duk_push_object(Context->Handle);
	duk_push_c_function(Context->Handle, cb_resolve_module, DUK_VARARGS);
	duk_put_prop_string(Context->Handle, -2, "resolve");
	duk_push_c_function(Context->Handle, cb_load_module, DUK_VARARGS);
	duk_put_prop_string(Context->Handle, -2, "load");
	duk_module_node_init(Context->Handle);*/
	RETURN(Context);
}
