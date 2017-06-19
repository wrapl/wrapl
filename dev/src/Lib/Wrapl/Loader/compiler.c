#include "compiler.h"
#include "missing.h"
#include "debugger.h"
#include "wrapl.h"
#include "system.h"

#include <Std.h>
#include <Riva/Config.h>
#include <Riva/Debug.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

SYMBOL($AT, "@");

TYPE(ScopeT);

#if 0
#define DEBUG printf("%s.%d\n", __FILE__, __LINE__);
#else
#define DEBUG
#endif

void compiler_t::raise_error(int LineNo, const Std$Type_t *Type, const char *Format, ...) {DEBUG
	va_list Args;
	va_start(Args, Format);
	int Length = vasprintf((char **)&Error.Message, Format, Args);
	va_end(Args);
	Error.LineNo = LineNo;
	Error.Type = Type;
	Error.Count = Riva$Debug$stack_trace((void **)&LineNo, Error.Stack, 12);
	longjmp(Error.Handler, 1);
};

compiler_t::function_t::function_t(int LineNo) {DEBUG
	Loop = new loop_t;
	Loop->Expression = new loop_t::expression_t;
	Loop->Expression->Temps = new bitset_t();
	Frame.NoOfScopes = 1;
	Frame.Resend = new label_t;
	Frame.Resend->resend(LineNo);
};

operand_t *compiler_t::new_parameter(bool Indirect, bool Variadic, bool Default) {DEBUG
	++Function->Frame.NoOfParams;
	Function->Frame.Variadic |= Variadic;
	parameter_t *Parameter = new parameter_t;
	Parameter->Indirect = Indirect;
	Parameter->Default = Default;
	Parameter->Next = Function->Frame.Params;
	Function->Frame.Params = Parameter;
	operand_t *Operand = new operand_t;
	Operand->Type = Variadic ? operand_t::LVAR : operand_t::LREF;
	Operand->Index = Scope->LastIndex++;
	integertable_put(Function->VarTable, (uint32_t)Operand, Operand);
	return Operand;
};

operand_t *compiler_t::new_local(void) {DEBUG
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::LREF;
	Operand->Index = Scope->LastIndex++;
	integertable_put(Function->VarTable, (uint32_t)Operand, Operand);
	return Operand;
};

uint32_t compiler_t::new_temporary(uint32_t Count) {DEBUG
	int Index = Function->Loop->Expression->Temps->allocate(Count);
	if (Function->Frame.NoOfTemps <= Index + Count) Function->Frame.NoOfTemps = Index + Count + 1;
	//printf("Allocating temps: %d - %d\n", Index, Index + Count - 1);
	return Index;
};

void compiler_t::push_loop(uint32_t LineNo, label_t *Step, label_t *Exit) {DEBUG
	function_t::loop_t *Loop = new function_t::loop_t;
	Loop->Trap = Function->Loop->Trap;
	Loop->Receiver = Function->Loop->Receiver;
	Loop->Step = Step;
	Loop->Exit = Exit;
	Loop->Prev = Function->Loop;
	Loop->Must = 0;
	Loop->Expression = Function->Loop->Expression;
	Loop->LineNo = LineNo;
	Function->Loop = Loop;
};

void compiler_t::pop_loop() {DEBUG
	Function->Loop = Function->Loop->Prev;
};

void compiler_t::push_must(uint32_t LineNo, label_t *Entry, uint32_t Gate, operand_t *Temp) {
	function_t::loop_t::must_t *Must = new function_t::loop_t::must_t;
	Must->LineNo = LineNo;
	Must->Entry = Entry;
	Must->Gate = Gate;
	Must->Temp = Temp;
	Must->Prev = Function->Loop->Must;
	Function->Loop->Must = Must;
};

void compiler_t::pop_must() {
	Function->Loop->Must = Function->Loop->Must->Prev;
};

label_t *compiler_t::function_t::loop_t::must_t::ret() {
	if (!Return) {
		Return = new label_t;
		Return->load(LineNo, Temp);
		if (Prev) {
			Return->store_reg(LineNo, Prev->Temp);
			Return->store_link(LineNo, Prev->Gate, Prev->ret());
			Return->link(LineNo, Prev->Entry);
		} else {
			Return->ret(LineNo);
		};
	};
	return Return;
};

label_t *compiler_t::function_t::loop_t::must_t::fail() {
	if (!Fail) {
		Fail = new label_t;
		if (Prev) {
			Fail->store_link(LineNo, Prev->Gate, Prev->fail());
			Fail->link(LineNo, Prev->Entry);
		} else {
			Fail->fail(LineNo);
		};
	};
	return Fail;
};

void compiler_t::push_expression() {DEBUG
	//printf("push_expression\n");
	function_t::loop_t::expression_t *New = new function_t::loop_t::expression_t;
	function_t::loop_t::expression_t *Old = Function->Loop->Expression;
	New->Temps = new bitset_t(Old->Temps);
	New->Prev = Old;
	Function->Loop->Expression = New;
};

void compiler_t::pop_expression() {DEBUG
	//printf("pop_expression\n");
	function_t::loop_t::expression_t *Expression = Function->Loop->Expression;
	//uint32_t NoOfTemps = Expression->Temps->size();
	//printf("NoOfTemps = %d\n", NoOfTemps);
	//if (NoOfTemps > Function->Frame.NoOfTemps) Function->Frame.NoOfTemps = NoOfTemps;
	Function->Loop->Expression = Expression->Prev;
};

label_t *compiler_t::push_trap(uint32_t LineNo, label_t *Start, label_t *Failure) {DEBUG
	function_t::loop_t::trap_t *Trap = new function_t::loop_t::trap_t;
	Trap->Failure = Failure;
	Trap->Reserved = new_temporary();
	(Trap->Start0 = Start)->link(LineNo, Trap->Start = new label_t);
	Trap->Index = 0xFFFFFFFF;
	Trap->Prev = Function->Loop->Trap;
	Trap->LineNo = LineNo;
	Function->Loop->Trap = Trap;
	return Trap->Start;
};

uint32_t compiler_t::use_trap() {DEBUG
	function_t::loop_t::trap_t *Trap = Function->Loop->Trap;
	function_t::loop_t::expression_t *Expr = Function->Loop->Expression;
	if (Trap->Index == 0xFFFFFFFF) {
		uint32_t Index = Trap->Index = Trap->Reserved;
		Trap->Start0->init_trap(Trap->LineNo, Index, Trap->Failure);
	};
	return Trap->Index;
};

void compiler_t::back_trap(label_t *Start) {DEBUG
	function_t::loop_t::trap_t *Trap = Function->Loop->Trap;
	if (Trap->Index == 0xFFFFFFFF) {
		Start->link(Trap->LineNo, Trap->Failure);
	} else {
		Start->back(Trap->LineNo, Trap->Index);
	};
};

void compiler_t::pop_trap() {DEBUG
	Function->Loop->Trap = Function->Loop->Trap->Prev;
};

void compiler_t::push_function(int LineNo) {DEBUG
	function_t *Function = new function_t(LineNo);
	Function->Up = this->Function;
	this->Function = Function;
	push_scope(compiler_t::scope_t::SC_LOCAL);
	if (DebugInfo) Function->DebugInfo = debug_function(DebugInfo, LineNo);
};

frame_t *compiler_t::pop_function() {DEBUG
	pop_scope();
	frame_t *Frame = &Function->Frame;
	Frame->DebugInfo = Function->DebugInfo;
	//uint32_t NoOfTemps = Function->Loop->Expression->Temps->size();
	//if (NoOfTemps > Frame->NoOfTemps) Frame->NoOfTemps = NoOfTemps;
	Function = Function->Up;
	return Frame;
};

GLOBAL_FUNCTION(ScopeNew, 0) {
	compiler_t::scope_t *Scope = new compiler_t::scope_t(compiler_t::scope_t::SC_GLOBAL);
	Scope->Type0 = ScopeT;
	Result->Val = (Std$Object_t *)Scope;
	return SUCCESS;
};

METHOD("def", TYP, ScopeT, TYP, Std$String$T, ANY) {
	compiler_t::scope_t *Scope = (compiler_t::scope_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::CNST;
	Operand->Value = Args[2].Val;
	stringtable_put(Scope->NameTable, Std$String$flatten(Args[1].Val), Operand);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("var", TYP, ScopeT, TYP, Std$String$T, ANY) {
	compiler_t::scope_t *Scope = (compiler_t::scope_t *)Args[0].Val;
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::GVAR;
	Operand->Address = Args[2].Ref;
	stringtable_put(Scope->NameTable, Std$String$flatten(Args[1].Val), Operand);
	Result->Arg = Args[0];
	return SUCCESS;
};

void compiler_t::push_scope() {DEBUG
	if (Scope->Type == scope_t::SC_LOCAL) {
		int LastIndex = Scope->LastIndex;
		Scope = new scope_t(scope_t::SC_LOCAL, Scope);
		Scope->LastIndex = LastIndex;
		Scope->Function = Function;
	} else {
		Scope = new scope_t(scope_t::SC_GLOBAL, Scope);
	};
};

void compiler_t::push_scope(scope_t::type_t Type) {DEBUG
	if (Type == scope_t::SC_LOCAL) {
		Scope = new scope_t(scope_t::SC_LOCAL, Scope);
		//Scope->LastIndex = 0;
		Scope->Function = Function;
	} else {
		Scope = new scope_t(scope_t::SC_GLOBAL, Scope);
	};
};

void compiler_t::pop_scope() {DEBUG
	if (Scope->Type == scope_t::SC_LOCAL) {
		if (Scope->LastIndex > Function->Frame.NoOfLocals) Function->Frame.NoOfLocals = Scope->LastIndex;
	};
	Scope = Scope->Up;
};

void compiler_t::declare(const char *Name, operand_t *Operand) {DEBUG
	stringtable_put(Scope->NameTable, Name, Operand);
	if (DebugInfo) {
		if (Operand->Type == operand_t::LREF || Operand->Type == operand_t::LVAR) {
			debug_add_local(Function->DebugInfo, Name, Operand->Index);
		} else if (Operand->Type == operand_t::GVAR) {
			debug_add_global(DebugInfo, Name, Operand->Address);
		};
	};
};

operand_t *compiler_t::function_t::lookup(operand_t *Operand) {DEBUG
	operand_t *Operand2 = (operand_t *)integertable_get(VarTable, (uint32_t)Operand);
	if (Operand2 != (operand_t *)0xFFFFFFFF) return Operand2;
	Operand2 = new operand_t;
	if (Operand->Type == operand_t::LREF) {
		Operand2->Type = operand_t::LREF;
		Operand2->Index = -(++Frame.NoOfUpValues);
		upvalue_t *UpValue = new(upvalue_t);
		UpValue->Index = Up->lookup(Operand)->Index;
		UpValue->Next = Frame.UpValues;
		Frame.UpValues = UpValue;
	} else if (Operand->Type == operand_t::TREG) {
		Operand2->Type = operand_t::TREG;
		Operand2->Index = -(++Frame.NoOfUpTemps);
		uptemp_t *UpTemp = new(uptemp_t);
		UpTemp->Index = Up->lookup(Operand)->Index;
		UpTemp->Next = Frame.UpTemps;
		Frame.UpTemps = UpTemp;
	} else if (Operand->Type == operand_t::CNST) {
		return Operand;
	} else {
		printf("Internal error at %s:%d: operand type is %d", __FILE__, __LINE__, Operand->Type);
	};
	integertable_put(VarTable, (uint32_t)Operand, Operand2);
	return Operand2;
};

TYPE(ExternErrorMessageT, ErrorMessageT);
TYPE(ScopeErrorMessageT, ErrorMessageT);
TYPE(SemanticErrorMessageT, ErrorMessageT);

struct future_t {
	int LineNo;
	
	future_t(int LineNo) : LineNo(LineNo) {};
	
	virtual void resolve(compiler_t *Compiler, operand_t *Operand) = 0;
};

operand_t *compiler_t::try_lookup(int LineNo, const char *Name) {DEBUG
	for (scope_t *Scope = this->Scope; Scope; Scope = Scope->Up) {
		operand_t *Operand = (operand_t *)stringtable_get(Scope->NameTable, Name);
		if (Operand) {
			switch (Scope->Type) {
			case scope_t::SC_GLOBAL: {
				if (Operand->Type == operand_t::FUTR) Operand->Future->resolve(this, Operand);
				return Operand;
			};
			case scope_t::SC_LOCAL: {
				if (Scope->Function == Function) return Operand;
				return Function->lookup(Operand);
			};
			};
		};
	};
	return 0;
};

operand_t *compiler_t::lookup(int LineNo, const char *Name) {DEBUG
	operand_t *Operand = try_lookup(LineNo, Name);
	if (Operand) return Operand;
	if (MissingIDFunc) {
		Std$Function_result Result[1];
		switch (Std$Function$call(MissingIDFunc, 1, Result, Std$String$new(Name), 0)) {
		case SUSPEND: case SUCCESS: {
			operand_t *Operand = new operand_t;
			if (Result->Ref) {
				Operand->Type = operand_t::GVAR;
				Operand->Address = Result->Ref;
			} else {
				Operand->Type = operand_t::CNST;
				Operand->Value = Result->Val;
			};
			//declare(Name, Operand);
			return Operand;
		};
		case FAILURE: case MESSAGE: {
			raise_error(LineNo, ScopeErrorMessageT, "Error: failed to provide identifier %s", Name);
		};
		};
	};
	raise_error(LineNo, ScopeErrorMessageT, "Error: identifier %s not declared", Name);
};

#if defined(PARSER_LISTING) || defined(ASSEMBLER_LISTING)

void assign_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(" <- ");
	Right->print(Indent);
};

void rassign_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(" -> ");
	Right->print(Indent);
};

