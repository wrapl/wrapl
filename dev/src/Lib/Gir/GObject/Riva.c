#include <Riva.h>
#include <Std.h>
#include <Gir/GObject/Object.h>
#include <Gir/GObject/Type.h>
#include <Gir/GObject/Enum.h>
#include <Gir/GObject/Closure.h>
#include <Gir/GObject/Value.h>
#include <Util/TypedFunction.h>

static void finalize(Gir$GObject$Object$t *Object, void *Data) {
	g_object_unref(Object->Handle);
};

static GQuark RivaQuark;

Gir$GObject$Object$t *_object_new(GObject *Handle, Std$Type$t *Type) {
	Gir$GObject$Object$t *Object = new(Gir$GObject$Object$t);
	Object->Type = Type;
	Object->Handle = Handle;
//	Object->Extra = Std$Object$Nil;
	g_object_set_qdata(Handle, RivaQuark, Object);	
	g_object_ref_sink(Handle);
	Riva$Memory$register_finalizer(Object, finalize, 0, 0, 0);
	return Object;
};

Gir$GObject$Object$t *_object_to_riva(GObject *Handle) {
	if (Handle == 0) return Gir$GObject$Object$Nil;
	Gir$GObject$Object$t *Object = g_object_get_qdata(Handle, RivaQuark);
	if (Object) return Object;
	Std$Type$t *Type = Gir$GObject$Type$to_riva(G_OBJECT_TYPE(Handle));
	if (Type == 0) return 0;
	return _object_new(Handle, Type);
};

static GType RivaType;

const Std$Object$t *_value_to_riva(const GValue *Value) {
	switch (G_VALUE_TYPE(Value)) {
	case G_TYPE_NONE: return Std$Object$Nil;
	case G_TYPE_CHAR: return Std$Integer$new_small(g_value_get_char(Value));
	case G_TYPE_UCHAR: return Std$Integer$new_small(g_value_get_uchar(Value));
	case G_TYPE_BOOLEAN: return g_value_get_boolean(Value) ? $true : $false;
	case G_TYPE_INT: return Std$Integer$new_small(g_value_get_int(Value));
	case G_TYPE_UINT: return Std$Integer$new_small(g_value_get_uint(Value));
	case G_TYPE_LONG: return Std$Integer$new_small(g_value_get_long(Value));
	case G_TYPE_ULONG: return Std$Integer$new_small(g_value_get_ulong(Value));
	case G_TYPE_ENUM: return Std$Integer$new_small(g_value_get_enum(Value));
	case G_TYPE_FLAGS: return Std$Integer$new_small(g_value_get_flags(Value));
	case G_TYPE_FLOAT: return Std$Real$new(g_value_get_float(Value));
	case G_TYPE_DOUBLE: return Std$Real$new(g_value_get_double(Value));
	case G_TYPE_STRING: return Std$String$new(g_value_get_string(Value));
	case G_TYPE_POINTER: return Std$Address$new(g_value_get_pointer(Value));
	default: {
		Std$Type$t *Type;
		if (G_VALUE_TYPE(Value) == Gir$GObject$Type$RIVA->Value) {
			return g_value_peek_pointer(Value);
		} else if (G_VALUE_HOLDS(Value, G_TYPE_OBJECT)) {
			return (const Std$Object$t *)_object_to_riva(g_value_get_object(Value));
		} else if (g_value_fits_pointer(Value) && (Type = Gir$GObject$Type$to_riva(G_VALUE_TYPE(Value)))) {
			Gir$GObject$Object$t *Object = new(Gir$GObject$Object$t);
			Object->Type = Type;
			Object->Handle = g_value_peek_pointer(Value);
			return (const Std$Object$t *)Object;
		} else {
			printf("Warning: Unknown parameter type: %s\n", G_VALUE_TYPE_NAME(Value));
			return Std$Address$new(g_value_peek_pointer(Value));
		};
	};
	};
};

static inline int is_gobject_object(Std$Object$t *Object) {
	for (const Std$Type$t **P = Object->Type->Types; *P; ++P) {
		if (*P == Gir$GObject$Object$T) return 1;
	};
	return 0;
};

