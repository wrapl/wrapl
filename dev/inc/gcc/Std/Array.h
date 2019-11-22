#ifndef STD_ARRAY_H
#define STD_ARRAY_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Array
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	Std$Object$t **Values;
	Std$Integer$smallt Length;
};

RIVA_TYPE(T);
RIVA_OBJECT(New);
RIVA_OBJECT(Alloc);

RIVA_CFUN(Std$Object$t *, new, int) __attribute__ ((malloc));

#undef RIVA_MODULE

#endif
