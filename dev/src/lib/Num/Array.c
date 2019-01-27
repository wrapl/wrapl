#include <Std.h>
#include <Riva.h>
#include <Num/Array.h>

TYPE(T);

TYPE(AnyT, T);
TYPE(Int8T, T);
TYPE(Int16T, T);
TYPE(Int32T, T);
TYPE(Int64T, T);
TYPE(UInt8T, T);
TYPE(UInt16T, T);
TYPE(UInt32T, T);
TYPE(UInt64T, T);
TYPE(Float32T, T);
TYPE(Float64T, T);

Num$Array$t *_new(Num$Array$format_t Format, int Degree) {
	static const Std$Type$t *Types[] = {
		[Num$Array$FORMAT_ANY] = AnyT,
		[Num$Array$FORMAT_I8] = Int8T,
		[Num$Array$FORMAT_U8] = UInt8T,
		[Num$Array$FORMAT_I16] = Int16T,
		[Num$Array$FORMAT_U16] = UInt16T,
		[Num$Array$FORMAT_I32] = Int32T,
		[Num$Array$FORMAT_U32] = UInt32T,
		[Num$Array$FORMAT_I64] = Int64T,
		[Num$Array$FORMAT_U64] = UInt64T,
		[Num$Array$FORMAT_F32] = Float32T,
		[Num$Array$FORMAT_F64] = Float64T
	};
	Num$Array$t *Array = Riva$Memory$alloc(sizeof(Num$Array$t) + Degree * sizeof(Num$Array$dimension_t));
	Array->Type = Types[Format];
	Array->Degree = Degree;
	Array->Format = Format;
	return Array;
}

GLOBAL_FUNCTION(New, 2) {
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	Num$Array$format_t Format;
	int ItemSize;
	if (Args[0].Val == AnyT) {
		Format = Num$Array$FORMAT_ANY;
		ItemSize = sizeof(Std$Object$t *);
	} else if (Args[0].Val == Int8T) {
		Format = Num$Array$FORMAT_I8;
		ItemSize = 1;
	} else if (Args[0].Val == UInt8T) {
		Format = Num$Array$FORMAT_U8;
		ItemSize = 1;
	} else if (Args[0].Val == Int16T) {
		Format = Num$Array$FORMAT_I16;
		ItemSize = 2;
	} else if (Args[0].Val == UInt16T) {
		Format = Num$Array$FORMAT_U16;
		ItemSize = 2;
	} else if (Args[0].Val == Int32T) {
		Format = Num$Array$FORMAT_I32;
		ItemSize = 4;
	} else if (Args[0].Val == UInt32T) {
		Format = Num$Array$FORMAT_U32;
		ItemSize = 4;
	} else if (Args[0].Val == Int64T) {
		Format = Num$Array$FORMAT_I64;
		ItemSize = 8;
	} else if (Args[0].Val == UInt64T) {
		Format = Num$Array$FORMAT_U64;
		ItemSize = 8;
	} else if (Args[0].Val == Float32T) {
		Format = Num$Array$FORMAT_F32;
		ItemSize = 4;
	} else if (Args[0].Val == Float64T) {
		Format = Num$Array$FORMAT_F64;
		ItemSize = 8;
	} else {
		SEND(Std$String$new("Unknown type for array"));
	}
	int Degree = Std$Integer$get_small(Args[1].Val);
	Num$Array$t *Array = _new(Format, Degree);
	int DataSize = ItemSize;
	for (int I = Degree; --I >= 0;) {
		if (Count <= I + 2) SEND(Std$String$new("Not enough dimensions"));
		CHECK_EXACT_ARG_TYPE(I + 2, Std$Integer$SmallT);
		Array->Dimensions[I].Stride = DataSize;
		int Size = Array->Dimensions[I].Size = Std$Integer$get_small(Args[I + 2].Val);
		DataSize *= Size;
	}
	Array->Data = Riva$Memory$alloc_atomic(DataSize);
	RETURN(Array);
}

