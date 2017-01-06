#ifndef GTK_GOBJECT_INTERFACE_H
#define GTK_GOBJECT_INTERFACE_H

#include <glib-object.h>
#include <Gtk/GObject/Object.h>
#include <Std.h>

#define RIVA_MODULE Gtk$GObject$Interface
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	GObject Parent;
	Std$Object_t *Extra;
};

RIVA_STRUCT(infot) {
	const Std$Type_t *Riva;
	GType Type;
	GInterfaceInfo Info;
};

RIVA_CFUN(GObject *, implementation, Std$Object_t *);

#undef RIVA_MODULE

#endif
