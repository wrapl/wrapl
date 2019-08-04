#include <Std.h>
#include <Riva.h>
#include <Num/Array.h>
#include <samplerate.h>
#include <math.h>

#define BUFFER_SIZE 1024

typedef struct converter_t {
	const Std$Type$t *Type;
	SRC_STATE *State;
	SRC_DATA Data[1];
	int CachedSamples, Channels;
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
	Converter->Channels = Channels;
	Result->Val = (Std$Object$t *)Converter;
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Num$Array$Int16T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Num$Array$t *Input = (Num$Array$t *)Args[1].Val;
	int Channels = Converter->Channels;
	if (Input->Degree == 1) {
		if (Channels != 1) SEND(Std$String$new("Invalid shape for sample rate conversion"));
		if (Input->Dimensions[0].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Input->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
	} else {
		SEND(Std$String$new("Invalid shape for sample rate conversion"));
	}
	int InputFrames = Input->Dimensions[0].Size;
	int OutputFrames = ceil(InputFrames * Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	int InputSpace = BUFFER_SIZE - Converter->CachedSamples;
	Converter->Data->end_of_input = 0;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const short *InputSamples = (short *)Input->Data;
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
	Num$Array$t *Output = Num$Array$new(Num$Array$FORMAT_F32, Input->Degree);
	Output->Data = OutputBuffer;
	int Samples = Converter->Data->data_out - OutputBuffer;
	Output->Dimensions[0].Size = Samples / Channels;
	Output->Dimensions[0].Stride = Channels * sizeof(float);
	if (Output->Degree > 1) {
		Output->Dimensions[1].Size = Channels;
		Output->Dimensions[1].Stride = sizeof(float);
	}
	RETURN(Output);
}

METHOD("process", TYP, T, TYP, Num$Array$Int32T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Num$Array$t *Input = (Num$Array$t *)Args[1].Val;
	int Channels = Converter->Channels;
	if (Input->Degree == 1) {
		if (Channels != 1) SEND(Std$String$new("Invalid shape for sample rate conversion"));
		if (Input->Dimensions[0].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Input->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
	} else {
		SEND(Std$String$new("Invalid shape for sample rate conversion"));
	}
	int InputFrames = Input->Dimensions[0].Size;
	int OutputFrames = ceil(InputFrames * Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	int InputSpace = BUFFER_SIZE - Converter->CachedSamples;
	Converter->Data->end_of_input = 0;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const int *InputSamples = (int *)Input->Data;
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
	Num$Array$t *Output = Num$Array$new(Num$Array$FORMAT_F32, Input->Degree);
	Output->Data = OutputBuffer;
	int Samples = Converter->Data->data_out - OutputBuffer;
	Output->Dimensions[0].Size = Samples / Channels;
	Output->Dimensions[0].Stride = Channels * sizeof(float);
	if (Output->Degree > 1) {
		Output->Dimensions[1].Size = Channels;
		Output->Dimensions[1].Stride = sizeof(float);
	}
	RETURN(Output);
}

METHOD("process", TYP, T, TYP, Num$Array$Float32T) {
	converter_t *Converter = (converter_t *)Args[0].Val;
	Num$Array$t *Input = (Num$Array$t *)Args[1].Val;
	int Channels = Converter->Channels;
	if (Input->Degree == 1) {
		if (Channels != 1) SEND(Std$String$new("Invalid shape for sample rate conversion"));
		if (Input->Dimensions[0].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Input->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
	} else {
		SEND(Std$String$new("Invalid shape for sample rate conversion"));
	}
	int InputFrames = Input->Dimensions[0].Size;
	int OutputFrames = ceil(InputFrames * Converter->Data->src_ratio);
	float *OutputBuffer = (float *)Riva$Memory$alloc_atomic(OutputFrames * sizeof(float));
	float *InputBuffer = Converter->Buffer + Converter->CachedSamples;
	Converter->Data->output_frames = OutputFrames;
	Converter->Data->data_out = OutputBuffer;
	const float *InputSamples = (float *)Input->Data;
	src_process(Converter->State, Converter->Data);
	Num$Array$t *Output = Num$Array$new(Num$Array$FORMAT_F32, Input->Degree);
	Output->Data = OutputBuffer;
	int Samples = Converter->Data->data_out - OutputBuffer;
	Output->Dimensions[0].Size = Samples / Channels;
	Output->Dimensions[0].Stride = Channels * sizeof(float);
	if (Output->Degree > 1) {
		Output->Dimensions[1].Size = Channels;
		Output->Dimensions[1].Stride = sizeof(float);
	}
	RETURN(Output);
}

ASYMBOL(Convert);

AMETHOD(Convert, TYP, Num$Array$Int16T, TYP, Num$Array$Float32T) {
	Num$Array$t *Input = (Num$Array$t *)Args[0].Val;
	Num$Array$t *Output = (Num$Array$t *)Args[1].Val;
	if (Input->Degree != Output->Degree) {
		Result->Val = Std$String$new("Array shapes do not match");
		return MESSAGE;
	}
	int Length;
	if (Input->Degree == 1) {
		if (Input->Dimensions[0].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[1].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		int Channels = Input->Dimensions[1].Size;
		if (Output->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != Channels * sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
		Length *= Channels;
	}
	src_short_to_float_array(Input->Data, Output->Data, Length);
	RETURN1;
}

AMETHOD(Convert, TYP, Num$Array$Int32T, TYP, Num$Array$Float32T) {
	Num$Array$t *Input = (Num$Array$t *)Args[0].Val;
	Num$Array$t *Output = (Num$Array$t *)Args[1].Val;
	if (Input->Degree != Output->Degree) {
		Result->Val = Std$String$new("Array shapes do not match");
		return MESSAGE;
	}
	int Length;
	if (Input->Degree == 1) {
		if (Input->Dimensions[0].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[1].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		int Channels = Input->Dimensions[1].Size;
		if (Output->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != Channels * sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
		Length *= Channels;
	}
	src_int_to_float_array(Input->Data, Output->Data, Length);
	RETURN1;
}

AMETHOD(Convert, TYP, Num$Array$Float32T, TYP, Num$Array$Int16T) {
	Num$Array$t *Input = (Num$Array$t *)Args[0].Val;
	Num$Array$t *Output = (Num$Array$t *)Args[1].Val;
	if (Input->Degree != Output->Degree) {
		Result->Val = Std$String$new("Array shapes do not match");
		return MESSAGE;
	}
	int Length;
	if (Input->Degree == 1) {
		if (Input->Dimensions[0].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[1].Stride != sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		int Channels = Input->Dimensions[1].Size;
		if (Output->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != Channels * sizeof(int16_t)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
		Length *= Channels;
	}
	src_float_to_short_array(Input->Data, Output->Data, Length);
	RETURN1;
}

AMETHOD(Convert, TYP, Num$Array$Float32T, TYP, Num$Array$Int32T) {
	Num$Array$t *Input = (Num$Array$t *)Args[0].Val;
	Num$Array$t *Output = (Num$Array$t *)Args[1].Val;
	if (Input->Degree != Output->Degree) {
		Result->Val = Std$String$new("Array shapes do not match");
		return MESSAGE;
	}
	int Length;
	if (Input->Degree == 1) {
		if (Input->Dimensions[0].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
	} else if (Input->Degree == 2) {
		if (Input->Dimensions[1].Stride != sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[1].Stride != sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		int Channels = Input->Dimensions[1].Size;
		if (Output->Dimensions[1].Size != Channels) SEND(Std$String$new("Number of channels do not match"));
		if (Input->Dimensions[0].Stride != Channels * sizeof(float)) SEND(Std$String$new("Only compact arrays are supported"));
		if (Output->Dimensions[0].Stride != Channels * sizeof(int32_t)) SEND(Std$String$new("Only compact arrays are supported"));
		Length = Input->Dimensions[0].Size;
		if (Output->Dimensions[0].Size != Length) SEND(Std$String$new("Number of samples do not match"));
		Length *= Channels;
	}
	src_float_to_int_array(Input->Data, Output->Data, Length);
	RETURN1;
}
