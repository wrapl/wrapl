#include <Riva.h>
#include <Std.h>
#include <IO/File.h>
#include <string.h>
#include <Sys/Module.h>
#include <Agg/List.h>
#include <Agg/Table.h>

#include <stdlib.h>
#include "parser.h"
#include "compiler.h"
#include "missing.h"
#include "system.h"
#include "assembler.h"
#include "debugger.h"
#include "wrapl.h"

CONSTANT(Message, Sys$Module$T) {
// Possible error messages sent by the compiler.
	Sys$Module$t *Module = (Sys$Module$t *)Sys$Module$new("Message");
	Sys$Module$export(Module, "Error", 0, (void *)ErrorMessageT);
	Sys$Module$export(Module, "SourceError", 0, (void *)SourceErrorMessageT);
	Sys$Module$export(Module, "ParseError", 0, (void *)ParseErrorMessageT);
	Sys$Module$export(Module, "InitError", 0, (void *)InitErrorMessageT);
	Sys$Module$export(Module, "TypeError", 0, (void *)TypeErrorMessageT);
	Sys$Module$export(Module, "ExternError", 0, (void *)ExternErrorMessageT);
	Sys$Module$export(Module, "ScopeErrorT", 0, (void *)ScopeErrorMessageT);
	return (Std$Object$t *)Module;
}

#if defined(WINDOWS) && !defined(CYGWIN)

static const char *wrapl_find(const char *Base) {
	char Buffer[strlen(Base) + 7];
	strcpy(stpcpy(Buffer, Base), ".wrapl");
	if (GetFileAttributes(Buffer) == INVALID_FILE_ATTRIBUTES) return 0;
	return Riva$Memory$strdup(Buffer);
}

#else

#include <sys/stat.h>

static const char *wrapl_find(const char *Base) {
	struct stat Stat[1];
	char Buffer[strlen(Base) + 7];
	strcpy(stpcpy(Buffer, Base), ".wrapl");
	if (stat(Buffer, Stat)) return 0;
	return Riva$Memory$strdup(Buffer);
}

#endif

static int wrapl_load(Riva$Module$provider_t *Provider, const char *Path) {
	Riva$Module$t *Module = Provider->Module;
	if (Module->Type == 0) Module->Type = Sys$Module$T;
	IO$Stream$t *Source = (IO$Stream$t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) return 0;
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		fprintf(stderr, "%s(%d): %s\n", Path, Scanner->Error.LineNo, Scanner->Error.Message);
		if (Riva$Config$get("Wrapl/Loader/StopOnWraplError")) exit(1);
		return 0;
	}
	module_expr_t *Expr;
	if (Debugger) Scanner->DebugInfo = debug_module(Path);
	if (Scanner->parse(tkHASH) || Scanner->parse(tkAT)) {
		Scanner->flush();
		Expr = parse_module(Scanner, Provider);
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		if (Expr == 0) {
			if (Riva$Config$get("Wrapl/Loader/StopOnWraplError")) exit(1);
			return 0;
		}
	} else {
		Expr = accept_module(Scanner, Provider);
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
	}
#ifdef PARSER_LISTING
	Expr->print(0);
#endif
	compiler_t *Compiler = new compiler_t(Path);
	Compiler->DebugInfo = Scanner->DebugInfo;
	Scanner = 0;
	if (setjmp(Compiler->Error.Handler)) {
		fprintf(stderr, "%s(%d): %s\n", Path, Compiler->Error.LineNo, Compiler->Error.Message);
		for (int I = 0; I < Compiler->Error.Count; ++I) fprintf(stderr, "\t%s\n", Compiler->Error.Stack[I]);
		if (Riva$Config$get("Wrapl/Loader/StopOnWraplError")) exit(1);
		return 0;
	}
	Expr->compile(Compiler);
	return 1;
}

TYPE(ErrorMessageT);
// The type of error messages sent by the compiler.

struct errormessage_t {
	const Std$Type$t *Type;
	int LineNo;
	const char *Message;
};

