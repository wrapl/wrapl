#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <Num/Range.h>

TYPE(T);

Num$Range$t *_new(int Min, int Max, int Step) {
	Num$Range$t *Range = new(Num$Range$t);
	Range->Type = T;
	Range->Min = Min;
	Range->Max = Max;
	Range->Step = Step;
	return Range;
}

GLOBAL_FUNCTION(New, 3) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(2, Std$Integer$SmallT);
	Num$Range$t *Range = new(Num$Range$t);
	Range->Type = T;
	Range->Min = Std$Integer$get_small(Args[0].Val);
	Range->Max = Std$Integer$get_small(Args[1].Val);
	Range->Step = Std$Integer$get_small(Args[2].Val);
	RETURN(Range);
}

AMETHOD(Std$String$Of, TYP, T) {
	Num$Range$t *Range = (Num$Range$t *)Args[0].Val;
	RETURN(Std$String$new_format("%d, %d, ... %d", Range->Min, Range->Min + Range->Step, Range->Max));
}