static inline int is_gobject_enum(Std$Object$t *Object) {
	for (const Std$Type$t **P = Object->Type->Types; *P; ++P) {
		if (*P == Gir$GObject$Enum$T) return 1;
	};
	return 0;
};

TYPED_FUNCTION(void, _to_value, Std$Object$t const *Source, GValue *Dest) {
	//printf("_to_value = 0x%x\n", _to_value);
	//printf("Converting object to gvalue\n");
	g_value_init(Dest, Gir$GObject$Type$RIVA->Value);
	Dest->data[0].v_pointer = Source;
};

TYPED_INSTANCE(void, _to_value, Std$Integer$SmallT, Std$Integer_smallt const *Source, GValue *Dest) {
	g_value_init(Dest, G_TYPE_LONG);
	g_value_set_long(Dest, Source->Value);
};

TYPED_INSTANCE(void, _to_value, Std$String$T, Std$String$t const *Source, GValue *Dest) {
	//printf("Converting string to gvalue\n");
	g_value_init(Dest, G_TYPE_STRING);
	g_value_set_string(Dest, Std$String$flatten(Source));
};

TYPED_INSTANCE(void, _to_value, Std$Real$T, Std$Real$t const *Source, GValue *Dest) {
	g_value_init(Dest, G_TYPE_DOUBLE);
	g_value_set_double(Dest, Source->Value);
};

TYPED_INSTANCE(void, _to_value, Std$Address$T, Std$Address$t const *Source, GValue *Dest) {
	g_value_init(Dest, G_TYPE_POINTER);
	g_value_set_pointer(Dest, Source->Value);
};

TYPED_INSTANCE(void, _to_value, Gir$GObject$Object$T, Gir$GObject$Object$t const *Source, GValue *Dest) {
	void *Object = Source->Handle;
	g_value_init(Dest, G_OBJECT_TYPE(Object));
	g_value_set_object(Dest, Object);
};

void _riva_to_value(Std$Object$t *Source, GValue *Dest) {
	//printf("Converting riva to gvalue\n");
	if (G_IS_VALUE(Dest)) g_value_unset(Dest);
	if (Source == Std$Object$Nil) {
		g_value_init(Dest, G_TYPE_NONE);
	} else if (Source == $true) {
		g_value_init(Dest, G_TYPE_BOOLEAN);
		g_value_set_boolean(Dest, TRUE);
	} else if (Source == $false) {
		g_value_init(Dest, G_TYPE_BOOLEAN);
		g_value_set_boolean(Dest, FALSE);
	} else {
		/*Util$TypedFunction$t *TypedFn = (Util$TypedFunction$t *)(&_to_value);
		printf("\n\n");
		for (int I = 0; I < 16; I += 2) {
			char *ModuleName, *SymbolName;
			void *Type = ((void **)TypedFn->_Entries)[I];
			if (Type) {
				Riva$Module$lookup(((void **)TypedFn->_Entries)[I], &ModuleName, &SymbolName);
				printf("%s.%s -> 0x%x\n", ModuleName, SymbolName, ((void **)TypedFn->_Entries)[I + 1]);
			} else {
				printf("<empty> -> 0x%x\n", ((void **)TypedFn->_Entries)[I + 1]);
			}
		}
		printf("\n\n");*/
		_to_value(Source, Dest);
	};
};

SYMBOL(AS, "@");

typedef struct val_closure_t {
	GClosure Parent;
	Std$Object$t *Function;
	Gir$GObject$Closure$t *Closure;
} val_closure_t;

