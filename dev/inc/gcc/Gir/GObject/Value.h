#ifndef GIR_GOBJECT_VALUE_H
#define GIR_GOBJECT_VALUE_H

#define RIVA_MODULE Gir$GObject$Value
#include <Riva-Header.h>

typedef struct Gir$GObject$Value$t {
	Std$Type$t *Type;
	GValue * Value;
} Gir$GObject$Value$t;

extern Std$Type$t Gir$GObject$Value$T[];

#undef RIVA_MODULE
    
#endif