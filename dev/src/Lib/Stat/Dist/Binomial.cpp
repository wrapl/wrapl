#include "Prologue.cpp"

#include <boost/math/distributions/binomial.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

struct dist_t {
	const Std$Type_t *Type;
	binomial Dist;
	dist_t(double N, double P) : Dist(N, P) {Type = T;};
};

GLOBAL_FUNCTION(New, 2) {
//@trials:Std$Real$T
//@probability:Std$Real@T
//:T
	Std$Object_t *Arg = Args[0].Val;
	double N;
	if (Arg->Type == Std$Integer$SmallT) {
		N = ((Std$Integer_smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		N = ((Std$Real_t *)Arg)->Value;
	} else {
		N = 0;
	};
	double P = ((Std$Real_t *)Args[1].Val)->Value;
	dist_t *Dist = new dist_t(N, P);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

METHOD("trials", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.trials());
	return SUCCESS;
};

METHOD("probability", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.success_fraction());
	return SUCCESS;
};

#include "Epilogue.cpp"

