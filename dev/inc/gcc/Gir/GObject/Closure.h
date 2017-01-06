#ifndef GIR_GOBJECT_CLOSURE_H
#define GIR_GOBJECT_CLOSURE_H

#define RIVA_MODULE Gir$GObject$Closure
#include <Riva-Header.h>

typedef struct Gir$GObject$Closure$t {
	Std$Type$t *Type;
	GClosure * Value;
} Gir$GObject$Closure$t;

extern Std$Type$t Gir$GObject$Closure$T[];

#undef RIVA_MODULE
    
#endif