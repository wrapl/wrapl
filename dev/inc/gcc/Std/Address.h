#ifndef STD_ADDRESS_H
#define STD_ADDRESS_H

#include <Std/Object.h>
#include <Std/Integer.h>

#define RIVA_MODULE Std$Address
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	void *Value;
	Std$Integer$smallt Length;
};

RIVA_TYPE(T);

RIVA_CFUN(Std$Object$t *, new, void *, size_t) __attribute__ ((malloc));

RIVA_OBJECT(Of);

#define Std$Address$get_value(A) ((Std$Address$t *)A)->Value
#define Std$Address$get_size(A) ((Std$Address$t *)A)->Length.Value


#undef RIVA_MODULE

#endif
