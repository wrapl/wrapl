#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <Riva/Module.h>
#include <Util/TypedFunction.h>
#include <stdio.h>
#include "duktape.h"

typedef struct context_t {
	const Std$Type$t *Type;
	duk_context *Handle;
	Std$Object$t *Error;
} context_t;

TYPE(T);

typedef struct object_t {
	const Std$Type$t *Type;
	void *Handle;
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


GLOBAL_FUNCTION(New, 0) {
	context_t *Context = new(context_t);
	Context->Type = T;
	Context->Handle = duk_create_heap(riva_alloc_fn, riva_realloc_fn, riva_free_fn, Context, 0);
	RETURN(Context);
}

TYPED_FUNCTION(void, to_duktape, Std$Object$t *Value, duk_context *Context) {
	//if (Value == Std$Object$Nil) duk_push_null(Context);
	duk_push_null(Context);
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
	duk_push_heapptr(Context, Object);
	if (duk_get_prop_lstring(Context, -1, Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop(Context);
		RETURN(Value);
	} else {
		duk_pop(Context);
		FAIL;
	}
}

METHOD("[]", TYP, ObjectT, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object);
	if (duk_get_prop_lstring(Context, -1, Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop(Context);
		RETURN(Value);
	} else {
		duk_pop(Context);
		FAIL;
	}
}

METHOD("[]", TYP, ObjectT, TYP, Std$Integer$SmallT) {
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	duk_push_heapptr(Context, Object);
	if (duk_get_prop_index(Context, -1, Std$Integer$get_small(Args[1].Val))) {
		Std$Object$t *Value = to_riva(Context, -1);
		duk_pop(Context);
		RETURN(Value);
	} else {
		duk_pop(Context);
		FAIL;
	}
}

METHOD("@", TYP, ObjectT, VAL, Std$String$T) {
	printf("ObjectT @ String.T : %d\n", __LINE__);
	object_t *Object = (object_t *)Args[0].Val;
	duk_context *Context = Object->Context;
	printf("ObjectT @ String.T : %d\n", __LINE__);
	duk_push_heapptr(Context, Object);
	printf("ObjectT @ String.T : %d\n", __LINE__);
	duk_to_string(Context, -1);
	printf("ObjectT @ String.T : %d\n", __LINE__);
	size_t Length;
	const char *String = duk_get_lstring(Context, -1, &Length);
	printf("ObjectT @ String.T : %d\n", __LINE__);
	duk_pop(Context);
	printf("ObjectT @ String.T : %d\n", __LINE__);
	RETURN(Std$String$new_length(String, Length));
}