GLOBAL_FUNCTION(LoadModule, 1) {
//@filename:Std$String$T
//:Sys$Module$T
// Loads and compiles the module contained in the file <var>filename</var> and returns the module handle.
	Sys$Module$t *Module = Sys$Module$new(0);
	const char *Path = Std$String$flatten(Args[0].Val);
	IO$Stream$t *Source = (IO$Stream$t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = SourceErrorMessageT;
		Error->LineNo = 0;
		Error->Message = "Error: error opening file";
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	if (Debugger) Scanner->DebugInfo = debug_module(Path);
	if (Scanner->parse(tkHASH) || Scanner->parse(tkAT)) Scanner->flush();
	module_expr_t *Expr = accept_module(Scanner, Riva$Module$get_default_provider(Module));
	IO$Stream$close(Source, IO$Stream$CLOSE_READ);
#ifdef PARSER_LISTING
	Expr->print(0);
#endif
	compiler_t *Compiler = new compiler_t(Path);
	Compiler->DebugInfo = Scanner->DebugInfo;
	Scanner = 0;
	if (setjmp(Compiler->Error.Handler)) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = Compiler->Error.Type;
		Error->LineNo = Compiler->Error.LineNo;
		Error->Message = Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	Expr->compile(Compiler);
	Result->Val = (Std$Object$t *)Module;
	return SUCCESS;
}

GLOBAL_FUNCTION(LoadExpr, 1) {
//@filename:Std$String$T
//:ANY
// Evaluates the expression contained in the file <var>filename</var> and returns the result.
	const char *Path = Std$String$flatten(Args[0].Val);
	IO$Stream$t *Source = (IO$Stream$t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = SourceErrorMessageT;
		Error->LineNo = 0;
		Error->Message = "Error: error opening file";
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	expr_t *Expr = accept_expr(Scanner);
	IO$Stream$close(Source, IO$Stream$CLOSE_READ);
	Scanner = 0;
	compiler_t *Compiler = new compiler_t(Path);
	if (setjmp(Compiler->Error.Handler)) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = Compiler->Error.Type;
		Error->LineNo = Compiler->Error.LineNo;
		Error->Message = Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	if (Count > 1) Compiler->MissingIDFunc = Args[1].Val;
	return Expr->evaluate(Compiler, Result);
}

GLOBAL_FUNCTION(ReadExpr, 1) {
//@source:IO$Stream$T
//:ANY
// Evaluates the expression read from <var>source</var> and returns the result.
	IO$Stream$t *Source = Args[0].Val;
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	expr_t *Expr = accept_expr(Scanner);
	Scanner = 0;
	compiler_t *Compiler = new compiler_t("reader");
	if (setjmp(Compiler->Error.Handler)) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = Compiler->Error.Type;
		Error->LineNo = Compiler->Error.LineNo;
		Error->Message = Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	if (Count > 1) Compiler->MissingIDFunc = Args[1].Val;
	return Expr->evaluate(Compiler, Result);
}

ASYMBOL(WriteExpr);
// Writes the representation for a value to a stream.

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, TYP, Std$Number$T) {
//:IO$Stream$T
	Std$Function$result Result0;
	Std$Function$call(Std$String$Of, 1, &Result0, Args[1].Val, 0);
	Std$String$t *String = (Std$String$t *)Result0.Val;
	for (Std$String$block *Block = String->Blocks; Block->Length.Value; ++Block) {
		IO$Stream$write(Args[0].Val, (const char *)Block->Chars.Value, Block->Length.Value, 1);
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, TYP, Std$String$T) {
//:IO$Stream$T
	Std$Object$t *Stream = Args[0].Val;
	static char Hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	IO$Stream$write(Stream, "\"", 1, 1);
	Std$String$t *String = (Std$String$t *)Args[1].Val;
	for (Std$String$block *Block = String->Blocks; Block->Length.Value; ++Block) {
		const char *I = Block->Chars.Value;
		const char *J = I;
		const char *L = I + Block->Length.Value;
		for (; J < L; ++J) {
			char Char = *J;
			if (Char == '\"') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\\"", 2, 1);
				I = J;
			} else if (Char == '\\') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\\\", 2, 1);
				I = J;
			} else if (Char == '\'') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\\'", 2, 1);
				I = J;
			} else if (Char == '\t') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\t", 2, 1);
				I = J;
			} else if (Char == '\r') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\r", 2, 1);
				I = J;
			} else if (Char == '\n') {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				IO$Stream$write(Stream, "\\n", 2, 1);
				I = J;
			} else if ((Char < ' ') || (Char >= 0x80)) {
				if (I < J) IO$Stream$write(Stream, I, J - I, 1);
				char Tmp[4] = {'\\', 'x', Hex[Char / 16], Hex[Char % 16]};
				IO$Stream$write(Stream, Tmp, 4, 1);
				I = J;
			}
		}
		if (I < J) IO$Stream$write(Stream, I, J - I, 1);
	}
	IO$Stream$write(Stream, "\"", 1, 1);
	Result->Arg = Args[0];
	return SUCCESS;
}

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, TYP, Std$Symbol$T) {
//:IO$Stream$T
	Std$Object$t *Stream = Args[0].Val;
	IO$Stream$write(Stream, ":", 1, 1);
	return Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, ((Std$Symbol$t *)Args[1].Val)->Name, 0);
}

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, TYP, Agg$List$T) {
//:IO$Stream$T
	Std$Object$t *Stream = Args[0].Val;
	Agg$List$node *Node = ((Agg$List$t *)Args[1].Val)->Head;
	if (Node) {
		IO$Stream$write(Stream, "[", 1, 1);
		Std$Function$result Buffer;
		switch (Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			break;
		case FAILURE:
			Result->Val = Std$String$new("WriteExpr error");
		case MESSAGE:
			return MESSAGE;
		};
		while (Node = Node->Next) {
			IO$Stream$write(Stream, ", ", 2, 1);
			switch (Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, Node->Value, 0)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				Result->Val = Std$String$new("WriteExpr error");
			case MESSAGE:
				return MESSAGE;
			};
		};
		IO$Stream$write(Stream, "]", 1, 1);
	} else {
		IO$Stream$write(Stream, "[]", 2, 1);
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, TYP, Agg$Table$T) {
//:IO$Stream$T
	Std$Object$t *Stream = Args[0].Val;
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	Std$Object$t *Node = Agg$Table$trav_first(Trav, Args[1].Val);
	if (!Node) {
		IO$Stream$write(Stream, "{}", 2, 1);
		Result->Arg = Args[0];
		return SUCCESS;
	}

	IO$Stream$write(Stream, "{", 1, 1);
	do {
		Std$Function$result Buffer;
		switch (Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, Agg$Table$node_key(Node), 0)) {
		case SUSPEND: case SUCCESS:
			break;
		case FAILURE:
			Result->Val = Std$String$new("WriteExpr error");
		case MESSAGE:
			return MESSAGE;
		};
		Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Value != Std$Object$Nil) {
			IO$Stream$write(Stream, " IS ", 4, 1);
			switch (Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, Value, 0)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				Result->Val = Std$String$new("WriteExpr error");
			case MESSAGE:
				return MESSAGE;
			}
		}
		Node = Agg$Table$trav_next(Trav);
		if (Node) IO$Stream$write(Stream, ", ", 2, 1);
	} while (Node);
	IO$Stream$write(Stream, "}", 1, 1);
	Result->Arg = Args[0];
	return SUCCESS;
};

