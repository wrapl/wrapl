#ifndef GIR_GOBJECT_INTERFACE_H
#define GIR_GOBJECT_INTERFACE_H

#include <glib-object.h>
#include <Gir/GObject/Object.h>
#include <Std.h>

#define RIVA_MODULE Gir$GObject$Interface
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	GObject Parent;
	Std$Object$t *Extra;
};

RIVA_STRUCT(infot) {
	const Std$Type$t *Riva;
	GType Type;
	GInterfaceInfo Info;
};

RIVA_CFUN(GObject *, implementation, Std$Object$t *);

#undef RIVA_MODULE

#endif
