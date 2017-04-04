#ifndef AGG_BUFFER_H
#define AGG_BUFFER_H

#include <Std/Object.h>
#include <Std/Integer.h>
#include <stddef.h>

#define RIVA_MODULE Agg$Buffer
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

RIVA_TYPE(Int8$T);
RIVA_TYPE(Int16$T);
RIVA_TYPE(Int32$T);
RIVA_TYPE(Float32$T);
RIVA_TYPE(Float64$T);

RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));

#define Agg$Buffer$get_value(A) ((Agg$Buffer$t *)A)->Value
#define Agg$Buffer$get_length(A) ((Agg$Buffer$t *)A)->Length.Value

#undef RIVA_MODULE

#define RIVA_MODULE Agg$Buffer$Int8
RIVA_TYPE(T);
RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));
#undef RIVA_MODULE

#define RIVA_MODULE Agg$Buffer$Int16
RIVA_TYPE(T);
RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));
#undef RIVA_MODULE

#define RIVA_MODULE Agg$Buffer$Int32
RIVA_TYPE(T);
RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));
#undef RIVA_MODULE

#define RIVA_MODULE Agg$Buffer$Float32
RIVA_TYPE(T);
RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));
#undef RIVA_MODULE

#define RIVA_MODULE Agg$Buffer$Float64
RIVA_TYPE(T);
RIVA_CFUN(Std$Object_t *, new, void *, size_t) __attribute__ ((malloc));
#undef RIVA_MODULE

#endif
