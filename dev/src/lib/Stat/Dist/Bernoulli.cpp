#include "Prologue.cpp"

#include <boost/math/distributions/bernoulli.hpp>
#include <boost/random/bernoulli_distribution.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type_t *Type;
	bernoulli Dist;
	boost::bernoulli_distribution<double> Rand;
	dist_t(double P) : Dist(P), Rand(P) {Type = T;};
};

GLOBAL_FUNCTION(New, 1) {
//@probability:Std$Real@T
//:T
	double P = ((Std$Real_t *)Args[0].Val)->Value;
	dist_t *Dist = new dist_t(P);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

METHOD("probability", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.success_fraction());
	return SUCCESS;
};

#include "Epilogue.cpp"

