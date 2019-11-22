#include <Std.h>
#include <Riva/Memory.h>
#include <Math/Vector.h>
#include "RTree.h"

struct rtree_search_fn {
	Std$Object$t *Fn;
	Std$Function$result Result;
	Std$Function$status Status;
	int operator()(Std$Object$t *Value) {
		Status = Std$Function$call(Fn, 1, &Result, Value, 0);
		return Status != MESSAGE;
	}
};

#define RTREE(DIM) \
	struct rtree_ ## DIM ## _t { \
		const Std$Type$t *Type; \
		RTree<Std$Object$t *, double, DIM> Value; \
	}; \
	\
	TYPE(T ## DIM, T); \
	\
	METHOD("insert", TYP, T ## DIM, TYP, Math$Vector$T, TYP, Math$Vector$T, ANY) { \
		rtree_ ## DIM ## _t *Tree = (rtree_ ## DIM ## _t *)Args[0].Val; \
		Math$Vector$t *MinVector = (Math$Vector$t *)Args[1].Val; \
		Math$Vector$t *MaxVector = (Math$Vector$t *)Args[2].Val; \
		if (MinVector->Length.Value != DIM) { \
			SEND(Std$String$new("Minimum vector dimensions does not math")); \
		} \
		if (MaxVector->Length.Value != DIM) { \
			SEND(Std$String$new("Maximum vector dimensions does not math")); \
		} \
		double MinReal[DIM]; \
		double MaxReal[DIM]; \
		for (int I = 0; I < DIM; ++I) { \
			MinReal[I] = Std$Real$double(MinVector->Entries[I]); \
			MaxReal[I] = Std$Real$double(MaxVector->Entries[I]); \
		} \
		Tree->Value.Insert(MinReal, MaxReal, Args[3].Val); \
		RETURN(Tree); \
	} \
	\
	METHOD("delete", TYP, T ## DIM, TYP, Math$Vector$T, TYP, Math$Vector$T, ANY) { \
		rtree_ ## DIM ## _t *Tree = (rtree_ ## DIM ## _t *)Args[0].Val; \
		Math$Vector$t *MinVector = (Math$Vector$t *)Args[1].Val; \
		Math$Vector$t *MaxVector = (Math$Vector$t *)Args[2].Val; \
		if (MinVector->Length.Value != DIM) { \
			SEND(Std$String$new("Minimum vector dimensions does not math")); \
		} \
		if (MaxVector->Length.Value != DIM) { \
			SEND(Std$String$new("Maximum vector dimensions does not math")); \
		} \
		double MinReal[DIM]; \
		double MaxReal[DIM]; \
		for (int I = 0; I < DIM; ++I) { \
			MinReal[I] = Std$Real$double(MinVector->Entries[I]); \
			MaxReal[I] = Std$Real$double(MaxVector->Entries[I]); \
		} \
		Tree->Value.Remove(MinReal, MaxReal, Args[3].Val); \
		RETURN(Tree); \
	} \
	\
	METHOD("clear", TYP, T ## DIM) { \
		rtree_ ## DIM ## _t *Tree = (rtree_ ## DIM ## _t *)Args[0].Val; \
		Tree->Value.RemoveAll(); \
		RETURN(Tree); \
	} \
	\
	METHOD("size", TYP, T ## DIM) { \
		rtree_ ## DIM ## _t *Tree = (rtree_ ## DIM ## _t *)Args[0].Val; \
		RETURN(Std$Integer$new_small(Tree->Value.Count())); \
	} \
	\
	METHOD("search", TYP, T ## DIM, TYP, Math$Vector$T, TYP, Math$Vector$T, ANY) { \
		rtree_ ## DIM ## _t *Tree = (rtree_ ## DIM ## _t *)Args[0].Val; \
		Math$Vector$t *MinVector = (Math$Vector$t *)Args[1].Val; \
		Math$Vector$t *MaxVector = (Math$Vector$t *)Args[2].Val; \
		if (MinVector->Length.Value != DIM) { \
			SEND(Std$String$new("Minimum vector dimensions does not math")); \
		} \
		if (MaxVector->Length.Value != DIM) { \
			SEND(Std$String$new("Maximum vector dimensions does not math")); \
		} \
		double MinReal[DIM]; \
		double MaxReal[DIM]; \
		for (int I = 0; I < DIM; ++I) { \
			MinReal[I] = Std$Real$double(MinVector->Entries[I]); \
			MaxReal[I] = Std$Real$double(MaxVector->Entries[I]); \
		} \
		rtree_search_fn SearchFn; \
		SearchFn.Fn = Args[3].Val; \
		Tree->Value.Search(MinReal, MaxReal, SearchFn); \
		if (SearchFn.Status == MESSAGE) SEND(SearchFn.Result.Val); \
		RETURN(Tree); \
	}


TYPE(T);

RTREE(2);
RTREE(3);
RTREE(4);
RTREE(5);
RTREE(6);
RTREE(7);
RTREE(8);
RTREE(9);
RTREE(10);
RTREE(11);
RTREE(12);
RTREE(13);
RTREE(14);
RTREE(15);
RTREE(16);
RTREE(17);
RTREE(18);
RTREE(19);
RTREE(20);

#define RTREE_NEW(DIM) \
	case DIM: { \
		rtree_ ## DIM ## _t *Tree = new rtree_ ## DIM ## _t; \
		Tree->Type = T ## DIM; \
		RETURN(Tree); \
	}

GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	int Dim = Std$Integer$get_small(Args[0].Val);
	switch (Dim) {
	RTREE_NEW(2);
	RTREE_NEW(3);
	RTREE_NEW(4);
	RTREE_NEW(5);
	RTREE_NEW(6);
	RTREE_NEW(7);
	RTREE_NEW(8);
	RTREE_NEW(9);
	RTREE_NEW(10);
	RTREE_NEW(11);
	RTREE_NEW(12);
	RTREE_NEW(13);
	RTREE_NEW(14);
	RTREE_NEW(15);
	RTREE_NEW(16);
	RTREE_NEW(17);
	RTREE_NEW(18);
	RTREE_NEW(19);
	RTREE_NEW(20);
	default: SEND(Std$String$new("Unsupported dimension count"));
	}
}
