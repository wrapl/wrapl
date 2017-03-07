#ifndef STD_BUFFER_H
#define STD_BUFFER_H

#include <Std/Object.h>
#include <Std/Integer.h>
#include <stddef.h>

#define RIVA_MODULE Std$Buffer
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	void *Value;
	Std$Integer$smallt Length;
};

RIVA_STRUCT(constt) {
	const Std$Type_t *Type;
	const void *Value;
	const Std$Integer$smallt Length;
};

RIVA_TYPE(T);

RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));

#define Std$Buffer$get_value(A) ((Std$Buffer$t *)A)->Value
#define Std$Buffer$get_length(A) ((Std$Buffer$t *)A)->Length.Value

#undef RIVA_MODULE

#endif
