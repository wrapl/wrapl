#include "Prologue.cpp"

#include <boost/math/distributions/normal.hpp>
#include <boost/random/normal_distribution.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type_t *Type;
	normal Dist;
	boost::normal_distribution<double> Rand;
	dist_t(double Mean, double StdDev) : Dist(Mean, StdDev), Rand(Mean, StdDev) {Type = T;};
};

GLOBAL_FUNCTION(New, 0) {
//@mean:Std$Real$T=0.0
//@variance:Std$Real$T=1.0
//:T
	double Mean = Count > 0 ? ((Std$Real_t *)Args[0].Val)->Value : 0.0;
	double StdDev = Count > 1 ? ((Std$Real_t *)Args[1].Val)->Value : 1.0;
	dist_t *Dist = new dist_t(Mean, StdDev);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

CONSTANT(Default, T) {
	dist_t *Dist = new dist_t(0.0, 1.0);
	return (Std$Object_t *)Dist;
};

#include "Epilogue.cpp"

