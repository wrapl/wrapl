#include <Gtk/GObject/Value.h>
#include <Gtk/GObject/Object.h>
#include <Gtk/GObject/Type.h>
#include <Gtk/GObject/Enum.h>
#include <Riva.h>
#include <Util/TypedFunction.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

TYPE(T);

static GType RivaType;

const Std$Object$t *_to_riva(const GValue *Value) {
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
		if (G_VALUE_TYPE(Value) == Gtk$GObject$Type$RIVA->Value) {
			return g_value_peek_pointer(Value);
		} else if (G_VALUE_HOLDS(Value, G_TYPE_OBJECT)) {
			return (const Std$Object$t *)Gtk$GObject$Object$to_riva(g_value_get_object(Value));
		} else if (g_value_fits_pointer(Value) && (Type = Gtk$GObject$Type$to_riva(G_VALUE_TYPE(Value)))) {
			Gtk$GObject$Object_t *Object = new(Gtk$GObject$Object_t);
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

static inline int is_gtk_object(Std$Object$t *Object) {
	for (const Std$Type$t **P = Object->Type->Types; *P; ++P) {
		if (*P == Gtk$GObject$Object$T) return 1;
	};
	return 0;
};

static inline int is_gtk_enum(Std$Object$t *Object) {
	for (const Std$Type$t **P = Object->Type->Types; *P; ++P) {
		if (*P == Gtk$GObject$Enum$T) return 1;
	};
	return 0;
};

TYPED_FUNCTION(void, _to_value, Std$Object$t const *Source, GValue *Dest) {
	g_value_init(Dest, Gtk$GObject$Type$RIVA->Value);
	Dest->data[0].v_pointer = Source;
};

TYPED_INSTANCE(void, _to_value, Std$Integer$SmallT, Std$Integer$smallt const *Source, GValue *Dest) {
	g_value_init(Dest, G_TYPE_LONG);
	g_value_set_long(Dest, Source->Value);
};

TYPED_INSTANCE(void, _to_value, Std$String$T, Std$String$t const *Source, GValue *Dest) {
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

void _to_gtk(Std$Object$t *Source, GValue *Dest) {
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
		_to_value(Source, Dest);
	};
};

va_list _valist(Agg$List$t *List) {
	int Size = 0;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		Std$Object$t *Value = Node->Value;
		if (Value->Type == Std$Integer$SmallT) {
			Size += sizeof(int);
		} else if (Value->Type == Std$String$T) {
			Size += sizeof(char *);
		} else if (Value->Type == Std$Real$T) {
			Size += sizeof(double);
		} else if (Value->Type == Std$Address$T) {
			Size += sizeof(void *);
		} else {
			Size += sizeof(void *);
		};
	};
	char *Valist = Riva$Memory$alloc(Size);
	union {int *Integer; const char **String; double *Real; void **Other;} P;
	P.Other = (void **)Valist;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		Std$Object$t *Value = Node->Value;
		if (Value->Type == Std$Integer$SmallT) {
			*(P.Integer++) = ((Std$Integer$smallt *)Value)->Value;
		} else if (Value->Type == Std$String$T) {
			*(P.String++) = Std$String$flatten(Value);
		} else if (Value->Type == Std$Real$T) {
			*(P.Real++) = ((Std$Real$t *)Value)->Value;
		} else if (Value->Type == Std$Address$T) {
			*(P.Other++) = ((Std$Address$t *)Value)->Value;
		} else if (Value == $true) {
			*(P.Integer++) = 1;
		} else if (Value == $false) {
			*(P.Integer++) = 0;
		} else if (is_gtk_object(Value)) {
			*(P.Other++) = ((Gtk$GObject$Object_t *)Value)->Handle;
		} else {
			*(P.Other++) = Value;
		};
	};
	return Valist;
};

