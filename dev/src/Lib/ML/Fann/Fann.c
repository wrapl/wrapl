#include <fann.h>
#include <Std.h>
#include <Riva.h>
#include <Math/Vector.h>
#include <Math/Matrix.h>

typedef struct {
	const Std$Type$t *Type;
	struct fann *Handle;
	Std$Object$t *Callback;
	Std$Function$argument CallbackArgs[5];
} fann_t;

TYPE(T);

GLOBAL_FUNCTION(Standard, 1) {
	unsigned int Layers[Count];
	for (int I = 0; I < Count; ++I) {
		CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
		Layers[I] = Std$Integer$get_small(Args[I].Val);
	}
	fann_t *Fann = new(fann_t);
	Fann->Type = T;
	Fann->Handle = fann_create_standard_array(Count, Layers);
	fann_set_user_data(Fann->Handle, Fann);
	Result->Val = (Std$Object$t *)Fann;
	return SUCCESS;
}

GLOBAL_FUNCTION(Sparse, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Real$T);
	unsigned int Layers[Count - 1];
	for (int I = 1; I < Count; ++I) {
		CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
		Layers[I] = Std$Integer$get_small(Args[I + 1].Val);
	}
	fann_t *Fann = new(fann_t);
	Fann->Type = T;
	Fann->Handle = fann_create_sparse_array(Std$Real$get_value(Args[0].Val), Count, Layers);
	fann_set_user_data(Fann->Handle, Fann);
	Result->Val = (Std$Object$t *)Fann;
	return SUCCESS;
}

GLOBAL_FUNCTION(Shortcut, 1) {
	unsigned int Layers[Count];
	for (int I = 0; I < Count; ++I) {
		CHECK_EXACT_ARG_TYPE(I, Std$Integer$SmallT);
		Layers[I] = Std$Integer$get_small(Args[I].Val);
	}
	fann_t *Fann = new(fann_t);
	Fann->Type = T;
	Fann->Handle = fann_create_shortcut_array(Count, Layers);
	fann_set_user_data(Fann->Handle, Fann);
	Result->Val = (Std$Object$t *)Fann;
	return SUCCESS;
}

GLOBAL_FUNCTION(Read, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	fann_t *Fann = new(fann_t);
	Fann->Type = T;
	Fann->Handle = fann_create_from_file(Std$String$flatten(Args[0].Val));
	if (!Fann->Handle) {
		Result->Val = Std$String$new("Error reading network");
		return MESSAGE;
	}
	fann_set_user_data(Fann->Handle, Fann);
	Result->Val = (Std$Object$t *)Fann;
	return SUCCESS;
}

METHOD("get_MSE", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_MSE(Fann->Handle));
	return SUCCESS;
}

METHOD("get_bit_fail", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_bit_fail(Fann->Handle));
	return SUCCESS;
}

