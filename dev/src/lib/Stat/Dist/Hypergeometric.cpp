#include "Prologue.cpp"

#include <boost/math/distributions/hypergeometric.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

struct dist_t {
	const Std$Type$t *Type;
	hypergeometric Dist;
	dist_t(int Total, int Marked, int Sample) : Dist(Marked, Sample, Total) {Type = T;};
};

GLOBAL_FUNCTION(New, 3) {
//@Total:Std$Real$T
//@Marked:Std$Real$T
//@Sample:Std$Real$T
//:T
	int Total = 0, Marked = 0, Sample = 0;
	Std$Object$t *Arg = Args[0].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Total = ((Std$Integer$smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Total = ((Std$Real$t *)Arg)->Value;
	};
	Arg = Args[1].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Marked = ((Std$Integer$smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Marked = ((Std$Real$t *)Arg)->Value;
	};
	Arg = Args[2].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Sample = ((Std$Integer$smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Sample = ((Std$Real$t *)Arg)->Value;
	};
	dist_t *Dist = new dist_t(Total, Marked, Sample);
	Result->Val = (Std$Object$t *)Dist;
	return SUCCESS;
};

METHOD("total", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.total());
	return SUCCESS;
};

METHOD("marked", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.defective());
	return SUCCESS;
};

METHOD("sample", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.sample_count());
	return SUCCESS;
};

#include "Epilogue.cpp"