va_list _varargs(int Count, Std$Function$argument *Args) {
	int Size = 0;
	for (int I = 0; I < Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Value->Type == Std$Integer$SmallT) {
			Size += sizeof(int);
		} else if (Value->Type == Std$String$T) {
			Size += sizeof(char *);
		} else if (Value->Type == Std$Real$T) {
			Size += sizeof(double);
		} else if (Value->Type == Std$Address$T) {
			Size += sizeof(void *);
		} else {
			Size += sizeof(void *);
		};
	};
	void *Valist = Riva$Memory$alloc(Size);
	union {int *Integer; const char **String; double *Real; void **Other;} P;
	P.Other = (void **)Valist;
	for (int I = 0; I < Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Value->Type == Std$Integer$SmallT) {
			*(P.Integer++) = ((Std$Integer$smallt *)Value)->Value;
		} else if (Value->Type == Std$String$T) {
			*(P.String++) = Std$String$flatten(Value);
		} else if (Value->Type == Std$Real$T) {
			*(P.Real++) = ((Std$Real$t *)Value)->Value;
		} else if (Value->Type == Std$Address$T) {
			*(P.Other++) = ((Std$Address$t *)Value)->Value;
		} else if (Value == $true) {
			*(P.Integer++) = 1;
		} else if (Value == $false) {
			*(P.Integer++) = 0;
		} else if (is_gtk_object(Value)) {
			*(P.Other++) = ((Gtk$GObject$Object_t *)Value)->Handle;
		} else {
			*(P.Other++) = Value;
		};
	};
	return Valist;
};

va_list _valist_terminated(Agg$List$t *List, int Terminator) {
	int Size = sizeof(int);
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		Std$Object$t *Value = Node->Value;
		if (Value->Type == Std$Integer$SmallT) {
			Size += sizeof(int);
		} else if (Value->Type == Std$String$T) {
			Size += sizeof(char *);
		} else if (Value->Type == Std$Real$T) {
			Size += sizeof(double);
		} else if (Value->Type == Std$Address$T) {
			Size += sizeof(void *);
		} else {
			Size += sizeof(void *);
		};
	};
	char *Valist = Riva$Memory$alloc(Size);
	union {int *Integer; const char **String; double *Real; void **Other;} P;
	P.Other = (void **)Valist;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		Std$Object$t *Value = Node->Value;
		if (Value->Type == Std$Integer$SmallT) {
			*(P.Integer++) = ((Std$Integer$smallt *)Value)->Value;
		} else if (Value->Type == Std$String$T) {
			*(P.String++) = Std$String$flatten(Value);
		} else if (Value->Type == Std$Real$T) {
			*(P.Real++) = ((Std$Real$t *)Value)->Value;
		} else if (Value->Type == Std$Address$T) {
			*(P.Other++) = ((Std$Address$t *)Value)->Value;
		} else if (Value == $true) {
			*(P.Integer++) = 1;
		} else if (Value == $false) {
			*(P.Integer++) = 0;
		} else if (is_gtk_object(Value)) {
			*(P.Other++) = ((Gtk$GObject$Object_t *)Value)->Handle;
		} else {
			*(P.Other++) = Value;
		};
	};
	*(P.Integer) = Terminator;
	return Valist;
};

va_list _varargs_terminated(int Count, Std$Function$argument *Args, int Terminator) {
	int Size = sizeof(int);
	for (int I = 0; I < Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Value->Type == Std$Integer$SmallT) {
			Size += sizeof(int);
		} else if (Value->Type == Std$String$T) {
			Size += sizeof(char *);
		} else if (Value->Type == Std$Real$T) {
			Size += sizeof(double);
		} else if (Value->Type == Std$Address$T) {
			Size += sizeof(void *);
		} else {
			Size += sizeof(void *);
		};
	};
	void *Valist = Riva$Memory$alloc(Size);
	union {int *Integer; const char **String; double *Real; void **Other;} P;
	P.Other = (void **)Valist;
	for (int I = 0; I < Count; ++I) {
		Std$Object$t *Value = Args[I].Val;
		if (Value->Type == Std$Integer$SmallT) {
			*(P.Integer++) = ((Std$Integer$smallt *)Value)->Value;
		} else if (Value->Type == Std$String$T) {
			*(P.String++) = Std$String$flatten(Value);
		} else if (Value->Type == Std$Real$T) {
			*(P.Real++) = ((Std$Real$t *)Value)->Value;
		} else if (Value->Type == Std$Address$T) {
			*(P.Other++) = ((Std$Address$t *)Value)->Value;
		} else if (Value == $true) {
			*(P.Integer++) = 1;
		} else if (Value == $false) {
			*(P.Integer++) = 0;
		} else if (is_gtk_object(Value)) {
			*(P.Other++) = ((Gtk$GObject$Object_t *)Value)->Handle;
		} else {
			*(P.Other++) = Value;
		};
	};
	*(P.Integer) = Terminator;
	return Valist;
};


