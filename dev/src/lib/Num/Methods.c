#include <Std.h>
#include <Riva.h>
#include <Num/Array.h>
#include <Num/Bitset.h>
#include <Agg/List.h>

#include "roaring.c"

METHOD("shape", TYP, Num$Array$T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	Std$Object$t *Shape = Agg$List$new0();
	for (int I = 0; I < Array->Degree; ++I) {
		Agg$List$put(Shape, Std$Integer$new_small(Array->Dimensions[I].Size));
	}
	RETURN(Shape);
}

METHOD("degree", TYP, Num$Array$T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	RETURN(Std$Integer$new_small(Array->Degree));
}

METHOD("transpose", TYP, Num$Array$T) {
	Num$Array$t *Source = (Num$Array$t *)Args[0].Val;
	int Degree = Source->Degree;
	Num$Array$t *Target = Num$Array$new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) {
		Target->Dimensions[I] = Source->Dimensions[Degree - I - 1];
	}
	Target->Data = Source->Data;
	RETURN(Target);
}

static bool bitmap_select_iterator(uint32_t Index, roaring_bitmap_t **Bitmaps) {
	uint32_t Value;
	if (roaring_bitmap_select(Bitmaps[0], Index, &Value)) {
		roaring_bitmap_add(Bitmaps[1], Value);
	}
	return true;
}

METHOD("[]", TYP, Num$Array$T) {
	Num$Array$t *Source = (Num$Array$t *)Args[0].Val;
	if (Count - 1 > Source->Degree) SEND(Std$String$new("Too many indices"));
	Num$Array$dimension_t TargetDimensions[Source->Degree];
	Num$Array$dimension_t *TargetDimension = TargetDimensions;
	Num$Array$dimension_t *SourceDimension = Source->Dimensions;
	void *Data = Source->Data;
	for (int I = 1; I < Count; ++I) {
		Std$Object$t *Index = Args[I].Val;
		if (Index->Type == Std$Integer$SmallT) {
			int IndexValue = Std$Integer$get_small(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) FAIL;
			if (IndexValue >= SourceDimension->Size) FAIL;
			if (SourceDimension->Bitmap) {
				roaring_bitmap_select(SourceDimension->Bitmap, IndexValue, &IndexValue);
			}
			Data += SourceDimension->Stride * IndexValue;
		} else if (Index->Type == Num$Bitset$T) {
			roaring_bitmap_t *IndexValue = ((Num$Bitset$t *)Index)->Value;
			TargetDimension->Stride = SourceDimension->Stride;
			if (SourceDimension->Bitmap) {
				roaring_bitmap_t **Bitmaps = {SourceDimension->Bitmap, roaring_bitmap_create()};
				roaring_iterate(IndexValue, bitmap_select_iterator, Bitmaps);
				TargetDimension->Size = roaring_bitmap_get_cardinality(Bitmaps[0]);
				TargetDimension->Bitmap = Bitmaps[0];
			} else {
				TargetDimension->Size = roaring_bitmap_get_cardinality(IndexValue);
				TargetDimension->Bitmap = IndexValue;
			}
			++TargetDimension;
		} else if (Index == Std$Object$Nil) {
			*TargetDimension = *SourceDimension;
			++TargetDimension;
		} else {
			SEND(Std$String$new("Unknown index type"));
		}
		++SourceDimension;
	}
	for (int I = Count - 1; I < Source->Degree; ++I) {
		*TargetDimension = *SourceDimension;
		++TargetDimension;
		++SourceDimension;
	}
	int Degree = TargetDimension - TargetDimensions;
	Num$Array$t *Target = Num$Array$new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = TargetDimensions[I];
	Target->Data = Data;
	RETURN(Target);
}