void ref_assign_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(" <<= ");
	Right->print(Indent);
};

void invoke_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Function->print(Indent);
	printf("(");
	if (Args) {
		Args->print(Indent);
		for (expr_t *Arg = Args->Next; Arg; Arg = Arg->Next) {
			printf(", ");
			Arg->print(Indent);
		};
	};
	printf(")");
};

void parallel_invoke_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Function->print(Indent);
	printf("{");
	if (Args) {
		Args->print(Indent);
		for (expr_t *Arg = Args->Next; Arg; Arg = Arg->Next) {
			printf(", ");
			Arg->print(Indent);
		};
	};
	printf("}");
};

void func_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("<");
	if (Parameters) {
		printf("%s", Parameters->Name);
		for (parameter_t *Param = Parameters->Next; Param; Param = Param->Next) {
			printf(", %s", Param->Name);
		};
	};
	printf("> ");
	Body->print(Indent);
};

void code_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("[");
	printf("] ");
	Body->print(Indent);
};

void ident_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("%s", Name);
};

void qualident_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("%s", Names->Ident);
	for (qualident_expr_t::name_t *Name = Names->Next; Name; Name = Name->Next) printf(".%s", Name->Ident);
};

void const_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Std$Function_result Result;
	if (Std$Function$call((Std$Object_t *)$AT, 2, &Result, Operand->Value, 0, Std$String$T, 0) < FAILURE) {
		printf("%s", Std$String$flatten(Result.Val));
	} else {
		printf("#%x", Operand->Value);
	};
};

void backquote_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("`");
	Expr->print(Indent);
	printf("`");
};

void ret_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("RET ");
	Value->print(Indent);
};

void susp_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("SUSP ");
	Value->print(Indent);
};

void fail_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("FAIL");
};

void back_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("BACK");
};

void with_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("WITH %s %s <- ", Parallel ? "PAR" : "SEQ", Bindings->Name);
	Bindings->Value->print(Indent);
	for (binding_t *Binding = Bindings->Next; Binding; Binding = Binding->Next) {
		printf(", %s <- ", Binding->Name);
		Binding->Value->print(Indent);
	};
	printf(" DO ");
	Body->print(Indent);
};

void rep_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("REP ");
	Body->print(Indent);
};

void exit_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("EXIT ");
	Value->print(Indent);
};

void step_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("STEP");
};

void every_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("EVERY ");
	Condition->print(Indent);
	printf(" DO ");
	Body->print(Indent);
};

void all_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("ALL ");
	Value->print(Indent);
};

void uniq_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("UNIQ ");
	Value->print(Indent);
};

void count_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("count ");
	Value->print(Indent);
};

void send_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("SEND ");
	Value->print(Indent);
};

void self_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("$");
};

void sequence_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Exprs->print(Indent);
	for (expr_t *Expr = Exprs->Next; Expr; Expr = Expr->Next) {
		printf(" | ");
		Expr->print(Indent);
	};
};

void typeof_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("?");
	Expr->print(Indent);
};

void valueof_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf(".");
	Expr->print(Indent);
};

void limit_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Limit->print(Indent);
	printf(" OF ");
	Expr->print(Indent);
};

void skip_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Expr->print(Indent);
	printf(" SKIP ");
	Skip->print(Indent);
};

void infinite_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("|");
	Expr->print(Indent);
};

void parallel_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Exprs->print(Indent);
	for (expr_t *Expr = Exprs->Next; Expr; Expr = Expr->Next) {
		printf(" ! ");
		Expr->print(Indent);
	};
};

void interleave_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Exprs->print(Indent);
	for (expr_t *Expr = Exprs->Next; Expr; Expr = Expr->Next) {
		printf(" # ");
		Expr->print(Indent);
	};
};

void left_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(" \\ ");
	Right->print(Indent);
};

void right_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(" & ");
	Right->print(Indent);
};

void cond_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Condition->print(Indent);
	if (Success) {
		printf(" => ");
		Success->print(Indent);
	};
	if (Failure) {
		printf(" // ");
		Failure->print(Indent);
	};
};

void comp_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	Left->print(Indent);
	printf(Eq ? " == " : " ~== ");
	Right->print(Indent);
};

void when_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("WHEN ");
	Condition->print(Indent);
	printf("\n");
	for (case_t *Case = Cases; Case; Case = Case->Next) {
		for (int I = Indent; I--;) printf("    ");
		printf("IS ");
		case_t::range_t *Range = Case->Ranges;
		Range->Min->print(Indent);
		if (Range->Max) {
			printf(" .. ");
			Range->Max->print(Indent);
		};
		for (Range = Range->Next; Range; Range = Range->Next) {
			printf(", ");
			Range->Min->print(Indent);
			if (Range->Max) {
				printf(" .. ");
				Range->Max->print(Indent);
			};
		};
		printf(" DO ");
		Case->Body->print(Indent);
		printf("\n");
	};
	for (int I = Indent; I--;) printf("    ");
	printf("DO ");
	Default->print(Indent);
};

void whentype_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("WHEN ");
	Condition->print(Indent);
	printf("\n");
	for (case_t *Case = Cases; Case; Case = Case->Next) {
		for (int I = Indent; I--;) printf("    ");
		printf("IS ");
		expr_t *Type = Case->Types;
		Type->print(Indent);
		for (Type = Type->Next; Type; Type = Type->Next) {
			printf(", ");
			Type->print(Indent);
		};
		printf(" DO ");
		Case->Body->print(Indent);
		printf("\n");
	};
	for (int I = Indent; I--;) printf("    ");
	printf("DO ");
	Default->print(Indent);
};

void block_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	if (Vars || Body || Receiver.Body) {
		printf("(\n");
		++Indent;
		for (localvar_t *Var = Vars; Var; Var = Var->Next) {
			for (int I = Indent; I--;) printf("    ");
			printf(Var->Name);
			printf(";\n");
		};
		for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
			for (int I = Indent; I--;) printf("    ");
			Expr->print(Indent);
			printf(";\n");
		};
		if (Receiver.Body) {
			for (int I = Indent; I--;) printf("    ");
			printf("RECV %s DO ", Receiver.Var);
			Receiver.Body->print(Indent);
			printf(";\n");
		};
		if (Final) {
			for (int I = Indent; I--;) printf("    ");
			Final->print(Indent);
			printf(";\n");
		};
		--Indent;
		for (int I = Indent; I--;) printf("    ");
		printf(")");
	} else if (Final) {
		printf("(");
		Final->print(Indent);
		printf(")");
	} else {
		printf("()");
	};
};

void module_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	printf("MOD %s;\n", Name);
	printf("\n");
	if (Imps) {
		for (globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
			printf("IMP ");
			if (Imp->Relative) printf(".");
			printf("%s", Imp->Path->Part);
			for (globalimp_t::path_t *Path = Imp->Path->Next; Path; Path = Path->Next) printf(".%s", Path->Part);
			printf(" AS %s", Imp->Alias);
			if (Imp->Uses) {
				printf(" USE %s", Imp->Uses->Name);
				for (globalimp_t::uselist_t *Use = Imp->Uses->Next; Use; Use = Use->Next) printf(", %s", Use->Name);
			};
			printf(";\n");
		};
		printf("\n");
	};
	if (Defs) {
		for (globaldef_t *Def = Defs; Def; Def = Def->Next) {
			printf("DEF %s%s <- ", Def->Name, Def->Exported ? "!" : "");
			Def->Value->print(0);
			printf(";\n");
		};
		printf("\n");
	};
	if (Vars) {
		for (globalvar_t *Var = Vars; Var; Var = Var->Next) {
			printf("VAR %s%s;\n", Var->Name, Var->Exported ? "!" : "");
		};
		printf("\n");
	};
	if (Body) {
		for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
			Expr->print(0);
			printf(";\n");
		};
		printf("\n");
	};
	printf("END %s.\n", Name);
};

void command_expr_t::print(int Indent) {
	printf("[L%d]", LineNo);
	if (Imps) {
		for (globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
			printf("IMP ");
			if (Imp->Relative) printf(".");
			printf("%s", Imp->Path->Part);
			for (globalimp_t::path_t *Path = Imp->Path->Next; Path; Path = Path->Next) printf(".%s", Path->Part);
			printf(" AS %s", Imp->Alias);
			if (Imp->Uses) {
				printf(" USE %s", Imp->Uses->Name);
				for (globalimp_t::uselist_t *Use = Imp->Uses->Next; Use; Use = Use->Next) printf(", %s", Use->Name);
			};
			printf(";\n");
		};
	};
	if (Defs) {
		for (globaldef_t *Def = Defs; Def; Def = Def->Next) {
			printf("DEF %s <- ", Def->Name);
			Def->Value->print(0);
			printf(";\n");
		};
	};
	if (Vars) {
		for (globalvar_t *Var = Vars; Var; Var = Var->Next) {
			printf("VAR %s;\n", Var->Name);
		};
	};
	if (Body) {
		for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
			Expr->print(0);
			printf(";\n");
		};
	};
};

#endif

operand_t *assign_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *Dest = Left->compile(Compiler, Start, Label0);
	if (operand_t *Src = Right->constant(Compiler)) {
		Label0->store_con(LineNo, Dest, Src->Value);
		Label0->link(LineNo, Success);
		return Src;
	};
	if (Dest == Register) {
		label_t *Label1 = new label_t;
		Dest = new operand_t;
		Dest->Type = operand_t::TREF;
		Dest->Index = Compiler->new_temporary();
		Label0->store_ref(LineNo, Dest);
		Label0->link(LineNo, Label1);
		Label0 = Label1;
	};
	operand_t *OldSelf = Compiler->Function->Loop->Self;
	Compiler->Function->Loop->Self = Dest;
		label_t *Label1 = new label_t;
		operand_t *Src = Right->compile(Compiler, Label0, Label1);
		if (Src->Type == operand_t::CNST) {
			Label1->store_con(LineNo, Dest, Src->Value);
		} else {
			Label1->load(LineNo, Src);
			Label1->store_val(LineNo, Dest);
		};
		Label1->link(LineNo, Success);
	Compiler->Function->Loop->Self = OldSelf;
	return Register;
};

