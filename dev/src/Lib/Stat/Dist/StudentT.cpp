#include "Prologue.cpp"

#include <boost/math/distributions/students_t.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

struct dist_t {
	const Std$Type_t *Type;
	students_t Dist;
	dist_t(double V) : Dist(V) {Type = T;};
};

GLOBAL_FUNCTION(New, 1) {
//@degrees_of_freedom:Std$Real$T
//:T
	Std$Object_t *Arg = Args[0].Val;
	double V;
	if (Arg->Type == Std$Integer$SmallT) {
		V = ((Std$Integer_smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		V = ((Std$Real_t *)Arg)->Value;
	} else {
		V = 0;
	};
	dist_t *Dist = new dist_t(V);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

METHOD("degrees", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.degrees_of_freedom());
	return SUCCESS;
};

#include "Epilogue.cpp"