AMETHOD(WriteExpr, TYP, IO$Stream$WriterT, VAL, Std$Object$Nil) {
//:IO$Stream$T
	IO$Stream$write(Args[0].Val, "NIL", 3, 1);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(SaveExpr, 2) {
//@filename:Std$String$T
//expr:ANY
//:ANY
// Evaluates the expression contained in the file <var>filename</var> and returns the result.
	const char *Path = Std$String$flatten(Args[0].Val);
	IO$Stream$t *Stream = (IO$Stream$t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_WRITE | IO$File$OPEN_TEXT));
	if (Stream == 0) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = SourceErrorMessageT;
		Error->LineNo = 0;
		Error->Message = "Error: error opening file";
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	Std$Function$status Status = Std$Function$call((Std$Object$t *)WriteExpr, 2, Result, Stream, 0, Args[1].Val, 0);
	IO$Stream$close(Stream, IO$Stream$CLOSE_WRITE);
	switch (Status) {
	case SUSPEND: case SUCCESS:
		Result->Arg = Args[1];
		return SUCCESS;
	case FAILURE:
		Result->Val = Std$String$new("WriteExpr error");
	case MESSAGE:
		return MESSAGE;
	}
}


struct session_t {
	const Std$Type$t *Type;
	scanner_t *Scanner;
	compiler_t *Compiler;
};