operand_t *rassign_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *Src = Left->compile(Compiler, Start, Label0);
	if (Src == Register) {
		label_t *Label1 = new label_t;
		Src = new operand_t;
		Src->Type = operand_t::TREG;
		Src->Index = Compiler->new_temporary();
		Label0->store_reg(LineNo, Src);
		Label0->link(LineNo, Label1);
		Label0 = Label1;
	};
	label_t *Label1 = new label_t;
	operand_t *Dest = Right->compile(Compiler, Label0, Label1);
	if (Dest == Register) {
		label_t *Label2 = new label_t;
		Dest = new operand_t;
		Dest->Type = operand_t::TREF;
		Dest->Index = Compiler->new_temporary();
		Label1->store_ref(LineNo, Dest);
		Label1->link(LineNo, Label2);
		Label1 = Label2;
	};
	if (Src->Type == operand_t::CNST) {
		Label1->store_con(LineNo, Dest, Src->Value);
	} else {
		Label1->load(LineNo, Src);
		Label1->store_val(LineNo, Dest);
	};
	Label1->link(LineNo, Success);
	return Register;
};

operand_t *ref_assign_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *Dest = Left->compile(Compiler, Start, Label0);
	label_t *Label1 = new label_t;
	Label1->load(LineNo, Right->compile(Compiler, Label0, Label1));
	Label1->store_ref(LineNo, Dest);
	Label1->link(LineNo, Success);
	return Register;
};

operand_t *invoke_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *FunctionOperand = Function->compile(Compiler, Start, Label0);
	if ((FunctionOperand->Type == operand_t::CNST) && (FunctionOperand->Value->Type == Std$Integer$SmallT)) {
		int Selected = ((Std$Integer_smallt *)FunctionOperand->Value)->Value;
		if (Selected <= 0) {
			for (expr_t *Arg = Args; Arg; Arg = Arg->Next) Selected++;
			Selected++;
		};
		label_t *Label2 = new label_t;
		uint32_t Count = 0;
		operand_t *Result = 0;
		for (expr_t *Arg = Args; Arg; Arg = Arg->Next) {
			Count++;
			label_t *Label1 = new label_t;
			if (Count == Selected) {
				Result = Arg->compile(Compiler, Label0, Label1);
				if ((Result == Register) && (Arg->Next)) {
					Result = new operand_t;
					Result->Type = operand_t::TREG;
					Result->Index = Compiler->new_temporary();
					Label1->store_reg(LineNo, Result);
				};
			} else {
				Arg->compile(Compiler, Label0, Label1);
			};
			Label0 = new label_t;
			Label1->link(LineNo, Label0);
		};
		if (Result) {
			Label0->link(LineNo, Success);
			return Result;
		} else {
			Compiler->back_trap(Label0);
			return Register;
		};
	} else {
		label_t *Label2 = new label_t;
		uint32_t Count = 0;
		uint32_t ArgsArray = 0;
		uint32_t Trap = Compiler->use_trap();
		if (Args) {
			for (expr_t *Arg = Args; Arg; Arg = Arg->Next) ++Count;
			ArgsArray = Compiler->new_temporary(Count);
			if (FunctionOperand == Register) {
				FunctionOperand = new operand_t;
				FunctionOperand->Type = operand_t::TVAR;
				FunctionOperand->Index = Compiler->new_temporary();
				label_t *Label1 = new label_t;
				Label0->store_reg(LineNo, FunctionOperand);
				Label0->link(LineNo, Label1);
				Label0 = Label1;
			};
			uint32_t Index = ArgsArray;
			for (expr_t *Arg = Args; Arg; Arg = Arg->Next) {
				label_t *Label1 = new label_t;
				operand_t *Value = Arg->compile(Compiler, Label0, Label1);
				Label1->store_arg(LineNo, Index, Value);
				Label2->fixup_arg(LineNo, Index, Value);
				Label0 = new label_t;
				Label1->link(LineNo, Label0);
				++Index;
			};
		};
		Label0->link(LineNo, Label2);
		Label2->load(LineNo, FunctionOperand);
		Label2->invoke(LineNo, Trap, ArgsArray, Count, Label2);
		Label2->link(LineNo, Success);
	};
	return Register;
};

operand_t *parallel_invoke_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *FunctionOperand = Function->compile(Compiler, Start, Label0);
	//if ((FunctionOperand->Type == operand_t::CNST) && (FunctionOperand->Value->Type == Std$Integer$SmallT)) {
	//	printf("%s:%d: I'm not done yet!\n", __FILE__, __LINE__);
	//} else {
		label_t *LabelF = new label_t;
		uint32_t Count = 0;
		uint32_t ArgsArray = 0;
		uint32_t Trap = Compiler->use_trap();
		if (Args) {
			for (expr_t *Arg = Args; Arg; Arg = Arg->Next) ++Count;
			ArgsArray = Compiler->new_temporary(Count);
			if (FunctionOperand == Register) {
				FunctionOperand = new operand_t;
				FunctionOperand->Type = operand_t::TVAR;
				FunctionOperand->Index = Compiler->new_temporary();
				label_t *Label1 = new label_t;
				Label0->store_reg(LineNo, FunctionOperand);
				Label0->link(LineNo, Label1);
				Label0 = Label1;
			};
			uint32_t Index = ArgsArray;
			expr_t *Arg = Args;
			uint32_t Gate = Compiler->new_temporary();

			label_t *LabelH = new label_t;
			label_t *LabelG = new label_t;
			label_t *Label4 = LabelH;

			while (Arg->Next) {
				label_t *Label2 = new label_t;
				label_t *Label3 = new label_t;
				label_t *Label5 = new label_t;
				label_t *Label6 = new label_t;

				Label0 = Compiler->push_trap(LineNo, Label0, LabelG);
					Label0->store_link(LineNo, Gate, Label2);
					Label0->link(LineNo, Label6);
					operand_t *Value = Arg->compile(Compiler, Label6, Label3);
					Label3->store_arg(LineNo, Index, Value);
					LabelF->fixup_arg(LineNo, Index, Value);
					Label4->store_link(LineNo, Gate, Label5);
					Compiler->back_trap(Label4);
					Label4 = Label5;
					Label3->jump_link(LineNo, Gate);
					Label0 = Label2;
				Compiler->pop_trap();

				Arg = Arg->Next;
				++Index;
			};
			label_t *Label3 = new label_t;
			Label0 = Compiler->push_trap(LineNo, Label0, LabelG);
				operand_t *Value = Arg->compile(Compiler, Label0, Label3);
				Label3->store_arg(LineNo, Index, Value);
				LabelF->fixup_arg(LineNo, Index, Value);
				Compiler->back_trap(Label4);
			Compiler->pop_trap();
			Label3->push_trap(LineNo, Trap, LabelH, Gate);
			Label0 = Label3;
			LabelG->back(LineNo, Trap);
		};
		Label0->link(LineNo, LabelF);
		LabelF->load(LineNo, FunctionOperand);
		LabelF->invoke(LineNo, Trap, ArgsArray, Count, LabelF);
		LabelF->link(LineNo, Success);
	//};
	return Register;
};

operand_t *func_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	if (Constant && Constant->Value->Type == WraplT) return Constant;
	if (Parameters == 0) {
		operand_t *Constant = Body->constant(Compiler);
		if (Constant) {
			operand_t *Closure = new operand_t;
			Closure->Type = operand_t::CNST;
			Closure->Value = Std$Function$constant_new(Constant->Value);
			Start->link(LineNo, Success);
			return Closure;
		};
	};
	label_t *StartF = new label_t;
	Compiler->push_function(LineNo);
		Compiler->push_expression();
			label_t *Start0 = StartF;
			label_t *Success0 = new label_t;
			label_t *Failure0 = new label_t;
			for (func_expr_t::parameter_t *Parameter = Parameters; Parameter; Parameter = Parameter->Next) {
				if (Parameter->Default) {
					operand_t *Operand = Compiler->new_parameter(Parameter->Reference, Parameter->Variadic, true);
					Compiler->declare(Parameter->Name, Operand);
					label_t *Label0 = new label_t;
					label_t *Label1 = new label_t;
					label_t *Label2 = new label_t;
					Compiler->push_expression();
						Start0 = Compiler->push_trap(LineNo, Start0, Failure0);
							Start0->test_param(LineNo, Parameter->Reference, Operand->Index, Label0);
							Start0->link(LineNo, Label1);
							operand_t *Default = Parameter->Default->compile(Compiler, Label1, Label2);
							Label2->load(LineNo, Default);
							Label2->default_param(LineNo, Parameter->Reference, Operand->Index);
						Compiler->pop_trap();
					Compiler->pop_expression();
					Start0 = new label_t;
					Label0->link(LineNo, Start0);
					Label2->link(LineNo, Start0);
				} else {
					operand_t *Operand = Compiler->new_parameter(Parameter->Reference, Parameter->Variadic, false);
					Compiler->declare(Parameter->Name, Operand);
				};
			};
			Success0->load(LineNo, Body->compile(Compiler, Compiler->push_trap(LineNo, Start0, Failure0), Success0));
			Success0->ret(LineNo);
			Failure0->fail(LineNo);
			Compiler->pop_trap();
		Compiler->pop_expression();
	frame_t *Frame = Compiler->pop_function();
//#ifdef ASSEMBLER_LISTING
//	print(0);
//	printf("\n");
//#endif
	operand_t *Closure = StartF->assemble(Frame, Compiler->SourceName, LineNo, Constant);
	if (Closure == 0) Compiler->raise_error(LineNo, ScopeErrorMessageT, "Error: context does not allow access to surrounding scope");
	Start->link(LineNo, Success);
	return Closure;
};

extern "C" void compile_prefunc(func_expr_t *Expr) {
	compiler_t *Compiler = Expr->_Compiler;
	printf("Using new compile on demand feature in %s @ %x\n", Compiler->SourceName, Expr->Constant);
	Expr->compile(Expr->_Compiler, new label_t, new label_t);
	Expr->_Compiler = 0;
	printf("Used new compile on demand feature in %s @ %x\n", Compiler->SourceName, Expr->Constant);
};

operand_t *func_expr_t::precompile(compiler_t *Compiler, precomp_t &Type) {DEBUG
	Constant = new operand_t;
	Constant->Type = operand_t::CNST;
	if (Parameters == 0) {
		operand_t *Value = Body->constant(Compiler);
		if (Value) {
			Constant->Value = Std$Function$constant_new(Value->Value);
			Type = _PC_FULL;
			return Constant;
		};
	};
	closure_t *Closure = new closure_t;
	Closure->Type = WraplPreT;
	_Compiler = Compiler;
	Closure->Entry = (void *)this;
	Constant->Value = (Std$Object_t *)Closure;
	Type = _PC_PARTIAL;
	return Constant;
};

operand_t *code_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *StartF = new label_t;
	Compiler->push_expression();
		label_t *Start0 = StartF;
		label_t *Success0 = new label_t;
		label_t *Failure0 = new label_t;
		label_t *Start1 = Compiler->push_trap(LineNo, Start0, Failure0);
		Success0->load(LineNo, Body->compile(Compiler, Start1, Success0));
		Success0->susp(LineNo, true);
		Compiler->back_trap(Success0);
		Failure0->fail(LineNo, true);
		Compiler->pop_trap();
	Compiler->pop_expression();
//#ifdef ASSEMBLER_LISTING
//	print(0);
//	printf("\n");
//#endif
	Start->load_code(LineNo, StartF);
	Start->link(LineNo, Success);
	return Register;
};

operand_t *ident_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Start->link(LineNo, Success);
	return Compiler->lookup(LineNo, Name);
};

operand_t *ident_expr_t::constant(compiler_t *Compiler) {DEBUG
	operand_t *Operand = Compiler->lookup(LineNo, Name);
	if (Operand->Type != operand_t::CNST) return 0;
	return Operand;
};

TYPE(TypeErrorMessageT, ErrorMessageT);
SYMBOL($DOT, ".");

