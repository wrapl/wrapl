#include <Riva.h>
#include <Std.h>
#include <IO/File.h>
#include <string.h>
#include <Sys/Module.h>

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
	Sys$Module_t *Module = (Sys$Module_t *)Sys$Module$new("Message");
	Sys$Module$export(Module, "Error", 0, (void *)ErrorMessageT);
	Sys$Module$export(Module, "SourceError", 0, (void *)SourceErrorMessageT);
	Sys$Module$export(Module, "ParseError", 0, (void *)ParseErrorMessageT);
	Sys$Module$export(Module, "InitError", 0, (void *)InitErrorMessageT);
	Sys$Module$export(Module, "TypeError", 0, (void *)TypeErrorMessageT);
	Sys$Module$export(Module, "ExternError", 0, (void *)ExternErrorMessageT);
	Sys$Module$export(Module, "ScopeErrorT", 0, (void *)ScopeErrorMessageT);
	return (Std$Object_t *)Module;
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
	IO$Stream_t *Source = (IO$Stream_t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) return 0;
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		printf("%s(%d): %s\n", Path, Scanner->Error.LineNo, Scanner->Error.Message);
		if (!Riva$Config$get("Wrapl/Loader/ContinueOnError")) exit(1);
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
		printf("%s(%d): %s\n", Path, Compiler->Error.LineNo, Compiler->Error.Message);
		for (int I = 0; I < Compiler->Error.Count; ++I) printf("\t%s\n", Compiler->Error.Stack[I]);
		if (!Riva$Config$get("Wrapl/Loader/ContinueOnError")) exit(1);
		return 0;
	}
	Expr->compile(Compiler);
	return 1;
}

TYPE(ErrorMessageT);
// The type of error messages sent by the compiler.

struct errormessage_t {
	const Std$Type_t *Type;
	int LineNo;
	const char *Message;
};

GLOBAL_FUNCTION(LoadModule, 1) {
//@filename:Std$String$T
//:Sys$Module$T
// Loads and compiles the module contained in the file <var>filename</var> and returns the module handle.
	Sys$Module_t *Module = Sys$Module$new(0);
	const char *Path = Std$String$flatten(Args[0].Val);
	IO$Stream_t *Source = (IO$Stream_t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = SourceErrorMessageT;
		Error->LineNo = 0;
		Error->Message = "Error: error opening file";
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	Expr->compile(Compiler);
	Result->Val = (Std$Object_t *)Module;
	return SUCCESS;
}

GLOBAL_FUNCTION(LoadExpr, 1) {
//@filename:Std$String$T
//:ANY
// Evaluates the expression contained in the file <var>filename</var> and returns the result.
	const char *Path = Std$String$flatten(Args[0].Val);
	IO$Stream_t *Source = (IO$Stream_t *)IO$File$open(Path, (IO$File_openflag)(IO$File$OPEN_READ | IO$File$OPEN_TEXT));
	if (Source == 0) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = SourceErrorMessageT;
		Error->LineNo = 0;
		Error->Message = "Error: error opening file";
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		IO$Stream$close(Source, IO$Stream$CLOSE_READ);
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	return Expr->evaluate(Compiler, Result);
}

GLOBAL_FUNCTION(ReadExpr, 1) {
//@source:IO$Stream$T
//:ANY
// Evaluates the expression read from <var>source</var> and returns the result.
	IO$Stream_t *Source = Args[0].Val;
	scanner_t *Scanner = new scanner_t(Source);
	if (setjmp(Scanner->Error.Handler)) {
		errormessage_t *Error = new errormessage_t;
		Error->Type = Scanner->Error.Type;
		Error->LineNo = Scanner->Error.LineNo;
		Error->Message = Scanner->Error.Message;
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	return Expr->evaluate(Compiler, Result);
}

struct session_t {
	const Std$Type_t *Type;
	scanner_t *Scanner;
	compiler_t *Compiler;
};

TYPE(SessionT);
//  An incremental Wrapl compiler/interpreter

METHOD("@", TYP, ErrorMessageT, VAL, Std$String$T) {
	errormessage_t *Error = (errormessage_t *)Args[0].Val;
	char *Buffer;
	Result->Val = (Std$Object_t *)Std$String$new_length(Buffer, asprintf(&Buffer, "(%d): %s", Error->LineNo, Error->Message));
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
	Result->Val = (Std$Object_t *)Session;
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
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	return Command->compile(Session->Compiler, Result);
}

GLOBAL_FUNCTION(SessionLine, 1) {
//@session : SessionT
//: Std$String$T
// Returns the rest of the current line stored by the scanner in <var>session</var>.
	session_t *Session = (session_t *)Args[0].Val;
	Result->Val = (Std$Object_t *)Std$String$new(Session->Scanner->NextChar);
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
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	command_expr_t *Command = accept_command(Session->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	Args[1].Ref[0] = (Std$Object_t *)Std$String$new(Session->Scanner->NextChar);
	Session->Scanner->NextChar = "";
	if (setjmp(Session->Compiler->Error.Handler)) {
		Session->Compiler->flush();
		errormessage_t *Error = new errormessage_t;
		Error->Type = Session->Compiler->Error.Type;
		Error->LineNo = Session->Compiler->Error.LineNo;
		Error->Message = Session->Compiler->Error.Message;
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
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
		Result->Val = (Std$Object_t *)Error;
		return MESSAGE;
	}
	return Expr->evaluate(Session->Compiler, Result);
}

METHOD("line", TYP, SessionT) {
//@session
//:Std$String$T
// The same as <code>SessionLine(session)</code>.
	session_t *Session = (session_t *)Args[0].Val;
	Result->Val = (Std$Object_t *)Std$String$new(Session->Scanner->NextChar);
	Session->Scanner->NextChar = "";
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
	operand_t *Operand = Session->Compiler->lookup(0, Std$String$flatten(Args[1].Val));
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

extern Std$Type_t ScopeT[];

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
