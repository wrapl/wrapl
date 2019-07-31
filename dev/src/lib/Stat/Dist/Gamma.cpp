#include "Prologue.cpp"

#include <boost/math/distributions/gamma.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

struct dist_t {
	const Std$Type$t *Type;
	gamma_distribution<double> Dist;
	dist_t(double Shape, double Scale) : Dist(Shape, Scale) {Type = T;};
};

GLOBAL_FUNCTION(New, 2) {
//@lambda:Std$Real$T
//:T
	double Shape = ((Std$Real$t *)Args[0].Val)->Value;
	double Scale = ((Std$Real$t *)Args[1].Val)->Value;
	dist_t *Dist = new dist_t(Shape, Scale);
	Result->Val = (Std$Object$t *)Dist;
	return SUCCESS;
};

METHOD("shape", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.shape());
	return SUCCESS;
};

METHOD("scale", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.scale());
	return SUCCESS;
};

#include "Epilogue.cpp"