operand_t *qualident_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	static operand_t DotOp[1] = {{0, operand_t::CNST, $DOT}};
	const char *Ident = Names->Ident;
	operand_t *Operand = Compiler->lookup(LineNo, Ident);
	for (qualident_expr_t::name_t *Name = Names->Next; Name; Name = Name->Next) {
		if ((Operand->Type != operand_t::CNST) || (Operand->Value->Type != Sys$Module$T)) {
			//Compiler->raise_error(LineNo, TypeErrorMessageT, "Error: %s is not constant", Ident);
			operand_t *Field = new operand_t;
			Field->Type = operand_t::CNST;
			Field->Value = (Std$Object_t *)Std$String$new(Name->Ident);

			uint32_t Trap = Compiler->use_trap();
			uint32_t Args = Compiler->new_temporary(2);
			label_t *Label1 = new label_t;
			label_t *Label2 = new label_t;

			Start->store_arg(LineNo, Args, Operand);
			Start->store_arg(LineNo, Args + 1, Field);
			Start->link(LineNo, Label1);

			Label1->fixup_arg(LineNo, Args, Operand);
			Label1->fixup_arg(LineNo, Args + 1, Field);
			Label1->load(LineNo, DotOp);
			Label1->invoke(LineNo, Trap, Args, 2, Label1);
			Label1->link(LineNo, Label2);

			Start = Label2;
			Operand = Register;
		} else {
			Sys$Module_t *Module = (Sys$Module_t *)Operand->Value;
			Operand = new operand_t;
			if (Sys$Module$import(Module, Name->Ident, (int *)&Operand->Type, (void **)&Operand->Value) == 0) {
				Compiler->raise_error(LineNo, ExternErrorMessageT, "Error: import not found %s.%s", Ident, Name->Ident);
			};
			Ident = Name->Ident;
		};
	};
	Start->link(LineNo, Success);
	return Operand;
};

operand_t *qualident_expr_t::constant(compiler_t *Compiler) {DEBUG
	const char *Ident = Names->Ident;
	operand_t *Operand = Compiler->try_lookup(LineNo, Ident);
	if (Operand == 0) return 0;
	for (qualident_expr_t::name_t *Name = Names->Next; Name; Name = Name->Next) {
		if (Operand->Type != operand_t::CNST) return 0;
		if (Operand->Value->Type != Sys$Module$T) return 0;
		Sys$Module_t *Module = (Sys$Module_t *)Operand->Value;
		Operand = new operand_t;
		if (Sys$Module$import(Module, Name->Ident, (int *)&Operand->Type, (void **)&Operand->Value) == 0) {
			Compiler->raise_error(LineNo, ExternErrorMessageT, "Error: import not found %s.%s", Ident, Name->Ident);
		};
		if (Operand->Type != operand_t::CNST) return 0;
		Ident = Name->Ident;
	};
	return Operand;
};

operand_t *const_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Start->link(LineNo, Success);
	return Operand;
};

operand_t *backquote_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Std$Function_result Result;
	switch (Expr->evaluate(Compiler, &Result)) {
	case SUSPEND: // Should we create a Function.IteratorT here???
	case SUCCESS: {
		operand_t *Operand = new operand_t;
		if (Result.Ref) {
			Operand->Type = operand_t::GVAR;
			Operand->Address = Result.Ref;
		} else {
			Operand->Type = operand_t::CNST;
			Operand->Value = Result.Val;
		};
		Start->link(LineNo, Success);
		return Operand;
	};
	case FAILURE: {
		Compiler->back_trap(Start);
		return Register;
	};
	case MESSAGE: {
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::CNST;
		Operand->Value = Result.Val;
		Start->load(LineNo, Operand);
		Start->send(LineNo);
		return Register;
	};
	};
};

/*
operand_t *backquote_expr_t::constant(compiler_t *Compiler) {DEBUG
};
*/

operand_t *ret_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;
	Start = Compiler->push_trap(LineNo, Start, Label1);
		Label0->load(LineNo, Value->compile(Compiler, Start, Label0));
	Compiler->pop_trap();
	compiler_t::function_t::loop_t::must_t *Must = Compiler->Function->Loop->Must;
	if (Must) {
		Label0->store_reg(LineNo, Must->Temp);
		Label0->store_link(LineNo, Must->Gate, Must->ret());
		Label0->link(LineNo, Must->Entry);
		Label1->store_link(LineNo, Must->Gate, Must->fail());
		Label1->link(LineNo, Must->Entry);
	} else {
		Label0->ret(LineNo);
		Label1->fail(LineNo);
	};
	return Register;
};

operand_t *susp_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	Label0->load(LineNo, Value->compile(Compiler, Start, Label0));
	Label0->susp(LineNo, false);
	Label0->link(LineNo, Success);
	return Register;
};

operand_t *fail_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	compiler_t::function_t::loop_t::must_t *Must = Compiler->Function->Loop->Must;
	if (Must) {
		Start->store_link(LineNo, Must->Gate, Must->fail());
		Start->link(LineNo, Must->Entry);
	} else {
		Start->fail(LineNo);
	};
	return Register;
};

operand_t *back_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Compiler->back_trap(Start);
	return Register;
};

operand_t *with_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {
	if (Parallel) {
		Compiler->push_scope();
			uint32_t Gate = Compiler->new_temporary();
			uint32_t Trap = Compiler->use_trap();

			label_t *Label1 = Start;
			label_t *LabelF = new label_t;
			label_t *LabelG = new label_t;
			label_t *Label4 = LabelF;

			binding_t *Binding = Bindings;

			while (Binding->Next) {
				label_t *Label2 = new label_t;
				label_t *Label3 = new label_t;
				label_t *Label5 = new label_t;
				label_t *Label6 = new label_t;

				Label1 = Compiler->push_trap(LineNo, Label1, LabelG);
					Label1->store_link(LineNo, Gate, Label2);
					Label1->link(LineNo, Label6);

					operand_t *Value = Binding->Value->compile(Compiler, Label6, Label3);
					if (Value == Register) {
						Value = new operand_t;
						Value->Type = operand_t::TREG;
						Value->Index = Compiler->new_temporary();
						integertable_put(Compiler->Function->VarTable, (uint32_t)Value, Value);
						Label3->store_reg(LineNo, Value);
					};
					Compiler->declare(Binding->Name, Value);

					Label4->store_link(LineNo, Gate, Label5);
					Compiler->back_trap(Label4);
					Label4 = Label5;
					Label3->jump_link(LineNo, Gate);
					Label1 = Label2;
				Compiler->pop_trap();
				Binding = Binding->Next;
			};
			label_t *Label3 = new label_t;
			Label1 = Compiler->push_trap(LineNo, Label1, LabelG);
				operand_t *Value = Binding->Value->compile(Compiler, Label1, Label3);
				if (Value == Register) {
					Value = new operand_t;
					Value->Type = operand_t::TREG;
					Value->Index = Compiler->new_temporary();
					integertable_put(Compiler->Function->VarTable, (uint32_t)Value, Value);
					Label3->store_reg(LineNo, Value);
				};
				Compiler->declare(Binding->Name, Value);
				Compiler->back_trap(Label4);
			Compiler->pop_trap();
			Label3->push_trap(LineNo, Trap, LabelF, Gate);
			label_t *Label0 = new label_t;
			Label3->link(LineNo, Label0);
			LabelG->back(LineNo, Trap);
			operand_t *Result = Body->compile(Compiler, Label0, Success);
		Compiler->pop_scope();
		return Result;
	} else {
		Compiler->push_scope();
			label_t *Label0 = Start;
			for (binding_t *Binding = Bindings; Binding; Binding = Binding->Next) {
				label_t *Label1 = new label_t;
				operand_t *Value = Binding->Value->compile(Compiler, Label0, Label1);
				if (Value == Register) {
					Value = new operand_t;
					Value->Type = operand_t::TREG;
					Value->Index = Compiler->new_temporary();
					integertable_put(Compiler->Function->VarTable, (uint32_t)Value, Value);
					Label1->store_reg(LineNo, Value);
					label_t *Label2 = new label_t;
					Label1->link(LineNo, Label2);
					Label0 = Label2;
				} else {
					Label0 = Label1;
				};
				Compiler->declare(Binding->Name, Value);
			};
			operand_t *Result = Body->compile(Compiler, Label0, Success);
		Compiler->pop_scope();
		return Result;
	};
};

operand_t *rep_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label2 = new label_t;
	Label0->link(LineNo, Start);
	label_t *Label1 = Compiler->push_trap(LineNo, Start, Label0);
		Compiler->push_loop(LineNo, Label1, Success);
			Body->compile(Compiler, Label1, Label2);
			Label2->flush(LineNo);
			Label2->link(LineNo, Start);
		Compiler->pop_loop();
	Compiler->pop_trap();
	return Register;
};

operand_t *exit_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	compiler_t::function_t::loop_t *Loop = Compiler->Function->Loop;
	compiler_t::function_t::loop_t *Prev = Loop->Prev;
	if (Prev == 0) Compiler->raise_error(LineNo, SemanticErrorMessageT, "Error: not inside loop");
	if (Prev->Receiver != Loop->Receiver) {
		label_t *Label1 = new label_t;
		Start->recv(LineNo, Prev->Receiver);
		Start->link(LineNo, Label1);
		Start = Label1;
	};
	label_t *Label1 = new label_t;
	label_t *Label2 = new label_t;
	Compiler->Function->Loop = Prev;
		label_t *Label0 = Compiler->push_trap(LineNo, Start, Label2);
			Label1->load(LineNo, Value->compile(Compiler, Label0, Label1));
			Label1->link(LineNo, Loop->Exit);
		Compiler->pop_trap();
		Compiler->back_trap(Label2);
	Compiler->Function->Loop = Loop;
	return Register;
};

operand_t *step_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	compiler_t::function_t::loop_t *Loop = Compiler->Function->Loop;
	compiler_t::function_t::loop_t *Prev = Loop->Prev;
	if (Prev == 0) Compiler->raise_error(LineNo, SemanticErrorMessageT, "Error: not inside loop");
	if (Prev->Receiver != Loop->Receiver) {
		Start->recv(LineNo, Prev->Receiver);
	};
	Start->link(LineNo, Loop->Step);
	return Register;
};

operand_t *every_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label1 = new label_t;
	uint32_t Trap = Compiler->use_trap();
	Compiler->push_loop(LineNo, Label1, Success);
		expr_t *Expr = Condition;
		while (Expr) {
			label_t *Label0 = new label_t;
			Expr->compile(Compiler, Start, Label0);
			Start = Label0;
			Expr = Expr->Next;
		};
		Start = Compiler->push_trap(LineNo, Start, Label1);
			Body->compile(Compiler, Start, Label1);
		Compiler->pop_trap();
	Compiler->pop_loop();
	Label1->back(LineNo, Trap);
	return Register;
};

operand_t *all_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	uint32_t Index = Compiler->new_temporary();
	operand_t *Result = new operand_t;
	Result->Type = operand_t::TVAR;
	Result->Index = Index;

	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;

	Start->new_list(LineNo, Index);
	Start->link(LineNo, Label0);

	Label0 = Compiler->push_trap(LineNo, Label0, Success);
		Label1->load(LineNo, Value->compile(Compiler, Label0, Label1));
		Label1->store_list(LineNo, Index);
		Compiler->back_trap(Label1);
	Compiler->pop_trap();

	return Result;
};

operand_t *uniq_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	uint32_t Index = Compiler->new_temporary();
	operand_t *Result = new operand_t;
	Result->Type = operand_t::TVAR;
	Result->Index = Index;

	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;

	Start->new_table(LineNo, Index);
	Start->link(LineNo, Label0);

	Label0 = Compiler->push_trap(LineNo, Label0, Success);
		Label1->load(LineNo, Value->compile(Compiler, Label0, Label1));
		Label1->store_table(LineNo, Index);
		Compiler->back_trap(Label1);
	Compiler->pop_trap();

	return Result;
};

operand_t *count_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	uint32_t Index = Compiler->new_temporary();
	operand_t *Result = new operand_t;
	Result->Type = operand_t::TVAR;
	Result->Index = Index;

	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;

	Start->new_count(LineNo, Index);
	Start->link(LineNo, Label0);

	Label0 = Compiler->push_trap(LineNo, Label0, Success);
		Label1->load(LineNo, Value->compile(Compiler, Label0, Label1));
		Label1->inc_count(LineNo, Index);
		Compiler->back_trap(Label1);
	Compiler->pop_trap();

	return Result;
};

operand_t *send_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	Label0->load(LineNo, Value->compile(Compiler, Start, Label0));
	Label0->send(LineNo);
	return Register;
};

