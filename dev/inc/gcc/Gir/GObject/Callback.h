#ifndef GIR_GOBJECT_CALLBACK_H
#define GIR_GOBJECT_CALLBACK_H

#define RIVA_MODULE Gir$GObject$Callback
#include <Riva-Header.h>

typedef struct {
	Std$Function_ct _Base;
	GCallback _function;
} Gir$GObject$Callback$t;

extern Std$Type$t Gir$GObject$Callback$T[];

RIVA_CFUN(GCallback, new, int);
RIVA_CFUN(void, invoke, void);

#undef RIVA_MODULE
    
#endif