#include "Prologue.cpp"

#include <math.h>

#include <boost/math/distributions/lognormal.hpp>
#include <boost/random/lognormal_distribution.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type$t *Type;
	lognormal Dist;
	boost::lognormal_distribution<double> Rand;
	dist_t(double Mean, double StdDev, double Location, double Scale) : Dist(Location, Scale), Rand(Mean, StdDev) {Type = T;};
};

GLOBAL_FUNCTION(New, 0) {
//@mean:Std$Real$T=0.0
//@variance:Std$Real$T=1.0
//:T
	double Mean = Count > 0 ? ((Std$Real$t *)Args[0].Val)->Value : 1.0;
	double StdDev = Count > 1 ? ((Std$Real$t *)Args[1].Val)->Value : 1.0;
	double Scale = sqrt(log((StdDev * StdDev / Mean / Mean) + 1));
	double Location = log(Mean) - Scale * Scale / 2.0;
	dist_t *Dist = new dist_t(Mean, StdDev, Location, Scale);
	Result->Val = (Std$Object$t *)Dist;
	return SUCCESS;
};

CONSTANT(Default, T) {
	dist_t *Dist = new dist_t(1.0, 1.0, -0.5, 1.0);
	return (Std$Object$t *)Dist;
};

#include "Epilogue.cpp"

