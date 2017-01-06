#ifndef GIR_GOBJECT_OBJECT_H
#define GIR_GOBJECT_OBJECT_H

#define RIVA_MODULE Gir$GObject$Object
#include <Riva-Header.h>

typedef struct Gir$GObject$Object$t {
	Std$Type$t *Type;
	GObject *Handle;
} Gir$GObject$Object$t;

extern Std$Type$t Gir$GObject$Object$T[];
extern Gir$GObject$Object$t Gir$GObject$Object$Nil[];

#undef RIVA_MODULE
    
#endif