operand_t *self_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Start->link(LineNo, Success);
	operand_t *Self = Compiler->Function->Loop->Self;
	if (Self == 0) Compiler->raise_error(LineNo, SemanticErrorMessageT, "Error: not inside assignment/filter");
	return Self;
};

operand_t *typeof_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	operand_t *Constant = Expr->constant(Compiler);
	if (Constant) {
		Start->link(LineNo, Success);
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::CNST;
		Operand->Value = (Std$Object_t *)Constant->Value->Type;
		return Operand;
	};
	label_t *Label0 = new label_t;
	Label0->load(LineNo, Expr->compile(Compiler, Start, Label0));
	Label0->type_of(LineNo);
	Label0->link(LineNo, Success);
	return Register;
};

operand_t *valueof_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	operand_t *Constant = Expr->constant(Compiler);
	if (Constant) {
		Start->link(LineNo, Success);
		return Constant;
	};
	label_t *Label0 = new label_t;
	Label0->load(LineNo, Expr->compile(Compiler, Start, Label0));
	Label0->value_of(LineNo);
	Label0->link(LineNo, Success);
	return Register;
};

operand_t *sequence_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = Start;
	uint32_t Temp = Compiler->new_temporary();
	uint32_t Trap = Compiler->use_trap();

	compiler_t::function_t::loop_t::expression_t *Expr0 = Compiler->Function->Loop->Expression;
	bitset_t *Temps = Expr0->Temps;
	bitset_t *TotalTemps = new bitset_t(Temps);

	expr_t *Expr = Exprs;
	while (Expr->Next) {
		label_t *Label1 = new label_t;
		label_t *Label2 = new label_t;
		label_t *Label3 = new label_t;
		Label0->push_trap(LineNo, Trap, Label3, Temp);
		Label0->link(LineNo, Label1);

		Expr0->Temps = new bitset_t(Temps);
		Label2->load(LineNo, Expr->compile(Compiler, Label1, Label2));
		TotalTemps->update(Expr0->Temps);

		Label2->link(LineNo, Success);
		Label0 = Label3;
		Expr = Expr->Next;
	};
	label_t *Label2 = new label_t;
	Expr0->Temps = new bitset_t(Temps);
	Label2->load(LineNo, Expr->compile(Compiler, Label0, Label2));
	TotalTemps->update(Expr0->Temps);
	Expr0->Temps = TotalTemps;
	Label2->link(LineNo, Success);
	return Register;
};

operand_t *limit_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;

	operand_t *Operand = Limit->compile(Compiler, Start, Label0);
	if (Operand->Type == operand_t::CNST) {
		Std$Object_t *Value = Operand->Value;
		if (Value->Type != Std$Integer$SmallT) Compiler->raise_error(LineNo, TypeErrorMessageT, "Error: limit is not an integer");
		switch (((Std$Integer_smallt *)Value)->Value) {
		case 0: {
			Compiler->back_trap(Label0);
			return Register;
		};
		case 1: {
			Compiler->push_expression();
			Label0 = Compiler->push_trap(LineNo, Label0, Label1);
				operand_t *Result = Expr->compile(Compiler, Label0, Success);
			Compiler->pop_trap();
			Compiler->pop_expression();
			Compiler->back_trap(Label1);
			return Result;
		};
		default: break;
		};
	};
	uint32_t Trap = Compiler->use_trap();
	uint32_t Temp0 = Compiler->new_temporary();
	uint32_t Temp1 = Compiler->new_temporary();

	label_t *Label2 = new label_t;
	label_t *Label3 = new label_t;
	label_t *Label4 = new label_t;

	Label0->load(LineNo, Operand);
	Label0->limit(LineNo, Trap, Temp0);
	Label0->link(LineNo, Label1);

	Label1 = Compiler->push_trap(LineNo, Label1, Label2);
		operand_t *Result = Expr->compile(Compiler, Label1, Label3);
		Compiler->back_trap(Label4);
	Compiler->pop_trap();

	Label3->test_limit(LineNo, Temp0, Success);
	Label3->push_trap(LineNo, Trap, Label4, Temp1);
	Label3->link(LineNo, Success);
	Compiler->back_trap(Label2);

	return Result;
};

operand_t *skip_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;
	label_t *Label2 = new label_t;

	uint32_t Trap = Compiler->use_trap();
	uint32_t Temp = Compiler->new_temporary();

	Label0->load(LineNo, Skip->compile(Compiler, Start, Label0));
	Label0->skip(LineNo, Temp);
	Label0->link(LineNo, Label1);
	operand_t *Result = Expr->compile(Compiler, Label1, Label2);
	Label2->test_skip(LineNo, Trap, Temp);
	Label2->link(LineNo, Success);
	return Result;
};

operand_t *infinite_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	uint32_t Temp = Compiler->new_temporary();
	uint32_t Trap = Compiler->use_trap();
	label_t *Label1 = new label_t;
	Label1->back(LineNo, Trap);
	Start->push_trap(LineNo, Trap, Label1, Temp);
	Start->link(LineNo, Label0);
	label_t *Label2 = new label_t;
	Label2->store_trap(LineNo, Temp, Start);
	Label2->link(LineNo, Success);
	operand_t *Result = Expr->compile(Compiler, Label0, Label2);
	return Result;
};

operand_t *parallel_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	uint32_t Gate = Compiler->new_temporary();
	uint32_t Trap = Compiler->use_trap();

	label_t *Label1 = Start;
	label_t *LabelF = new label_t;
	label_t *LabelG = new label_t;
	label_t *Label4 = LabelF;

	expr_t *Expr = Exprs;
	while (Expr->Next) {
		label_t *Label2 = new label_t;
		label_t *Label3 = new label_t;
		label_t *Label5 = new label_t;
		label_t *Label6 = new label_t;

		Label1 = Compiler->push_trap(LineNo, Label1, LabelG);
			Label1->store_link(LineNo, Gate, Label2);
			Label1->link(LineNo, Label6);
			Expr->compile(Compiler, Label6, Label3);
			Label4->store_link(LineNo, Gate, Label5);
			Compiler->back_trap(Label4);
			Label4 = Label5;
			Label3->jump_link(LineNo, Gate);
			Label1 = Label2;
		Compiler->pop_trap();

		Expr = Expr->Next;
	};
	label_t *Label3 = new label_t;
	Label1 = Compiler->push_trap(LineNo, Label1, LabelG);
		operand_t *Result = Expr->compile(Compiler, Label1, Label3);
		Compiler->back_trap(Label4);
	Compiler->pop_trap();
	Label3->push_trap(LineNo, Trap, LabelF, Gate);
	Label3->link(LineNo, Success);
	LabelG->back(LineNo, Trap);
	return Result;
};

operand_t *interleave_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	uint32_t Gate = Compiler->new_temporary();

	label_t *LabelG = new label_t;
	label_t *LabelS = new label_t;
	label_t *Label0 = Start;
	label_t *LabelF = new label_t;
	label_t *Label1 = new label_t;
	label_t *LabelX = new label_t;
	Compiler->back_trap(LabelX);

	expr_t *Expr = Exprs;
	Label0 = Compiler->push_trap(LineNo, Label0, LabelX);
		Label1 = new label_t;
		Label0->store_link(LineNo, Gate, Label1);
		label_t *Label2 = new label_t;
		Label0->link(LineNo, Label2);
		label_t *Label5 = new label_t;
		Label5->load(LineNo, Expr->compile(Compiler, Label2, Label5));
		Label5->link(LineNo, LabelS);

		label_t *LabelZ = new label_t;
		LabelF = new label_t;
		LabelZ->store_link(LineNo, Gate, LabelF);
		Compiler->back_trap(LabelZ);

		Label0 = Label1;
	Compiler->pop_trap();
	while (Expr = Expr->Next) {
		Label0 = Compiler->push_trap(LineNo, Label0, LabelX);
			Label1 = new label_t;
			Label0->store_link(LineNo, Gate, Label1);
			Label2 = new label_t;
			Label0->link(LineNo, Label2);
			Label5 = new label_t;
			Label5->load(LineNo, Expr->compile(Compiler, Label2, Label5));
			Label5->link(LineNo, LabelS);

			label_t *Label4 = new label_t;
			LabelF->store_link(LineNo, Gate, Label4);
			Compiler->back_trap(LabelF);

			Label0 = Label1;
			LabelF = Label4;
		Compiler->pop_trap();
	};
	Label1->link(LineNo, LabelZ);
	LabelF->link(LineNo, LabelZ);

	uint32_t Temp = Compiler->new_temporary();
	uint32_t Trap = Compiler->use_trap();
	LabelS->push_trap(LineNo, Trap, LabelG, Temp);
	LabelS->link(LineNo, Success);
	LabelG->jump_link(LineNo, Gate);

	return Register;
};

operand_t *left_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *Result = Left->compile(Compiler, Start, Label0);
	if (Result == Register) {
		Result = new operand_t;
		Result->Type = operand_t::TREG;
		Result->Index = Compiler->new_temporary();
		Label0->store_reg(LineNo, Result);
		label_t *Label1 = new label_t;
		Label0->link(LineNo, Label1);
		Label0 = Label1;
	};

    operand_t *OldSelf = Compiler->Function->Loop->Self;
	Compiler->Function->Loop->Self = Result;
	label_t *Label1 = new label_t;
	Right->compile(Compiler, Label0, Success);
	Compiler->Function->Loop->Self = OldSelf;
	return Result;
};

operand_t *right_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	Left->compile(Compiler, Start, Label0);
	return Right->compile(Compiler, Label0, Success);
};

operand_t *cond_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	if (this->Failure == 0) {
		label_t *Success0 = new label_t;
		label_t *Failure0 = new label_t;
		Compiler->push_expression();
			Start = Compiler->push_trap(LineNo, Start, Failure0);
				Condition->compile(Compiler, Start, Success0);
			Compiler->pop_trap();
		Compiler->pop_expression();
		Compiler->back_trap(Failure0);
		return this->Success->compile(Compiler, Success0, Success);
	} else if (this->Success == 0) {
#define DEFAULTBACK
#ifdef DEFAULTBACK
		label_t *Success0 = new label_t;
		label_t *Failure0 = new label_t;
		Compiler->push_expression();
			Start = Compiler->push_trap(LineNo, Start, Failure0);
				Condition->compile(Compiler, Start, Success0);
			Compiler->pop_trap();
		Compiler->pop_expression();
		Compiler->back_trap(Success0);
		return this->Failure->compile(Compiler, Failure0, Success);
#else
		label_t *Success0 = new label_t;
		label_t *Failure0 = new label_t;
		label_t *Failure1 = new label_t;
		Compiler->push_expression();
			Start = Compiler->push_trap(LineNo, Start, Failure0);
				Success0->load(LineNo, Condition->compile(Compiler, Start, Success0));
				Success0->link(LineNo, Success);
			Compiler->pop_trap();
		Compiler->pop_expression();
		Failure1->load(LineNo, this->Failure->compile(Compiler, Failure0, Failure1));
		Failure1->link(LineNo, Success);
		return Register;
#endif
	} else {
		label_t *Success0 = new label_t;
		label_t *Failure0 = new label_t;
		label_t *Success1 = new label_t;
		label_t *Failure1 = new label_t;
		Compiler->push_expression();
			Start = Compiler->push_trap(LineNo, Start, Failure0);
				Condition->compile(Compiler, Start, Success0);
			Compiler->pop_trap();
		Compiler->pop_expression();

		compiler_t::function_t::loop_t::expression_t *Expr = Compiler->Function->Loop->Expression;
		bitset_t *SuccessTemps = new bitset_t(Expr->Temps);
		bitset_t *FailureTemps = Expr->Temps;

		Expr->Temps = SuccessTemps;
		Success1->load(LineNo, this->Success->compile(Compiler, Success0, Success1));
		Success1->link(LineNo, Success);

		Expr->Temps = FailureTemps;
		Failure1->load(LineNo, this->Failure->compile(Compiler, Failure0, Failure1));
		Failure1->link(LineNo, Success);

		Expr->Temps->reserve(SuccessTemps);

		return Register;
	};
};

