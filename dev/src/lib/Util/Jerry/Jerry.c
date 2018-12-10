#include <Std.h>
#include <Agg.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <jerryscript.h>

typedef struct value_t {
	const Std$Type$t *Type;
	jerry_value_t Handle;
} value_t;

extern Std$Type$t ValueT[];

SYMBOL($strict, "strict");
SYMBOL($undefined, "undefined");

TYPED_FUNCTION(jerry_value_t, to_jerry, Std$Object$t *Value) {
	//if (Value == Std$Object$Nil) return jerry_create_null();
	return jerry_create_null();
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Std$Symbol$T, Std$Object$t *Value) {
	if (Value == $true) return jerry_create_boolean(true);
	if (Value == $false) return jerry_create_boolean(false);
	if (Value == $undefined) return jerry_create_undefined();
	return jerry_create_undefined();
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Std$Integer$SmallT, Std$Object$t *Value) {
	return jerry_create_number(Std$Integer$get_small(Value));
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Std$Real$T, Std$Object$t *Value) {
	return jerry_create_number(Std$Real$get_value(Value));
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Std$String$T, Std$Object$t *Value) {
	return jerry_create_string_sz_from_utf8(Std$String$flatten(Value), Std$String$get_length(Value));
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Agg$List$T, Std$Object$t *Value) {
	jerry_value_t *List = jerry_create_array(Agg$List$length(Value));
	int Index = 0;
	for (Agg$List$node *Node = Agg$List$tail(Value); Node; Node = Node->Prev) {
		jerry_set_property_by_index(List, Index, to_jerry(Node->Value));
		++Index;
	}
	return List;
}

static int add_property(Std$Object$t *Key, Std$Object$t *Value, jerry_value_t Object) {
	if (Key->Type == Std$String$T) {
		jerry_value_t Property = jerry_create_string_sz(Std$String$flatten(Key), Std$String$get_length(Key));
		jerry_set_property(Object, Property, to_jerry(Value));
		jerry_release_value(Property);
		return 0;
	} else {
		return 1;
	}
}

TYPED_INSTANCE(jerry_value_t, to_jerry, Agg$Table$T, Std$Object$t *Value) {
	jerry_value_t *Object = jerry_create_object();
	Agg$Table$foreach(Value, add_property, Object);
	return Object;
}

TYPED_INSTANCE(jerry_value_t, to_jerry, ValueT, value_t *Value) {
	return Value->Handle;
}

static Std$Function$status *from_jerry(jerry_value_t *Value, Std$Function$result *Result) {
	switch (jerry_value_get_type(Value)) {
	case JERRY_TYPE_NONE: RETURN(Std$Object$Nil);
	case JERRY_TYPE_UNDEFINED: RETURN($undefined);
	case JERRY_TYPE_NULL: RETURN(Std$Object$Nil);
	case JERRY_TYPE_BOOLEAN: RETURN(jerry_get_boolean_value(Value) ? $true : $false);
	case JERRY_TYPE_NUMBER: RETURN (Std$Real$new(jerry_get_number_value(Value)));
	case JERRY_TYPE_STRING: {
		size_t Length = jerry_get_utf8_string_size(Value);
		char *Buffer = Riva$Memory$alloc_atomic(Length + 1);
		jerry_string_to_utf8_char_buffer(Value, Buffer, Length);
		Buffer[Length] = 0;
		RETURN(Std$String$new_length(Buffer, Length));
	}
	case JERRY_TYPE_OBJECT:
	case JERRY_TYPE_FUNCTION: {
		value_t *Value0 = new(value_t);
		Value0->Type = ValueT;
		Value0->Handle = Value;
		RETURN(Value0);
	}
	case JERRY_TYPE_ERROR: {
		from_jerry(jerry_get_value_from_error(Value, true), Result);
		return MESSAGE;
	}
	}
}

METHOD(".", TYP, ValueT, TYP, Std$String$T) {
	value_t *Value = (value_t *)Args[0].Val;
	jerry_value_t Property = jerry_create_string_sz(Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val));
	jerry_value_t *Return = jerry_get_property(Value->Handle, Property);
	jerry_release_value(Property);
	return from_jerry(Return, Result);
}

METHOD("[]", TYP, ValueT, TYP, Std$Integer$SmallT) {
	value_t *Value = (value_t *)Args[0].Val;
	jerry_value_t *Return = jerry_get_property_by_index(Value->Handle, Std$Integer$get_small(Args[1].Val));
	return from_jerry(Return, Result);
}

METHOD("new", TYP, ValueT) {
	value_t *Value = (value_t *)Args[0].Val;
	jerry_value_t *Arguments[Count - 1];
	for (int I = 1; I < Count; ++I) Arguments[I - 1] = to_jerry(Args[I].Val);
	jerry_value_t Object = jerry_construct_object(Value->Handle, Arguments, Count - 1);
	return from_jerry(Object, Result);
}

Std$Function$status jerry_invoke(value_t *Function, unsigned long Count, const Std$Function$argument *Args, Std$Function$result *Result) {
	jerry_value_t *Arguments[Count];
	for (int I = 0; I < Count; ++I) Arguments[I] = to_jerry(Args[I].Val);
	jerry_value_t Value = jerry_call_function(Function->Handle, Function->Handle, Arguments, Count);
	return from_jerry(Value, Result);
}

GLOBAL_FUNCTION(Parse, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	jerry_parse_opts_t Options = JERRY_PARSE_NO_OPTS;
	if (Count > 2 && Args[2].Val == $strict) Options = JERRY_PARSE_STRICT_MODE;
	jerry_value_t *Value = jerry_parse(
		Std$String$flatten(Args[0].Val),
		Std$String$get_length(Args[0].Val),
		Std$String$flatten(Args[1].Val),
		Std$String$get_length(Args[1].Val),
		Options
	);
	return from_jerry(Value, Result);
}

GLOBAL_FUNCTION(Eval, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	jerry_parse_opts_t Options = JERRY_PARSE_NO_OPTS;
	if (Count > 1 && Args[1].Val == $strict) Options = JERRY_PARSE_STRICT_MODE;
	jerry_value_t *Value = jerry_eval(
		Std$String$flatten(Args[0].Val),
		Std$String$get_length(Args[0].Val),
		Options
	);
	return from_jerry(Value, Result);
}

INITIAL() {
	jerry_init(JERRY_INIT_EMPTY);
}
