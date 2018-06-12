#include "Prologue.cpp"

#include <boost/math/distributions/poisson.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

struct dist_t {
	const Std$Type_t *Type;
	poisson Dist;
	dist_t(double M) : Dist(M) {Type = T;};
};

GLOBAL_FUNCTION(New, 1) {
//@lambda:Std$Real$T
//:T
	double M = ((Std$Real_t *)Args[0].Val)->Value;
	dist_t *Dist = new dist_t(M);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

#include "Epilogue.cpp"

