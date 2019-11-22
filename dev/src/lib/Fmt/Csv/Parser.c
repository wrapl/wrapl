#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <setjmp.h>
#include "csv.h"

typedef struct parser_t {
	Std$Type$t *Type;
	struct csv_parser Handle[1];
	jmp_buf OnError[1];
	Std$Object$t *UserData;
	Std$Object$t *FieldHandler;
	Std$Object$t *RecordHandler;
} parser_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

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
	Std$String$t *Field = Std$String$copy_length(Chars, Length);
	Std$Function$result Result;
	if (Std$Function$call(Parser->FieldHandler, 2, &Result, Parser->UserData, &Parser->UserData, Field, 0) == MESSAGE) {
		longjmp(Parser->OnError, Result.Val);
	};
};

static void record_callback(int Term, parser_t *Parser) {
	Std$Function$result Result;
	if (Std$Function$call(Parser->RecordHandler, 1, &Result, Parser->UserData, &Parser->UserData) == MESSAGE) {
		longjmp(Parser->OnError, Result.Val);
	};
};

METHOD("parse", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String$t *String = Args[1].Val;
	Std$Object$t *Message = setjmp(Parser->OnError);
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

TYPED_INSTANCE(int, IO$Stream$write, T, parser_t *Parser, const char *Source, int Length, int Block) {
	size_t Parsed = csv_parse(Parser->Handle, Source, Length, field_callback, record_callback, Parser);
	if (Parsed < Length) {
		return -1;
	} else {
		return Parsed;
	}
}

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	parser_t *Parser = Args[0].Val;
	char *String = Std$Address$get_value(Args[1].Val);
	int Length = Std$Integer$get_small(Args[2].Val);
	Std$Object$t *Message = setjmp(Parser->OnError);
	if (Message) {
		Result->Val = Message;
		return MESSAGE;
	};
	size_t Parsed = csv_parse(Parser->Handle, String, Length, field_callback, record_callback, Parser);
	if (Parsed < Length) {
		Result->Val = Std$String$new(csv_strerror(csv_error(Parser->Handle)));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	};
}

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

METHOD("delim", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String$t *String = Args[1].Val;
	csv_set_delim(Parser->Handle, ((char *)String->Blocks->Chars.Value)[0]);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("quote", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String$t *String = Args[1].Val;
	csv_set_quote(Parser->Handle, ((char *)String->Blocks->Chars.Value)[0]);
	Result->Arg = Args[0];
	return SUCCESS;
};
