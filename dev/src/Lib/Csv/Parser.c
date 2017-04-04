#include <Std.h>
#include <Riva/Memory.h>
#include <setjmp.h>
#include "csv.h"

typedef struct parser_t {
	Std$Type_t *Type;
	struct csv_parser Handle[1];
	jmp_buf OnError[1];
	Std$Object_t *UserData;
	Std$Object_t *FieldHandler;
	Std$Object_t *RecordHandler;
} parser_t;

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	csv_init(Parser->Handle, 0);
	Parser->UserData = Std$Object$Nil;
	Parser->FieldHandler = Std$Object$Nil;
	Parser->RecordHandler = Std$Object$Nil;
	Result->Val = Parser;
	return SUCCESS;
};

static void field_callback(void *Chars, size_t Length, parser_t *Parser) {
	Std$String_t *Field = Std$String$copy_length(Chars, Length);
	Std$Function_result Result;
	if (Std$Function$call(Parser->FieldHandler, 2, &Result, Parser->UserData, &Parser->UserData, Field, 0) == MESSAGE) {
		longjmp(Parser->OnError, Result.Val);
	};
};

static void record_callback(int Term, parser_t *Parser) {
	Std$Function_result Result;
	if (Std$Function$call(Parser->RecordHandler, 1, &Result, Parser->UserData, &Parser->UserData) == MESSAGE) {
		longjmp(Parser->OnError, Result.Val);
	};
};

METHOD("parse", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String_t *String = Args[1].Val;
	Std$Object_t *Message = setjmp(Parser->OnError);
	if (Message) {
		Result->Val = Message;
		return MESSAGE;
	};
	size_t Parsed = csv_parse(Parser->Handle, Std$String$flatten(String), String->Length.Value, field_callback, record_callback, Parser);
	if (Parsed < String->Length.Value) {
		Result->Val = Std$String$new(csv_strerror(csv_error(Parser->Handle)));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	};
};

METHOD("finish", TYP, T) {
	parser_t *Parser = Args[0].Val;
	if (csv_fini(Parser->Handle, field_callback, record_callback, Parser)) {
		Result->Val = Std$String$new(csv_strerror(csv_error(Parser->Handle)));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	};
};

METHOD("userdata", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->UserData);
	return SUCCESS;
};

METHOD("fieldhandler", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->FieldHandler);
	return SUCCESS;
};

METHOD("recordhandler", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->RecordHandler);
	return SUCCESS;
};

METHOD("set_delim", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String_t *String = Args[1].Val;
	csv_set_delim(Parser->Handle, ((char *)String->Blocks->Chars.Value)[0]);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("set_quote", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String_t *String = Args[1].Val;
	csv_set_quote(Parser->Handle, ((char *)String->Blocks->Chars.Value)[0]);
	Result->Arg = Args[0];
	return SUCCESS;
};
