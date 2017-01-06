#ifndef GTK_GOBJECT_ENUM_H
#define GTK_GOBJECT_ENUM_H

#include <glib-object.h>
#include <stdarg.h>
#include <Std.h>
#include <Agg.h>

#define RIVA_MODULE Gtk$GObject$Enum
#include <Riva-Header.h>

typedef struct Gtk$GObject$Enum_t {
	const Std$Type_t *Type;
	int Value;
	GType (*GetType)();
} Gtk$GObject$Enum_t;


extern Std$Type_t Gtk$GObject$Enum$T[];

#undef RIVA_MODULE

#endif
