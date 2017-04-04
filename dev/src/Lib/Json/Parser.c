#include <Riva/Memory.h>
#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>

typedef struct map_key_t {
	struct map_key_t *Prev;
	Std$Object_t *Key;
} map_key_t;

typedef struct parser_t {
	const Std$Type_t *Type;
	yajl_handle *Handle;
	Std$Object_t *UserData;
	map_key_t *MapKey;
	Std$Function_t *FinalHandler;
	Std$Function_t *ValueHandler;
	Std$Function_t *MapStartHandler;
	Std$Function_t *MapPairHandler;
	Std$Function_t *MapEndHandler;
	Std$Function_t *ArrayStartHandler;
	Std$Function_t *ArrayEndHandler;
} parser_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

int value_handler(parser_t *Parser, Std$Object_t *Value) {
	if (Parser->MapKey == 0) {
		if (Parser->FinalHandler != Std$Object$Nil) {
			Std$Function_result Result;
			int Status = Std$Function$call(Parser->FinalHandler, 2, &Result, Parser->UserData, &Parser->UserData, Value, 0);
			return Status < FAILURE;
		} else {
			return 1;
		};
	} else if (Parser->MapKey->Key) {
		if (Parser->MapPairHandler != Std$Object$Nil) {
			Std$Function_result Result;
			int Status = Std$Function$call(Parser->MapPairHandler, 3, &Result, Parser->UserData, &Parser->UserData, Parser->MapKey->Key, 0, Value, 0);
			return Status < FAILURE;
		} else {
			return 1;
		};
	} else {
		if (Parser->ValueHandler != Std$Object$Nil) {
			Std$Function_result Result;
			int Status = Std$Function$call(Parser->ValueHandler, 2, &Result, Parser->UserData, &Parser->UserData, Value, 0);
			return Status < FAILURE;
		} else {
			return 1;
		};
	};
};

int null_handler(parser_t *Parser) {
	return value_handler(Parser, Std$Object$Nil);
};

int boolean_handler(parser_t *Parser, int BoolVal) {
	return value_handler(Parser, BoolVal ? $true : $false);
};

static inline Std$Object_t *finish_rational(mpq_t R) {
	if (mpz_cmp_si(mpq_denref(R), 1)) {
		return Std$Rational$new(R);
	} else if (mpz_fits_slong_p(mpq_numref(R))) {
		return Std$Integer$new_small(mpz_get_si(mpq_numref(R)));
	} else {
		return Std$Integer$new_big(mpq_numref(R));
	};
};

int number_handler(parser_t *Parser, const char *NumberVal, unsigned int NumberLen) {
	Std$Object_t *Value;
	char Buffer[NumberLen + 1];
	memcpy(Buffer, NumberVal, NumberLen);
	Buffer[NumberLen] = 0;
	mpq_t R;
	mpq_init(R);
	if (mpq_set_str(R, Buffer, 10) == 0) {
		Value = finish_rational(R);
	} else {
		char *Tail;
		double Val = strtod(Buffer, &Tail);
		if (Tail > Buffer) {
			Value = Std$Real$new(atof(Buffer));
		} else {
			Value = Std$Object$Nil;
		};
	};
	return value_handler(Parser, Value);
};

int string_handler(parser_t *Parser, const unsigned char *StringVal, unsigned int StringLen) {
	return value_handler(Parser, Std$String$copy_length(StringVal, StringLen));
};

