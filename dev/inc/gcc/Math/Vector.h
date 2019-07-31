#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H

#define RIVA_MODULE Math$Vector
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	Std$Integer$smallt Length;
	Std$Object$t *Entries[];
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