METHOD("reset_MSE", TYP, T) {
	fann_t *Fann = Args[0].Val;
	fann_reset_MSE(Fann->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("randomize_weights", TYP, T, TYP, Std$Real$T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_randomize_weights(Fann->Handle, Std$Real$get_value(Args[1].Val), Std$Real$get_value(Args[2].Val));
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("print_connections", TYP, T) {
	fann_t *Fann = Args[0].Val;
	fann_print_connections(Fann->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("print_parameters", TYP, T) {
	fann_t *Fann = Args[0].Val;
	fann_print_parameters(Fann->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get_num_input", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_num_input(Fann->Handle));
	return SUCCESS;
}

METHOD("get_num_output", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_num_output(Fann->Handle));
	return SUCCESS;
}

METHOD("get_num_layers", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_num_layers(Fann->Handle));
	return SUCCESS;
}

METHOD("get_total_neurons", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_total_neurons(Fann->Handle));
	return SUCCESS;
}

METHOD("get_total_connections", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Integer$new_small(fann_get_total_connections(Fann->Handle));
	return SUCCESS;
}

METHOD("save", TYP, T, TYP, Std$String$T) {
	fann_t *Fann = Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	if (fann_save(Fann->Handle, FileName)) {
		Result->Val = Std$String$new("Error saving network");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("run", TYP, T, TYP, Math$Vector$T) {
	fann_t *Fann = Args[0].Val;
	unsigned int NumInput = fann_get_num_input(Fann->Handle);
	unsigned int NumOutput = fann_get_num_output(Fann->Handle);
	Math$Vector$t *InputVector = Args[1].Val;
	if (NumInput != InputVector->Length.Value) {
		Result->Val = Std$String$new("Number of inputs does not match");
		return MESSAGE;
	}
	fann_type Input[NumInput];
	Std$Object$t **Entry = InputVector->Entries;
	for (int I = 0; I < NumInput; ++I) Input[I] = Std$Real$double(*Entry++);
	fann_type *Output = fann_run(Fann->Handle, Input);
	Math$Vector$t *OutputVector = Riva$Memory$alloc(sizeof(Math$Vector$t) + NumOutput * sizeof(Std$Object$t *));
	OutputVector->Type = Math$Matrix$T;
	OutputVector->Length.Type = Std$Integer$SmallT;
	OutputVector->Length.Value = NumOutput;
	Entry = OutputVector->Entries;
	for (int I = 0; I < NumOutput; ++I) *Entry++ = Std$Real$new(Output[I]);
	Result->Val = OutputVector;
	return SUCCESS;
}

METHOD("run", TYP, T, TYP, Std$Number$T) {
	fann_t *Fann = Args[0].Val;
	unsigned int NumInput = fann_get_num_input(Fann->Handle);
	unsigned int NumOutput = fann_get_num_output(Fann->Handle);
	if (NumInput != Count - 1) {
		Result->Val = Std$String$new("Number of inputs does not match");
		return MESSAGE;
	}
	fann_type Input[NumInput];
	Std$Function$argument *Arg = Args + 1;
	for (int I = 0; I < NumInput; ++I) Input[I] = Std$Real$double((Arg++)->Val);
	fann_type *Output = fann_run(Fann->Handle, Input);
	Math$Vector$t *OutputVector = Riva$Memory$alloc(sizeof(Math$Vector$t) + NumOutput * sizeof(Std$Object$t *));
	OutputVector->Type = Math$Matrix$T;
	OutputVector->Length.Type = Std$Integer$SmallT;
	OutputVector->Length.Value = NumOutput;
	Std$Object$t **Entry = OutputVector->Entries;
	for (int I = 0; I < NumOutput; ++I) *Entry++ = Std$Real$new(Output[I]);
	Result->Val = OutputVector;
	return SUCCESS;
}


static int riva_fann_callback(struct fann *Handle, struct fann_train_data *TrainData, unsigned int MaxEpochs, unsigned int ReportInterval, float DesiredError, unsigned int Epochs) {
	fann_t *Fann = fann_get_user_data(Handle);
	Fann->CallbackArgs[4].Val = Std$Integer$new_small(Epochs);
	Std$Function$result Result[1];
	if (Std$Function$invoke(Fann->Callback, 5, Result, Fann->CallbackArgs) < FAILURE) {
		return 0;
	} else {
		return -1;
	}
}

METHOD("set_callback", TYP, T, ANY) {
	fann_t *Fann = Args[0].Val;
	if (Args[1].Val == Std$Object$Nil) {
		Fann->Callback = 0;
		fann_set_callback(Fann->Handle, 0);
	} else {
		Fann->Callback = Args[1].Val;
		fann_set_callback(Fann->Handle, riva_fann_callback);
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

typedef struct {
	const Std$Type$t *Type;
	struct fann_train_data *Handle;
} fann_data_t;

TYPE(DataT);

GLOBAL_FUNCTION(DataNew, 3) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(2, Std$Integer$SmallT);
	unsigned int NumData = Std$Integer$get_small(Args[0].Val);
	unsigned int NumInput = Std$Integer$get_small(Args[1].Val);
	unsigned int NumOutput = Std$Integer$get_small(Args[2].Val);
	fann_data_t *Data = new(fann_data_t);
	Data->Type = DataT;
	Data->Handle = fann_create_train(NumData, NumInput, NumOutput);
	Result->Val = (Std$Object$t *)Data;
	return SUCCESS;
}

GLOBAL_FUNCTION(DataRead, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	fann_data_t *Data = new(fann_data_t);
	Data->Type = DataT;
	Data->Handle = fann_read_train_from_file(Std$String$flatten(Args[0].Val));
	Result->Val = (Std$Object$t *)Data;
	return SUCCESS;
}

METHOD("set", TYP, DataT, TYP, Math$Matrix$T) {
	fann_data_t *Data = Args[0].Val;
	Math$Matrix$t *Values = Args[1].Val;
	unsigned int NumData = fann_length_train_data(Data->Handle);
	unsigned int NumInput = fann_num_input_train_data(Data->Handle);
	unsigned int NumOutput = fann_num_output_train_data(Data->Handle);
	if (NumData != Values->NoOfRows.Value) {
		Result->Val = Std$String$new("Number of rows does not match");
		return MESSAGE;
	}
	if (NumInput + NumOutput != Values->NoOfCols.Value) {
		Result->Val = Std$String$new("Number of columns does not match");
		return MESSAGE;
	}
	Std$Object$t **Entry = Values->Entries;
	for (int I = 0; I < NumData; ++I) {
		fann_type *Input = fann_get_train_input(Data->Handle, I);
		fann_type *Output = fann_get_train_output(Data->Handle, I);
		for (int J = 0; J < NumInput; ++J) Input[J] = Std$Real$double(*Entry++);
		for (int J = 0; J < NumOutput; ++J) Output[J] = Std$Real$double(*Entry++);
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set", TYP, DataT, TYP, Std$Integer$SmallT, TYP, Math$Vector$T) {
	fann_data_t *Data = Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	Math$Vector$t *Values = Args[2].Val;
	unsigned int NumData = fann_length_train_data(Data->Handle);
	unsigned int NumInput = fann_num_input_train_data(Data->Handle);
	unsigned int NumOutput = fann_num_output_train_data(Data->Handle);
	if (Index < 1 || Index > NumData) {
			Result->Val = Std$String$new("Index out of range");
			return MESSAGE;
		}
	if (NumInput + NumOutput != Values->Length.Value) {
		Result->Val = Std$String$new("Number of values does not match");
		return MESSAGE;
	}
	fann_type *Input = fann_get_train_input(Data->Handle, Index - 1);
	fann_type *Output = fann_get_train_output(Data->Handle, Index - 1);
	Std$Object$t **Entry = Values->Entries;
	for (int J = 0; J < NumInput; ++J) Input[J] = Std$Real$double(*Entry++);
	for (int J = 0; J < NumOutput; ++J) Output[J] = Std$Real$double(*Entry++);
	Result->Arg = Args[0];
	return SUCCESS;
}


METHOD("set", TYP, DataT, TYP, Std$Integer$T, TYP, Std$Number$T) {
	fann_data_t *Data = Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	unsigned int NumData = fann_length_train_data(Data->Handle);
	unsigned int NumInput = fann_num_input_train_data(Data->Handle);
	unsigned int NumOutput = fann_num_output_train_data(Data->Handle);
	if (Index < 1 || Index > NumData) {
		Result->Val = Std$String$new("Index out of range");
		return MESSAGE;
	}
	if (NumInput + NumOutput != Count - 2) {
		Result->Val = Std$String$new("Incorrect number of arguments");
		return MESSAGE;
	}
	fann_type *Input = fann_get_train_input(Data->Handle, Index - 1);
	fann_type *Output = fann_get_train_output(Data->Handle, Index - 1);
	Std$Function$argument *Arg = Args + 2;
	for (int J = 0; J < NumInput; ++J) Input[J] = Std$Real$double((Arg++)->Val);
	for (int J = 0; J < NumOutput; ++J) Output[J] = Std$Real$double((Arg++)->Val);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("shuffle", TYP, DataT) {
	fann_data_t *Data = Args[0].Val;
	fann_shuffle_train_data(Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("min_input", TYP, DataT) {
	fann_data_t *Data = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_min_train_input(Data->Handle));
	return SUCCESS;
}

METHOD("max_input", TYP, DataT) {
	fann_data_t *Data = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_max_train_input(Data->Handle));
	return SUCCESS;
}

METHOD("min_output", TYP, DataT) {
	fann_data_t *Data = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_min_train_output(Data->Handle));
	return SUCCESS;
}

METHOD("max_output", TYP, DataT) {
	fann_data_t *Data = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_max_train_output(Data->Handle));
	return SUCCESS;
}

METHOD("init_weights", TYP, T, TYP, DataT) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	fann_init_weights(Fann->Handle, Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("train", TYP, T, TYP, Math$Vector$T, TYP, Math$Vector$T) {
	fann_t *Fann = Args[0].Val;
	unsigned int NumInput = fann_get_num_input(Fann->Handle);
	unsigned int NumOutput = fann_get_num_output(Fann->Handle);
	Math$Vector$t *InputVector = Args[1].Val;
	Math$Vector$t *OutputVector = Args[2].Val;
	if (NumInput != InputVector->Length.Value) {
		Result->Val = Std$String$new("Number of inputs does not match");
		return MESSAGE;
	}
	if (NumOutput != OutputVector->Length.Value) {
		Result->Val = Std$String$new("Number of outputs does not match");
		return MESSAGE;
	}
	fann_type Input[NumInput];
	fann_type Output[NumOutput];
	Std$Object$t **Entry = InputVector->Entries;
	for (int I = 0; I < NumInput; ++I) Input[I] = Std$Real$double(*Entry++);
	Entry = OutputVector->Entries;
	for (int I = 0; I < NumOutput; ++I) Output[I] = Std$Real$double(*Entry++);
	fann_train(Fann->Handle, Input, Output);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("test", TYP, T, TYP, Math$Vector$T, TYP, Math$Vector$T) {
	fann_t *Fann = Args[0].Val;
	unsigned int NumInput = fann_get_num_input(Fann->Handle);
	unsigned int NumOutput = fann_get_num_output(Fann->Handle);
	Math$Vector$t *InputVector = Args[1].Val;
	Math$Vector$t *OutputVector = Args[2].Val;
	if (NumInput != InputVector->Length.Value) {
		Result->Val = Std$String$new("Number of inputs does not match");
		return MESSAGE;
	}
	if (NumOutput != OutputVector->Length.Value) {
		Result->Val = Std$String$new("Number of outputs does not match");
		return MESSAGE;
	}
	fann_type Input[NumInput];
	fann_type Output[NumOutput];
	Std$Object$t **Entry = InputVector->Entries;
	for (int I = 0; I < NumInput; ++I) Input[I] = Std$Real$double(*Entry++);
	Entry = OutputVector->Entries;
	for (int I = 0; I < NumOutput; ++I) Output[I] = Std$Real$double(*Entry++);
	fann_test(Fann->Handle, Input, Output);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("train", TYP, T, TYP, DataT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	unsigned int MaxEpochs = Std$Integer$get_small(Args[2].Val);
	unsigned int ReportInterval = Std$Integer$get_small(Args[3].Val);
	double DesiredError = Std$Real$get_value(Args[4].Val);
	Fann->CallbackArgs[0].Val = Fann;
	Fann->CallbackArgs[1].Val = Std$Integer$new_small(MaxEpochs);
	Fann->CallbackArgs[2].Val = Std$Integer$new_small(ReportInterval);
	Fann->CallbackArgs[3].Val = Std$Real$new(DesiredError);
	fann_train_on_data(Fann->Handle, Data->Handle, MaxEpochs, ReportInterval, DesiredError);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("train", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	unsigned int MaxEpochs = Std$Integer$get_small(Args[2].Val);
	unsigned int ReportInterval = Std$Integer$get_small(Args[3].Val);
	double DesiredError = Std$Real$get_value(Args[4].Val);
	fann_train_on_file(Fann->Handle, FileName, MaxEpochs, ReportInterval, DesiredError);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("train", TYP, T, TYP, DataT) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	fann_train_epoch(Fann->Handle, Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("test", TYP, T, TYP, DataT) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	fann_test_data(Fann->Handle, Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("scale", TYP, T, TYP, DataT) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	fann_scale_train(Fann->Handle, Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("descale", TYP, T, TYP, DataT) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	fann_descale_train(Fann->Handle, Data->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_input_scaling_params", TYP, T, TYP, DataT, TYP, Std$Real$T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	double NewMin = Std$Real$get_value(Args[2].Val);
	double NewMax = Std$Real$get_value(Args[3].Val);
	fann_set_input_scaling_params(Fann->Handle, Data->Handle, NewMin, NewMax);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_output_scaling_params", TYP, T, TYP, DataT, TYP, Std$Real$T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	double NewMin = Std$Real$get_value(Args[2].Val);
	double NewMax = Std$Real$get_value(Args[3].Val);
	fann_set_output_scaling_params(Fann->Handle, Data->Handle, NewMin, NewMax);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_scaling_params", TYP, T, TYP, DataT, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_data_t *Data = Args[1].Val;
	double NewInputMin = Std$Real$get_value(Args[2].Val);
	double NewInputMax = Std$Real$get_value(Args[3].Val);
	double NewOutputMin = Std$Real$get_value(Args[4].Val);
	double NewOutputMax = Std$Real$get_value(Args[5].Val);
	fann_set_scaling_params(Fann->Handle, Data->Handle, NewInputMin, NewInputMax, NewOutputMin, NewOutputMax);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("clear_scaling_params", TYP, T) {
	fann_t *Fann = Args[0].Val;
	fann_clear_scaling_params(Fann->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

TYPE(TrainingT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

const Std$Integer$smallt TrainingINCREMENTAL[1] = {{TrainingT, FANN_TRAIN_INCREMENTAL}};
const Std$Integer$smallt TrainingBATCH[1] = {{TrainingT, FANN_TRAIN_BATCH}};
const Std$Integer$smallt TrainingRPROP[1] = {{TrainingT, FANN_TRAIN_RPROP}};
const Std$Integer$smallt TrainingQUICKPROP[1] = {{TrainingT, FANN_TRAIN_QUICKPROP}};
const Std$Integer$smallt TrainingSARPROP[1] = {{TrainingT, FANN_TRAIN_SARPROP}};

METHOD("get_training_algorithm", TYP, T) {
	fann_t *Fann = Args[0].Val;
	switch (fann_get_training_algorithm(Fann->Handle)) {
	case FANN_TRAIN_INCREMENTAL:
		Result->Val = TrainingINCREMENTAL;
		break;
	case FANN_TRAIN_BATCH:
		Result->Val = TrainingBATCH;
		break;
	case FANN_TRAIN_RPROP:
		Result->Val = TrainingRPROP;
		break;
	case FANN_TRAIN_QUICKPROP:
		Result->Val = TrainingQUICKPROP;
		break;
	case FANN_TRAIN_SARPROP:
		Result->Val = TrainingSARPROP;
		break;
	}
	return SUCCESS;
}

METHOD("set_training_algorithm", TYP, T, TYP, TrainingT) {
	fann_t *Fann = Args[0].Val;
	fann_set_training_algorithm(Fann->Handle, Std$Integer$get_small(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get_learning_rate", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_learning_rate(Fann->Handle));
	return SUCCESS;
}

METHOD("set_learning_rate", TYP, T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_set_learning_rate(Fann->Handle, Std$Real$get_value(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get_learning_momentum", TYP, T) {
	fann_t *Fann = Args[0].Val;
	Result->Val = Std$Real$new(fann_get_learning_momentum(Fann->Handle));
	return SUCCESS;
}

METHOD("set_learning_momentum", TYP, T, TYP, Std$Real$T) {
	fann_t *Fann = Args[0].Val;
	fann_set_learning_momentum(Fann->Handle, Std$Real$get_value(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
}

TYPE(ActivationT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

const Std$Integer$smallt ActivationLINEAR[1] = {{ActivationT, FANN_LINEAR}};
const Std$Integer$smallt ActivationTHRESHOLD[1] = {{ActivationT, FANN_THRESHOLD}};
const Std$Integer$smallt ActivationTHRESHOLD_SYMMETRIC[1] = {{ActivationT, FANN_THRESHOLD_SYMMETRIC}};
const Std$Integer$smallt ActivationSIGMOID[1] = {{ActivationT, FANN_SIGMOID}};
const Std$Integer$smallt ActivationSIGMOID_STEPWISE[1] = {{ActivationT, FANN_SIGMOID_STEPWISE}};
const Std$Integer$smallt ActivationSIGMOID_SYMMETRIC[1] = {{ActivationT, FANN_SIGMOID_SYMMETRIC}};
const Std$Integer$smallt ActivationSIGMOID_SYMMETRIC_STEPWISE[1] = {{ActivationT, FANN_SIGMOID_SYMMETRIC_STEPWISE}};
const Std$Integer$smallt ActivationGAUSSIAN[1] = {{ActivationT, FANN_GAUSSIAN}};
const Std$Integer$smallt ActivationGAUSSIAN_SYMMETRIC[1] = {{ActivationT, FANN_GAUSSIAN_SYMMETRIC}};
const Std$Integer$smallt ActivationELLIOT[1] = {{ActivationT, FANN_ELLIOT}};
const Std$Integer$smallt ActivationELLIOT_SYMMETRIC[1] = {{ActivationT, FANN_ELLIOT_SYMMETRIC}};
const Std$Integer$smallt ActivationLINEAR_PIECE[1] = {{ActivationT, FANN_LINEAR_PIECE}};
const Std$Integer$smallt ActivationLINEAR_PIECE_SYMMETRIC[1] = {{ActivationT, FANN_LINEAR_PIECE_SYMMETRIC}};
const Std$Integer$smallt ActivationSIN_SYMMETRIC[1] = {{ActivationT, FANN_SIN_SYMMETRIC}};
const Std$Integer$smallt ActivationCOS_SYMMETRIC[1] = {{ActivationT, FANN_COS_SYMMETRIC}};
const Std$Integer$smallt ActivationSIN[1] = {{ActivationT, FANN_SIN}};
const Std$Integer$smallt ActivationCOS[1] = {{ActivationT, FANN_COS}};

METHOD("set_activation", TYP, T, TYP, ActivationT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	fann_t *Fann = Args[0].Val;
	enum fann_activationfunc_enum Activation = Std$Integer$get_small(Args[1].Val);
	int Layer = Std$Integer$get_small(Args[2].Val);
	int Neuron = Std$Integer$get_small(Args[3].Val);
	fann_set_activation_function(Fann->Handle, Activation, Layer, Neuron);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_activation", TYP, T, TYP, ActivationT, TYP, Std$Integer$SmallT) {
	fann_t *Fann = Args[0].Val;
	enum fann_activationfunc_enum Activation = Std$Integer$get_small(Args[1].Val);
	int Layer = Std$Integer$get_small(Args[2].Val);
	fann_set_activation_function_layer(Fann->Handle, Activation, Layer);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_activation", TYP, T, TYP, ActivationT) {
	fann_t *Fann = Args[0].Val;
	enum fann_activationfunc_enum Activation = Std$Integer$get_small(Args[1].Val);
	fann_set_activation_function_hidden(Fann->Handle, Activation);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("set_output_activation", TYP, T, TYP, ActivationT) {
	fann_t *Fann = Args[0].Val;
	enum fann_activationfunc_enum Activation = Std$Integer$get_small(Args[1].Val);
	fann_set_activation_function_output(Fann->Handle, Activation);
	Result->Arg = Args[0];
	return SUCCESS;
}

TYPE(ErrorT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

const Std$Integer$smallt ErrorLINEAR[1] = {{ErrorT, FANN_ERRORFUNC_LINEAR}};
const Std$Integer$smallt ErrorTANH[1] = {{ErrorT, FANN_ERRORFUNC_TANH}};

METHOD("set_error", TYP, T, TYP, ErrorT) {
	fann_t *Fann = Args[0].Val;
	enum fann_errorfunc_enum Error = Std$Integer$get_small(Args[1].Val);
	fann_set_train_error_function(Fann->Handle, Error);
	Result->Arg = Args[0];
	return SUCCESS;
}

TYPE(StopT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

const Std$Integer$smallt StopMSE[1] = {{ErrorT, FANN_STOPFUNC_MSE}};
const Std$Integer$smallt StopBIT[1] = {{ErrorT, FANN_STOPFUNC_BIT}};

METHOD("set_stop", TYP, T, TYP, ErrorT) {
	fann_t *Fann = Args[0].Val;
	enum fann_stopfunc_enum Error = Std$Integer$get_small(Args[1].Val);
	fann_set_train_stop_function(Fann->Handle, Error);
	Result->Arg = Args[0];
	return SUCCESS;
}

