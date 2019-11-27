#include <Std.h>
#include <Agg/Table.h>
#include <Agg/List.h>
#include <Sys/Module.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <sox.h>

typedef struct signalinfo_t {
	const Std$Type$t *Type;
	sox_signalinfo_t Value[1];
} signal_info_t;

TYPE(SignalInfoT);

GLOBAL_FUNCTION(SignalInfoNew, 0) {
	signal_info_t *SignalInfo = new(signal_info_t);
	SignalInfo->Type = SignalInfoT;
	if (Count > 0) SignalInfo->Value->rate = Std$Real$double(Args[0].Val);
	if (Count > 1) SignalInfo->Value->channels = Std$Integer$int(Args[1].Val);
	if (Count > 2) SignalInfo->Value->precision = Std$Integer$int(Args[2].Val);
	if (Count > 3) SignalInfo->Value->precision = Std$Integer$get_u64(Args[3].Val);
	Result->Val = (Std$Object$t *)SignalInfo;
	return SUCCESS;
}

typedef struct format_t {
	const Std$Type$t *Type;
	sox_format_t *Handle;
} format_t;

TYPE(FormatT);

ASYMBOL(OpenRead);

AMETHOD(OpenRead, TYP, Std$String$T) {
	format_t *Format = new(format_t);
	Format->Type = FormatT;
	const char *FileName = Std$String$flatten(Args[0].Val);
	// TODO: Support optional parameters
	sox_signalinfo_t *Signal = 0;
	sox_encodinginfo_t *Encoding = 0;
	char const *FileType = 0;
	Format->Handle = sox_open_read(FileName, Signal, Encoding, FileType);
	if (!Format->Handle) {
		Result->Val = Std$String$new("Error opening sox stream");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Format;
	return SUCCESS;
}

AMETHOD(OpenRead, TYP, Std$Address$T) {
	format_t *Format = new(format_t);
	Format->Type = FormatT;
	Std$Address$t *Buffer = (Std$Address$t *)Args[0].Val;
	// TODO: Support optional parameters
	sox_signalinfo_t *Signal = 0;
	sox_encodinginfo_t *Encoding = 0;
	char const *FileType = 0;
	Format->Handle = sox_open_mem_read(Buffer->Value, Buffer->Length.Value, Signal, Encoding, FileType);
	if (!Format->Handle) {
		Result->Val = Std$String$new("Error opening sox stream");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Format;
	return SUCCESS;
}

AMETHOD(OpenRead, TYP, Std$Address$T) {
	format_t *Format = new(format_t);
	Format->Type = FormatT;
	void *Buffer = Std$Address$get_value(Args[0].Val);
	size_t Length = Std$Address$get_length(Args[0].Val);
	// TODO: Support optional parameters
	sox_signalinfo_t *Signal = 0;
	sox_encodinginfo_t *Encoding = 0;
	char const *FileType = 0;
	Format->Handle = sox_open_mem_read(Buffer, Length, Signal, Encoding, FileType);
	if (!Format->Handle) {
		Result->Val = Std$String$new("Error opening sox stream");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Format;
	return SUCCESS;
}

ASYMBOL(OpenWrite);

AMETHOD(OpenWrite, TYP, Std$String$T, TYP, SignalInfoT) {
	format_t *Format = new(format_t);
	Format->Type = FormatT;
	const char *FileName = Std$String$flatten(Args[0].Val);
	sox_signalinfo_t *Signal = ((signal_info_t *)Args[1].Val)->Value;
	// TODO: Support optional parameters
	sox_encodinginfo_t *Encoding = 0;
	char const *FileType = 0;
	sox_oob_t *OOB = 0;
	void *OverwritePermitted = 0;
	Format->Handle = sox_open_write(FileName, Signal, Encoding, FileType, OOB, OverwritePermitted);
	if (!Format->Handle) {
		Result->Val = Std$String$new("Error opening sox stream");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Format;
	return SUCCESS;
}

AMETHOD(OpenWrite, TYP, Std$String$T, TYP, SignalInfoT) {
	format_t *Format = new(format_t);
	Format->Type = FormatT;
	const char *FileName = Std$String$flatten(Args[0].Val);
	sox_signalinfo_t *Signal = ((signal_info_t *)Args[1].Val)->Value;
	// TODO: Support optional parameters
	sox_encodinginfo_t *Encoding = 0;
	char const *FileType = 0;
	sox_oob_t *OOB = 0;
	void *OverwritePermitted = 0;
	Format->Handle = sox_open_write(FileName, Signal, Encoding, FileType, OOB, OverwritePermitted);
	if (!Format->Handle) {
		Result->Val = Std$String$new("Error opening sox stream");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Format;
	return SUCCESS;
}

METHOD("close", TYP, FormatT) {
	format_t *Format = (format_t *)Args[0].Val;
	sox_close(Format->Handle);
	return SUCCESS;
}

typedef struct effects_chain_t {
	const Std$Type$t *Type;
	sox_effects_chain_t *Handle;
} effects_chain_t;

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	sox_encoding_t *InEncoding = 0;
	sox_encoding_t *OutEncoding = 0;
	effects_chain_t *Chain = new(effects_chain_t);
	Chain->Type = T;
	Chain->Handle = sox_create_effects_chain(InEncoding, OutEncoding);
	Result->Val = (Std$Object$t *)Chain;
	return SUCCESS;
}

static int flow_callback(sox_bool AllDone, effects_chain_t *Sox) {
	return 0;
}

METHOD("flow", TYP, T) {
	effects_chain_t *Chain = (effects_chain_t *)Args[0].Val;
	if (sox_flow_effects(Chain->Handle, (void *)flow_callback, Chain) == SOX_SUCCESS) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

typedef struct effect_t {
	const Std$Type$t *Type;
	sox_effect_t *Handle;
} effect_t;

TYPE(EffectT);

GLOBAL_FUNCTION(EffectFind, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	effect_t *Effect = new(effect_t);
	Effect->Type = EffectT;
	Effect->Handle = sox_create_effect(sox_find_effect(Std$String$flatten(Args[0].Val)));
	Result->Val = (Std$Object$t *)Effect;
	return SUCCESS;
}

typedef struct riva_effect_t {
	const Std$Type$t *Type;
	sox_effect_t *Handle;
	Std$Object$t *GetOpts;
	Std$Object$t *Start;
	Std$Object$t *Flow;
	Std$Object$t *Drain;
	Std$Object$t *Stop;
	Std$Object$t *Kill;
	sox_effect_handler_t Handler[1];
} riva_effect_t;

static int riva_effect_getopts(sox_effect_t *E, int Argc, char *Argv[]) {
	riva_effect_t *Effect = E->priv;
	Std$Object$t *Args = Agg$List$new0();
		for (int I = 0; I < Argc; ++I) Agg$List$put(Args, Std$String$copy(Argv[I]));
	Std$Function$result Result[1];
	if (Std$Function$call(Effect->GetOpts, 2, Result, Effect, 0, Args, 0) == MESSAGE) return SOX_EINVAL;
	return SOX_SUCCESS;
}

static int riva_effect_simple(sox_effect_t *E) {
	riva_effect_t *Effect = E->priv;
	Std$Function$result Result[1];
	if (Std$Function$call(Effect->Start, 1, Result, Effect, 0) == MESSAGE) return SOX_EINVAL;
	return SOX_SUCCESS;
}

static int riva_effect_flow(sox_effect_t *E, sox_sample_t *IBuf, sox_sample_t *OBuf, size_t *ISamp, size_t *OSamp) {
	riva_effect_t *Effect = E->priv;
	Std$Object$t *ISampVal = Std$Integer$new_small(ISamp[0]);
	Std$Object$t *OSampVal = Std$Integer$new_small(OSamp[0]);
	Std$Function$argument Args[5] = {
		{Effect, 0},
		{Std$Address$new(IBuf, ISamp[0] * sizeof(sox_sample_t)), 0},
		{Std$Address$new(OBuf, OSamp[0] * sizeof(sox_sample_t)), 0},
		{ISampVal, &ISampVal},
		{OSampVal, &OSampVal}
	};
	Std$Function$result Result[1];
	if (Std$Function$invoke(Effect->Flow, 5, Result, Args) == MESSAGE) return SOX_EINVAL;
	ISamp[0] = Std$Integer$get_small(ISampVal);
	OSamp[0] = Std$Integer$get_small(OSampVal);
	return SOX_SUCCESS;
}

static int riva_effect_drain(sox_effect_t *E, sox_sample_t *OBuf, size_t *OSamp) {
	riva_effect_t *Effect = E->priv;
	Std$Object$t *OSampVal = Std$Integer$new_small(OSamp[0]);
	Std$Function$argument Args[3] = {
		{Effect, 0},
		{Std$Address$new(OBuf, OSamp[0] * sizeof(sox_sample_t)), 0},
		{OSampVal, &OSampVal}
	};
	Std$Function$result Result[1];
	if (Std$Function$invoke(Effect->Flow, 3, Result, Args) == MESSAGE) return SOX_EINVAL;
	OSamp[0] = Std$Integer$get_small(OSampVal);
	return SOX_SUCCESS;
}

TYPE(RivaEffectT, EffectT);

SYMBOL($name, "name");
SYMBOL($usage, "usage");
SYMBOL($flags, "flags");
SYMBOL($getopts, "getopts");
SYMBOL($start, "start");
SYMBOL($flow, "flow");
SYMBOL($drain, "drain");
SYMBOL($stop, "stop");
SYMBOL($kill, "kill");

GLOBAL_FUNCTION(EffectCreate, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Symbol$ArrayT);
	riva_effect_t *Effect = new(riva_effect_t);
	Effect->Type = RivaEffectT;
	Std$Symbol$array *Names = (Std$Symbol$array *)Args[0].Val;
	for (int I = 0; I < Names->Count; ++I) {
		Std$Symbol$t *Name = Names->Values[I];
		Std$Object$t *Value = Args[I + 1].Val;
		if (Name == $name) {
			CHECK_EXACT_ARG_TYPE(I + 1, Std$String$T);
			Effect->Handler->name = Std$String$flatten(Value);
		} else if (Name == $usage) {
			CHECK_EXACT_ARG_TYPE(I + 1, Std$String$T);
			Effect->Handler->usage = Std$String$flatten(Value);
		} else if (Name == $flags) {
			CHECK_EXACT_ARG_TYPE(I + 1, Std$Integer$SmallT);
			Effect->Handler->flags = Std$Integer$get_small(Value);
		} else if (Name == $getopts) {
			Effect->Handler->getopts = riva_effect_getopts;
			Effect->GetOpts = Value;
		} else if (Name == $start) {
			Effect->Handler->start = riva_effect_simple;
			Effect->GetOpts = Value;
		} else if (Name == $flow) {
			Effect->Handler->flow = riva_effect_flow;
			Effect->Flow = Value;
		} else if (Name == $drain) {
			Effect->Handler->drain = riva_effect_drain;
			Effect->Drain = Value;
		} else if (Name == $stop) {
			Effect->Handler->stop = riva_effect_simple;
			Effect->Stop = Value;
		} else if (Name == $kill) {
			Effect->Handler->kill = riva_effect_simple;
			Effect->Kill = Value;
		}
	}
	sox_effect_t *Handle = Effect->Handle = sox_create_effect(Effect->Handler);
	Handle->priv = Effect;
	Result->Val = (Std$Object$t *)Effect;
	return SUCCESS;
}

METHOD("add", TYP, T, TYP, EffectT) {
	effects_chain_t *Chain = (effects_chain_t *)Args[0].Val;
	effect_t *Effect = (effect_t *)Args[1].Val;
	sox_add_effect(Chain->Handle, Effect->Handle, 0, 0);
	Result->Arg = Args[0];
	return SUCCESS;
}

INITIAL() {
	sox_init();
	sox_format_init();
}

/*
sox_format_t sox_open_read(const char *path, const sox_signalinfo_t *info, const char *filetype);

sox_format_t sox_open_write(sox_bool (*overwrite_permitted)(const char *filename), const char *path, const sox_signalinfo_t *info, const char *filetype, const char *comment, sox_size_t length, const sox_instrinfo_t *instr, const sox_loopinfo_t *loops);

sox_size_t sox_read(sox_format_t ft, sox_ssample_t *buf, sox_size_t len);

sox_size_t sox_write(sox_format_t ft, sox_ssample_t *buf, sox_size_t len);

int sox_close(sox_format_t ft);

int sox_seek(sox_format_t ft, sox_size_t offset, int whence);

sox_effect_handler_t const *sox_find_effect(char const *name);

sox_effect_t *sox_create_effect(sox_effect_handler_t const *eh);

int sox_effect_options(sox_effect_t *effp, int argc, char * const argv[]);

sox_effects_chain_t *sox_create_effects_chain(sox_encodinginfo_t const *in_enc, sox_encodinginfo_t const *out_enc);

void sox_delete_effects_chain(sox_effects_chain_t *ecp);

int sox_add_effect(sox_effects_chaint_t *chain, sox_effect_t*effp, sox_signalinfo_t *in, sox_signalinfo_t const *out);
 */