#define UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, NAME, OP) \
\
static void NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(Num$Array$dimension_t *TargetDimension, void *TargetData, Num$Array$dimension_t *SourceDimension, void *SourceData) { \
	if (TargetDimension->Bitmap) { \
		if (SourceDimension->Bitmap) { \
			roaring_uint32_iterator_t TargetIterator[1]; \
			roaring_init_iterator(TargetDimension->Bitmap, TargetIterator); \
			roaring_uint32_iterator_t SourceIterator[1]; \
			roaring_init_iterator(SourceDimension->Bitmap, SourceIterator); \
			while (TargetIterator->has_value && SourceIterator->has_value) { \
				*(CTYPE1 *)(TargetData + (TargetIterator->current_value - 1) * TargetDimension->Stride) OP *(CTYPE2 *)(SourceData + (SourceIterator->current_value - 1) * SourceDimension->Stride); \
				roaring_advance_uint32_iterator(TargetIterator); \
				roaring_advance_uint32_iterator(SourceIterator); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			roaring_uint32_iterator_t TargetIterator[1]; \
			roaring_init_iterator(TargetDimension->Bitmap, TargetIterator); \
			while (TargetIterator->has_value) { \
				*(CTYPE1 *)(TargetData + (TargetIterator->current_value - 1) * TargetDimension->Stride) OP *(CTYPE2 *)SourceData; \
				roaring_advance_uint32_iterator(TargetIterator); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Bitmap) { \
			roaring_uint32_iterator_t SourceIterator[1]; \
			roaring_init_iterator(SourceDimension->Bitmap, SourceIterator); \
			while (SourceIterator->has_value) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)(SourceData + (SourceIterator->current_value - 1) * SourceDimension->Stride); \
				TargetData += TargetStride; \
				roaring_advance_uint32_iterator(SourceIterator); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = TargetDimension->Size; --I >= 0;) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)SourceData; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(Num$Array$dimension_t *TargetDimension, void *TargetData, int SourceDegree, Num$Array$dimension_t *SourceDimension, void *SourceData) { \
	if (SourceDegree == 0) return NAME ## _value_array0_ ## CTYPE1(TargetDimension, TargetData, *(CTYPE2 *)SourceData); \
	if (SourceDegree == 1) return NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDimension, SourceData); \
	if (TargetDimension->Bitmap) { \
		if (SourceDimension->Bitmap) { \
			roaring_uint32_iterator_t TargetIterator[1]; \
			roaring_init_iterator(TargetDimension->Bitmap, TargetIterator); \
			roaring_uint32_iterator_t SourceIterator[1]; \
			roaring_init_iterator(SourceDimension->Bitmap, SourceIterator); \
			while (TargetIterator->has_value && SourceIterator->has_value) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + (TargetIterator->current_value - 1) * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + (SourceIterator->current_value - 1) * SourceDimension->Stride); \
				roaring_advance_uint32_iterator(TargetIterator); \
				roaring_advance_uint32_iterator(SourceIterator); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			roaring_uint32_iterator_t TargetIterator[1]; \
			roaring_init_iterator(TargetDimension->Bitmap, TargetIterator); \
			while (TargetIterator->has_value) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + (TargetIterator->current_value - 1) * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData); \
				roaring_advance_uint32_iterator(TargetIterator); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Bitmap) { \
			roaring_uint32_iterator_t SourceIterator[1]; \
			roaring_init_iterator(SourceDimension->Bitmap, SourceIterator); \
			while (SourceIterator->has_value) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + (SourceIterator->current_value - 1) * SourceDimension->Stride); \
				TargetData += TargetStride; \
				roaring_advance_uint32_iterator(SourceIterator); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = TargetDimension->Size; --I >= 0;) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(int PrefixDegree, Num$Array$dimension_t *TargetDimension, void *TargetData, int SourceDegree, Num$Array$dimension_t *SourceDimension, void *SourceData) { \
	if (PrefixDegree == 0) return NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData); \
	if (TargetDimension->Bitmap) { \
		roaring_uint32_iterator_t TargetIterator[1]; \
		roaring_init_iterator(TargetDimension->Bitmap, TargetIterator); \
		while (TargetIterator->has_value) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData + (TargetIterator->current_value - 1) * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData); \
			roaring_advance_uint32_iterator(TargetIterator); \
		} \
	} else { \
		int Stride = TargetDimension->Stride; \
		for (int I = TargetDimension->Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData); \
			TargetData += Stride; \
		} \
	} \
} \
\
METHOD(#NAME, TYP, ATYPE1, TYP, ATYPE2) { \
	Num$Array$t *Target = (Num$Array$t *)Args[0].Val; \
	Num$Array$t *Source = (Num$Array$t *)Args[1].Val; \
	if (Source->Degree > Target->Degree) SEND(Std$String$new("Incompatible assignment")); \
	int PrefixDegree = Target->Degree - Source->Degree; \
	for (int I = 0; I < Source->Degree; ++I) { \
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) SEND(Std$String$new("Incompatible assignment")); \
	} \
	NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree, Target->Dimensions, Target->Data, Source->Degree, Source->Dimensions, Source->Data); \
	RETURN0; \
}

#define UPDATE_ARRAY_METHODS(ATYPE1, CTYPE1, ATYPE2, CTYPE2) \
\
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, set, =); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, add, +=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, sub, -=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, mul, *=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, div, /=);

#define UPDATE_METHOD(ATYPE, CTYPE, RFUNC, NAME, OP) \
\
static void NAME ## _value_array0_ ## CTYPE(Num$Array$dimension_t *Dimension, void *Data, CTYPE Value) { \
	Std$Object$t *String = LeftSquare; \
	if (Dimension->Bitmap) { \
		roaring_uint32_iterator_t Iterator[1]; \
		roaring_init_iterator(Dimension->Bitmap, Iterator); \
		while (Iterator->has_value) { \
			*(CTYPE *)(Data + (Iterator->current_value - 1) * Dimension->Stride) OP Value; \
			roaring_advance_uint32_iterator(Iterator); \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		for (int I = Dimension->Size; --I >= 0;) { \
			*(CTYPE *)Data OP Value; \
			Data += Stride; \
		} \
	} \
} \
\
static void NAME ## _value_array_ ## CTYPE(int Degree, Num$Array$dimension_t *Dimension, void *Data, CTYPE Value) { \
	if (Degree == 1) return set_value_array0_ ## CTYPE(Dimension, Data, Value); \
	int Stride = Dimension->Stride; \
	if (Dimension->Bitmap) { \
		roaring_uint32_iterator_t Iterator[1]; \
		roaring_init_iterator(Dimension->Bitmap, Iterator); \
		while (Iterator->has_value) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Iterator->current_value - 1) * Dimension->Stride, Value); \
			roaring_advance_uint32_iterator(Iterator); \
		} \
	} else { \
		for (int I = Dimension->Size; --I >= 0;) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Data, Value); \
			Data += Stride; \
		} \
	} \
} \
\
METHOD(#NAME, TYP, ATYPE, TYP, Std$Number$T) { \
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val; \
	CTYPE Value = RFUNC(Args[1].Val); \
	if (Array->Degree == 0) { \
		*(CTYPE *)Array->Data OP Value; \
	} else { \
		NAME ## _value_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Data, Value); \
	} \
	RETURN0; \
}