operand_t *comp_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	operand_t *Operand = Left->compile(Compiler, Start, Label0);
	if (Operand == Register) {
		Operand = new operand_t;
		Operand->Type = operand_t::TVAR;
		Operand->Index = Compiler->new_temporary();
		Label0->store_reg(LineNo, Operand);
		label_t *Label1 = new label_t;
		Label0->link(LineNo, Label1);
		Label0 = Label1;
	};
	label_t *Label1 = new label_t;
	label_t *Label2 = new label_t;
	operand_t *Result = Right->compile(Compiler, Label0, Label1);
	if (Result->Type == operand_t::CNST && Operand->Type == operand_t::CNST) {
		if ((Result->Value == Operand->Value) == Eq) {
			Label1->link(LineNo, Success);
		} else {
			Compiler->back_trap(Label1);
		};
	} else {
		Label1->load(LineNo, Result);
		Label1->comp(LineNo, Eq, Operand, Label2);
		Label1->link(LineNo, Success);
		Compiler->back_trap(Label2);
	};
	return Result;
};

operand_t *when_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;
	Label0->load(LineNo, Condition->compile(Compiler, Start, Label0));
	Label0->link(LineNo, Label1);
	Label0 = Label1;
	if (Cases == 0) {
		if (Default) {
			return Default->compile(Compiler, Label0, Success);
		} else {
			Compiler->back_trap(Label0);
			return Register;
		};
	};
	case_t *Case = Cases;
	Label1 = new label_t;
	label_t *Label2 = new label_t;
	compiler_t::function_t::loop_t::expression_t *Expr = Compiler->Function->Loop->Expression;
	bitset_t *InitialTemps = Expr->Temps;
	bitset_t *FinalTemps = new bitset_t(Expr->Temps);
	Expr->Temps = new bitset_t(InitialTemps);
	Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
	Label2->link(LineNo, Success);
	FinalTemps->reserve(Expr->Temps);
	case_t::range_t *Range = Case->Ranges;
	operand_t *Operand = Range->Min->constant(Compiler);
	if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
	Std$Object_t *Value = Operand->Value;
	if (Std$Type$is(Value->Type, Std$Integer$SmallT)) {
		select_integer_case_t *ICase = new select_integer_case_t;
		ICase->Min = ((Std$Integer_smallt *)Value)->Value;
		if (Range->Max) {
			Operand = Range->Max->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			if (!Std$Type$is(Value->Type, Std$Integer$SmallT)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
			ICase->Max = ((Std$Integer_smallt *)Value)->Value;
		} else {
			ICase->Max = ICase->Min;
		};
		ICase->Body = Label1;
		while (Range = Range->Next) {
			select_integer_case_t *ICase0 = new select_integer_case_t;
			Operand = Range->Min->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			if (!Std$Type$is(Value->Type, Std$Integer$SmallT)) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
			ICase0->Min = ((Std$Integer_smallt *)Value)->Value;
			if (Range->Max) {
				Operand = Range->Max->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				if (!Std$Type$is(Value->Type, Std$Integer$SmallT)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
				ICase0->Max = ((Std$Integer_smallt *)Value)->Value;
			} else {
				ICase0->Max = ICase0->Min;
			};
			ICase0->Body = Label1;
			ICase0->Next = ICase;
			ICase = ICase0;
		};
		while (Case = Case->Next) {
			Label1 = new label_t;
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
			for (Range = Case->Ranges; Range; Range = Range->Next) {
				select_integer_case_t *ICase0 = new select_integer_case_t;
				Operand = Range->Min->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				if (!Std$Type$is(Value->Type, Std$Integer$SmallT)) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
				ICase0->Min = ((Std$Integer_smallt *)Value)->Value;
				if (Range->Max) {
					Operand = Range->Max->constant(Compiler);
					if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
					Value = Operand->Value;
					if (!Std$Type$is(Value->Type, Std$Integer$SmallT)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
					ICase0->Max = ((Std$Integer_smallt *)Value)->Value;
				} else {
					ICase0->Max = ICase0->Min;
				};
				ICase0->Body = Label1;
				ICase0->Next = ICase;
				ICase = ICase0;
			};
		};
		Label1 = new label_t;
		if (Default) {
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Default->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
		} else {
			Compiler->back_trap(Label1);
		};
		Expr->Temps = FinalTemps;
		Label0->select_integer(LineNo, ICase, Label1);
		return Register;
	} else if (Std$Type$is(Value->Type, Std$String$T)) {
		if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
		select_string_case_t *ICase = new select_string_case_t;
		ICase->Key = Std$String$flatten(Value);
		ICase->Length = ((Std$String_t *)Value)->Length.Value;
		ICase->Body = Label1;
		while (Range = Range->Next) {
			select_string_case_t *ICase0 = new select_string_case_t;
			Operand = Range->Min->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			if (!Std$Type$is(Value->Type, Std$String$T)) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
			ICase0->Key = Std$String$flatten(Value);
			ICase0->Length = ((Std$String_t *)Value)->Length.Value;
			if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
			ICase0->Body = Label1;
			ICase0->Next = ICase;
			ICase = ICase0;
		};
		while (Case = Case->Next) {
			Label1 = new label_t;
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
			for (Range = Case->Ranges; Range; Range = Range->Next) {
				select_string_case_t *ICase0 = new select_string_case_t;
				Operand = Range->Min->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				if (!Std$Type$is(Value->Type, Std$String$T)) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
				ICase0->Key = Std$String$flatten(Value);
				ICase0->Length = ((Std$String_t *)Value)->Length.Value;
				if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
				ICase0->Body = Label1;
				ICase0->Next = ICase;
				ICase = ICase0;
			};
		};
		Label1 = new label_t;
		if (Default) {
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Default->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
		} else {
			Compiler->back_trap(Label1);
		};
		Expr->Temps = FinalTemps;
		Label0->select_string(LineNo, ICase, Label1);
		return Register;
	} else if (Std$Type$is(Value->Type, Std$Real$T)) {
		select_real_case_t *ICase = new select_real_case_t;
		select_real_case_t *Tail = ICase;
		ICase->Min = ((Std$Real_t *)Value)->Value;
		if (Range->Max) {
			Operand = Range->Max->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			if (!Std$Type$is(Value->Type, Std$Real$T)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
			ICase->Max = ((Std$Real_t *)Value)->Value;
		} else {
			ICase->Max = ICase->Min;
		};
		ICase->Body = Label1;
		while (Range = Range->Next) {
			select_real_case_t *ICase0 = new select_real_case_t;
			Operand = Range->Min->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			if (Value->Type != Std$Real$T) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
			ICase0->Min = ((Std$Real_t *)Value)->Value;
			if (Range->Max) {
				Operand = Range->Max->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				if (!Std$Type$is(Value->Type, Std$Real$T)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
				ICase0->Max = ((Std$Real_t *)Value)->Value;
			} else {
				ICase0->Max = ICase0->Min;
			};
			ICase0->Body = Label1;
			Tail->Next = ICase0;
			Tail = ICase0;
		};
		while (Case = Case->Next) {
			Label1 = new label_t;
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
			for (Range = Case->Ranges; Range; Range = Range->Next) {
				select_real_case_t *ICase0 = new select_real_case_t;
				Operand = Range->Min->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				if (!Std$Type$is(Value->Type, Std$Real$T)) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not correct type");
				ICase0->Min = ((Std$Real_t *)Value)->Value;
				if (Range->Max) {
					Operand = Range->Max->constant(Compiler);
					if (Operand == 0) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not constant");
					Value = Operand->Value;
					if (!Std$Type$is(Value->Type, Std$Real$T)) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case is not correct type");
					ICase0->Max = ((Std$Real_t *)Value)->Value;
				} else {
					ICase0->Max = ICase0->Min;
				};
				ICase0->Body = Label1;
				Tail->Next = ICase0;
				Tail = ICase0;
			};
		};
		Label1 = new label_t;
		if (Default) {
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Default->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
		} else {
			Compiler->back_trap(Label1);
		};
		Expr->Temps = FinalTemps;
		Label0->select_real(LineNo, ICase, Label1);
		return Register;
	} else {
		if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
		select_object_case_t *ICase = new select_object_case_t;
		ICase->Object = Value;
		ICase->Body = Label1;
		while (Range = Range->Next) {
			select_object_case_t *ICase0 = new select_object_case_t;
			Operand = Range->Min->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			ICase0->Object = Value;
			if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
			ICase0->Body = Label1;
			ICase0->Next = ICase;
			ICase = ICase0;
		};
		while (Case = Case->Next) {
			Label1 = new label_t;
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
			for (Range = Case->Ranges; Range; Range = Range->Next) {
				select_object_case_t *ICase0 = new select_object_case_t;
				Operand = Range->Min->constant(Compiler);
				if (Operand == 0) Compiler->raise_error(Range->Min->LineNo, TypeErrorMessageT, "Error: case is not constant");
				Value = Operand->Value;
				ICase0->Object = Value;
				if (Range->Max) Compiler->raise_error(Range->Max->LineNo, TypeErrorMessageT, "Error: case can not be a range");
				ICase0->Body = Label1;
				ICase0->Next = ICase;
				ICase = ICase0;
			};
		};
		Label1 = new label_t;
		if (Default) {
			Label2 = new label_t;
			Expr->Temps = new bitset_t(InitialTemps);
			Label2->load(LineNo, Default->compile(Compiler, Label1, Label2));
			Label2->link(LineNo, Success);
			FinalTemps->reserve(Expr->Temps);
		} else {
			Compiler->back_trap(Label1);
		};
		Expr->Temps = FinalTemps;
		Label0->select_object(LineNo, ICase, Label1);
		return Register;
	};
};

operand_t *whentype_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	label_t *Label0 = new label_t;
	label_t *Label1 = new label_t;
	Label0->load(LineNo, Condition->compile(Compiler, Start, Label0));
	Label0->link(LineNo, Label1);
	Label0 = Label1;
	if (Cases == 0) {
		if (Default) {
			return Default->compile(Compiler, Label0, Success);
		} else {
			Compiler->back_trap(Label0);
			return Register;
		};
	};
	case_t *Case = Cases;
	Label1 = new label_t;
	label_t *Label2 = new label_t;
	compiler_t::function_t::loop_t::expression_t *Expr = Compiler->Function->Loop->Expression;
	bitset_t *InitialTemps = Expr->Temps;
	bitset_t *FinalTemps = new bitset_t(Expr->Temps);
	Expr->Temps = new bitset_t(InitialTemps);
	Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
	Label2->link(LineNo, Success);
	FinalTemps->reserve(Expr->Temps);
	expr_t *Type = Case->Types;
	operand_t *Operand = Type->constant(Compiler);
	if (Operand == 0) Compiler->raise_error(Type->LineNo, TypeErrorMessageT, "Error: case is not constant");
	Std$Object_t *Value = Operand->Value;
	select_type_case_t *ICase = new select_type_case_t;
	ICase->Type = Value;
	ICase->Body = Label1;
	while (Type = Type->Next) {
		select_type_case_t *ICase0 = new select_type_case_t;
		Operand = Type->constant(Compiler);
		if (Operand == 0) Compiler->raise_error(Type->LineNo, TypeErrorMessageT, "Error: case is not constant");
		Value = Operand->Value;
		ICase0->Type = Value;
		ICase0->Body = Label1;
		ICase0->Next = ICase;
		ICase = ICase0;
	};
	while (Case = Case->Next) {
		Label1 = new label_t;
		Label2 = new label_t;
		Expr->Temps = new bitset_t(InitialTemps);
		Label2->load(LineNo, Case->Body->compile(Compiler, Label1, Label2));
		Label2->link(LineNo, Success);
		FinalTemps->reserve(Expr->Temps);
		for (Type = Case->Types; Type; Type = Type->Next) {
			select_type_case_t *ICase0 = new select_type_case_t;
			Operand = Type->constant(Compiler);
			if (Operand == 0) Compiler->raise_error(Type->LineNo, TypeErrorMessageT, "Error: case is not constant");
			Value = Operand->Value;
			ICase0->Type = Value;
			ICase0->Body = Label1;
			ICase0->Next = ICase;
			ICase = ICase0;
		};
	};
	Label1 = new label_t;
	if (Default) {
		Label2 = new label_t;
		Expr->Temps = new bitset_t(InitialTemps);
		Label2->load(LineNo, Default->compile(Compiler, Label1, Label2));
		Label2->link(LineNo, Success);
		FinalTemps->reserve(Expr->Temps);
	} else {
		Compiler->back_trap(Label1);
	};
	Expr->Temps = FinalTemps;
	Label0->select_type(LineNo, ICase, Label1);
	return Register;
};

TYPE(InitErrorMessageT, ErrorMessageT);

operand_t *block_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	operand_t *Result = 0;
	Compiler->push_scope();
	for (block_expr_t::localdef_t *Def = Defs; Def; Def = Def->Next) {
		operand_t *Operand = Def->Value->precompile(Compiler, Def->Type);
		if (Operand) Compiler->declare(Def->Name, Operand);
	};
	for (block_expr_t::localdef_t *Def = Defs; Def; Def = Def->Next) {
		switch (Def->Type) {
		case _PC_NONE: {
			if (Def->Type == _PC_WAIT) {
				Compiler->raise_error(Def->LineNo, InitErrorMessageT, "Error: recursive constant initialization");
			};
			Def->Type = _PC_WAIT;
			Std$Object_t *Value = Def->Value->evaluate(Compiler);
			if (Value == 0) {
				Compiler->raise_error(Def->LineNo, InitErrorMessageT, "Error: constant initialization failed");
			} else {
				Def->Type = _PC_FULL;
				operand_t *Operand = new operand_t;
				Operand->Type = operand_t::CNST;
				Operand->Value = Value;
				Compiler->declare(Def->Name, Operand);
			};
			break;
		};
		case _PC_PARTIAL: {
			Def->Value->compile(Compiler, new label_t, new label_t);
			break;
		};
		case _PC_FULL: {
			break;
		};
		};
	};
	if (Vars) {
		for (block_expr_t::localvar_t *Var = Vars; Var; Var = Var->Next) {
			operand_t *Operand = Compiler->new_local();
			Start->local(LineNo, Operand->Index);
			Compiler->declare(Var->Name, Operand);
		};
		label_t *Start0 = new label_t;
		Start->link(LineNo, Start0);
		Start = Start0;
	};
	if (Receiver.Body) {
		compiler_t::function_t::loop_t *Loop = Compiler->Function->Loop;
		compiler_t::function_t::loop_t::expression_t *Expr0 = Loop->Expression;
		bitset_t *RecvTemps = new bitset_t(Expr0->Temps);
		bitset_t *BodyTemps = Expr0->Temps;
		Expr0->Temps = RecvTemps;
		label_t *OldReceiver = Loop->Receiver;
		label_t *NewReceiver = new label_t;
		Compiler->push_scope();
			operand_t *Message = Compiler->new_local();
			Compiler->declare(Receiver.Var, Message);
			NewReceiver->store_local(LineNo, Message->Index);
			NewReceiver->recv(LineNo, OldReceiver);
			label_t *Label0 = new label_t;
			label_t *Label1 = new label_t;
			NewReceiver->link(LineNo, Label0);
			Label1->load(LineNo, Receiver.Body->compile(Compiler, Label0, Label1));
			Label1->link(LineNo, Success);
		Compiler->pop_scope();
		//printf("After receiver, NoOfTemps = %d\n", Expr0->Temps->size());
		Expr0->Temps = BodyTemps;
		Label0 = new label_t;
		Start->recv(LineNo, Loop->Receiver = NewReceiver);
		Start->link(LineNo, Label0);
		for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
			Label1 = new label_t;
			Compiler->push_expression();
				Label0 = Compiler->push_trap(Expr->LineNo, Label0, Label1);
					Expr->compile(Compiler, Label0, Label1);
					Label1->flush(Expr->LineNo);
					Label1->link(Expr->LineNo, Label0 = new label_t);
				Compiler->pop_trap();
			Compiler->pop_expression();
		};
		if (Final) {
			Label1 = new label_t;
			label_t *Label2 = new label_t;
			Label0 = Compiler->push_trap(Final->LineNo, Label0, Label2);
				Result = Final->compile(Compiler, Label0, Label1);
				Label0 = Label1;
			Compiler->pop_trap();
			Label2->recv(Final->LineNo, OldReceiver);
			Compiler->back_trap(Label2);
		};
		//printf("After block, NoOfTemps = %d\n", Expr0->Temps->size());
		Label0->recv(LineNo, Loop->Receiver = OldReceiver);
		Label0->link(LineNo, Success);
		
		Expr0->Temps->reserve(RecvTemps);
	} else if (Must) {
		compiler_t::function_t::loop_t *Loop = Compiler->Function->Loop;
		compiler_t::function_t::loop_t::expression_t *Expr0 = Loop->Expression;
		
		uint32_t Gate = Compiler->new_temporary(2);
		
		label_t *MustLabel0 = new label_t;
		label_t *MustLabel1 = new label_t;
		
		label_t *OldReceiver = Loop->Receiver;
		label_t *NewReceiver = new label_t;
		operand_t *Temp = new operand_t;
		Temp->Type = operand_t::TVAR;
		Temp->Index = Gate + 1;
		label_t *Message = new label_t;
		Message->load(LineNo, Temp);
		Message->send(LineNo);
		NewReceiver->store_reg(LineNo, Temp);
		NewReceiver->recv(LineNo, OldReceiver);
		NewReceiver->store_link(LineNo, Gate, Message);
		NewReceiver->link(LineNo, MustLabel0);
		
		//printf("After receiver, NoOfTemps = %d\n", Expr0->Temps->size());
		Compiler->push_must(LineNo, MustLabel0, Gate, Temp);
			label_t *Label0 = new label_t;
			Start->recv(LineNo, Loop->Receiver = NewReceiver);
			Start->link(LineNo, Label0);
			for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
				label_t *Label1 = new label_t;
				Compiler->push_expression();
					Label0 = Compiler->push_trap(Expr->LineNo, Label0, Label1);
						Expr->compile(Compiler, Label0, Label1);
						Label1->flush(Expr->LineNo);
						Label1->link(Expr->LineNo, Label0 = new label_t);
					Compiler->pop_trap();
				Compiler->pop_expression();
			};
			if (Final) {
				label_t *Label1 = new label_t;
				label_t *Label2 = new label_t;
				Label0 = Compiler->push_trap(Final->LineNo, Label0, Label2);
					Result = Final->compile(Compiler, Label0, Label1);
					Label0 = Label1;
				Compiler->pop_trap();
				label_t *Failure = new label_t;
				Compiler->back_trap(Failure);
				Label2->recv(Final->LineNo, OldReceiver);
				Label2->store_link(LineNo, Gate, Failure);
				Label2->link(LineNo, MustLabel0);
			};
			//printf("After block, NoOfTemps = %d\n", Expr0->Temps->size());
			if (Result == Register) {
				Result = new operand_t;
				Result->Type = operand_t::TREG;
				Result->Index = Gate + 1;
				Label0->store_reg(LineNo, Result);
			};
			Label0->recv(LineNo, Loop->Receiver = OldReceiver);
			Label0->store_link(LineNo, Gate, Success);
			Label0->link(LineNo, MustLabel0);
		Compiler->pop_must();
			
		Must->compile(Compiler, MustLabel0, MustLabel1);
		MustLabel1->jump_link(LineNo, Gate);
	} else {
		label_t *Label0 = Start;
		for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
			label_t *Label1 = new label_t;
			Compiler->push_expression();
				Label0 = Compiler->push_trap(Expr->LineNo, Label0, Label1);
					Result = Expr->compile(Compiler, Label0, Label1);
					Label1->flush(Expr->LineNo);
					Label1->link(Expr->LineNo, Label0 = new label_t);
				Compiler->pop_trap();
			Compiler->pop_expression();
		};
		if (Final) {
			label_t *Label1 = new label_t;
			Result = Final->compile(Compiler, Label0, Label1);
			Label0 = Label1;
		};
		Label0->link(LineNo, Success);
	};
	if (Result == 0) {
		Result = new operand_t;
		Result->Type = operand_t::CNST;
		Result->Value = Std$Object$Nil;
	};
	Compiler->pop_scope();
	return Result;
};

