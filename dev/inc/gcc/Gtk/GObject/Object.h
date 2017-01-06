#ifndef GTK_GOBJECT_OBJECT_H
#define GTK_GOBJECT_OBJECT_H

#include <glib-object.h>
#include <Std.h>

#define RIVA_MODULE Gtk$GObject$Object
#include <Riva-Header.h>

typedef struct GtkGOobject$Object_t {
	const Std$Type_t *Type;
	GObject *Handle;
	Std$Object_t *Extra;
} Gtk$GObject$Object_t;

extern Std$Type_t Gtk$GObject$Object$T[];
extern Gtk$GObject$Object_t Gtk$GObject$Object$Nil[];

RIVA_CFUN(const Gtk$GObject$Object_t *, new, GObject *, Std$Type_t *);
RIVA_CFUN(const Gtk$GObject$Object_t *, to_riva, GObject *);

#undef RIVA_MODULE

#endif
