#include <Std.h>
#include <Sys/Module.h>
#include <Riva/Memory.h>
#include <fluidsynth.h>

typedef struct settings_t {
	const Std$Type_t *Type;
	fluid_settings_t *Handle;
} settings_t;

TYPE(SettingsT);

typedef struct synth_t {
	const Std$Type_t *Type;
	fluid_synth_t *Handle;
} synth_t;

TYPE(SynthT);

typedef struct sequencer_t {
	const Std$Type_t *Type;
	fluid_sequencer_t *Handle;
} sequencer_t;

TYPE(SequencerT);

typedef struct event_t {
	const Std$Type_t *Type;
	fluid_event_t *Handle;
} event_t;

TYPE(EventT);

typedef struct audio_driver_t {
	const Std$Type_t *Type;
	fluid_audio_driver_t *Handle;	
} audio_driver_t;

TYPE(AudioDriverT);

typedef struct sfont_t {
	const Std$Type_t *Type;
	fluid_sfont_t *Handle;
} sfont_t;

TYPE(SFontT);

typedef struct float_block_t {
	const Std$Type_t *Type;
	int Len;
	void *Lout, *Rout;
	int Loff, Roff, Lincr, Rincr;
} float_block_t;

TYPE(FloatBlockT);

static void settings_finalize(settings_t *Settings, void *Data) {
	delete_fluid_settings(Settings->Handle);
};

GLOBAL_FUNCTION(SettingsNew, 0) {
	settings_t *Settings = new(settings_t);
	Settings->Type = SettingsT;
	Settings->Handle = new_fluid_settings();
	Riva$Memory$register_finalizer((char *)Settings->Handle, (Riva$Memory_finalizer)settings_finalize, 0, 0, 0);
	Result->Val = (Std$Object_t *)Settings;
	return SUCCESS;
};

