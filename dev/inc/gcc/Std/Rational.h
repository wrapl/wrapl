#ifndef STD_RATIONAL_H
#define STD_RATIONAL_H

#include <Std/Object.h>
#include <Std/Function.h>

#define RIVA_MODULE Std$Rational
#include <Riva-Header.h>

#include <gmp.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	mpq_t Value;
};

RIVA_TYPE(T);

RIVA_CFUN(Std$Object_t *, new, mpq_t) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_string, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_small_small, int, int) __attribute__ ((malloc));

#undef RIVA_MODULE

#endif
