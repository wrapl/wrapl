#ifndef STD_ADDRESS_H
#define STD_ADDRESS_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Address
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	void *Value;
};

RIVA_TYPE(T);

RIVA_CFUN(Std$Object$t *, new, void *) __attribute__ ((malloc));

#define Std$Address$get_value(A) ((Std$Address$t *)A)->Value

#undef RIVA_MODULE

#endif
