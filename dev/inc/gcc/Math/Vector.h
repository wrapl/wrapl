#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H

#define RIVA_MODULE Math$Vector
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	Std$Integer_smallt Length;
	Std$Object_t *Entries[];
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
