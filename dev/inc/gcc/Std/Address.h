#ifndef STD_ADDRESS_H
#define STD_ADDRESS_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Address
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	char *Value;
};

RIVA_STRUCT(constt) {
	const Std$Type_t *Type;
	const char *Value;
};

RIVA_TYPE(T);

RIVA_CFUN(Std$Object_t *, new, const char *) __attribute__ ((malloc));

#define Std$Address$get_value(A) ((Std$Address_t *)A)->Value

#undef RIVA_MODULE

#endif
