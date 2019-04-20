#include <Gtk/GObject/Init.h>
#include <Gtk/GObject/Type.h>
#include <Gtk/GObject/Object.h>
#include <Gtk/TypeMap.h>
#include <Riva.h>
#include <Agg/StringTable.h>
#include <Agg/ObjectTable.h>
#include <Sys/Module.h>
#include <stdio.h>

#include <gobject/gvaluecollector.h>

#include <string.h>

TYPE(T, Gtk$GObject$Object$T);

AMETHOD(Std$String$Of, TYP, T) {
	Gtk$GObject$Type_t *Type = (Gtk$GObject$Type_t *)Args[0].Val;
	Result->Val = Std$String$new(g_type_name(Type->Value));
	return SUCCESS;
};

Gtk$GObject$Type_t INVALID[] = {{T, G_TYPE_INVALID}};
Gtk$GObject$Type_t NONE[] = {{T, G_TYPE_NONE}};
Gtk$GObject$Type_t INTERFACE[] = {{T, G_TYPE_INTERFACE}};
Gtk$GObject$Type_t CHAR[] = {{T, G_TYPE_CHAR}};
Gtk$GObject$Type_t UCHAR[] = {{T, G_TYPE_UCHAR}};
Gtk$GObject$Type_t BOOLEAN[] = {{T, G_TYPE_BOOLEAN}};
Gtk$GObject$Type_t INT[] = {{T, G_TYPE_INT}};
Gtk$GObject$Type_t UINT[] = {{T, G_TYPE_UINT}};
Gtk$GObject$Type_t LONG[] = {{T, G_TYPE_LONG}};
Gtk$GObject$Type_t ULONG[] = {{T, G_TYPE_ULONG}};
Gtk$GObject$Type_t INT64[] = {{T, G_TYPE_INT64}};
Gtk$GObject$Type_t UINT64[] = {{T, G_TYPE_UINT64}};
Gtk$GObject$Type_t ENUM[] = {{T, G_TYPE_ENUM}};
Gtk$GObject$Type_t FLAGS[] = {{T, G_TYPE_FLAGS}};
Gtk$GObject$Type_t FLOAT[] = {{T, G_TYPE_FLOAT}};
Gtk$GObject$Type_t DOUBLE[] = {{T, G_TYPE_DOUBLE}};
Gtk$GObject$Type_t STRING[] = {{T, G_TYPE_STRING}};
Gtk$GObject$Type_t POINTER[] = {{T, G_TYPE_POINTER}};
Gtk$GObject$Type_t BOXED[] = {{T, G_TYPE_BOXED}};
Gtk$GObject$Type_t PARAM[] = {{T, G_TYPE_PARAM}};
Gtk$GObject$Type_t OBJECT[] = {{T, G_TYPE_OBJECT}};
Gtk$GObject$Type_t RIVA[] = {{T, G_TYPE_INVALID}};

CONSTANT(Fundamental, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("Fundamental");
	Sys$Module$export(Module, "Invalid", 0, INVALID);
	Sys$Module$export(Module, "None", 0, NONE);
	Sys$Module$export(Module, "Interface", 0, INTERFACE);
	Sys$Module$export(Module, "Char", 0, CHAR);
	Sys$Module$export(Module, "UChar", 0, UCHAR);
	Sys$Module$export(Module, "Boolean", 0, BOOLEAN);
	Sys$Module$export(Module, "Int", 0, INT);
	Sys$Module$export(Module, "UInt", 0, UINT);
	Sys$Module$export(Module, "Long", 0, LONG);
	Sys$Module$export(Module, "ULong", 0, ULONG);
	Sys$Module$export(Module, "Int64", 0, INT64);
	Sys$Module$export(Module, "UInt64", 0, UINT64);
	Sys$Module$export(Module, "Enum", 0, ENUM);
	Sys$Module$export(Module, "Flags", 0, FLAGS);
	Sys$Module$export(Module, "Float", 0, FLOAT);
	Sys$Module$export(Module, "Double", 0, DOUBLE);
	Sys$Module$export(Module, "String", 0, STRING);
	Sys$Module$export(Module, "Pointer", 0, POINTER);
	Sys$Module$export(Module, "Boxed", 0, BOXED);
	Sys$Module$export(Module, "Param", 0, PARAM);
	Sys$Module$export(Module, "Object", 0, OBJECT);
	Sys$Module$export(Module, "Riva", 0, RIVA);
	return (Std$Object_t *)Module;
};

static Agg$ObjectTable_t GTypeToRiva[] = {Agg$ObjectTable$INIT};

void _register_type(GType GtkType, Std$Type_t *RivaType) {
	Agg$ObjectTable$put(GTypeToRiva, (void *)GtkType, RivaType);
};

Std$Type_t *_to_riva(GType GtkType) {
	Std$Type_t *RivaType = Agg$ObjectTable$get(GTypeToRiva, (void *)GtkType);
	if (RivaType != (Std$Type_t *)0xFFFFFFFF) return RivaType;
	const char *GtkName = g_type_name(GtkType);
	const char *RivaName = Agg$StringTable$get(Gtk$TypeMap$Table, GtkName, strlen(GtkName));
	if (RivaName == 0) {
		Riva$Log$errorf("Warning: Gtk type is not mapped: %s\n", GtkName);
		return 0;
	};
	Riva$Module_t *Module = Riva$Module$load(0, RivaName);
	if (Module == 0) {
		Riva$Log$errorf("Error: Module not found: %s\n", RivaName);
		return 0;
	};
	int Flag;
	if (Riva$Module$import(Module, "T", &Flag, (void **)&RivaType) == 0) {
		Riva$Log$errorf("Error: Export not found: %s.t\n", RivaName);
		return 0;
	};
	Agg$ObjectTable$put(GTypeToRiva, (void *)GtkType, RivaType);
	return RivaType;
};

static gpointer riva_value_copy(gpointer Value) {
    return Value;
};

static void riva_value_free(gpointer Value) {
};

INITIAL() {
    g_type_init();
    RIVA->Value = g_pointer_type_register_static("riva");
};