GLOBAL_FUNCTION(New, 0) {
	Gtk$GObject$Value_t *Value = new(Gtk$GObject$Value_t);
	Value->Type = T;
	Value->Value = new(GValue);
	if (Count > 0) _to_gtk(Args[0].Val, Value->Value);
	Result->Val = (Std$Object$t *)Value;
	return SUCCESS;
};

METHOD("Get", TYP, T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)_to_riva(Value->Value);
	return SUCCESS;
};

/*
METHOD("Set", TYP, T, ANY) {
	Gtk$GObject$Value_t *Value = Args[0].Val;
	_to_gtk(Args[1].Val, Value->Value);
	return SUCCESS;
};
*/

METHOD("Set", TYP, T, TYP, Gtk$GObject$Object$T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[1].Val;
	g_value_init(Value->Value, G_OBJECT_TYPE(Object->Handle));
	g_value_set_object(Value->Value, Object->Handle);
	return SUCCESS;
};

METHOD("Set", TYP, T, VAL, Std$Object$Nil) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_NONE);
	return SUCCESS;
};

METHOD("Set", TYP, T, TYP, Std$Integer$SmallT) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_LONG);
	g_value_set_long(Value->Value, ((Std$Integer$smallt *)Args[1].Val)->Value);
	return SUCCESS;
};

METHOD("Set", TYP, T, TYP, Std$String$T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_STRING);
	g_value_set_string(Value->Value, Std$String$flatten(Args[1].Val));
	return SUCCESS;
};

METHOD("Set", TYP, T, TYP, Std$Real$T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_DOUBLE);
	g_value_set_double(Value->Value, ((Std$Real$t *)Args[1].Val)->Value);
	return SUCCESS;
};

METHOD("Set", TYP, T, TYP, Std$Address$T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_POINTER);
	g_value_set_pointer(Value->Value, ((Std$Address$t *)Args[1].Val)->Value);
	return SUCCESS;
};

METHOD("Set", TYP, T, VAL, $true) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_BOOLEAN);
	g_value_set_boolean(Value->Value, TRUE);
	return SUCCESS;
};

METHOD("Set", TYP, T, VAL, $false) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_init(Value->Value, G_TYPE_BOOLEAN);
	g_value_set_boolean(Value->Value, FALSE);
	return SUCCESS;
};

METHOD("Set", TYP, T, ANY) {
    Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
    g_value_init(Value->Value, Gtk$GObject$Type$RIVA->Value);
    Value->Value->data[0].v_pointer = Args[1].Val;
    return SUCCESS;
};

METHOD("Unset", TYP, T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	g_value_unset(Value->Value);
	return SUCCESS;
};

METHOD("Copy", TYP, T) {
	Gtk$GObject$Value_t *Value = (Gtk$GObject$Value_t *)Args[0].Val;
	Gtk$GObject$Value_t *Copy = new(Gtk$GObject$Value_t);
	Copy->Type = T;
	Copy->Value = new(GValue);
	g_value_copy(Value->Value, Copy->Value);
	Result->Val = (Std$Object$t *)Copy;
	return SUCCESS;
};
