#ifndef GIR_GOBJECT_TYPE_H
#define GIR_GOBJECT_TYPE_H

#include <glib-object.h>
#include <Std.h>

#define RIVA_MODULE Gir$GObject$Type
#include <Riva-Header.h>

typedef struct Gir$GObject$Type$t {
	const Std$Type_t *Type;
	GType Value;
} Gir$GObject$Type$t;

extern Std$Type_t Gir$GObject$Type$T[];

extern Gir$GObject$Type$t Gir$GObject$Type$RIVA[];

RIVA_CFUN(Std$Type_t *, to_riva, GType);
RIVA_CFUN(void, register_type, GType, Std$Type$t *);

#undef RIVA_MODULE

#endif
