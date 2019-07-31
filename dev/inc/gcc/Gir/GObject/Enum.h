#ifndef GIR_GOBJECT_ENUM_H
#define GIR_GOBJECT_ENUM_H

#include <glib-object.h>
#include <stdarg.h>
#include <Std.h>
#include <Agg.h>

#define RIVA_MODULE Gir$GObject$Enum
#include <Riva-Header.h>

typedef struct Gir$GObject$Enum_t {
	const Std$Type$t *Type;
	int Value;
	GType (*GetType)();
} Gir$GObject$Enum_t;


extern Std$Type$t Gir$GObject$Enum$T[];

#undef RIVA_MODULE

#endif
