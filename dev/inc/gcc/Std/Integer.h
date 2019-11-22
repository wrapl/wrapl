#ifndef STD_INTEGER_H
#define STD_INTEGER_H

#include <Std/Object.h>
#include <Std/Function.h>

#define RIVA_MODULE Std$Integer
#include <Riva-Header.h>

#include <stdint.h>

RIVA_STRUCT(smallt) {
	const Std$Type$t *Type;
	int32_t Value;
};

#include <gmp.h>

RIVA_STRUCT(bigt) {
	const Std$Type$t *Type;
	mpz_t Value;
};

RIVA_TYPE(T);
RIVA_TYPE(SmallT);
RIVA_TYPE(BigT);

RIVA_OBJECT(Zero);
RIVA_OBJECT(One);
RIVA_OBJECT(Of);

RIVA_OBJECT(ToSmallSmall);

RIVA_CFUN(Std$Object$t *, new_small, int32_t) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_big, mpz_t) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_string, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_string_base, const char *, int) __attribute__ ((malloc));

RIVA_CFUN(Std$Object$t *, new_u64, uint64_t) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_s64, int64_t) __attribute__ ((malloc));
RIVA_CFUN(uint64_t, get_u64, Std$Object$t *) __attribute__ ((const));
RIVA_CFUN(int64_t, get_s64, Std$Object$t *) __attribute__ ((const));;

#define Std$Integer$get_small(A) ((Std$Integer$smallt *)A)->Value
#define Std$Integer$get_big(A) ((Std$Integer$bigt *)A)->Value

RIVA_CFUN(int, int, Std$Object$t *);

#undef RIVA_MODULE

#endif