METHOD("get", TYP, SettingsT, TYP, Std$String$T) {
	fluid_settings_t *Handle = ((settings_t *)Args[0].Val)->Handle;
	const char *Name = Std$String$flatten(Args[1].Val);
	switch (fluid_settings_get_type(Handle, Name)) {
	case FLUID_NO_TYPE: return FAILURE;
	case FLUID_NUM_TYPE: {
		double Value;
		if (fluid_settings_getnum(Handle, Name, &Value)) {
			Result->Val = Std$Real$new(Value);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
	case FLUID_INT_TYPE: {
		int Value;
		if (fluid_settings_getint(Handle, Name, &Value)) {
			Result->Val = Std$Integer$new_small(Value);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
	case FLUID_STR_TYPE: {
		char *Value;
		if (fluid_settings_getstr(Handle, Name, &Value)) {
			Result->Val = Std$String$new(Value);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
	case FLUID_SET_TYPE: {
		return FAILURE;
	};
	};
};

METHOD("set", TYP, SettingsT, TYP, Std$String$T, ANY) {
	fluid_settings_t *Handle = ((settings_t *)Args[0].Val)->Handle;
	const char *Name = Std$String$flatten(Args[1].Val);
	if (Args[2].Val->Type == Std$String$T) {
		return fluid_settings_setstr(Handle, Name, Std$String$flatten(Args[2].Val)) ? SUCCESS : FAILURE;
	} else if (Args[2].Val->Type == Std$Real$T) {
		return fluid_settings_setnum(Handle, Name, Std$Real$get_value(Args[2].Val)) ? SUCCESS : FAILURE;
	} else if (Args[2].Val->Type == Std$Integer$SmallT) {
		return fluid_settings_setint(Handle, Name, Std$Integer$get_small(Args[2].Val)) ? SUCCESS : FAILURE;
	} else {
		return FAILURE;
	};
};

static void synth_finalize(synth_t *Synth, void *Data) {
	delete_fluid_synth(Synth->Handle);
};

GLOBAL_FUNCTION(SynthNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, SettingsT);
	settings_t *Settings = (settings_t *)Args[0].Val;
	synth_t *Synth = new(synth_t);
	Synth->Type = SynthT;
	Synth->Handle = new_fluid_synth(Settings->Handle);
	Riva$Memory$register_finalizer((char *)Synth->Handle, (Riva$Memory_finalizer)synth_finalize, 0, 0, 0);
	Result->Val = (Std$Object_t *)Synth;
	return SUCCESS;
};

METHOD("bank_select", TYP, SynthT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int Chan = Std$Integer$get_small(Args[1].Val);
	unsigned int Bank = Std$Integer$get_small(Args[2].Val);
	return fluid_synth_bank_select(Handle, Chan, Bank) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("sfont_select", TYP, SynthT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int Chan = Std$Integer$get_small(Args[1].Val);
	unsigned int SFontID = Std$Integer$get_small(Args[2].Val);
	return fluid_synth_sfont_select(Handle, Chan, SFontID) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("program_select", TYP, SynthT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int Chan = Std$Integer$get_small(Args[1].Val);
	unsigned int SFontID = Std$Integer$get_small(Args[2].Val);
	unsigned int BankNum = Std$Integer$get_small(Args[3].Val);
	unsigned int PresetNum = Std$Integer$get_small(Args[4].Val);
	return fluid_synth_program_select(Handle, Chan, SFontID, BankNum, PresetNum) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("unset_program", TYP, SynthT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int Chan = Std$Integer$get_small(Args[1].Val);
	return fluid_synth_unset_program(Handle, Chan) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("program_reset", TYP, SynthT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	return fluid_synth_program_reset(Handle) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("system_reset", TYP, SynthT) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	return fluid_synth_system_reset(Handle) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("sfload", TYP, SynthT, TYP, Std$String$T, TYP, Std$Symbol$T) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	const char *FileName = Std$String$flatten(Args[1].Val);
	int ResetPresets = Args[2].Val == $true;
	int SFontID = fluid_synth_sfload(Handle, FileName, ResetPresets);
	if (SFontID == FLUID_FAILED) return FAILURE;
	Result->Val = Std$Integer$new_small(SFontID);
	return SUCCESS;
};

METHOD("add_sfont", TYP, SynthT, TYP, SFontT) {
	fluid_synth_t *Synth = ((synth_t *)Args[0].Val)->Handle;
	fluid_sfont_t *SFont = ((sfont_t *)Args[1].Val)->Handle;
	int SFontID = fluid_synth_add_sfont(Synth, SFont);
	if (SFontID == FLUID_FAILED) return FAILURE;
	Result->Val = Std$Integer$new_small(SFontID);
	return SUCCESS;
};

METHOD("sfreload", TYP, SynthT, TYP, Std$String$T, TYP, Std$Symbol$T) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int SFontID = Std$Integer$get_small(Args[1].Val);
	return fluid_synth_sfreload(Handle, SFontID) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("sfunload", TYP, SynthT, TYP, Std$String$T, TYP, Std$Symbol$T) {
	fluid_synth_t *Handle = ((synth_t *)Args[0].Val)->Handle;
	int SFontID = Std$Integer$get_small(Args[1].Val);
	int ResetPresets = Args[2].Val == $true;
	return fluid_synth_sfunload(Handle, SFontID, ResetPresets) == FLUID_OK ? SUCCESS : FAILURE;
};

METHOD("get_sfont", TYP, SynthT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Synth = ((synth_t *)Args[0].Val)->Handle;
	int SFontID = Std$Integer$get_small(Args[1].Val);
	sfont_t *SFont = new(sfont_t);
	SFont->Type = SFontT;
	SFont->Handle = fluid_synth_get_sfont_by_id(Synth, SFontID);
	Result->Val = (Std$Object_t *)SFont;
	return SUCCESS;
};

METHOD("set_reverb", TYP, SynthT, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T) {
	fluid_synth_set_reverb(
		((synth_t *)Args[0].Val)->Handle,
		Std$Real$get_value(Args[1].Val),
		Std$Real$get_value(Args[2].Val),
		Std$Real$get_value(Args[3].Val),
		Std$Real$get_value(Args[4].Val)
	);
	return SUCCESS;	
};

METHOD("set_reverb_on", TYP, SynthT, TYP, Std$Symbol$T) {
	fluid_synth_set_reverb_on(((synth_t *)Args[0].Val)->Handle, Args[1].Val == $true);
	return SUCCESS;	
};

METHOD("set_chorus", TYP, SynthT, TYP, Std$Integer$SmallT, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Integer$SmallT) {
	fluid_synth_set_chorus(
		((synth_t *)Args[0].Val)->Handle,
		Std$Integer$get_small(Args[1].Val),
		Std$Real$get_value(Args[2].Val),
		Std$Real$get_value(Args[3].Val),
		Std$Real$get_value(Args[4].Val),
		Std$Integer$get_small(Args[5].Val)
	);
	return SUCCESS;	
};

METHOD("set_chorus_on", TYP, SynthT, TYP, Std$Symbol$T) {
	fluid_synth_set_chorus_on(((synth_t *)Args[0].Val)->Handle, Args[1].Val == $true);
	return SUCCESS;	
};

METHOD("set_sample_rate", TYP, SynthT, TYP, Std$Real$T) {
	fluid_synth_set_sample_rate(((synth_t *)Args[0].Val)->Handle, Std$Real$get_value(Args[1].Val));
	return SUCCESS;	
};

METHOD("set_gain", TYP, SynthT, TYP, Std$Real$T) {
	fluid_synth_set_gain(((synth_t *)Args[0].Val)->Handle, Std$Real$get_value(Args[1].Val));
	return SUCCESS;	
};

METHOD("set_polyphony", TYP, SynthT, TYP, Std$Integer$SmallT) {
	fluid_synth_set_polyphony(((synth_t *)Args[0].Val)->Handle, Std$Integer$get_small(Args[1].Val));
	return SUCCESS;	
};

METHOD("write_float", TYP, SynthT, TYP, Std$Integer$SmallT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_synth_t *Synth = ((synth_t *)Args[0].Val)->Handle;
	int Len = Std$Integer$get_small(Args[1].Val);
	void *Lout = Std$Address$get_value(Args[2].Val);
	int Loff = Std$Integer$get_small(Args[3].Val);
	int Lincr = Std$Integer$get_small(Args[4].Val);
	void *Rout = Std$Address$get_value(Args[5].Val);
	int Roff = Std$Integer$get_small(Args[6].Val);
	int Rincr = Std$Integer$get_small(Args[7].Val);
	return fluid_synth_write_float(Synth, Len, Lout, Loff, Lincr, Rout, Roff, Rincr) == FLUID_OK ? SUCCESS : FAILURE;
};

GLOBAL_FUNCTION(FloatBlockNew, 7) {
	float_block_t *Block = new(float_block_t);
	Block->Type = FloatBlockT;
	Block->Len = Std$Integer$get_small(Args[0].Val);
	Block->Lout = Std$Address$get_value(Args[1].Val);
	Block->Loff = Std$Integer$get_small(Args[2].Val);
	Block->Lincr = Std$Integer$get_small(Args[3].Val);
	Block->Rout = Std$Address$get_value(Args[4].Val);
	Block->Roff = Std$Integer$get_small(Args[5].Val);
	Block->Rincr = Std$Integer$get_small(Args[6].Val);
	Result->Val = (Std$Object_t *)Block;
	return SUCCESS;
};

METHOD("write_float", TYP, SynthT, TYP, FloatBlockT) {
	fluid_synth_t *Synth = ((synth_t *)Args[0].Val)->Handle;
	float_block_t *Block = (float_block_t *)Args[1].Val;
	return fluid_synth_write_float(Synth, Block->Len, Block->Lout, Block->Loff, Block->Lincr, Block->Rout, Block->Roff, Block->Rincr) == FLUID_OK ? SUCCESS : FAILURE;
};

typedef struct preset_t {
	const Std$Type$t *Type;
	Std$Object$t *Name, *Bank, *Program;
} preset_t;

TYPE(PresetT);

METHOD("name", TYP, PresetT) {
	preset_t *Preset = (preset_t *)Args[0].Val;
	Result->Val = Preset->Name;
	return SUCCESS;
};

METHOD("bank", TYP, PresetT) {
	preset_t *Preset = (preset_t *)Args[0].Val;
	Result->Val = Preset->Bank;
	return SUCCESS;
};

METHOD("program", TYP, PresetT) {
	preset_t *Preset = (preset_t *)Args[0].Val;
	Result->Val = Preset->Program;
	return SUCCESS;
};

typedef struct presets_generator {
	Std$Function_cstate State;
	fluid_sfont_t *SFont;
	fluid_preset_t Preset[1];
} presets_generator;

typedef struct presets_resume_data {
	presets_generator *Generator;
	Std$Function_argument Result;
} presets_resume_data;

static Std$Function$status resume_presets(presets_resume_data *Data) {
	presets_generator *Generator = Data->Generator;
	if (Generator->SFont->iteration_next(Generator->SFont, Generator->Preset)) {
		preset_t *Preset = new(preset_t);
		Preset->Type = PresetT;
		Preset->Name = Std$String$new(Generator->Preset->get_name(Generator->Preset));
		Preset->Bank = Std$Integer$new_small(Generator->Preset->get_banknum(Generator->Preset));
		Preset->Program = Std$Integer$new_small(Generator->Preset->get_num(Generator->Preset));
		Data->Result.Val = (Std$Object$t *)Preset;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("presets", TYP, SFontT) {
	fluid_sfont_t *SFont = ((sfont_t *)Args[0].Val)->Handle;
	presets_generator *Generator = new(presets_generator);
	SFont->iteration_start(SFont);
	if (SFont->iteration_next(SFont, Generator->Preset)) {
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function_cresumefn)resume_presets;
		Generator->SFont = SFont;
		preset_t *Preset = new(preset_t);
		Preset->Type = PresetT;
		Preset->Name = Std$String$new(Generator->Preset->get_name(Generator->Preset));
		Preset->Bank = Std$Integer$new_small(Generator->Preset->get_banknum(Generator->Preset));
		Preset->Program = Std$Integer$new_small(Generator->Preset->get_num(Generator->Preset));
		Result->Val = (Std$Object$t *)Preset;
		Result->State = Generator;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

static void sequencer_finalize(sequencer_t *Sequencer, void *Data) {
	delete_fluid_sequencer(Sequencer->Handle);
};

GLOBAL_FUNCTION(SequencerNew, 0) {
	sequencer_t *Sequencer = new(sequencer_t);
	Sequencer->Type = SequencerT;
	if (Count > 0) {
		Sequencer->Handle = new_fluid_sequencer2(Args[0].Val == $true);
	} else {
		Sequencer->Handle = new_fluid_sequencer();
	};
	Riva$Memory$register_finalizer((char *)Sequencer->Handle, (Riva$Memory_finalizer)sequencer_finalize, 0, 0, 0);
	Result->Val = (Std$Object_t *)Sequencer;
	return SUCCESS;
};

METHOD("tick", TYP, SequencerT) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	Result->Val = Std$Integer$new_small(fluid_sequencer_get_tick(Sequencer));
	return SUCCESS;
};

METHOD("time_scale", TYP, SequencerT) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	Result->Val = Std$Real$new(fluid_sequencer_get_time_scale(Sequencer));
	return SUCCESS;
};

METHOD("set_time_scale", TYP, SequencerT, TYP, Std$Real$T) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	double Scale = Std$Real$get_value(Args[1].Val);
	fluid_sequencer_set_time_scale(Sequencer, Scale);
	return SUCCESS;
};

METHOD("send_now", TYP, SequencerT, TYP, EventT) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	fluid_event_t *Event = ((event_t *)Args[1].Val)->Handle;
	fluid_sequencer_send_now(Sequencer, Event);
	return SUCCESS;
};

METHOD("send_at", TYP, SequencerT, TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Symbol$T) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	fluid_event_t *Event = ((event_t *)Args[1].Val)->Handle;
	unsigned int Time = Std$Integer$get_small(Args[2].Val);
	int Absolute = Args[3].Val == $true;
	fluid_sequencer_send_at(Sequencer, Event, Time, Absolute);
	return SUCCESS;
};

METHOD("register", TYP, SequencerT, TYP, SynthT) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	fluid_synth_t *Synth = ((synth_t *)Args[1].Val)->Handle;
	short ClientID = fluid_sequencer_register_fluidsynth(Sequencer, Synth);
	if (ClientID == FLUID_FAILED) return FAILURE;
	Result->Val = Std$Integer$new_small(ClientID);
	return SUCCESS;
};

SYMBOL($write, "write");

static void riva_event_callback(unsigned int Time, fluid_event_t *Event, fluid_sequencer_t *Sequencer, Std$Object_t *Function) {
	event_t *EventArg = new(event_t);
	EventArg->Type = EventT;
	EventArg->Handle = Event;
	sequencer_t *SequencerArg = new(sequencer_t);
	SequencerArg->Type = SequencerT;
	SequencerArg->Handle = Sequencer;
	Std$Function_result Result;
	if (Std$Function$call(Function, 3, &Result, Std$Integer$new_small(Time), 0, EventArg, 0, SequencerArg, 0) == MESSAGE) {
		
	};
};

METHOD("register", TYP, SequencerT, TYP, Std$String$T) {
	fluid_sequencer_t *Sequencer = ((sequencer_t *)Args[0].Val)->Handle;
	const char *Name = Std$String$flatten(Args[1].Val);
	short ClientID;
	if (Count > 2) {
		ClientID = fluid_sequencer_register_client(Sequencer, Name, (fluid_event_callback_t)riva_event_callback, Args[2].Val);
	} else {
		ClientID = fluid_sequencer_register_client(Sequencer, Name, 0, 0);
	}
	if (ClientID == FLUID_FAILED) return FAILURE;
	Result->Val = Std$Integer$new_small(ClientID);
	return SUCCESS;
};

static void event_finalize(event_t *Event, void *Data) {
	delete_fluid_event(Event->Handle);
};

GLOBAL_FUNCTION(EventNew, 0) {
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Handle = new_fluid_event();
	Riva$Memory$register_finalizer((char *)Event->Handle, (Riva$Memory_finalizer)event_finalize, 0, 0, 0);
	Result->Val = (Std$Object_t *)Event;
	return SUCCESS;
};

METHOD("data", TYP, EventT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	Result->Val = (Std$Object_t *)fluid_event_get_data(Event);
	return SUCCESS;
};

METHOD("set_source", TYP, EventT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	short Src = Std$Integer$get_small(Args[1].Val);
	fluid_event_set_source(Event, Src);
	return SUCCESS;
};

METHOD("set_dest", TYP, EventT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	short Dest = Std$Integer$get_small(Args[1].Val);
	fluid_event_set_dest(Event, Dest);
	return SUCCESS;
};

METHOD("timer", TYP, EventT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	fluid_event_timer(Event, Count > 1 ? Args[1].Val : Std$Object$Nil);
	return SUCCESS;
};

METHOD("note", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Key = Std$Integer$get_small(Args[2].Val);
	short Vel = Std$Integer$get_small(Args[3].Val);
	unsigned int Duration = Std$Integer$get_small(Args[4].Val);
	fluid_event_note(Event, Channel, Key, Vel, Duration);
	return SUCCESS;
};

METHOD("noteon", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Key = Std$Integer$get_small(Args[2].Val);
	short Vel = Std$Integer$get_small(Args[3].Val);
	fluid_event_noteon(Event, Channel, Key, Vel);
	return SUCCESS;
};

METHOD("noteoff", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Key = Std$Integer$get_small(Args[2].Val);
	fluid_event_noteoff(Event, Channel, Key);
	return SUCCESS;
};

METHOD("all_notes_off", TYP, EventT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	fluid_event_all_notes_off(Event, Channel);
	return SUCCESS;
};

METHOD("all_sounds_off", TYP, EventT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	fluid_event_all_sounds_off(Event, Channel);
	return SUCCESS;
};

METHOD("pitch_bend", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	int Pitch = Std$Integer$get_small(Args[2].Val);
	fluid_event_pitch_bend(Event, Channel, Pitch);
	return SUCCESS;
};

METHOD("pitch_wheelsens", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_pitch_wheelsens(Event, Channel, Val);
	return SUCCESS;
};

METHOD("modulation", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_modulation(Event, Channel, Val);
	return SUCCESS;
};

METHOD("sustain", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_sustain(Event, Channel, Val);
	return SUCCESS;
};

METHOD("volume", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_volume(Event, Channel, Val);
	return SUCCESS;
};

METHOD("pan", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_pan(Event, Channel, Val);
	return SUCCESS;
};

METHOD("reverb", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_reverb_send(Event, Channel, Val);
	return SUCCESS;
};

METHOD("chorus", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_chorus_send(Event, Channel, Val);
	return SUCCESS;
};

METHOD("pressure", TYP, EventT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	int Channel = Std$Integer$get_small(Args[1].Val);
	short Val = Std$Integer$get_small(Args[2].Val);
	fluid_event_channel_pressure(Event, Channel, Val);
	return SUCCESS;
};

METHOD("system_reset", TYP, EventT) {
	fluid_event_t *Event = ((event_t *)Args[0].Val)->Handle;
	fluid_event_system_reset(Event);
	return SUCCESS;
};

static void audio_driver_finalize(audio_driver_t *AudioDriver, void *Data) {
	delete_fluid_audio_driver(AudioDriver->Handle);
};

GLOBAL_FUNCTION(AudioDriverNew, 2) {
	CHECK_EXACT_ARG_TYPE(0, SettingsT);
	CHECK_EXACT_ARG_TYPE(1, SynthT);
	settings_t *Settings = (settings_t *)Args[0].Val;
	synth_t *Synth = (synth_t *)Args[1].Val;
	audio_driver_t *AudioDriver = new(audio_driver_t);
	AudioDriver->Type = AudioDriverT;
	AudioDriver->Handle = new_fluid_audio_driver(Settings->Handle, Synth->Handle);
	Riva$Memory$register_finalizer((char *)AudioDriver->Handle, (Riva$Memory_finalizer)audio_driver_finalize, 0, 0, 0);
	Result->Val = (Std$Object_t *)AudioDriver;
	return SUCCESS;
};
