#include <Std.h>
#include <Riva.h>
#include <Num/Array.h>
#include <Num/Bitset.h>
#include <Num/Range.h>
#include <Agg/List.h>

METHOD("shape", TYP, Num$Array$T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	Std$Object$t *Shape = Agg$List$new0();
	for (int I = 0; I < Array->Degree; ++I) {
		Agg$List$put(Shape, Std$Integer$new_small(Array->Dimensions[I].Size));
	}
	RETURN(Shape);
}

METHOD("strides", TYP, Num$Array$T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	Std$Object$t *Strides = Agg$List$new0();
	for (int I = 0; I < Array->Degree; ++I) {
		Agg$List$put(Strides, Std$Integer$new_small(Array->Dimensions[I].Stride));
	}
	RETURN(Strides);
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

METHOD("permute", TYP, Num$Array$T, TYP, Agg$List$T) {
	Num$Array$t *Source = (Num$Array$t *)Args[0].Val;
	int Degree = Source->Degree;
	if (Agg$List$length(Args[1].Val) != Degree) SEND("List length must match degree");
	Num$Array$t *Target = Num$Array$new(Source->Format, Degree);
	Agg$List$node *Node = Agg$List$head(Args[1].Val);
	for (int I = 0; I < Degree; ++I) {
		if (Node->Value->Type != Std$Integer$SmallT) SEND("Invalid index");
		int J = Std$Integer$get_small(Node->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) SEND("Invalid index");
		Target->Dimensions[I] = Source->Dimensions[J - 1];
		Node = Node->Next;
	}
	Target->Data = Source->Data;
	RETURN(Target);
}

METHOD("[]", TYP, Num$Array$T) {
	Num$Array$t *Source = (Num$Array$t *)Args[0].Val;
	if (Count - 1 > Source->Degree) SEND(Std$String$new("Too many indices"));
	Num$Array$dimension_t TargetDimensions[Source->Degree];
	Num$Array$dimension_t *TargetDimension = TargetDimensions;
	Num$Array$dimension_t *SourceDimension = Source->Dimensions;
	void *Data = Source->Data;
	int Min, Max, Step;
	for (int I = 1; I < Count; ++I) {
		Std$Object$t *Index = Args[I].Val;
		if (Index->Type == Std$Integer$SmallT) {
			int IndexValue = Std$Integer$get_small(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) FAIL;
			if (IndexValue >= SourceDimension->Size) FAIL;
			if (SourceDimension->Indices) IndexValue = SourceDimension->Indices[IndexValue];
			Data += SourceDimension->Stride * IndexValue;
		} else if (Index->Type == Agg$List$T) {
			int Size = TargetDimension->Size = Agg$List$length(Index);
			// TODO: Check if list can be replaced by a range
			if (Size == 1) {
				Agg$List$node *Node = Agg$List$head(Index);
				if (Node->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Invalid index"));
				Max = Min = Std$Integer$get_small(Node->Value);
				Step = 1;
				goto as_range;
			} else if (Size == 2) {
				Agg$List$node *Node = Agg$List$head(Index);
				if (Node->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Invalid index"));
				Min = Std$Integer$get_small(Node->Value);
				Node = Node->Next;
				if (Node->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Invalid index"));
				Max = Std$Integer$get_small(Node->Value);
				Step = Max - Min;
				goto as_range;
			}
			int *Indices = TargetDimension->Indices = (int *)Riva$Memory$alloc_atomic(Size * sizeof(int));
			int *IndexPtr = Indices;
			for (Agg$List$node *Node = Agg$List$head(Index); Node; Node = Node->Next) {
				if (Node->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Invalid index"));
				int IndexValue = Std$Integer$get_small(Node->Value);
				if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
				if (--IndexValue < 0) FAIL;
				if (IndexValue >= SourceDimension->Size) FAIL;
				*IndexPtr++ = IndexValue;
			}
			TargetDimension->Stride = SourceDimension->Stride;
			++TargetDimension;
		} else if (Index->Type == Num$Range$T) {
			Num$Range$t *IndexValue = (Num$Range$t *)Index;
			Min = IndexValue->Min;
			Max = IndexValue->Max;
			Step = IndexValue->Step;
			if (Min <= 1) Min += SourceDimension->Size + 1;
			if (Max <= 1) Max += SourceDimension->Size + 1;
		as_range:
			if (--Min <= 0) FAIL;
			if (Min >= SourceDimension->Size) FAIL;
			if (--Max <= 0) FAIL;
			if (Max >= SourceDimension->Size) FAIL;
			if (Step == 0) FAIL;
			int Size = TargetDimension->Size = (IndexValue->Max - IndexValue->Min) / IndexValue->Step + 1;
			if (Size < 0) FAIL;
			TargetDimension->Stride = SourceDimension->Stride * IndexValue->Step;
			Data += SourceDimension->Stride * IndexValue->Min;
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
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)SourceData; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
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
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
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
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		for (int I = Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData); \
		} \
	} else { \
		int Stride = TargetDimension->Stride; \
		for (int I = Size; --I >= 0;) { \
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
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			*(CTYPE *)(Data + (Indices[I]) * Dimension->Stride) OP Value; \
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
	if (Degree == 1) return NAME ## _value_array0_ ## CTYPE(Dimension, Data, Value); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Indices[I]) * Dimension->Stride, Value); \
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

#define METHODS(ATYPE, CTYPE, FORMAT, RFUNC, RNEW) \
\
static Std$Object$t *to_string_array0_ ## CTYPE(Num$Array$dimension_t *Dimension, void *Data) { \
	Std$Object$t *String = LeftSquare; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			String = Std$String$add_format(String, FORMAT, *(CTYPE *)(Data + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				String = Std$String$add_format(String, ", "FORMAT, *(CTYPE *)(Data + (Indices[I]) * Dimension->Stride)); \
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
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				String = Std$String$add_chars(String, ", ", 2); \
				String = Std$String$add(String, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Data + (Indices[I]) * Dimension->Stride)); \
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
AMETHOD(Std$String$Of, TYP, ATYPE) { \
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
} \
\
METHOD("get", TYP, ATYPE) { \
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val; \
	RETURN(RNEW(*(CTYPE *)Array->Data)); \
} \

void partial_sums_int32_t(int Degree, Num$Array$dimension_t *Dimension, void *Data, int32_t *Sums) {
	if (Degree == 0) {
		*(int32_t *)Data = Sums[0];
	} else {
		Sums[Degree - 1] = Sums[Degree];
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			void *Data2 = Data;
			for (int I = 0; I < Dimension->Size; ++I) {
				Data2 = ((char *)Data + Indices[I] * Dimension->Stride);
				partial_sums_int32_t(Degree - 1, Dimension + 1, Data2, Sums);
				Sums[Degree - 1] += *(int32_t *)Data2;
			}
		} else {
			for (int I = 0; I < Dimension->Size; ++I) {
				partial_sums_int32_t(Degree - 1, Dimension + 1, Data, Sums);
				Data += Dimension->Stride;
				Sums[Degree - 1] += *(int32_t *)Data;
			}
		}
	}
}

METHOD("partial_sums", TYP, Num$Array$Int32T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	int32_t Sums[Array->Degree + 1];
	Sums[Array->Degree] = 0;
	partial_sums_int32_t(Array->Degree, Array->Dimensions, Array->Data, Sums);
	RETURN0;
}

METHODS(Num$Array$Int8T, int8_t, "%d", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$UInt8T, uint8_t, "%ud", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$Int16T, int16_t, "%d", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$UInt16T, uint16_t, "%ud", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$Int32T, int32_t, "%d", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$UInt32T, uint32_t, "%ud", Std$Integer$int, Std$Integer$new_small);
METHODS(Num$Array$Int64T, int64_t, "%ld", Std$Integer$int, Std$Integer$new_s64);
METHODS(Num$Array$UInt64T, uint64_t, "%uld", Std$Integer$int, Std$Integer$new_u64);
METHODS(Num$Array$Float32T, float, "%f", Std$Real$double, Std$Real$new);
METHODS(Num$Array$Float64T, double, "%f", Std$Real$double, Std$Real$new);
