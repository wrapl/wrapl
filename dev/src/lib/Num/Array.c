#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <Num/Array.h>

TYPE(T, Std$Address$T);

TYPE(AnyT, T, Std$Address$T);
TYPE(Int8T, T, Std$Address$T);
TYPE(Int16T, T, Std$Address$T);
TYPE(Int32T, T, Std$Address$T);
TYPE(Int64T, T, Std$Address$T);
TYPE(UInt8T, T, Std$Address$T);
TYPE(UInt16T, T, Std$Address$T);
TYPE(UInt32T, T, Std$Address$T);
TYPE(UInt64T, T, Std$Address$T);
TYPE(Float32T, T, Std$Address$T);
TYPE(Float64T, T, Std$Address$T);

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
	if (Count < 2) SEND(Std$String$new("Missing dimensions for array"));
	Num$Array$t *Array;
	if (Args[1].Val->Type == Agg$List$T) {
		int Degree = Agg$List$length(Args[1].Val);
		Array = _new(Format, Degree);
		Agg$List$node *Node = Agg$List$head(Args[1].Val);
		for (int I = 0; I < Degree; ++I, Node = Node->Next) {
			if (Node->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Dimension is not an integer"));
			Array->Dimensions[I].Size = Std$Integer$get_small(Node->Value);
		}
	} else {
		int Degree = Count - 1;
		Array = _new(Format, Degree);
		for (int I = 1; I < Count; ++I) {
			CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
			Array->Dimensions[I - 1].Size = Std$Integer$get_small(Args[I].Val);
		}
	}
	int DataSize = ItemSize;
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		DataSize *= Array->Dimensions[I].Size;
	}
	Array->Data = Riva$Memory$alloc_atomic(DataSize);
	RETURN(Array);
}

GLOBAL_FUNCTION(Wrap, 4) {
	CHECK_ARG_TYPE(1, Std$Address$T);
	CHECK_ARG_TYPE(2, Agg$List$T);
	CHECK_ARG_TYPE(3, Agg$List$T);
	Num$Array$format_t Format;
	if (Args[0].Val == AnyT) {
		Format = Num$Array$FORMAT_ANY;
	} else if (Args[0].Val == Int8T) {
		Format = Num$Array$FORMAT_I8;
	} else if (Args[0].Val == UInt8T) {
		Format = Num$Array$FORMAT_U8;
	} else if (Args[0].Val == Int16T) {
		Format = Num$Array$FORMAT_I16;
	} else if (Args[0].Val == UInt16T) {
		Format = Num$Array$FORMAT_U16;
	} else if (Args[0].Val == Int32T) {
		Format = Num$Array$FORMAT_I32;
	} else if (Args[0].Val == UInt32T) {
		Format = Num$Array$FORMAT_U32;
	} else if (Args[0].Val == Int64T) {
		Format = Num$Array$FORMAT_I64;
	} else if (Args[0].Val == UInt64T) {
		Format = Num$Array$FORMAT_U64;
	} else if (Args[0].Val == Float32T) {
		Format = Num$Array$FORMAT_F32;
	} else if (Args[0].Val == Float64T) {
		Format = Num$Array$FORMAT_F64;
	} else {
		SEND(Std$String$new("Unknown type for array"));
	}
	int Degree = Agg$List$length(Args[2].Val);
	if (Degree != Agg$List$length(Args[3].Val)) SEND(Std$String$new("Dimensions and strides must have same length"));
	Num$Array$t *Array = _new(Format, Degree);
	Agg$List$node *SizeNode = Agg$List$head(Args[2].Val);
	Agg$List$node *StrideNode = Agg$List$head(Args[3].Val);
	for (int I = 0; I < Degree; ++I) {
		if (SizeNode->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Dimension is not an integer"));
		if (StrideNode->Value->Type != Std$Integer$SmallT) SEND(Std$String$new("Stride is not an integer"));
		Array->Dimensions[I].Size = Std$Integer$get_small(SizeNode->Value);
		Array->Dimensions[I].Stride = Std$Integer$get_small(StrideNode->Value);
		SizeNode = SizeNode->Next;
		StrideNode = StrideNode->Next;
	}
	Array->Data = Std$Address$get_value(Args[1].Val);
	RETURN(Array);
}