STRING(LeftSquare, "[");
STRING(RightSquare, "]");

#define METHODS(ATYPE, CTYPE, FORMAT, RFUNC) \
\
static Std$Object$t *to_string_array0_ ## CTYPE(Num$Array$dimension_t *Dimension, void *Data) { \
	Std$Object$t *String = LeftSquare; \
	if (Dimension->Bitmap) { \
		roaring_uint32_iterator_t Iterator[1]; \
		roaring_init_iterator(Dimension->Bitmap, Iterator); \
		if (Iterator->has_value) { \
			String = Std$String$add_format(String, FORMAT, *(CTYPE *)(Data + (Iterator->current_value - 1) * Dimension->Stride)); \
			while (roaring_advance_uint32_iterator(Iterator)) { \
				String = Std$String$add_format(String, ", "FORMAT, *(CTYPE *)(Data + (Iterator->current_value - 1) * Dimension->Stride)); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		String = Std$String$add_format(String, FORMAT, *(CTYPE *)Data); \
		Data += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			String = Std$String$add_format(String, ", "FORMAT, *(CTYPE *)Data); \
			Data += Stride; \
		} \
	} \
	String = Std$String$add(String, RightSquare); \
	return String; \
} \
\
static Std$Object$t *to_string_array_ ## CTYPE(int Degree, Num$Array$dimension_t *Dimension, void *Data) { \
	if (Degree == 1) return to_string_array0_ ## CTYPE(Dimension, Data); \
	Std$Object$t *String = LeftSquare; \
	int Stride = Dimension->Stride; \
	if (Dimension->Bitmap) { \
		roaring_uint32_iterator_t Iterator[1]; \
		roaring_init_iterator(Dimension->Bitmap, Iterator); \
		if (Iterator->has_value) { \
			String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Iterator->current_value - 1) * Dimension->Stride)); \
			while (roaring_advance_uint32_iterator(Iterator)) { \
				String = Std$String$add_chars(String, ", ", 2); \
				String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Iterator->current_value - 1) * Dimension->Stride)); \
			} \
		} \
	} else { \
		String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data)); \
		Data += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			String = Std$String$add_chars(String, ", ", 2); \
			String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data)); \
			Data += Stride; \
		} \
	} \
	String = Std$String$add(String, RightSquare); \
	return String; \
} \
\
METHOD("@", TYP, ATYPE, VAL, Std$String$T) { \
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val; \
	if (Array->Degree == 0) { \
		RETURN(Std$String$new_format(FORMAT, *(CTYPE *)Array->Data)); \
	} else { \
		RETURN(to_string_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Data)); \
	} \
} \
\
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, set, =); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, add, +=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, sub, -=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, mul, *=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, div, /=); \
\
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Int8T, int8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$UInt8T, uint8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Int16T, int16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$UInt16T, uint16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Int32T, int32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$UInt32T, uint32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Int64T, int64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$UInt64T, uint64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Float32T, float); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, Num$Array$Float64T, double); \
\
METHOD("copy", TYP, ATYPE) { \
	Num$Array$t *Source = (Num$Array$t *)Args[0].Val; \
	int Degree = Source->Degree; \
	Num$Array$t *Target = Num$Array$new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	Target->Data = Riva$Memory$alloc_atomic(DataSize); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Data, Degree, Source->Dimensions, Source->Data); \
	RETURN(Target); \
}

METHODS(Num$Array$Int8T, int8_t, "%d", Std$Integer$int);
METHODS(Num$Array$UInt8T, uint8_t, "%ud", Std$Integer$int);
METHODS(Num$Array$Int16T, int16_t, "%d", Std$Integer$int);
METHODS(Num$Array$UInt16T, uint16_t, "%ud", Std$Integer$int);
METHODS(Num$Array$Int32T, int32_t, "%d", Std$Integer$int);
METHODS(Num$Array$UInt32T, uint32_t, "%ud", Std$Integer$int);
METHODS(Num$Array$Int64T, int64_t, "%ld", Std$Integer$int);
METHODS(Num$Array$UInt64T, uint64_t, "%uld", Std$Integer$int);
METHODS(Num$Array$Float32T, float, "%f", Std$Real$double);
METHODS(Num$Array$Float64T, double, "%f", Std$Real$double);
