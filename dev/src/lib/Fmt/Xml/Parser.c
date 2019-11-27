#include <Std.h>
#include <Agg.h>
#include <Riva.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <expat.h>

typedef struct parser_t {
	Std$Type$t *Type;
	XML_Parser Handle;
	Std$Object$t *UserData;
	Std$Function$t *StartElementHandler;
	Std$Function$t *EndElementHandler;
	Std$Function$t *CharacterDataHandler;
	Std$Function$t *SkippedEntityHandler;
	Std$Function$t *DefaultHandler;
} parser_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

extern int Riva$Symbol[];

static void startelementhandler(parser_t *Parser, const XML_Char *Name, const XML_Char **Attrs) {
	if (Parser->StartElementHandler != Std$Object$Nil) {
		Std$Function$result Result0;
		Std$Object$t *Table = Agg$Table$new(0, 0);
		int Type; void *Value;
		Riva$Module$import(Riva$Symbol, Name, &Type, &Value);
		for (const XML_Char **Attr = Attrs; Attr[0]; Attr += 2) {
			Agg$Table$insert(Table, Std$String$copy(Attr[0]), Std$String$copy(Attr[1]));
		}
		Std$Function$call(Parser->StartElementHandler, 3, &Result0, Parser->UserData, &Parser->UserData, Value, 0, Table, 0);
	}
}

static void endelementhandler(parser_t *Parser, const XML_Char *Name) {
	if (Parser->EndElementHandler != Std$Object$Nil) {
		Std$Function$result Result0;
		int Type; void *Value;
		Riva$Module$import(Riva$Symbol, Name, &Type, &Value);
		Std$Function$call(Parser->EndElementHandler, 2, &Result0, Parser->UserData, &Parser->UserData, Value, 0);
	}
}

static void characterdatahandler(parser_t *Parser, const XML_Char *String, int Length) {
	if (Parser->CharacterDataHandler != Std$Object$Nil) {
		Std$Function$result Result0;
		Std$Function$call(Parser->CharacterDataHandler, 2, &Result0, Parser->UserData, &Parser->UserData, Std$String$copy_length(String, Length), 0);
	}
}

static void skippedentityhandler(parser_t *Parser, const XML_Char *EntityName, int IsParameterEntity) {
	if (Parser->SkippedEntityHandler != Std$Object$Nil) {
		Std$Function$result Result0;
		Std$Function$call(Parser->SkippedEntityHandler, 3, &Result0, Parser->UserData, &Parser->UserData, Std$String$copy(EntityName), 0, IsParameterEntity ? $true : $false, 0);
	}
}

static void defaulthandler(parser_t *Parser, const XML_Char *String, int Length) {
	if (Parser->DefaultHandler != Std$Object$Nil) {
		Std$Function$result Result0;
		Std$Function$call(Parser->DefaultHandler, 2, &Result0, Parser->UserData, &Parser->UserData, Std$String$copy_length(String, Length), 0);
	}
}

GLOBAL_FUNCTION(New, 0) {
//:T
//  Creates and returns a new parser
	XML_Memory_Handling_Suite Suite = {
		Riva$Memory$alloc,
		Riva$Memory$realloc,
		Riva$Memory$free
	};
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	Parser->Handle = XML_ParserCreate_MM(0, &Suite, NULL);
	XML_SetUserData(Parser->Handle, Parser);

	Parser->UserData = Std$Object$Nil;
	Parser->StartElementHandler = Std$Object$Nil;
	Parser->EndElementHandler = Std$Object$Nil;
	Parser->CharacterDataHandler = Std$Object$Nil;
	Parser->SkippedEntityHandler = Std$Object$Nil;
	Parser->DefaultHandler = Std$Object$Nil;

	XML_SetElementHandler(Parser->Handle, startelementhandler, endelementhandler);
	XML_SetCharacterDataHandler(Parser->Handle, characterdatahandler);
	XML_SetSkippedEntityHandler(Parser->Handle, skippedentityhandler);
	XML_SetDefaultHandlerExpand(Parser->Handle, defaulthandler);

	Result->Val = Parser;
	return SUCCESS;
}

METHOD("use_foreign_dtd", TYP, T) {
	XML_Bool UseDTD = XML_TRUE;
	if (Count > 1 && Args[1].Val == $false) UseDTD = XML_FALSE;
	XML_Parser *Parser = ((parser_t *)Args[0].Val)->Handle;
	XML_UseForeignDTD(Parser, UseDTD);
}

METHOD("parse", TYP, T, TYP, Std$String$T) {
//@t
//@string
//:T
//  Parses string and calls the appropriate event handlers
	XML_Parser *Parser = ((parser_t *)Args[0].Val)->Handle;
	Std$String$t *String = Args[1].Val;
	for (int I = 0; I < String->Count; ++I) {
		switch (XML_Parse(Parser, String->Blocks[I].Value, String->Blocks[I].Length.Value, 0)) {
		case XML_STATUS_ERROR: {
			enum XML_Error Error = XML_GetErrorCode(Parser);
			SEND(Std$String$new(XML_ErrorString(Error)));
		}
		case XML_STATUS_OK:
			break;
		case XML_STATUS_SUSPENDED:
			SEND(Std$String$new("Expat suspend"));
		}
	}
	RETURN0;
}

METHOD("parse", TYP, T, TYP, Std$Address$T) {
//@t
//@string
//:T
//  Parses string and calls the appropriate event handlers
	XML_Parser *Parser = ((parser_t *)Args[0].Val)->Handle;
	switch (XML_Parse(Parser, Std$Address$get_value(Args[1].Val), Std$Address$get_length(Args[1].Val), 0)) {
	case XML_STATUS_ERROR: {
		enum XML_Error Error = XML_GetErrorCode(Parser);
		SEND(Std$String$new(XML_ErrorString(Error)));
	}
	case XML_STATUS_OK:
		break;
	case XML_STATUS_SUSPENDED:
		SEND(Std$String$new("Expat suspend"));
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("finish", TYP, T) {
	XML_Parser *Parser = ((parser_t *)Args[0].Val)->Handle;
	switch (XML_Parse(Parser, 0, 0, 1)) {
	case XML_STATUS_ERROR: {
		enum XML_Error Error = XML_GetErrorCode(Parser);
		SEND(Std$String$new(XML_ErrorString(Error)));
	}
	case XML_STATUS_OK:
		break;
	case XML_STATUS_SUSPENDED:
		SEND(Std$String$new("Expat suspend"));
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("userdata", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->UserData);
	return SUCCESS;
}

METHOD("onstartelement", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->StartElementHandler);
	return SUCCESS;
}

METHOD("onendelement", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->EndElementHandler);
	return SUCCESS;
}

METHOD("oncharacterdata", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->CharacterDataHandler);
	return SUCCESS;
}

METHOD("onskippedentity", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->SkippedEntityHandler);
	return SUCCESS;
}

METHOD("ondefault", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->DefaultHandler);
	return SUCCESS;
}

TYPED_INSTANCE(int, IO$Stream$write, T, parser_t *Stream, const char *Buffer, int Count, int Blocks) {
	XML_Parser *Handle = Stream->Handle;
	XML_Parse(Handle, Buffer, Count, 0);
	return Count;
}
