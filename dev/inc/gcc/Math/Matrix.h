#ifndef MATH_MATRIX_H
#define MATH_MATRIX_H

#define RIVA_MODULE Math$Matrix
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	Std$Integer$smallt NoOfRows, NoOfCols;
	Std$Object$t *Entries[];
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