operand_t *module_expr_t::precompile(compiler_t *Compiler, precomp_t &Type) {DEBUG
	operand_t *Constant = new operand_t;
	Constant->Type = operand_t::CNST;
	Constant->Value = (Std$Object_t *)Provider->Module;
	Type = _PC_PARTIAL;
	return Constant;
};

static int default_import(void *Module, const char *Symbol, int *IsRef, void **Data) {
	return 0;
};

Std$Object_t *expr_t::evaluate(compiler_t *Compiler) {
	Compiler->push_function(LineNo);
	label_t *Start = new label_t;
	label_t *Success = new label_t;
	label_t *Failure = new label_t;
		Compiler->push_expression();
			Success->load(LineNo, compile(Compiler, Compiler->push_trap(LineNo, Start, Failure), Success));
			Compiler->pop_trap();
			Success->ret(LineNo);
			Failure->fail(LineNo);
		Compiler->pop_expression();
	frame_t *Frame = Compiler->pop_function();
	operand_t *Closure = Start->assemble(Frame, Compiler->SourceName, LineNo);
	if (Closure == 0) Compiler->raise_error(LineNo, ScopeErrorMessageT, "Error: context does not allow access to surrounding scope");
	Std$Function_result Result;
	switch (Std$Function$call(Closure->Value, 0, &Result)) {
	case SUSPEND:
	case SUCCESS:
		return Result.Val;
	case FAILURE:
		return 0;
	case MESSAGE:
		if (Std$Function$call($AT, 2, &Result, Result.Val, 0, Std$String$T, 0) < FAILURE) {
			Compiler->raise_error(LineNo, InitErrorMessageT, Std$String$flatten(Result.Val));
		};
		return 0;
	};
};

Std$Function_status expr_t::evaluate(compiler_t *Compiler, Std$Function_result *Result) {
	Compiler->push_function(LineNo);
	label_t *Start = new label_t;
	label_t *Success = new label_t;
	label_t *Failure = new label_t;
		Compiler->push_expression();
			Success->load(LineNo, compile(Compiler, Compiler->push_trap(LineNo, Start, Failure), Success));
			Compiler->pop_trap();
			Success->ret(LineNo);
			Failure->fail(LineNo);
		Compiler->pop_expression();
	frame_t *Frame = Compiler->pop_function();
	operand_t *Closure = Start->assemble(Frame, Compiler->SourceName, LineNo);
	if (Closure == 0) Compiler->raise_error(LineNo, ScopeErrorMessageT, "Error: context does not allow access to surrounding scope");
	return Std$Function$call(Closure->Value, 0, Result);
};

static int constant_import(module_expr_t *Module, const char *Symbol, int *IsRef, void **Data) {
	operand_t *Operand = stringtable_get(Module->Exports, Symbol);
	if (!Operand) return 0;
	if (Operand->Type == operand_t::FUTR) Operand->Future->resolve(Module->_Compiler, Operand);
	*IsRef = Operand->Type;
	*Data = Operand->Value;
	return 1;
};

struct future_imp_t : future_t {
	const char *Path, *Name;
	
	future_imp_t(int LineNo, const char *Path, const char *Name) : future_t(LineNo), Path(Path), Name(Name) {};
	
	void resolve(compiler_t *Compiler, operand_t *Operand) {
		Sys$Module_t *Module = Sys$Module$load(Path, Name);
		if (Module == 0) Compiler->raise_error(LineNo, ExternErrorMessageT, "Error: module not found %s/%s", Path, Name);
		Operand->Type = operand_t::CNST;
		Operand->Value = (Std$Object_t *)Module;
	};
};

struct future_use_t : future_t {
	const char *Path, *Name, *Import;
	
	future_use_t(int LineNo, const char *Path, const char *Name, const char *Import) : future_t(LineNo), Path(Path), Name(Name), Import(Import) {};
	