int start_map_handler(parser_t *Parser) {
	map_key_t *MapKey = new(map_key_t);
	MapKey->Prev = Parser->MapKey;
	MapKey->Key = 0;
	Parser->MapKey = MapKey;
	if (Parser->MapStartHandler != Std$Object$Nil) {
		Std$Function_result Result;
		int Status = Std$Function$call(Parser->MapStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		return Status < FAILURE;
	} else {
		return 1;
	};
};

int map_key_handler(parser_t *Parser, const unsigned char *Key, unsigned int StringLen) {
	Parser->MapKey->Key = Std$String$copy_length(Key, StringLen);
	return 1;
};

int end_map_handler(parser_t *Parser) {
	Parser->MapKey = Parser->MapKey->Prev;
	if (Parser->MapEndHandler != Std$Object$Nil) {
		Std$Function_result Result;
		switch (Std$Function$call(Parser->MapEndHandler, 1, &Result, Parser->UserData, &Parser->UserData)) {
		case SUSPEND: case SUCCESS: return value_handler(Parser, Result.Val);
		case FAILURE: return 1;
		case MESSAGE: return 0;
		};
	} else {
		return 1;
	};
};

int start_array_handler(parser_t *Parser) {
	map_key_t *MapKey = new(map_key_t);
	MapKey->Prev = Parser->MapKey;
	MapKey->Key = 0;
	Parser->MapKey = MapKey;
	if (Parser->ArrayStartHandler != Std$Object$Nil) {
		Std$Function_result Result;
		int Status = Std$Function$call(Parser->ArrayStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		return Status < FAILURE;
	} else {
		return 1;
	};
};

int end_array_handler(parser_t *Parser) {
	Parser->MapKey = Parser->MapKey->Prev;
	if (Parser->ArrayEndHandler != Std$Object$Nil) {
		Std$Function_result Result;
		switch (Std$Function$call(Parser->ArrayEndHandler, 1, &Result, Parser->UserData, &Parser->UserData)) {
		case SUSPEND: case SUCCESS: return value_handler(Parser, Result.Val);
		case FAILURE: return 1;
		case MESSAGE: return 0;
		};
	} else {
		return 1;
	};
};

static yajl_callbacks Callbacks = {
	.yajl_null = (void *)null_handler,
	.yajl_boolean = (void *)boolean_handler,
	.yajl_integer = 0,
	.yajl_double = 0,
	.yajl_number = (void *)number_handler,
	.yajl_string = (void *)string_handler,
	.yajl_start_map = (void *)start_map_handler,
	.yajl_map_key = (void *)map_key_handler,
	.yajl_end_map = (void *)end_map_handler,
	.yajl_start_array = (void *)start_array_handler,
	.yajl_end_array = (void *)end_array_handler
};

static void *riva_alloc(void *Ctx, unsigned int Size) {
	return Riva$Memory$alloc(Size);
};

static void *riva_realloc(void *Ctx, void *Pointer, unsigned int Size) {
	return Riva$Memory$realloc(Pointer, Size);
};

static void *riva_free(void *Ctx, void *Pointer) {
};

GLOBAL_FUNCTION(New, 0) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	Parser->UserData = Std$Object$Nil;
	Parser->MapKey = 0;
	Parser->FinalHandler = Std$Object$Nil;
	Parser->ValueHandler = Std$Object$Nil;
	Parser->MapStartHandler = Std$Object$Nil;
	Parser->MapPairHandler = Std$Object$Nil;
	Parser->MapEndHandler = Std$Object$Nil;
	Parser->ArrayStartHandler = Std$Object$Nil;
	Parser->ArrayEndHandler = Std$Object$Nil;
	
	yajl_alloc_funcs AllocFuncs = {riva_alloc, riva_realloc, riva_free, 0};
	
	Parser->Handle = yajl_alloc(&Callbacks, &AllocFuncs, (void *)Parser);
	yajl_config(Parser->Handle, yajl_allow_comments, 1);
	yajl_config(Parser->Handle, yajl_allow_multiple_values, 1);
	Result->Val = Parser;
	return SUCCESS;
};

METHOD("parse", TYP, T, TYP, Std$String$T) {
//@t
//@string
//:T
//  Parses string and calls the appropiate event handlers
	yajl_handle *Handle = ((parser_t *)Args[0].Val)->Handle;
	Std$String_t *String = Args[1].Val;
	for (int I = 0; I < String->Count; ++I) {
		switch (yajl_parse(Handle, String->Blocks[I].Chars.Value, String->Blocks[I].Length.Value)) {
		case yajl_status_ok: break;
		case yajl_status_client_canceled: {
			Result->Val = Std$String$new("Parse cancelled!");
			return MESSAGE;
		};
		case yajl_status_error: {
			Result->Val = Std$String$new(yajl_get_error(Handle, 0, 0, 0));
			return MESSAGE;
		};
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("parse", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
//@t
//@buffer
//@length
//:T
//  Parses <var>length</var> bytes from <var>buffer</var> and calls the appropiate event handlers
	yajl_handle *Handle = ((parser_t *)Args[0].Val)->Handle;
	switch (yajl_parse(Handle, Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val))) {
	case yajl_status_ok: break;
	case yajl_status_client_canceled: {
		Result->Val = Std$String$new("Parse cancelled!");
		return MESSAGE;
	};
	case yajl_status_error: {
		Result->Val = Std$String$new(yajl_get_error(Handle, 0, 0, 0));
		return MESSAGE;
	};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("finish", TYP, T) {
//@t
//:T
//  Finishes parsing
	yajl_handle *Handle = ((parser_t *)Args[0].Val)->Handle;
	switch (yajl_complete_parse(Handle)) {
	case yajl_status_ok: break;
	case yajl_status_client_canceled: {
		Result->Val = Std$String$new("Parse cancelled!");
		return MESSAGE;
	};
	case yajl_status_error: {
		Result->Val = Std$String$new(yajl_get_error(Handle, 0, 0, 0));
		return MESSAGE;
	};
	};
	Result->Arg = Args[0];
	return SUCCESS;	
};

METHOD("userdata", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->UserData);
	return SUCCESS;
};

METHOD("onfinal", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->FinalHandler);
	return SUCCESS;
};

METHOD("onvalue", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ValueHandler);
	return SUCCESS;
};

METHOD("onmapstart", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapStartHandler);
	return SUCCESS;
};

METHOD("onmappair", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapPairHandler);
	return SUCCESS;
};

METHOD("onmapend", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapEndHandler);
	return SUCCESS;
};

METHOD("onarraystart", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ArrayStartHandler);
	return SUCCESS;
};

METHOD("onarrayend", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ArrayEndHandler);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$write, T, parser_t *Stream, const char *Buffer, int Count, int Blocks) {
	yajl_handle *Handle = Stream->Handle;
	switch (yajl_parse(Handle, Buffer, Count)) {
	case yajl_status_ok: return Count;
	case yajl_status_client_canceled: return -1;
	case yajl_status_error: return -1;
	default: return -1;
	};
};