static void __marshal_val(val_closure_t *Closure, GValue *Result, guint NoOfArgs, const GValue *Args, gpointer Hint, gpointer Data) {
	Std$Function_argument Args0[NoOfArgs];
	for (guint I = 0; I < NoOfArgs; ++I) {
		Args0[I].Val = _value_to_riva(Args + I);
		Args0[I].Ref = 0;
	};
	Std$Function_result Result0;
	switch (Std$Function$invoke(Closure->Function, NoOfArgs, &Result0, Args0)) {
	case MESSAGE:
		if (Result0.Val->Type == Std$Symbol$NoMethodMessageT) {
			Std$Symbol$nomethodmessage *Message = (Std$Symbol$nomethodmessage *)Result0.Val;
			for (int I = 0; I < Message->Count; ++I) printf("\t%s\n", Message->Stack[I]);
			Std$Function$call((Std$Object_t *)AS, 2, &Result0, Result0.Val, 0, Std$String$T, 0);
			printf("Warning: Closure sent message: %s.\n", Std$String$flatten(Result0.Val));
		} else if (Std$Function$call(AS, 2, &Result0, Result0.Val, 0, Std$String$T, 0) < FAILURE) {
			printf("Warning: Closure sent message: %s.\n", Std$String$flatten(Result0.Val));
		} else {
			printf("Warning: Closure sent message: <unknown>.\n");
		};
	case FAILURE:
		return;
	case SUSPEND:
	case SUCCESS:
		break;
	};
	if (Result) _riva_to_value(Result0.Val, Result);
};

Gir$GObject$Closure$t *_closure_from_val(Std$Object$t *Function) {
	val_closure_t *Handle = (val_closure_t *)g_closure_new_simple(sizeof(val_closure_t), 0);
	Handle->Function = Function;
	g_closure_set_marshal((GClosure *)Handle, (GClosureMarshal)__marshal_val);
	Gir$GObject$Closure$t *Closure = new(Gir$GObject$Closure$t);
	Closure->Type = Gir$GObject$Closure$T;
	Closure->Value = (GClosure *)Handle;
	Handle->Closure = Closure;
	return Closure;
};

typedef struct ref_closure_t {
	GClosure Parent;
	Std$Object$t **Function;
	Gir$GObject$Closure$t *Closure;
} ref_closure_t;

static void __marshal_ref(ref_closure_t *Closure, GValue *Result, guint NoOfArgs, const GValue *Args, gpointer Hint, gpointer Data) {
	Std$Function_argument Args0[NoOfArgs];
	for (guint I = 0; I < NoOfArgs; ++I) {
		Args0[I].Val = _value_to_riva(Args + I);
		Args0[I].Ref = 0;
	};
	Std$Function_result Result0;
	switch (Std$Function$invoke(Closure->Function[0], NoOfArgs, &Result0, Args0)) {
	case MESSAGE:
		if (Std$Function$call(AS, 2, &Result0, Result0.Val, 0, Std$String$T, 0) < FAILURE) {
			printf("Warning: Closure sent message: %s.\n", Std$String$flatten(Result0.Val));
		} else {
			printf("Warning: Closure sent message: <unknown>.\n");
		};
	case FAILURE:
		return;
	case SUSPEND:
	case SUCCESS:
		break;
	};
	if (Result) _riva_to_value(Result0.Val, Result);
};

Gir$GObject$Closure$t *_closure_from_ref(Std$Object$t **Function) {
	ref_closure_t *Handle = (ref_closure_t *)g_closure_new_simple(sizeof(ref_closure_t), 0);
	Handle->Function = Function;
	g_closure_set_marshal((GClosure *)Handle, (GClosureMarshal)__marshal_ref);
	Gir$GObject$Closure$t *Closure = new(Gir$GObject$Closure$t);
	Closure->Type = Gir$GObject$Closure$T;
	Closure->Value = (GClosure *)Handle;
	Handle->Closure = Closure;
	return Closure;
};

METHOD("connect", TYP, Gir$GObject$Object$T, TYP, Std$String$T, TYP, Std$Function$T) {
	Gir$GObject$Object$t *Object = (Gir$GObject$Object$t *)Args[0].Val;
	const char *Signal = Std$String$flatten(Args[1].Val);
	gboolean After = (Count > 3) && (Args[3].Val == $true);
	gulong ID = g_signal_connect_closure(Object->Handle, Signal, _closure_from_val(Args[2].Val)->Value, After);
	Result->Val = Std$Integer$new_small(ID);
	return SUCCESS;
};

INITIAL() {
	RivaQuark = g_quark_from_static_string("<<riva>>");
};

