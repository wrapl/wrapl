#include <Std.h>
#include <Riva.h>
#include <Agg/Buffer.h>
#include <samplerate.h>
#include <math.h>

#define BUFFER_SIZE 1024

typedef struct converter_t {
	const Std$Type$t *Type;
	SRC_STATE *State;
	SRC_DATA Data[1];
	int CachedSamples;
	float Buffer[BUFFER_SIZE];
} converter_t;

TYPE(T);

Std$Integer$smallt TypeSincBestQuality[1] = {{Std$Integer$SmallT, SRC_SINC_BEST_QUALITY}};
Std$Integer$smallt TypeSincMediumQuality[1] = {{Std$Integer$SmallT, SRC_SINC_MEDIUM_QUALITY}};
Std$Integer$smallt TypeSincFastest[1] = {{Std$Integer$SmallT, SRC_SINC_FASTEST}};
Std$Integer$smallt TypeZeroOrderHold[1] = {{Std$Integer$SmallT, SRC_ZERO_ORDER_HOLD}};
Std$Integer$smallt TypeLinear[1] = {{Std$Integer$SmallT, SRC_LINEAR}};

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
	int Error;
	Converter->State = src_new(ConverterType, Channels, &Error);
	if (!Converter->State) {
		Result->Val = Std$String$new(src_strerror(Error));
		return MESSAGE;
	}
	Converter->Data->data_in = Converter->Buffer;
	Converter->Data->src_ratio = SrcRatio;
	Result->Val = (Std$Object$t *)Converter;
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int16$T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[1].Val;
	int OutputFrames = ceil(Input->Length.Value * Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	int InputSpace = BUFFER_SIZE - Converter->CachedSamples;
	int InputFrames = Input->Length.Value;
	Converter->Data->end_of_input = 0;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const short *InputSamples = (short *)Input->Value;
	while (InputFrames >= InputSpace) {
		src_short_to_float_array(InputSamples, InputBuffer, InputSpace);
		Converter->Data->input_frames = InputSpace;
		int Error = src_process(Converter->State, Converter->Data);
		if (Error) {
			Result->Val = Std$String$new(src_strerror(Error));
			return MESSAGE;
		}
		int Remaining = InputSpace - Converter->Data->input_frames_used;
		if (Remaining) {
			float *Source = InputBuffer + Converter->Data->input_frames_used;
			float *Dest = InputBuffer;
			InputSpace = BUFFER_SIZE - Remaining;
			while (--Remaining >= 0) *(Dest++) = *(Source++);
			InputBuffer = Dest;
		} else {
			InputBuffer = Converter->Buffer;
			InputSpace = BUFFER_SIZE;
		}
		InputSamples += InputSpace;
		InputFrames -= InputSpace;
		Converter->Data->data_out += Converter->Data->output_frames_gen;
		Converter->Data->output_frames -= Converter->Data->output_frames_gen;
	}
	if (InputFrames) {
		src_short_to_float_array(InputSamples, InputBuffer, InputFrames);
		Converter->Data->input_frames = InputFrames;
		int Error = src_process(Converter->State, Converter->Data);
		if (Error) {
			Result->Val = Std$String$new(src_strerror(Error));
			return MESSAGE;
		}
		int Remaining = InputFrames - Converter->Data->input_frames_used;
		if (Remaining) {
			float *Source = InputBuffer + Converter->Data->input_frames_used;
			float *Dest = InputBuffer;
			Converter->CachedSamples = Remaining;
			while (--Remaining >= 0) *(Dest++) = *(Source++);
		} else {
			Converter->CachedSamples = 0;
		}
		Converter->Data->data_out += Converter->Data->output_frames_gen;
		Converter->Data->output_frames -= Converter->Data->output_frames_gen;
	}
	Result->Val = Agg$Buffer$Float32$new(OutputBuffer, Converter->Data->data_out - OutputBuffer);
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int32$T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[1].Val;
	int OutputFrames = ceil(Input->Length.Value * Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	int InputSpace = BUFFER_SIZE - Converter->CachedSamples;
	int InputFrames = Input->Length.Value;
	Converter->Data->end_of_input = 0;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const int *InputSamples = (int *)Input->Value;
	while (InputFrames >= InputSpace) {
		src_int_to_float_array(InputSamples, InputBuffer, InputSpace);
		Converter->Data->input_frames = InputSpace;
		int Error = src_process(Converter->State, Converter->Data);
		if (Error) {
			Result->Val = Std$String$new(src_strerror(Error));
			return MESSAGE;
		}
		int Remaining = InputSpace - Converter->Data->input_frames_used;
		if (Remaining) {
			float *Source = InputBuffer + Converter->Data->input_frames_used;
			float *Dest = InputBuffer;
			InputSpace = BUFFER_SIZE - Remaining;
			while (--Remaining >= 0) *(Dest++) = *(Source++);
			InputBuffer = Dest;
		} else {
			InputBuffer = Converter->Buffer;
			InputSpace = BUFFER_SIZE;
		}
		InputSamples += InputSpace;
		InputFrames -= InputSpace;
		Converter->Data->data_out += Converter->Data->output_frames_gen;
		Converter->Data->output_frames -= Converter->Data->output_frames_gen;
	}
	if (InputFrames) {
		src_int_to_float_array(InputSamples, InputBuffer, InputFrames);
		Converter->Data->input_frames = InputFrames;
		int Error = src_process(Converter->State, Converter->Data);
		if (Error) {
			Result->Val = Std$String$new(src_strerror(Error));
			return MESSAGE;
		}
		int Remaining = InputFrames - Converter->Data->input_frames_used;
		if (Remaining) {
			float *Source = InputBuffer + Converter->Data->input_frames_used;
			float *Dest = InputBuffer;
			Converter->CachedSamples = Remaining;
			while (--Remaining >= 0) *(Dest++) = *(Source++);
		} else {
			Converter->CachedSamples = 0;
		}
		Converter->Data->data_out += Converter->Data->output_frames_gen;
		Converter->Data->output_frames -= Converter->Data->output_frames_gen;
	}
	Result->Val = Agg$Buffer$Float32$new(OutputBuffer, Converter->Data->data_out - OutputBuffer);
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Float32$T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[1].Val;
	int OutputFrames = ceil(Input->Length.Value / Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	int InputFrames = Input->Length.Value;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const float *InputSamples = (float *)Input->Value;
	src_process(Converter->State, Converter->Data);
	Result->Val = Agg$Buffer$Float32$new(OutputBuffer, Converter->Data->data_out - OutputBuffer);
	return SUCCESS;
}

ASYMBOL(Convert);

AMETHOD(Convert, TYP, Agg$Buffer$Int16$T, TYP, Agg$Buffer$Float32$T) {
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[0].Val;
	Agg$Buffer$t *Output = (Agg$Buffer$t *)Args[1].Val;
	if (Input->Length.Value != Output->Length.Value) {
		Result->Val = Std$String$new("Buffer lengths do not match");
		return MESSAGE;
	}
	src_short_to_float_array(Input->Value, Output->Value, Input->Length.Value);
	Result->Arg = Args[1];
	return SUCCESS;
}

AMETHOD(Convert, TYP, Agg$Buffer$Int32$T, TYP, Agg$Buffer$Float32$T) {
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[0].Val;
	Agg$Buffer$t *Output = (Agg$Buffer$t *)Args[1].Val;
	if (Input->Length.Value != Output->Length.Value) {
		Result->Val = Std$String$new("Buffer lengths do not match");
		return MESSAGE;
	}
	src_int_to_float_array(Input->Value, Output->Value, Input->Length.Value);
	Result->Arg = Args[1];
	return SUCCESS;
}

AMETHOD(Convert, TYP, Agg$Buffer$Float32$T, TYP, Agg$Buffer$Int16$T) {
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[0].Val;
	Agg$Buffer$t *Output = (Agg$Buffer$t *)Args[1].Val;
	if (Input->Length.Value != Output->Length.Value) {
		Result->Val = Std$String$new("Buffer lengths do not match");
		return MESSAGE;
	}
	src_float_to_short_array(Input->Value, Output->Value, Input->Length.Value);
	Result->Arg = Args[1];
	return SUCCESS;
}

AMETHOD(Convert, TYP, Agg$Buffer$Float32$T, TYP, Agg$Buffer$Int32$T) {
	Agg$Buffer$t *Input = (Agg$Buffer$t *)Args[0].Val;
	Agg$Buffer$t *Output = (Agg$Buffer$t *)Args[1].Val;
	if (Input->Length.Value != Output->Length.Value) {
		Result->Val = Std$String$new("Buffer lengths do not match");
		return MESSAGE;
	}
	src_float_to_int_array(Input->Value, Output->Value, Input->Length.Value);
	Result->Arg = Args[1];
	return SUCCESS;
}
