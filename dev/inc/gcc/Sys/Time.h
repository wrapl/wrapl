#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <Std/Object.h>

#define RIVA_MODULE Sys$Time
#include <Riva-Header.h>

#include <time.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	time_t Value;
};

RIVA_STRUCT(precise_t) {
	const Std$Type$t *Type;
	struct timeval Value;
};

RIVA_TYPE(T);
RIVA_TYPE(PreciseT);

RIVA_CFUN(Sys$Time_t *, new, time_t);

#undef RIVA_MODULE

#endif
