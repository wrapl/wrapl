#include "Prologue.cpp"

#include <boost/math/distributions/uniform.hpp>
#include <boost/random/uniform_real.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type_t *Type;
	uniform Dist;
	boost::uniform_real<double> Rand;
	dist_t(double Lower, double Upper) : Dist(Lower, Upper), Rand(Lower, Upper) {Type = T;};
};

GLOBAL_FUNCTION(New, 0) {
//@lower:Std$Real$T=0.0
//@upper:Std$Real$T=1.0
//:T
	double Lower = Count > 0 ? ((Std$Real_t *)Args[0].Val)->Value : 0.0;
	double Upper = Count > 1 ? ((Std$Real_t *)Args[1].Val)->Value : 1.0;
	dist_t *Dist = new dist_t(Lower, Upper);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

CONSTANT(Default, T) {
	dist_t *Dist = new dist_t(0.0, 1.0);
	return (Std$Object_t *)Dist;
};

METHOD("lower", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.lower());
	return SUCCESS;
};

METHOD("upper", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.upper());
	return SUCCESS;
};

#include "Epilogue.cpp"

