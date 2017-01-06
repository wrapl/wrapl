#ifndef STD_ARRAY_H
#define STD_ARRAY_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Array
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	Std$Object_t **Values;
	Std$Integer_smallt Length;
};

RIVA_TYPE(T);
RIVA_OBJECT(New);
RIVA_OBJECT(Alloc);

RIVA_CFUN(Std$Object_t *, new, int) __attribute__ ((malloc));

#undef RIVA_MODULE

#endif
