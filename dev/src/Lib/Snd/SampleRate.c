#include <Std.h>
#include <Riva.h>
#include <Agg/Buffer.h>
#include <samplerate.h>

typedef struct converter_t {
	const Std$Type$t *Type;
	SRC_STATE *State;
	SRC_DATA Data[1];
	float Buffer[1024];
} converter_t;

TYPE(T);

Std$Integer$smallt ConverterSincBestQuality[1] = {{Std$Integer$SmallT, SRC_SINC_BEST_QUALITY}};
Std$Integer$smallt ConverterSincMediumQuality[1] = {{Std$Integer$SmallT, SRC_SINC_MEDIUM_QUALITY}};
Std$Integer$smallt ConverterSincFastest[1] = {{Std$Integer$SmallT, SRC_SINC_FASTEST}};
Std$Integer$smallt ConverterZeroOrderHold[1] = {{Std$Integer$SmallT, SRC_ZERO_ORDER_HOLD}};
Std$Integer$smallt ConverterLinear[1] = {{Std$Integer$SmallT, SRC_LINEAR}};

GLOBAL_FUNCTION(New, 3) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	int ConverterType = Std$Integer$get_small(Args[0].Val);
	int Channels = Std$Integer$get_small(Args[1].Val);
	double SrcRatio;
	if (Args[2].Val->Type == Std$Integer$SmallT) {
		SrcRatio = Std$Integer$get_small(Args[2].Val);
	} else if (Args[2].Val->Type == Std$Real$T) {
		SrcRatio = Std$Real$get_value(Args[3].Val);
	} else if (Args[2].Val->Type == Std$Rational$T) {
		SrcRatio = mpq_get_d(Std$Rational$get_value(Args[2].Val));
	}
	converter_t *Converter = new(converter_t);
	Converter->Type = T;
	Converter->Data->src_ratio = SrcRatio;
	Result->Val = (Std$Object$t *)Converter;
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int8$T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[1].Val;
	size_t OutputSize = 
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(0);
	Result->Val = Agg$Buffer$Float32$new(0, 0);
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int16$T) {
	
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int32$T) {
	
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Float32$T) {
	
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Float64$T) {
	
}
