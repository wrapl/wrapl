#include <Gir/GObject/Type.h>
#include <Gir/GObject/Object.h>
#include <Gir/TypeMap.h>
#include <Riva.h>
#include <Agg/StringTable.h>
#include <Agg/ObjectTable.h>
#include <Sys/Module.h>
#include <stdio.h>

#include <gobject/gvaluecollector.h>

#include <string.h>

TYPE(T, Gir$GObject$Object$T);

METHOD("@", TYP, T, VAL, Std$String$T) {
	Gir$GObject$Type$t *Type = (Gir$GObject$Type$t *)Args[0].Val;
	Result->Val = Std$String$new(g_type_name(Type->Value));
	return SUCCESS;
};

Gir$GObject$Type$t INVALID[] = {{T, G_TYPE_INVALID}};
Gir$GObject$Type$t NONE[] = {{T, G_TYPE_NONE}};
Gir$GObject$Type$t INTERFACE[] = {{T, G_TYPE_INTERFACE}};
Gir$GObject$Type$t CHAR[] = {{T, G_TYPE_CHAR}};
Gir$GObject$Type$t UCHAR[] = {{T, G_TYPE_UCHAR}};
Gir$GObject$Type$t BOOLEAN[] = {{T, G_TYPE_BOOLEAN}};
Gir$GObject$Type$t INT[] = {{T, G_TYPE_INT}};
Gir$GObject$Type$t UINT[] = {{T, G_TYPE_UINT}};
Gir$GObject$Type$t LONG[] = {{T, G_TYPE_LONG}};
Gir$GObject$Type$t ULONG[] = {{T, G_TYPE_ULONG}};
Gir$GObject$Type$t INT64[] = {{T, G_TYPE_INT64}};
Gir$GObject$Type$t UINT64[] = {{T, G_TYPE_UINT64}};
Gir$GObject$Type$t ENUM[] = {{T, G_TYPE_ENUM}};
Gir$GObject$Type$t FLAGS[] = {{T, G_TYPE_FLAGS}};
Gir$GObject$Type$t FLOAT[] = {{T, G_TYPE_FLOAT}};
Gir$GObject$Type$t DOUBLE[] = {{T, G_TYPE_DOUBLE}};
Gir$GObject$Type$t STRING[] = {{T, G_TYPE_STRING}};
Gir$GObject$Type$t POINTER[] = {{T, G_TYPE_POINTER}};
Gir$GObject$Type$t BOXED[] = {{T, G_TYPE_BOXED}};
Gir$GObject$Type$t PARAM[] = {{T, G_TYPE_PARAM}};
Gir$GObject$Type$t OBJECT[] = {{T, G_TYPE_OBJECT}};
Gir$GObject$Type$t RIVA[] = {{T, G_TYPE_INVALID}};

static Agg$ObjectTable$t GTypeToRiva[] = {Agg$ObjectTable$INIT};

void _register_type(GType GtkType, Std$Type$t *RivaType) {
	Agg$ObjectTable$put(GTypeToRiva, (void *)GtkType, RivaType);
};

Std$Type$t *_to_riva(GType GtkType) {
	Std$Type$t *RivaType = Agg$ObjectTable$get(GTypeToRiva, (void *)GtkType);
	if (RivaType != (Std$Type$t *)0xFFFFFFFF) return RivaType;
	const char *GtkName = g_type_name(GtkType);
	const char *RivaName = Agg$StringTable$get(Gir$TypeMap$Table, GtkName, strlen(GtkName));
	if (RivaName == 0) {
		Riva$Log$errorf("Warning: Gtk type is not mapped: %s\n", GtkName);
		return 0;
	};
	Riva$Module$t *Module = Riva$Module$load(0, RivaName);
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
