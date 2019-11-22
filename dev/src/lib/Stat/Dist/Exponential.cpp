#include "Prologue.cpp"

#include <boost/math/distributions/exponential.hpp>
#include <boost/random/exponential_distribution.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type$t *Type;
	exponential Dist;
	boost::exponential_distribution<double> Rand;
	dist_t(double L) : Dist(L), Rand(L) {Type = T;};
};

GLOBAL_FUNCTION(New, 1) {
//@lambda:Std$Real$T
//:T
	double L = ((Std$Real$t *)Args[0].Val)->Value;
	dist_t *Dist = new dist_t(L);
	Result->Val = (Std$Object$t *)Dist;
	return SUCCESS;
};

METHOD("lambda", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.lambda());
	return SUCCESS;
};

#include "Epilogue.cpp"

