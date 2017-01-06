#ifndef GTK_GOBJECT_VALUE_H
#define GTK_GOBJECT_VALUE_H

#include <glib-object.h>
#include <stdarg.h>
#include <Std.h>
#include <Agg.h>

#define RIVA_MODULE Gtk$GObject$Value
#include <Riva-Header.h>

typedef struct Gtk$GObject$Value_t {
	const Std$Type_t *Type;
	GValue *Value;
} Gtk$GObject$Value_t;

extern Std$Type_t Gtk$GObject$Value$T[];

RIVA_CFUN(Std$Object_t *, to_riva, const GValue *);
RIVA_CFUN(void, to_gtk, Std$Object_t const *, GValue *);
RIVA_CFUN(va_list, valist, Agg$List_t *);
RIVA_CFUN(va_list, valist_terminated, Agg$List_t *, int);
RIVA_CFUN(va_list, varargs, int, Std$Function_argument *);
RIVA_CFUN(va_list, varargs_terminated, int, Std$Function_argument *, int);

RIVA_CFUN(void, to_value, Std$Object_t const *, GValue *);

#undef RIVA_MODULE

#endif
