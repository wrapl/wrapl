#ifdef DOCUMENTING
PUSHFILE("Epilogue.cpp");
#else
#line 1000
#endif

METHOD("pdf", TYP, T, TYP, Std$Real$T) {
//@dist
//@x
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	double X = ((Std$Real_t *)Args[1].Val)->Value;
	Result->Val = Std$Real$new(pdf(Dist->Dist, X));
	return SUCCESS;
};

METHOD("cdf", TYP, T, TYP, Std$Real$T) {
//@dist
//@x
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	double X = ((Std$Real_t *)Args[1].Val)->Value;
	Result->Val = Std$Real$new(cdf(Dist->Dist, X));
	return SUCCESS;
};

METHOD("quantile", TYP, T, TYP, Std$Real$T) {
//@dist
//@prob
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	double X = ((Std$Real_t *)Args[1].Val)->Value;
	Result->Val = Std$Real$new(quantile(Dist->Dist, X));
	return SUCCESS;
};


METHOD("mean", TYP, T) {
//@dist
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(mean(Dist->Dist));
	return SUCCESS;
};

METHOD("stddev", TYP, T) {
//@dist
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(standard_deviation(Dist->Dist));
	return SUCCESS;
};

METHOD("variance", TYP, T) {
//@dist
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(variance(Dist->Dist));
	return SUCCESS;
};

METHOD("skewness", TYP, T) {
//@dist
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(skewness(Dist->Dist));
	return SUCCESS;
};

METHOD("kurtosis", TYP, T) {
//@dist
//:Std$Real$T
	dist_t *Dist = (dist_t *)Args[0].Val;
	Result->Val = Std$Real$new(kurtosis(Dist->Dist));
	return SUCCESS;
};

#ifdef HAS_RANDOM_GENERATOR

struct urng_t {
	typedef double result_type;

	Std$Object_t *Rand;
	urng_t(Std$Object_t *_Rand) : Rand(_Rand) {};
	inline bool has_fixed_range(void) const {return true;};
	inline double min_value(void) const {return 0.0;};
	inline double max_value(void) const {return 1.0;};
	inline double min(void) const {return 0.0;};
	inline double max(void) const {return 1.0;};
	inline double operator()(void) {return Math$Random$uniform01(Rand);};
};

METHOD("generate", TYP, Math$Random$T, TYP, T) {
//@generator
//@dist
//:Std$Real$T
	urng_t Urng(Args[0].Val);
	dist_t *Dist = (dist_t *)Args[1].Val;
	Result->Val = Std$Real$new(Dist->Rand.operator()<urng_t&>(Urng));
	return SUCCESS;
};

#else

METHOD("generate", TYP, Math$Random$T, TYP, T) {
//@generator
//@dist
//:Std$Real$T
	double Quantile = Math$Random$uniform01(Args[0].Val);
	dist_t *Dist = (dist_t *)Args[1].Val;
	Result->Val = Std$Real$new(quantile(Dist->Dist, Quantile));
	return SUCCESS;
};

#endif

#ifdef DOCUMENTING
POPFILE();
#endif
