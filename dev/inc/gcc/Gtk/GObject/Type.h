#ifndef GTK_GOBJECT_TYPE_H
#define GTK_GOBJECT_TYPE_H

#include <glib-object.h>
#include <Std.h>

#define RIVA_MODULE Gtk$GObject$Type
#include <Riva-Header.h>

typedef struct Gtk$GObject$Type_t {
	const Std$Type_t *Type;
	GType Value;
} Gtk$GObject$Type_t;

extern Std$Type_t Gtk$GObject$Type$T[];

extern Gtk$GObject$Type_t Gtk$GObject$Type$RIVA[];

RIVA_CFUN(Std$Type_t *, to_riva, GType);
RIVA_CFUN(void, register_type, GType, Std$Type_t *);

#undef RIVA_MODULE

#endif
