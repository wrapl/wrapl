#ifndef GIR_GOBJECT_RIVA_H
#define GIR_GOBJECT_RIVA_H

#include <glib-object.h>
#include <Std.h>
#include <Gir/GObject/Object.h>
#include <Gir/GObject/Closure.h>

#define RIVA_MODULE Gir$GObject$Riva
#include <Riva-Header.h>

RIVA_CFUN(const Gir$GObject$Object$t *, object_new, GObject *, Std$Type$t *);
RIVA_CFUN(const Gir$GObject$Object$t *, object_to_riva, GObject *);

RIVA_CFUN(Std$Object_t *, value_to_riva, const GValue *);
RIVA_CFUN(void, riva_to_value, Std$Object_t const *, GValue *);
RIVA_CFUN(void, to_value, Std$Object_t const *, GValue *);

RIVA_CFUN(Gir$GObject$Closure$t *, closure_from_val, Std$Object_t *);
RIVA_CFUN(Gir$GObject$Closure$t *, closure_from_ref, Std$Object_t *);

#undef RIVA_MODULE

#endif