TYPE(SessionT);
//  An incremental Wrapl compiler/interpreter

AMETHOD(Std$String$Of, TYP, ErrorMessageT) {
	errormessage_t *Error = (errormessage_t *)Args[0].Val;
	char *Buffer;
	Result->Val = (Std$Object$t *)Std$String$new_length(Buffer, asprintf(&Buffer, "(%d): %s", Error->LineNo, Error->Message));
	return SUCCESS;
}

GLOBAL_FUNCTION(SessionNew, 1) {
//@src : IO$Stream$ReaderT
//: SessionT
// Returns a new <id>SessionT</id> which reads source from <var>src</var>.
	session_t *Session = new session_t;
	Session->Type = SessionT;
	Session->Scanner = new scanner_t(Args[0].Val);
	if (Debugger) Session->Scanner->DebugInfo = debug_module("console");
	if (Count > 1) {
		session_t *Existing = (session_t *)Args[1].Val;
		Session->Compiler = new compiler_t("console", Existing->Compiler->Global);
	} else {
		Session->Compiler = new compiler_t("console");
	}
	Session->Compiler->DebugInfo = Session->Scanner->DebugInfo;
	Result->Val = (Std$Object$t *)Session;
	return SUCCESS;
}

GLOBAL_FUNCTION(SessionEval, 1) {
//@session:SessionT
//:ANY
// Parses and evaluates the next value from <var>session</var>.
	session_t *Session = (session_t *)Args[0].Val;
	if (setjmp(Session->Scanner->Error.Handler)) {
		Session->Scanner->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Scanner->Error.Type;
		Error->LineNo = Session->Scanner->Error.LineNo;
		Error->Message = Session->Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	command_expr_t *Command = accept_command(Session->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	if (setjmp(Session->Compiler->Error.Handler)) {
		Session->Compiler->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Compiler->Error.Type;
		Error->LineNo = Session->Compiler->Error.LineNo;
		Error->Message = Session->Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	return Command->compile(Session->Compiler, Result);
}

GLOBAL_METHOD(SessionReset, 1, "reset", TYP, SessionT) {
	session_t *Session = (session_t *)Args[0].Val;
	Session->Scanner->flush();
	Session->Compiler->flush();
	RETURN(Session);
}

GLOBAL_FUNCTION(SessionLine, 1) {
//@session : SessionT
//: Std$String$T
// Returns the rest of the current line stored by the scanner in <var>session</var>.
	session_t *Session = (session_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Std$String$new(Session->Scanner->NextChar);
	Session->Scanner->NextChar = "";
	return SUCCESS;
}

GLOBAL_FUNCTION(SessionDef, 3) {
//@session:SessionT
//@name:Std$String$T
//@value:ANY
// Defines the constant <var>name</var> with value <var>value</var> in the global scope of <var>session</var>.
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::CNST;
	Operand->Value = Args[2].Val;
	const char *Name = Std$String$flatten(Args[1].Val);
	Session->Compiler->declare(Name, Operand);
	if (Debugger) debug_add_global_constant(Session->Compiler->DebugInfo, Name, Operand->Value);
	return SUCCESS;
}

GLOBAL_FUNCTION(SessionVar, 3) {
//@session:SessionT
//@name:Std$String$T
//@variable:ANY
// Defines <var>variable</var> as a variable called <var>name</var> in the global scope of <var>session</var>.
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::GVAR;
	Operand->Address = Args[2].Ref;
	const char *Name = Std$String$flatten(Args[1].Val);
	Session->Compiler->declare(Name, Operand);
	if (Debugger) debug_add_global_variable(Session->Compiler->DebugInfo, Name, Operand->Address);
	return SUCCESS;
}

GLOBAL_FUNCTION(SessionGet, 2) {
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = Session->Compiler->try_lookup(0, Std$String$flatten(Args[1].Val));
	if (!Operand) return FAILURE;
	if (Operand->Type == operand_t::GVAR) {
		Result->Val = *(Result->Ref = Operand->Address);
		return SUCCESS;
	} else if (Operand->Type == operand_t::CNST) {
		Result->Val = Operand->Value;
		Result->Ref = 0;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

GLOBAL_FUNCTION(SessionSuggest, 2) {
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	session_t *Session = (session_t *)Args[0].Val;
	const char *Prefix = Std$String$flatten(Args[1].Val);
	int Length = Std$String$get_length(Args[1].Val);
	Std$Object$t *Matches = Agg$List$new0();
	stringtable_node *Entry = Session->Compiler->Global->NameTable->Entries;
	for (int I = Session->Compiler->Global->NameTable->Size; --I >= 0; ++Entry) {
		if (Entry->Key && !memcmp(Entry->Key, Prefix, Length)) {
			Agg$List$put(Matches, Std$String$new(Entry->Key));
		}
	}
	RETURN(Matches);
}

#ifndef DOCUMENTING
GLOBAL_FUNCTION(SetMissingIDFunc, 2) {
	session_t *Session = (session_t *)Args[0].Val;
	Session->Compiler->MissingIDFunc = Args[1].Val;
	return SUCCESS;
}

METHOD("set_missing_id_func", TYP, SessionT, ANY) {
	session_t *Session = (session_t *)Args[0].Val;
	Session->Compiler->MissingIDFunc = Args[1].Val;
	return SUCCESS;
}
#endif

METHOD("eval", TYP, SessionT) {
//@session
//:ANY
// The same as <code>SessionEval(session)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	if (setjmp(Session->Scanner->Error.Handler)) {
		Session->Scanner->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Scanner->Error.Type;
		Error->LineNo = Session->Scanner->Error.LineNo;
		Error->Message = Session->Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	command_expr_t *Command = accept_command(Session->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	if (setjmp(Session->Compiler->Error.Handler)) {
		Session->Compiler->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Compiler->Error.Type;
		Error->LineNo = Session->Compiler->Error.LineNo;
		Error->Message = Session->Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	return Command->compile(Session->Compiler, Result);
}

METHOD("eval_line", TYP, SessionT, ANY) {
//@session
//:ANY
// The same as <code>SessionEval(session)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	if (setjmp(Session->Scanner->Error.Handler)) {
		Session->Scanner->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Scanner->Error.Type;
		Error->LineNo = Session->Scanner->Error.LineNo;
		Error->Message = Session->Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	command_expr_t *Command = accept_command(Session->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	Args[1].Ref[0] = (Std$Object$t *)Std$String$new(Session->Scanner->NextChar);
	Session->Scanner->NextChar = "";
	if (setjmp(Session->Compiler->Error.Handler)) {
		Session->Compiler->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Compiler->Error.Type;
		Error->LineNo = Session->Compiler->Error.LineNo;
		Error->Message = Session->Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	return Command->compile(Session->Compiler, Result);
}

METHOD("expr", TYP, SessionT) {
//@session
//:ANY
// The same as <code>SessionExpr(session)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	if (setjmp(Session->Scanner->Error.Handler)) {
		Session->Scanner->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Scanner->Error.Type;
		Error->LineNo = Session->Scanner->Error.LineNo;
		Error->Message = Session->Scanner->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	expr_t *Expr = accept_expr(Session->Scanner);
	//Session->Scanner->accept(tkRBRACE);
#ifdef PARSER_LISTING
	Expr->print(0);
#endif
	if (setjmp(Session->Compiler->Error.Handler)) {
		Session->Compiler->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Compiler->Error.Type;
		Error->LineNo = Session->Compiler->Error.LineNo;
		Error->Message = Session->Compiler->Error.Message;
		Result->Val = (Std$Object$t *)Error;
		return MESSAGE;
	}
	return Expr->evaluate(Session->Compiler, Result);
}

METHOD("line", TYP, SessionT) {
//@session
//:Std$String$T
// The same as <code>SessionLine(session)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Std$String$new(Session->Scanner->NextChar);
	Session->Scanner->NextChar = "";
	return SUCCESS;
}

METHOD("lineno", TYP, SessionT, TYP, Std$Integer$SmallT) {
	session_t *Session = (session_t *)Args[0].Val;
	Session->Scanner->NextToken.LineNo = Std$Integer$get_small(Args[1].Val);
	RETURN0;
}

GLOBAL_METHOD(SessionPeek, 1, "peek", TYP, SessionT) {
	session_t *Session = (session_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Std$String$new(Session->Scanner->NextChar);
	return SUCCESS;
}

METHOD("def", TYP, SessionT, TYP, Std$String$T, ANY) {
//@session
//@name
//@value
// The same as <code>SessionDef(session, name, value)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::CNST;
	Operand->Value = Args[2].Val;
	Session->Compiler->declare(Std$String$flatten(Args[1].Val), Operand);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("var", TYP, SessionT, TYP, Std$String$T, ANY) {
//@session
//@name
//@variable
// The same as <code>SessionVar(session, name, variable)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::GVAR;
	Operand->Address = Args[2].Ref;
	Session->Compiler->declare(Std$String$flatten(Args[1].Val), Operand);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get", TYP, SessionT, TYP, Std$String$T) {
//@session
//@name
//:ANY
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = Session->Compiler->try_lookup(0, Std$String$flatten(Args[1].Val));
	if (!Operand) return FAILURE;
	if (Operand->Type == operand_t::GVAR) {
		Result->Val = *(Result->Ref = Operand->Address);
		return SUCCESS;
	} else if (Operand->Type == operand_t::CNST) {
		Result->Val = Operand->Value;
		Result->Ref = 0;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

METHOD(".", TYP, SessionT, TYP, Std$String$T) {
//@session
//@name
//:ANY
	session_t *Session = (session_t *)Args[0].Val;
	operand_t *Operand = Session->Compiler->try_lookup(0, Std$String$flatten(Args[1].Val));
	if (!Operand) return FAILURE;
	if (Operand->Type == operand_t::GVAR) {
		Result->Val = *(Result->Ref = Operand->Address);
		return SUCCESS;
	} else if (Operand->Type == operand_t::CNST) {
		Result->Val = Operand->Value;
		Result->Ref = 0;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

METHOD("suggest", TYP, SessionT, TYP, Std$String$T) {
	session_t *Session = (session_t *)Args[0].Val;
	const char *Prefix = Std$String$flatten(Args[1].Val);
	int Length = Std$String$get_length(Args[1].Val);
	Std$Object$t *Matches = Agg$List$new0();
	stringtable_node *Entry = Session->Compiler->Global->NameTable->Entries;
	for (int I = Session->Compiler->Global->NameTable->Size; --I >= 0; ++Entry) {
		if (Entry->Key && !memcmp(Entry->Key, Prefix, Length)) {
			Agg$List$put(Matches, Std$String$new(Entry->Key));
		}
	}
	RETURN(Matches);
}

METHOD("add", TYP, SessionT, TYP, ScopeT) {
//@session
//@scope
// Adds an existing scope to the global scopes of <var>session</var>.
// This code needs to be fixed, since scopes are stored in a linked list, and adding more than one scope to multiple sessions will not work correctly.
	session_t *Session = (session_t *)Args[0].Val;
	compiler_t::scope_t *Scope = (compiler_t::scope_t *)Args[1].Val;
	compiler_t::scope_t *Last = Session->Compiler->Global;
	while (Last->Up) Last = Last->Up;
	Last->Up = Scope;
	Result->Arg = Args[0];
	return SUCCESS;
}

INITIAL() {
	Riva$Module$add_loader("Wrapl", 90, wrapl_find, wrapl_load);
	detect_cpu_features();
	const char *SocketPath;
	if ((SocketPath = Riva$Config$get("Wrapl/Debug/Server"))) {
		debug_enable(SocketPath, false);
	} else if ((SocketPath = Riva$Config$get("Wrapl/Debug/Client"))) {
		debug_enable(SocketPath, true);
	}
}