	void resolve(compiler_t *Compiler, operand_t *Operand) {
		Sys$Module_t *Module = Sys$Module$load(Path, Name);
		if (Module == 0) Compiler->raise_error(LineNo, ExternErrorMessageT, "Error: module not found %s/%s", Path, Name);
		if (Sys$Module$import(Module, Import, (int *)&Operand->Type, (void **)&Operand->Value) == 0) {
			Compiler->raise_error(LineNo, ExternErrorMessageT, "Error: import not found %s.%s", Name, Import);
		};
	};
};

struct future_def_t : future_t {
	globaldef_t *Def;
	compiler_t::scope_t *Scope;
	
	future_def_t(globaldef_t *Def, compiler_t::scope_t *Scope) : future_t(Def->LineNo), Def(Def), Scope(Scope) {};
	
	void resolve(compiler_t *Compiler, operand_t *Operand) {
		//printf("Resolving def %s.%s : %d\n", Compiler->SourceName, Def->Name, Def->Type);
		if (Def->Type != expr_t::_PC_NONE) {
			Compiler->raise_error(Def->LineNo, InitErrorMessageT, "Error: recursive constant initialization");
		};
		Def->Type = expr_t::_PC_WAIT;
		compiler_t::scope_t *OldScope = Compiler->Scope;
		Compiler->Scope = Scope;
			Std$Object_t *Value = Def->Value->evaluate(Compiler);
		Compiler->Scope = OldScope;
		if (Value == 0) {
			Compiler->raise_error(Def->LineNo, InitErrorMessageT, "Error: constant initialization failed");
		} else {
			Def->Type = expr_t::_PC_FULL;
			Operand->Type = operand_t::CNST;
			Operand->Value = Value;
		};
	};
};

operand_t *module_expr_t::compile(compiler_t *Compiler, label_t *Start, label_t *Success) {DEBUG
	Compiler->push_scope();
	const char *ModulePath = Sys$Module$get_path(Provider->Module);
	operand_t *Operand = new operand_t;
	Operand->Type = operand_t::CNST;
	Operand->Value = (Std$Object_t *)Provider->Module;
	Compiler->declare(Name, Operand);
	for (module_expr_t::globalvar_t *Var = Vars; Var; Var = Var->Next) {
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::GVAR;
		Std$Object_t **Address = new Std$Object_t *;
		Address[0] = Std$Object$Nil;
		Operand->Address = Address;
		Compiler->declare(Var->Name, Operand);
		if (Var->Exported) Sys$Module$export(Provider->Module, Var->Name, Operand->Type, Operand->Value);
	};
	for (module_expr_t::globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
		int Length = 0;
		for (module_expr_t::globalimp_t::path_t *Path = Imp->Path; Path; Path = Path->Next) Length += strlen(Path->Part) + 1;
		char *ImportPath = new char[Length];
		char *Temp = ImportPath;
		for (module_expr_t::globalimp_t::path_t *Path = Imp->Path; Path; Path = Path->Next) {
			Temp = stpcpy(Temp, Path->Part);
			Temp++[0] = PATHCHR;
		};
		(--Temp)[0] = 0;
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::FUTR;
		Operand->Future = new future_imp_t(Imp->LineNo, Imp->Relative ? ModulePath : 0, Imp->ImportPath = ImportPath);
		if (Imp->Exported) stringtable_put(Exports, Imp->Alias, Operand);
		Compiler->declare(Imp->Alias, Operand);
		
		for (module_expr_t::globalimp_t::uselist_t *Use = Imp->Uses; Use; Use = Use->Next) {
			operand_t *Operand = new operand_t;
			Operand->Type = operand_t::FUTR;
			Operand->Future = new future_use_t(Imp->LineNo, Imp->Relative ? ModulePath : 0, ImportPath, Use->Name);
			Compiler->declare(Use->Name, Operand);
		};
	};
	for (module_expr_t::globaldef_t *Def = Defs; Def; Def = Def->Next) {
		operand_t *Operand = Def->Value->precompile(Compiler, Def->Type);
		if (!Operand) {
			Operand = new operand_t;
			Operand->Type = operand_t::FUTR;
			Operand->Future = new future_def_t(Def, Compiler->Scope);
		};
		Compiler->declare(Def->Name, Operand);
		if (Def->Exported) stringtable_put(Exports, Def->Name, Operand);
	};
	_Compiler = Compiler;
	Riva$Module$set_import_func(Provider, this, (Riva$Module$import_func)constant_import);
	/*for (module_expr_t::globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
		if (Imp->ImportPath) {
			Riva$Module$t *Module = Sys$Module$load(Imp->Relative ? ModulePath : 0, Imp->ImportPath);
			if (Imp->Exported) Sys$Module$export(Provider->Module, Imp->Alias, 0, Module);
		};
	};*/
	for (module_expr_t::globaldef_t *Def = Defs; Def; Def = Def->Next) {
		//printf("Evaluating def %s.%s : %d\n", Compiler->SourceName, Def->Name, Def->Type);
		operand_t *Operand = Compiler->lookup(Def->LineNo, Def->Name);
		if (Operand->Type == operand_t::FUTR) Operand->Future->resolve(Compiler, Operand);
		if (Def->Type == expr_t::_PC_PARTIAL) Def->Value->compile(Compiler, new label_t, new label_t);
		if (Def->Exported) Sys$Module$export(Provider->Module, Def->Name, Operand->Type, Operand->Value);
	};
	_Compiler = 0;
	Riva$Module$set_import_func(Provider, 0, 0);
	if (Body) {
		Compiler->push_function(LineNo);
			label_t *Start = new label_t;
			label_t *Label0 = Start;
			for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
				label_t *Label1 = new label_t;
				Compiler->push_expression();
					Label0 = Compiler->push_trap(Expr->LineNo, Label0, Label1);
						Expr->compile(Compiler, Label0, Label1);
						Label1->flush(Expr->LineNo);
						Label1->link(Expr->LineNo, Label0 = new label_t);
					Compiler->pop_trap();
				Compiler->pop_expression();
			};
			Label0->ret(LineNo);
		frame_t *Frame = Compiler->pop_function();
		operand_t *Closure = Start->assemble(Frame, Compiler->SourceName, LineNo);
		if (Closure == 0) Compiler->raise_error(LineNo, ScopeErrorMessageT, "Error: context does not allow access to surrounding scope");
		Std$Function_result Result;
		//printf("Running module %s\n", Compiler->SourceName);
		if (Std$Function$call(Closure->Value, 0, &Result) == MESSAGE) {
			if (Result.Val->Type == Std$Symbol$NoMethodMessageT) {
				Std$Symbol$nomethodmessage *Message = (Std$Symbol$nomethodmessage *)Result.Val;
				for (int I = 0; I < Message->Count; ++I) printf("\t%s\n", Message->Stack[I]);
				Std$Function$call((Std$Object_t *)$AT, 2, &Result, Result.Val, 0, Std$String$T, 0);
				Compiler->raise_error(LineNo, InitErrorMessageT, Std$String$flatten(Result.Val));
			} else if (Std$Function$call((Std$Object_t *)$AT, 2, &Result, Result.Val, 0, Std$String$T, 0) < FAILURE) {
				Compiler->raise_error(LineNo, InitErrorMessageT, Std$String$flatten(Result.Val));
			} else {
				Compiler->raise_error(0, InitErrorMessageT, "Error: unhandled message");
			};
		};
		//printf("Completed module %s\n", Compiler->SourceName);
	};
	Start->link(LineNo, Success);
	Compiler->pop_scope();
	for (module_expr_t::globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
		if (Imp->ImportPath) {
			Riva$Module$t *Module = Sys$Module$load(Imp->Relative ? ModulePath : 0, Imp->ImportPath);
			if (Imp->Exported) Sys$Module$export(Provider->Module, Imp->Alias, 0, Module);
		};
	};
	return Operand;
};

void module_expr_t::compile(compiler_t *Compiler) {DEBUG
	compile(Compiler, new label_t, new label_t);
};

Std$Function_status command_expr_t::compile(compiler_t *Compiler, Std$Function_result *Result) {DEBUG
	char ModulePath[256];
	char *Tmp = getcwd(ModulePath, 256);
	strcat(ModulePath, PATHSTR);
	for (command_expr_t::globalvar_t *Var = Vars; Var; Var = Var->Next) {
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::GVAR;
		Std$Object_t **Address = new Std$Object_t *;
		Address[0] = Std$Object$Nil;
		Operand->Address = Address;
		Compiler->declare(Var->Name, Operand);
	};
	for (command_expr_t::globalimp_t *Imp = Imps; Imp; Imp = Imp->Next) {
		int Length = 0;
		for (command_expr_t::globalimp_t::path_t *Path = Imp->Path; Path; Path = Path->Next) Length += strlen(Path->Part) + 1;
		char *ImportPath = new char[Length];
		char *Temp = ImportPath;
		for (command_expr_t::globalimp_t::path_t *Path = Imp->Path; Path; Path = Path->Next) {
			Temp = stpcpy(Temp, Path->Part);
			Temp++[0] = PATHCHR;
		};
		(--Temp)[0] = 0;
		operand_t *Operand = new operand_t;
		Operand->Type = operand_t::FUTR;
		Operand->Future = new future_imp_t(Imp->LineNo, Imp->Relative ? ModulePath : 0, ImportPath);
		Compiler->declare(Imp->Alias, Operand);
		for (command_expr_t::globalimp_t::uselist_t *Use = Imp->Uses; Use; Use = Use->Next) {
			operand_t *Operand = new operand_t;
			Operand->Type = operand_t::FUTR;
			Operand->Future = new future_use_t(Imp->LineNo, Imp->Relative ? ModulePath : 0, ImportPath, Use->Name);
			Compiler->declare(Use->Name, Operand);
		};
	};
	for (command_expr_t::globaldef_t *Def = Defs; Def; Def = Def->Next) {
		operand_t *Operand = Def->Value->precompile(Compiler, Def->Type);
		if (Operand) Compiler->declare(Def->Name, Operand);
	};
	Std$Function_status Status = SUCCESS;
	for (command_expr_t::globaldef_t *Def = Defs; Def; Def = Def->Next) {
		switch (Def->Type) {
		case _PC_NONE: {
			Def->Type = _PC_WAIT;
			Std$Object_t *Value = Def->Value->evaluate(Compiler);
			if (Value == 0) {
				Compiler->raise_error(LineNo, InitErrorMessageT, "Error: constant initialization failed");
			} else {
				Def->Type = _PC_FULL;
				operand_t *Operand = new operand_t;
				Operand->Type = operand_t::CNST;
				Operand->Value = Value;
				Compiler->declare(Def->Name, Operand);
			};
			break;
		};
		case _PC_PARTIAL: {
			Def->Value->compile(Compiler, new label_t, new label_t);
			break;
		};
		case _PC_FULL: {
			break;
		};
		case _PC_WAIT: {
			Compiler->raise_error(Def->LineNo, InitErrorMessageT, "Error: recursive constant initialization");
			break;
		};
		};
	};
	if (Body) {
		Compiler->push_function(LineNo);
			label_t *Start = new label_t;
			label_t *Success = new label_t;
			label_t *Failure = new label_t;
			label_t *Label0 = Start;
			for (expr_t *Expr = Body; Expr; Expr = Expr->Next) {
				label_t *Label1 = new label_t;
				Compiler->push_expression();
					if (Expr->Next) {
						Label0 = Compiler->push_trap(Expr->LineNo, Label0, Label1);
							Expr->compile(Compiler, Label0, Label1);
							Label1->flush(Expr->LineNo);
							Label1->link(Expr->LineNo, Label0 = new label_t);
						Compiler->pop_trap();
					} else {
						Label0 = Compiler->push_trap(LineNo, Label0, Failure);
							Success->load(LineNo, Expr->compile(Compiler, Label0, Success));
						Compiler->pop_trap();
					};
				Compiler->pop_expression();
			};
			Success->ret(LineNo);
			Failure->fail(LineNo);
		frame_t *Frame = Compiler->pop_function();
		operand_t *Closure = Start->assemble(Frame, Compiler->SourceName, LineNo);
		if (Closure == 0) Compiler->raise_error(LineNo, ScopeErrorMessageT, "Error: context does not allow access to surrounding scope");
		Status = Std$Function$call(Closure->Value, 0, Result);
	};
	return Status;
};
