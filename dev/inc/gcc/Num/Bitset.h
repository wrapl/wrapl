#ifndef NUM_BITSET_H
#define NUM_BITSET_H

#include <Std/Type.h>

#define RIVA_MODULE Num$Bitset
#include <Riva-Header.h>

#include "roaring.h"

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	roaring_bitmap_t *Value;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
