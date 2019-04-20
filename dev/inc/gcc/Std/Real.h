#ifndef STD_REAL_H
#define STD_REAL_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Real
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	double Value;
};

RIVA_TYPE(T);

RIVA_OBJECT(Zero);
RIVA_OBJECT(One);
RIVA_OBJECT(From);

RIVA_CFUN(Std$Object_t *, new, double) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_string, const char *) __attribute__ ((malloc));

#define Std$Real$get_value(A) ((Std$Real_t *)A)->Value

RIVA_CFUN(double, double, Std$Object$t *);

#undef RIVA_MODULE

#endif
