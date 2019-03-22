#ifndef NUM_RANGE_H
#define NUM_RANGE_H

#include <Std/Type.h>

#define RIVA_MODULE Num$Range
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	int Min, Max, Step;
};

RIVA_CFUN(Num$Range$t *, new, int, int, int);

#undef RIVA_MODULE

#endif
