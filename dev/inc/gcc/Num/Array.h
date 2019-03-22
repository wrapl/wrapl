#ifndef NUM_ARRAY_H
#define NUM_ARRAY_H

#include <Std/Type.h>

#define RIVA_MODULE Num$Array
#include <Riva-Header.h>

RIVA_STRUCT(dimension_t) {
	int Size, Stride;
	struct roaring_bitmap_s *Bitmap;
};

typedef enum {
	Num$Array$FORMAT_ANY,
	Num$Array$FORMAT_I8, Num$Array$FORMAT_U8,
	Num$Array$FORMAT_I16, Num$Array$FORMAT_U16,
	Num$Array$FORMAT_I32, Num$Array$FORMAT_U32,
	Num$Array$FORMAT_I64, Num$Array$FORMAT_U64,
	Num$Array$FORMAT_F32, Num$Array$FORMAT_F64
} Num$Array$format_t;

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	void *Data;
	int Degree, Format;
	Num$Array$dimension_t Dimensions[];
};

RIVA_TYPE(T);
RIVA_TYPE(AnyT);
RIVA_TYPE(Int8T);
RIVA_TYPE(Int16T);
RIVA_TYPE(Int32T);
RIVA_TYPE(Int64T);
RIVA_TYPE(UInt8T);
RIVA_TYPE(UInt16T);
RIVA_TYPE(UInt32T);
RIVA_TYPE(UInt64T);
RIVA_TYPE(Float32T);
RIVA_TYPE(Float64T);

RIVA_CFUN(Num$Array$t *, new, Num$Array$format_t Format, int Degree);

#undef RIVA_MODULE

#endif
