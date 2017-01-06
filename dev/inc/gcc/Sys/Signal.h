#ifndef SYS_SIGNAL_H
#define SYS_SIGNAL_H

#include <Std/Object.h>

#define RIVA_MODULE Sys$Signal
#include <Riva-Header.h>

#include <signal.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	int Value;
};

RIVA_STRUCT(set_t) {
	const Std$Type$t *Type;
	sigset_t Value[1];
};

RIVA_TYPE(T);
RIVA_TYPE(SetT);

#undef RIVA_MODULE

#endif
