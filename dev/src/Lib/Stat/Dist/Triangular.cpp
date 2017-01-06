#include "Prologue.cpp"

#include <boost/math/distributions/triangular.hpp>
#include <boost/random/triangle_distribution.hpp>
using namespace boost::math;

void *__dso_handle = &__dso_handle;

TYPE(T, Stat$Dist$T);

#define HAS_RANDOM_GENERATOR

struct dist_t {
	const Std$Type_t *Type;
	triangular Dist;
	boost::triangle_distribution<double> Rand;
	dist_t(double Lower, double Mode, double Upper) : Dist(Lower, Mode, Upper), Rand(Lower, Mode, Upper) {Type = T;};
};

GLOBAL_FUNCTION(New, 3) {
//@Lower:Std$Real$T
//@Mode:Std$Real$T
//@Upper:Std$Real$T
//:T
	double Lower = 0.0, Mode = 0.0, Upper = 1.0;
	Std$Object_t *Arg = Args[0].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Lower = ((Std$Integer_smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Lower = ((Std$Real_t *)Arg)->Value;
	};
	Arg = Args[1].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Mode = ((Std$Integer_smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Mode = ((Std$Real_t *)Arg)->Value;
	};
	Arg = Args[2].Val;
	if (Arg->Type == Std$Integer$SmallT) {
		Upper = ((Std$Integer_smallt *)Arg)->Value;
	} else if (Arg->Type == Std$Real$T) {
		Upper = ((Std$Real_t *)Arg)->Value;
	};
	dist_t *Dist = new dist_t(Lower, Mode, Upper);
	Result->Val = (Std$Object_t *)Dist;
	return SUCCESS;
};

METHOD("lower", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.lower());
	return SUCCESS;
};

METHOD("mode", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.mode());
	return SUCCESS;
};

METHOD("upper", TYP, T) {
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(Dist->Dist.upper());
	return SUCCESS;
};

#include "Epilogue.cpp"

