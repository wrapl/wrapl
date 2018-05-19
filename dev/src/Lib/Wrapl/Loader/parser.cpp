#include "parser.h"
#include "scanner.h"
#include "compiler.h"
#include "missing.h"

#include <Riva.h>
#include <string.h>
#include <Std.h>
#include <Agg.h>

#include <stdio.h>

SYMBOL($AT, "@");
SYMBOL($QUERY, "?");
SYMBOL($EQUAL, "=");
SYMBOL($NOTEQ, "~=");
SYMBOL($LESS, "<");
SYMBOL($GREATER, ">");
SYMBOL($LSEQ, "<=");
SYMBOL($GREQ, ">=");
SYMBOL($IN, "in");
SYMBOL($IS, "is");
SYMBOL($SUBTYPE, "<:");
SYMBOL($PLUS, "+");
SYMBOL($MINUS, "-");
SYMBOL($MULTIPLY, "*");
SYMBOL($DIVIDE, "/");
SYMBOL($MODULO, "%");
SYMBOL($POWER, "^");
SYMBOL($PARTIAL, "!!");
SYMBOL($INDEX, "[]");
SYMBOL($BACKSLASH, "\\");
SYMBOL($INVERSE, "~");
SYMBOL($EXCLAIM, "!");
SYMBOL($HASH, "#");
SYMBOL($DOT, ".");
SYMBOL($LOGICALAND, "and");
SYMBOL($LOGICALOR, "or");

extern Riva$Module_t Riva$Symbol[];

expr_t *accept_expr(scanner_t *Scanner);
static expr_t *parse_expr(scanner_t *Scanner);

static expr_t *parse_term(scanner_t *Scanner);
static expr_t *accept_term(scanner_t *Scanner);
static expr_t *accept_factor(scanner_t *Scanner);

static func_expr_t::parameter_t *accept_parameters(scanner_t *Scanner) {
	if (Scanner->parse(tkIDENT)) {
		func_expr_t::parameter_t *Param = new func_expr_t::parameter_t;
		Param->Name = Scanner->Token.Ident;
		bool DefaultRequired = false;
		if (Param->Reference = Scanner->parse(tkPLUS)) {
			if (Scanner->parse(tkOR)) Param->Default = accept_term(Scanner);
		} else if (Scanner->parse(tkOR)) {
			Param->Default = accept_term(Scanner);
		} else {
			if (Param->Variadic = Scanner->parse(tkMULTIPLY)) return Param;
			if (Param->Variadic = Scanner->parse(tkDOTDOT)) return Param;
		};
		func_expr_t::parameter_t *Last = Param;
		while (Scanner->parse(tkCOMMA)) {
			func_expr_t::parameter_t *Next = new func_expr_t::parameter_t;
			Scanner->accept(tkIDENT);
			Last->Next = Next;
			Last = Next;
			Next->Name = Scanner->Token.Ident;
			if (Next->Reference = Scanner->parse(tkPLUS)) {
				if (Scanner->parse(tkOR)) Next->Default = accept_term(Scanner);
			} else if (Scanner->parse(tkOR)) {
				Next->Default = accept_term(Scanner);
			} else {
				if (Next->Variadic = Scanner->parse(tkMULTIPLY)) return Param;
				if (Next->Variadic = Scanner->parse(tkDOTDOT)) return Param;
			};
		};
		return Param;
	} else {
		return 0;
	};
};

struct typed_parameters {
	func_expr_t::parameter_t *Parameters;
	expr_t *Types;
};

static typed_parameters accept_typed_parameters(scanner_t *Scanner) {
	func_expr_t::parameter_t *Param = new func_expr_t::parameter_t;
	if (Scanner->parse(tkIDENT)) {
		Param->Name = Scanner->Token.Ident;
		Param->Variadic = Scanner->parse(tkMULTIPLY);
		Param->Reference = Scanner->parse(tkPLUS);
		if (Scanner->parse(tkOR)) Param->Default = accept_term(Scanner);
	} else {
		char *Name;
		int Length = asprintf(&Name, "anon:%x", (unsigned int)Param);
		Param->Name = Name;
	};
	expr_t *Type;
	if (Scanner->parse(tkAT)) {
		Type = new const_expr_t(Scanner->Token.LineNo, $IN);
		Type->Next = accept_expr(Scanner);
	} else if (Scanner->parse(tkEQUAL)) {
		Type = new const_expr_t(Scanner->Token.LineNo, $IS);
		Type->Next = accept_expr(Scanner);
	} else {
		Type = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
		Type->Next = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
	};
	if (Scanner->parse(tkCOMMA)) {
		typed_parameters TP = accept_typed_parameters(Scanner);
		Param->Next = TP.Parameters;
		TP.Parameters = Param;
		Type->Next->Next = TP.Types;
		TP.Types = Type;
		return TP;
	} else {
		return {Param, Type};
	};
};

static expr_t *accept_typed_arguments(scanner_t *Scanner) {
	expr_t *Type;
	if (Scanner->parse(tkAT)) {
		Type = new const_expr_t(Scanner->Token.LineNo, $IN);
		Type->Next = accept_expr(Scanner);
	} else if (Scanner->parse(tkEQUAL)) {
		Type = new const_expr_t(Scanner->Token.LineNo, $IS);
		Type->Next = accept_expr(Scanner);
	} else if (Scanner->parse(tkQUERY)) {
		Type = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
		Type->Next = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
	};
	if (Scanner->parse(tkCOMMA)) Type->Next->Next = accept_typed_arguments(Scanner);
	return Type;
};

#define PREFIX(SYMBOL)\
	if (Scanner->parse(tk ## SYMBOL)) {\
		expr_t *Function = new const_expr_t(Scanner->Token.LineNo, $ ## SYMBOL);\
		return new invoke_expr_t(Scanner->Token.LineNo, Function, accept_term(Scanner));\
	};

expr_t *accept_expr_list(scanner_t *Scanner) {
	expr_t *List = parse_expr(Scanner);
	if (List == 0) return 0;
	expr_t *Tail = List;
	while (Scanner->parse(tkCOMMA)) {
		Tail->Next = accept_expr(Scanner);
		Tail = Tail->Next;
	};
	return List;
};

expr_t *accept_named_arguments_rest(scanner_t *Scanner, Std$Symbol_array *&Symbols, int Index) {
	if (!Scanner->parse(tkCOMMA)) {
		Symbols = (Std$Symbol_array *)Riva$Memory$alloc(sizeof(Std$Symbol_array) + (Index + 1) * sizeof(Std$Symbol_t *));
		Symbols->Type = Std$Symbol$ArrayT;
		Symbols->Count = Index;
		return 0;
	};
	Std$Symbol_t *Symbol;
	if (Scanner->parse(tkSYMBOL)) {
		Symbol = (Std$Symbol_t *)Scanner->Token.Const;
	} else {
		Scanner->accept(tkIDENT);
		int Type;
		Riva$Module$import(Riva$Symbol, Scanner->Token.Ident, &Type, (void **)&Symbol);
	};
	Scanner->accept(tkIS);
	expr_t *List = accept_expr(Scanner);
	List->Next = accept_named_arguments_rest(Scanner, Symbols, Index + 1);
	Symbols->Values[Index] = Symbol;
	return List;
};

expr_t *accept_arguments_rest(scanner_t *Scanner) {
	if (!Scanner->parse(tkCOMMA)) return 0;
	expr_t *List = accept_expr(Scanner);
	if (Scanner->parse(tkIS)) {
		//if (ident_expr_t *Ident = dynamic_cast<ident_expr_t *>(List)) {
		if (!strcmp(List->classid(), ident_expr_t::_classid())) {
			ident_expr_t *Ident = (ident_expr_t *)List;
			Std$Symbol_t *Symbol;
			int Type;
			Riva$Module$import(Riva$Symbol, Ident->Name, &Type, (void **)&Symbol);
			Std$Symbol_array *Symbols;
			List = accept_expr(Scanner);
			List->Next = accept_named_arguments_rest(Scanner, Symbols, 1);
			Symbols->Values[0] = Symbol;
			expr_t *Names = new const_expr_t(Scanner->Token.LineNo, Symbols);
			Names->Next = List;
			return Names;
		} else if (!strcmp(List->classid(), const_expr_t::_classid())) {
			Std$Symbol_t *Symbol = (Std$Symbol_t *)((const_expr_t *)List)->Operand->Value;
			if (Symbol->Type != Std$Symbol$T) Scanner->raise_error(List->LineNo, ParseErrorMessageT, "Parameter names must be identifiers");
			Std$Symbol_array *Symbols;
			List = accept_expr(Scanner);
			List->Next = accept_named_arguments_rest(Scanner, Symbols, 1);
			Symbols->Values[0] = Symbol;
			expr_t *Names = new const_expr_t(Scanner->Token.LineNo, Symbols);
			Names->Next = List;
			return Names;
		} else {
			Scanner->raise_error(List->LineNo, ParseErrorMessageT, "Parameter names must be identifiers");
		};
	} else {
		List->Next = accept_arguments_rest(Scanner);
		return List;
	};
};

expr_t *accept_arguments(scanner_t *Scanner) {
	expr_t *List = parse_expr(Scanner);
	if (List == 0) return 0;
	if (Scanner->parse(tkIS)) {
		//if (ident_expr_t *Ident = dynamic_cast<ident_expr_t *>(List)) {
		if (!strcmp(List->classid(), ident_expr_t::_classid())) {
			ident_expr_t *Ident = (ident_expr_t *)(List);
			Std$Symbol_t *Symbol;
			int Type;
			Riva$Module$import(Riva$Symbol, Ident->Name, &Type, (void **)&Symbol);
			Std$Symbol_array *Symbols;
			List = accept_expr(Scanner);
			List->Next = accept_named_arguments_rest(Scanner, Symbols, 1);
			Symbols->Values[0] = Symbol;
			expr_t *Names = new const_expr_t(Scanner->Token.LineNo, Symbols);
			Names->Next = List;
			return Names;
		} else if (!strcmp(List->classid(), const_expr_t::_classid())) {
			Std$Symbol_t *Symbol = (Std$Symbol_t *)((const_expr_t *)List)->Operand->Value;
			if (Symbol->Type != Std$Symbol$T) Scanner->raise_error(List->LineNo, ParseErrorMessageT, "Parameter names must be identifiers");
			Std$Symbol_array *Symbols;
			List = accept_expr(Scanner);
			List->Next = accept_named_arguments_rest(Scanner, Symbols, 1);
			Symbols->Values[0] = Symbol;
			expr_t *Names = new const_expr_t(Scanner->Token.LineNo, Symbols);
			Names->Next = List;
			return Names;
		} else {
			Scanner->raise_error(List->LineNo, ParseErrorMessageT, "Parameter names must be identifiers");
		};
	} else {
		List->Next = accept_arguments_rest(Scanner);
		return List;
	};
};

static expr_t *accept_table_list(scanner_t *Scanner) {
	expr_t *List = parse_expr(Scanner);
	if (List == 0) return 0;
	expr_t *Tail = List;
	if (Scanner->parse(tkIS)) {
		Tail->Next = accept_expr(Scanner);
	} else {
		Tail->Next = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
	};
	Tail = Tail->Next;
	while (Scanner->parse(tkCOMMA)) {
		Tail->Next = accept_expr(Scanner);
		Tail = Tail->Next;
		if (Scanner->parse(tkIS)) {
			Tail->Next = accept_expr(Scanner);
		} else {
			Tail->Next = new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
		};
		Tail = Tail->Next;
	};
	return List;
};

static Std$Array_t *accept_fields(scanner_t *Scanner, int Index = 0) {
	Std$Object_t *Field;
	if (Scanner->parse(tkIDENT)) {
		int Type;
		Riva$Module$import(Riva$Symbol, Scanner->Token.Ident, &Type, (void **)&Field);
	} else if (Scanner->parse(tkSYMBOL)) {
		Field = Scanner->Token.Const;
	} else {
		return (Std$Array_t *)Std$Array$new(Index);
	};
	Scanner->parse(tkCOMMA);
	Std$Array_t *Fields = accept_fields(Scanner, Index + 1);
	Fields->Values[Index] = Field;
	return Fields;
};

static expr_t *accept_field_list(scanner_t *Scanner) {
	if (Scanner->parse(tkGREATER)) return 0;
	expr_t *List;
	if (Scanner->parse(tkIDENT)) {
		Std$Object_t *Field;
		int Type;
		Riva$Module$import(Riva$Symbol, Scanner->Token.Ident, &Type, (void **)&Field);
		List = new const_expr_t(Scanner->Token.LineNo, Field);
	} else if ((Scanner->NextToken.Type == tkCONST) && (Scanner->NextToken.Const->Type == Std$String$T)) {
		Std$Object_t *Field;
		int Type;
		Riva$Module$import(Riva$Symbol, Std$String$flatten(Scanner->NextToken.Const), &Type, (void **)&Field);
		Scanner->accept(tkCONST);
		List = new const_expr_t(Scanner->Token.LineNo, Field);
	} else {
		List = accept_factor(Scanner);
	};
	expr_t *Tail = List;
	while (!Scanner->parse(tkGREATER)) {
		Scanner->parse(tkCOMMA);
		if (Scanner->parse(tkIDENT)) {
			Std$Object_t *Field;
			int Type;
			Riva$Module$import(Riva$Symbol, Scanner->Token.Ident, &Type, (void **)&Field);
			Tail->Next = new const_expr_t(Scanner->Token.LineNo, Field);
		} else if ((Scanner->NextToken.Type == tkCONST) && (Scanner->NextToken.Const->Type == Std$String$T)) {
			Std$Object_t *Field;
			int Type;
			Riva$Module$import(Riva$Symbol, Std$String$flatten(Scanner->NextToken.Const), &Type, (void **)&Field);
			Scanner->accept(tkCONST);
			Tail->Next = new const_expr_t(Scanner->Token.LineNo, Field);
		} else {
			Tail->Next = accept_factor(Scanner);
		};
		Tail = Tail->Next;
	};
	return List;
};

static expr_t *accept_factor(scanner_t *Scanner);

static when_expr_t::case_t::range_t *accept_when_expr_range(scanner_t *Scanner) {
	expr_t *Min = accept_expr(Scanner);
	expr_t *Max = Scanner->parse(tkDOTDOT) ? accept_expr(Scanner) : 0;
	when_expr_t::case_t::range_t *Range = new when_expr_t::case_t::range_t(Scanner->Token.LineNo, Min, Max);
	if (Scanner->parse(tkCOMMA)) Range->Next = accept_when_expr_range(Scanner);
	return Range;
};

static when_expr_t *accept_when_expr_case(scanner_t *Scanner, bool Skip) {
	if (Skip || Scanner->parse(tkIS)) {
		when_expr_t::case_t::range_t *Ranges = accept_when_expr_range(Scanner);
		Scanner->accept(tkDO);
		when_expr_t::case_t *Case = new when_expr_t::case_t(Scanner->Token.LineNo, Ranges, accept_expr(Scanner));
		when_expr_t *Expr = accept_when_expr_case(Scanner, false);
		Case->Next = Expr->Cases;
		Expr->Cases = Case;
		return Expr;
	} else {
		when_expr_t *Expr = new when_expr_t;
		Expr->Default = Scanner->parse(tkDO) ? accept_expr(Scanner) : new back_expr_t(Scanner->Token.LineNo);
		return Expr;
	};
};

static whentype_expr_t *accept_whentype_expr_case(scanner_t *Scanner, bool Skip) {
	if (Skip || Scanner->parse(tkAS)) {
		expr_t *Type = accept_expr_list(Scanner);
		if (!Type) Scanner->raise_error(Scanner->Token.LineNo, ParseErrorMessageT, "Error: expected expression not %s", Tokens[Scanner->NextToken.Type]);
		Scanner->accept(tkDO);
		whentype_expr_t::case_t *Case = new whentype_expr_t::case_t(Scanner->Token.LineNo, Type, accept_expr(Scanner));
		whentype_expr_t *Expr = accept_whentype_expr_case(Scanner, false);
		Case->Next = Expr->Cases;
		Expr->Cases = Case;
		return Expr;
	} else {
		whentype_expr_t *Expr = new whentype_expr_t;
		Expr->Default = Scanner->parse(tkDO) ? accept_expr(Scanner) : new back_expr_t(Scanner->Token.LineNo);
		return Expr;
	};
};

static expr_t *accept_when_expr(scanner_t *Scanner) {
	expr_t *Condition = accept_expr(Scanner);
	int LineNo = Scanner->Token.LineNo;
	if (Scanner->parse(tkAS)) {
		whentype_expr_t *Expr = accept_whentype_expr_case(Scanner, true);
		Expr->Condition = Condition;
		Expr->LineNo = LineNo;
		return Expr;
	} else if (Scanner->parse(tkIS)) {
		when_expr_t *Expr = accept_when_expr_case(Scanner, true);
		Expr->Condition = Condition;
		Expr->LineNo = LineNo;
		return Expr;
	} else {
		return new right_expr_t(LineNo, Condition, accept_expr(Scanner));
	};
};

extern Std$Object_t Std$Type$New[];

static block_expr_t *accept_localstatement(scanner_t *Scanner);

static block_expr_t *accept_localvar(scanner_t *Scanner) {
	block_expr_t::localvar_t *Var = new block_expr_t::localvar_t;
	Var->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Var->Name = Scanner->Token.Ident;
	Var->Reference = Scanner->parse(tkPLUS);
	if (Scanner->parse(tkASSIGN)) {
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, accept_expr(Scanner));
		if (Scanner->parse(tkCOMMA)) {
			block_expr_t *Block = accept_localvar(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		} else {
			Scanner->accept(tkSEMICOLON);
			block_expr_t *Block = accept_localstatement(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		};
	//} else if (Scanner->parse(tkREFASSIGN)) {
	} else if (Scanner->parse(tkIS)) {
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		ref_assign_expr_t *Assign = new ref_assign_expr_t(Scanner->Token.LineNo, Ident, accept_expr(Scanner));
		if (Scanner->parse(tkCOMMA)) {
			block_expr_t *Block = accept_localvar(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		} else {
			Scanner->accept(tkSEMICOLON);
			block_expr_t *Block = accept_localstatement(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		};
	} else if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		func_expr_t *Func = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, Func);
		if (Scanner->parse(tkCOMMA)) {
			block_expr_t *Block = accept_localvar(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		} else {
			Scanner->accept(tkSEMICOLON);
			block_expr_t *Block = accept_localstatement(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			if (Block->Final) {
				Assign->Next = Block->Body;
				Block->Body = Assign;
			} else {
				Block->Final = Assign;
			};
			return Block;
		};
	} else {
		if (Scanner->parse(tkCOMMA)) {
			block_expr_t *Block = accept_localvar(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			return Block;
		} else {
			Scanner->accept(tkSEMICOLON);
			block_expr_t *Block = accept_localstatement(Scanner);
			Var->Next = Block->Vars;
			Block->Vars = Var;
			return Block;
		};
	};
};

static block_expr_t *accept_localdef(scanner_t *Scanner) {
	block_expr_t::localdef_t *Def = new block_expr_t::localdef_t;
	Def->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Def->Name = Scanner->Token.Ident;
	if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		Def->Value = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
		block_expr_t *Block;
		if (Scanner->parse(tkCOMMA)) {
			Block = accept_localvar(Scanner);
		} else {
			Scanner->accept(tkSEMICOLON);
			Block = accept_localstatement(Scanner);
		};
		Def->Next = Block->Defs;
		Block->Defs = Def;
		if (Block->Final == 0) Block->Final = new ident_expr_t(Scanner->Token.LineNo, Def->Name);
		return Block;
	} else {
//		Scanner->accept(tkASSIGN);
		Scanner->parse(tkASSIGN); // Allow <- to be optional
		Def->Value = accept_expr(Scanner);
		block_expr_t *Block;
		if (Scanner->parse(tkCOMMA)) {
			Block = accept_localdef(Scanner);
		} else {
			Scanner->accept(tkSEMICOLON);
			Block = accept_localstatement(Scanner);
		};
		Def->Next = Block->Defs;
		Block->Defs = Def;
		if (Block->Final == 0) Block->Final = new ident_expr_t(Scanner->Token.LineNo, Def->Name);
		return Block;
	};
};

static block_expr_t *accept_localrecv(scanner_t *Scanner) {
	int LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	const char *Var = Scanner->Token.Ident;
	expr_t *Body;
	if (Scanner->parse(tkAS)) {
		int LineNo = Scanner->Token.LineNo;
		whentype_expr_t *Expr = accept_whentype_expr_case(Scanner, true);
		Expr->Condition = new ident_expr_t(LineNo, Var);
		Expr->LineNo = LineNo;
		Body = Expr;
	} else if (Scanner->parse(tkIS)) {
		int LineNo = Scanner->Token.LineNo;
		when_expr_t *Expr = accept_when_expr_case(Scanner, true);
		Expr->Condition = new ident_expr_t(LineNo, Var);
		Expr->LineNo = LineNo;
		Body = Expr;
	} else {
		Scanner->accept(tkDO);
		Body = accept_expr(Scanner);
	};
	if (Scanner->parse(tkSEMICOLON)) {
		block_expr_t *Block = accept_localstatement(Scanner);
		if (Block->Receiver.Body) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can only have one receiver");
		if (Block->Must) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can not have both receiver and must");
		Block->Receiver.Var = Var;
		Block->Receiver.Body = Body;
		return Block;
	} else {
		block_expr_t *Block = new block_expr_t;
		if (Block->Receiver.Body) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can only have one receiver");
		if (Block->Must) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can not have both receiver and must");
		Block->Receiver.Var = Var;
		Block->Receiver.Body = Body;
		return Block;
	};
};

static block_expr_t *accept_localmust(scanner_t *Scanner) {
	int LineNo = Scanner->Token.LineNo;
	expr_t *Must = accept_expr(Scanner);
	if (Scanner->parse(tkSEMICOLON)) {
		block_expr_t *Block = accept_localstatement(Scanner);
		if (Block->Receiver.Body) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can not have both receiver and must");
		if (Block->Must) Scanner->raise_error(LineNo, ParseErrorMessageT, "Error: block can only have one must");
		Block->Must = Must;
		return Block;
	} else {
		block_expr_t *Block = new block_expr_t;
		Block->Must = Must;
		return Block;
	};
};

static block_expr_t *accept_localstatement(scanner_t *Scanner) {
	if (Scanner->parse(tkVAR)) return accept_localvar(Scanner);
	if (Scanner->parse(tkDEF)) return accept_localdef(Scanner);
	if (Scanner->parse(tkRECV)) return accept_localrecv(Scanner);
	if (Scanner->parse(tkMUST)) return accept_localmust(Scanner);
	expr_t *Expr = parse_expr(Scanner);
	if (Expr) {
		if (Scanner->parse(tkSEMICOLON)) {
			block_expr_t *Block = accept_localstatement(Scanner);
			if (Block->Final) {
				Expr->Next = Block->Body;
				Block->Body = Expr;
			} else {
				Block->Final = Expr;
			};
			return Block;
		} else {
			block_expr_t *Block = new block_expr_t;
			Block->Final = Expr;
			return Block;
		};
	};
	return new block_expr_t;
};

module_expr_t *accept_module(scanner_t *Scanner, Riva$Module$provider_t *Provider);

SYMBOL($MAX, "max");
SYMBOL($MIN, "min");

static expr_t *parse_factor(scanner_t *Scanner) {
	if (Scanner->parse(tkCONST)) return new const_expr_t(Scanner->Token.LineNo, Scanner->Token.Const);
	if (Scanner->parse(tkSTRBLOCK)) return new invoke_expr_t(Scanner->Token.LineNo,
		new const_expr_t(Scanner->Token.LineNo, Std$String$Create),
		(expr_t *)Scanner->Token.Const
	);
	if (Scanner->parse(tkSYMBOL)) return new const_expr_t(Scanner->Token.LineNo, Scanner->Token.Const);
	if (Scanner->parse(tkIDENT)) {
		const char *Ident = Scanner->Token.Ident;
		if (Scanner->parse(tkDOT)) {
			expr_t *Expr;
			if (Scanner->parse(tkIDENT)) {
			retry:
				qualident_expr_t::name_t *Head = new qualident_expr_t::name_t;
				Head->Ident = Ident;
				qualident_expr_t::name_t *Tail = new qualident_expr_t::name_t;
				Head->Next = Tail;
				Tail->Ident = Scanner->Token.Ident;
				while (Scanner->parse(tkDOT)) {
					if (Scanner->parse(tkIDENT)) {
					retry1:
						Tail = (Tail->Next = new qualident_expr_t::name_t);
						Tail->Ident = Scanner->Token.Ident;
					} else {
						if ((Scanner->NextToken.Type == tkCONST) && (Scanner->NextToken.Const->Type == Std$String$T)) {
							Scanner->NextToken.Ident = Std$String$flatten(Scanner->NextToken.Const);
							Scanner->accept(tkCONST);
							goto retry1;
						};
						Expr = new qualident_expr_t(Scanner->Token.LineNo, Head);
						goto mixed;
					};
				};
				return new qualident_expr_t(Scanner->Token.LineNo, Head);
			} else {
				if ((Scanner->NextToken.Type == tkCONST) && (Scanner->NextToken.Const->Type == Std$String$T)) {
					Scanner->NextToken.Ident = Std$String$flatten(Scanner->NextToken.Const);
					Scanner->accept(tkCONST);
					goto retry;
				};
				Expr = new ident_expr_t(Scanner->Token.LineNo, Ident);
			mixed:
				Expr->Next = accept_factor(Scanner);
				Expr = new invoke_expr_t(Scanner->Token.LineNo, new const_expr_t(Scanner->Token.LineNo, $DOT), Expr);
				while (Scanner->parse(tkDOT)) {
					if (Scanner->parse(tkIDENT)) {
						Expr->Next = new const_expr_t(Scanner->Token.LineNo, (Std$Object_t *)Std$String$new(Scanner->Token.Ident));
					} else {
						Expr->Next = accept_factor(Scanner);
					};
					Expr = new invoke_expr_t(Scanner->Token.LineNo, new const_expr_t(Scanner->Token.LineNo, $DOT), Expr);
				};
				return Expr;
			};
		} else {
			return new ident_expr_t(Scanner->Token.LineNo, Ident);
		};
	};
	if (Scanner->parse(tkSELF)) return new self_expr_t(Scanner->Token.LineNo);
	if (Scanner->parse(tkNIL)) return new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil);
	if (Scanner->parse(tkLBRACKET)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Expr = new invoke_expr_t(LineNo,
			new const_expr_t(LineNo, Agg$List$Make),
			accept_expr_list(Scanner)
		);
		Scanner->accept(tkRBRACKET);
		return Expr;
	};
	if (Scanner->parse(tkLBRACE)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Expr = new invoke_expr_t(LineNo,
			new const_expr_t(LineNo, Agg$Table$Make),
			accept_table_list(Scanner)
		);
		Scanner->accept(tkRBRACE);
		return Expr;
	};
	if (Scanner->parse(tkLESS)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		if (Scanner->parse(tkLBRACKET)) {
			expr_t *Parents = accept_expr_list(Scanner);
			Scanner->accept(tkRBRACKET);
			expr_t *Fields;
			//if (Scanner->parse(tkGREATER)) {
			//	Fields = new const_expr_t(Scanner->Token.LineNo, Std$Array$new(0));
			//} else {
				/*Fields = new const_expr_t(Scanner->Token.LineNo, accept_fields(Scanner));
				Scanner->accept(tkGREATER);*/
				Fields = accept_field_list(Scanner);
				bool UseArray = true;
				int NumFields = 0;
				for (expr_t *Field = Fields; Field; Field = Field->Next) {
					++NumFields;
					if (strcmp(Field->classid(), const_expr_t::_classid())) {
						UseArray = false;
						break;
					};
					if (!(((const_expr_t *)Field)->Operand->Value->Type == Std$Symbol$T)) {
						UseArray = false;
						break;
					};
				};
				if (UseArray) {
					Std$Array$t *FieldsArray = (Std$Array$t *)Std$Array$new(NumFields);
					int Index = 0;
					for (expr_t *Field = Fields; Field; Field = Field->Next) {
						FieldsArray->Values[Index] = ((const_expr_t *)Field)->Operand->Value;
						++Index;
					};
					Fields = new const_expr_t(Scanner->Token.LineNo, FieldsArray);
				} else {
					Fields = new invoke_expr_t(Scanner->Token.LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Array$New), Fields);
				};
			//};
			Fields->Next = Parents;
			return new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Type$New), Fields);
		} else if (Scanner->parse(tkHASH)) {
			expr_t *Value = accept_expr(Scanner);
			Scanner->accept(tkSEMICOLON);
			Scanner->accept(tkGREATER);
			return new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$ConstantNew), Value);
		} else if (Scanner->parse(tkQUERY)) {
			expr_t *Value = accept_expr(Scanner);
			Scanner->accept(tkSEMICOLON);
			Scanner->accept(tkGREATER);
			return new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$VariableNew), Value);
		} else {
			func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
			Scanner->accept(tkGREATER);
			if (Parameters == 0) {
				if (Scanner->parse(tkVAR)) {
					return new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$VariableNew), accept_expr(Scanner));
				} else if (Scanner->parse(tkDEF)) {
					return new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$ConstantNew), accept_expr(Scanner));
				};
			};
			return new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
		};
	};
	if (Scanner->parse(tkLPAREN)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Block = accept_localstatement(Scanner);
		Block->LineNo = LineNo;
		Scanner->accept(tkRPAREN);
		return Block;
	};
	if (Scanner->parse(tkBACK)) return new back_expr_t(Scanner->Token.LineNo);
	if (Scanner->parse(tkFAIL)) return new fail_expr_t(Scanner->Token.LineNo);
	if (Scanner->parse(tkRET)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Value = parse_expr(Scanner);
		if (Value == 0) Value = new const_expr_t(LineNo, Std$Object$Nil);
		return new ret_expr_t(LineNo, Value);
	};
	if (Scanner->parse(tkSUSP)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new susp_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkWITH)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		bool Parallel = false;
		if (!Scanner->parse(tkSEQ)) Parallel = Scanner->parse(tkPAR);
		with_expr_t::binding_t *Bindings = new with_expr_t::binding_t;
		Scanner->accept(tkIDENT);
		Bindings->Name = Scanner->Token.Ident;
		if (!Scanner->parse(tkASSIGN)) Scanner->accept(tkAS);
		Bindings->Value = accept_expr(Scanner);
		with_expr_t::binding_t *Last = Bindings;
		while (Scanner->parse(tkCOMMA)) {
			with_expr_t::binding_t *Binding = new with_expr_t::binding_t;
			Scanner->accept(tkIDENT);
			Binding->Name = Scanner->Token.Ident;
			if (!Scanner->parse(tkASSIGN)) Scanner->accept(tkAS);
			Binding->Value = accept_expr(Scanner);
			Last->Next = Binding;
			Last = Binding;
		};
		Scanner->accept(tkDO);
		return new with_expr_t(LineNo, Bindings, accept_expr(Scanner), Parallel);
	};
	if (Scanner->parse(tkREP)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new rep_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkALL)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new all_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkUNIQ)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new uniq_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkCOUNT)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new count_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkSEQ)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Exprs = accept_expr(Scanner);
		expr_t *Last = Exprs;
		while (Scanner->parse(tkCOMMA)) {
			Last->Next = accept_expr(Scanner);
			Last = Last->Next;
		};
		return new sequence_expr_t(Scanner->Token.LineNo, Exprs);
	};
	if (Scanner->parse(tkPAR)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Exprs = accept_expr(Scanner);
		expr_t *Last = Exprs;
		while (Scanner->parse(tkCOMMA)) {
			Last->Next = accept_expr(Scanner);
			Last = Last->Next;
		};
		return new parallel_expr_t(Scanner->Token.LineNo, Exprs);
	};
	if (Scanner->parse(tkINT)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Exprs = accept_expr(Scanner);
		expr_t *Last = Exprs;
		while (Scanner->parse(tkCOMMA)) {
			Last->Next = accept_expr(Scanner);
			Last = Last->Next;
		};
		return new interleave_expr_t(Scanner->Token.LineNo, Exprs);
	};
	if (Scanner->parse(tkEXIT)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Value = parse_expr(Scanner);
		if (Value == 0) Value = new const_expr_t(LineNo, Std$Object$Nil);
		return new exit_expr_t(LineNo, Value);
	};
	if (Scanner->parse(tkSTEP)) return new step_expr_t(Scanner->Token.LineNo);
	if (Scanner->parse(tkEVERY)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Condition = accept_expr_list(Scanner);
		if (Scanner->parse(tkDO)) return new every_expr_t(LineNo, Condition, accept_expr(Scanner));
		return new every_expr_t(LineNo, Condition, new const_expr_t(LineNo, Std$Object$Nil));
	};
	/*if (Scanner->parse(tkNOT)) return new cond_expr_t(Scanner->Token.LineNo,
		accept_expr(Scanner),
		new back_expr_t(Scanner->Token.LineNo),
		new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil)
	);*/
	if (Scanner->parse(tkWHILE)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new cond_expr_t(LineNo,
			accept_expr(Scanner),
			new back_expr_t(LineNo),
			new exit_expr_t(LineNo, new const_expr_t(LineNo, Std$Object$Nil))
		);
	};
	if (Scanner->parse(tkUNTIL)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new cond_expr_t(LineNo,
			accept_expr(Scanner),
			new exit_expr_t(LineNo, new const_expr_t(LineNo, Std$Object$Nil)),
			new back_expr_t(LineNo)
		);
	};
	if (Scanner->parse(tkWHEN)) return accept_when_expr(Scanner);
	if (Scanner->parse(tkSEND)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new send_expr_t(LineNo, accept_expr(Scanner));
	};
	if (Scanner->parse(tkTO)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Symbol = accept_factor(Scanner);
		Scanner->accept(tkLPAREN);
		typed_parameters TP = {0, 0};
		if (!Scanner->parse(tkRPAREN)) {
			TP = accept_typed_parameters(Scanner);
			Scanner->accept(tkRPAREN);
		};
		expr_t *Body;
		if (Scanner->parse(tkIS)) {
			Body = accept_expr(Scanner);
		} else if (Scanner->parse(tkVAR)) {
			Body = new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$VariableNew), accept_expr(Scanner));
		} else if (Scanner->parse(tkDEF)) {
			Body = new invoke_expr_t(LineNo, new const_expr_t(Scanner->Token.LineNo, Std$Function$ConstantNew), accept_expr(Scanner));
		} else {
			Body = new func_expr_t(LineNo, TP.Parameters, accept_expr(Scanner));
		};
		Symbol->Next = Body;
		Body->Next = TP.Types;
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Symbol$Set), Symbol);
	};
	if (Scanner->parse(tkIMP)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Symbol = accept_factor(Scanner);
		Scanner->accept(tkLPAREN);
		if (!Scanner->parse(tkRPAREN)) {
			Symbol->Next = accept_typed_arguments(Scanner);
			Scanner->accept(tkRPAREN);
		};
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Symbol$Get), Symbol);
	};
	if (Scanner->parse(tkDO)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Function = new func_expr_t(LineNo, 0,
			new every_expr_t(LineNo,
				new susp_expr_t(LineNo, accept_expr(Scanner)),
				new const_expr_t(LineNo, Std$Object$Nil)
			)
		);
		//return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Coexpr$New), Function);
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$IteratorNew), Function);
	};
	if (Scanner->parse(tkNEW)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		Scanner->accept(tkLPAREN);
		expr_t *Expr = new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Object$Create), accept_arguments(Scanner));
		Scanner->accept(tkRPAREN);
		return Expr;
	}
	if (Scanner->parse(tkSUM)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Args = new const_expr_t(LineNo, $PLUS);
		Args->Next = new code_expr_t(LineNo, accept_expr(Scanner));
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$Fold), Args);
	};
	if (Scanner->parse(tkPROD)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Args = new const_expr_t(LineNo, $MULTIPLY);
		Args->Next = new code_expr_t(LineNo, accept_expr(Scanner));
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$Fold), Args);
	};
	if (Scanner->parse(tkMAX)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Args = new const_expr_t(LineNo, $MAX);
		Args->Next = new code_expr_t(LineNo, accept_expr(Scanner));
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$Fold), Args);
	};
	if (Scanner->parse(tkMIN)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Args = new const_expr_t(LineNo, $MIN);
		Args->Next = new code_expr_t(LineNo, accept_expr(Scanner));
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$Fold), Args);
	};
	if (Scanner->parse(tkUSE)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Function$IteratorNext), accept_expr(Scanner));
	};
	//if (Scanner->parse(tkYIELD)) {
		//uint32_t LineNo = Scanner->Token.LineNo;
		//return new invoke_expr_t(LineNo, new const_expr_t(LineNo, Std$Coexpr$Yield), accept_expr(Scanner));
	//};
	if (Scanner->parse(tkMOD)) return accept_module(Scanner, 0);
	if (Scanner->parse(tkBACKQUOTE)) {
		uint32_t LineNo = Scanner->Token.LineNo;
		expr_t *Expr = accept_expr(Scanner);
		Scanner->accept(tkBACKQUOTE);
		return new backquote_expr_t(LineNo, Expr);
	};
	return 0;
};

static expr_t *accept_factor(scanner_t *Scanner) {
	expr_t *Expr = parse_factor(Scanner);
	if (Expr) return Expr;
	Scanner->raise_error(Scanner->Token.LineNo, ParseErrorMessageT, "Error: expected factor not %s", Tokens[Scanner->NextToken.Type]);
};

static expr_t *parse_term(scanner_t *Scanner) {
	PREFIX(MODULO);
	PREFIX(POWER);
	PREFIX(MINUS);
	PREFIX(DIVIDE);
	PREFIX(BACKSLASH);
	PREFIX(INVERSE);
	PREFIX(EXCLAIM);
	PREFIX(HASH);
	if (Scanner->parse(tkQUERY)) return new typeof_expr_t(Scanner->Token.LineNo, accept_term(Scanner));
	if (Scanner->parse(tkDOT)) return new valueof_expr_t(Scanner->Token.LineNo, accept_term(Scanner));
	if (Scanner->parse(tkAT)) return new infinite_expr_t(Scanner->Token.LineNo, accept_term(Scanner));
	expr_t *Expr = parse_factor(Scanner);
	if (Expr == 0) return 0;
start:
	if (Scanner->parse(tkSYMBOL)) {
		expr_t *Symbol = new const_expr_t(Scanner->Token.LineNo, Scanner->Token.Const);
		if (Scanner->parse(tkLPAREN)) {
			Expr->Next = accept_arguments(Scanner);
			if (Scanner->parse(tkSEMICOLON)) {
				func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
				Scanner->accept(tkRPAREN);
				expr_t **Slot = &Expr;
				while (*Slot) Slot = &Slot[0]->Next;
				*Slot = new func_expr_t(Scanner->Token.LineNo, Parameters, accept_expr(Scanner));
			} else {
				Scanner->accept(tkRPAREN);
			};
		} else if (Scanner->parse(tkLBRACE)) {
			Expr = new infinite_expr_t(Scanner->Token.LineNo, Expr);
			Expr->Next = accept_expr_list(Scanner);
			if (Scanner->parse(tkSEMICOLON)) {
				func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
				Scanner->accept(tkRBRACE);
				expr_t **Slot = &Expr;
				while (*Slot) Slot = &Slot[0]->Next;
				*Slot = new func_expr_t(Scanner->Token.LineNo, Parameters, accept_expr(Scanner));
			} else {
				Scanner->accept(tkRBRACE);
			};
			Expr = new parallel_invoke_expr_t(Scanner->Token.LineNo, Symbol, Expr);
			goto start;
		};
		Expr = new invoke_expr_t(Scanner->Token.LineNo, Symbol, Expr);
		goto start;
	};
	if (Scanner->parse(tkLPAREN)) {
		expr_t *Args = accept_arguments(Scanner);
		if (Scanner->parse(tkSEMICOLON)) {
			func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
			Scanner->accept(tkRPAREN);
			expr_t **Slot = &Args;
			while (*Slot) Slot = &Slot[0]->Next;
			*Slot = new func_expr_t(Scanner->Token.LineNo, Parameters, accept_expr(Scanner));
		} else {
			Scanner->accept(tkRPAREN);
		};
		Expr = new invoke_expr_t(Scanner->Token.LineNo, Expr, Args);
		goto start;
	};
	if (Scanner->parse(tkLBRACE)) {
		expr_t *Args = accept_expr_list(Scanner);
		if (Scanner->parse(tkSEMICOLON)) {
			func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
			Scanner->accept(tkRBRACE);
			expr_t **Slot = &Args;
			while (*Slot) Slot = &Slot[0]->Next;
			*Slot = new func_expr_t(Scanner->Token.LineNo, Parameters, accept_expr(Scanner));
		} else {
			Scanner->accept(tkRBRACE);
		};
		Expr = new parallel_invoke_expr_t(Scanner->Token.LineNo, Expr, Args);
		goto start;
	};
	if (Scanner->parse(tkLBRACKET)) {
		Expr->Next = accept_arguments(Scanner);
		if (Scanner->parse(tkSEMICOLON)) {
			func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
			Scanner->accept(tkRBRACKET);
			expr_t **Slot = &Expr;
			while (*Slot) Slot = &Slot[0]->Next;
			*Slot = new func_expr_t(Scanner->Token.LineNo, Parameters, accept_expr(Scanner));
		} else {
			Scanner->accept(tkRBRACKET);
		};
		Expr = new invoke_expr_t(Scanner->Token.LineNo, new const_expr_t(Scanner->Token.LineNo, $INDEX), Expr);
		goto start;
	};
	if (Scanner->parse(tkDOT)) {
		if (Scanner->parse(tkIDENT)) {
			Expr->Next = new const_expr_t(Scanner->Token.LineNo, (Std$Object_t *)Std$String$new(Scanner->Token.Ident));
		} else {
			Expr->Next = accept_factor(Scanner);
		};
		Expr = new invoke_expr_t(Scanner->Token.LineNo, new const_expr_t(Scanner->Token.LineNo, $DOT), Expr);
		goto start;
	};
	return Expr;
};

static expr_t *accept_term(scanner_t *Scanner) {
	expr_t *Expr = parse_term(Scanner);
	if (Expr) return Expr;
	Scanner->raise_error(Scanner->Token.LineNo, ParseErrorMessageT, "Error: expected term not %s", Tokens[Scanner->NextToken.Type]);
};

static expr_t *accept_expr2(scanner_t *Scanner, int Precedence = 0);

#define INFIX(SYMBOL, PRECEDENCE)\
	if (Scanner->parse(tk ## SYMBOL)) {\
		expr_t *Symbol = new const_expr_t(Scanner->Token.LineNo, $ ## SYMBOL);\
		Term->Next = accept_expr2(Scanner, PRECEDENCE + 1);\
		Term = new invoke_expr_t(Scanner->Token.LineNo, Symbol, Term);\
		goto start;\
	};

static expr_t *parse_expr2(scanner_t *Scanner, int Precedence = 0) {
	if (Scanner->parse(tkNOT)) return new cond_expr_t(Scanner->Token.LineNo,
		accept_expr2(Scanner),
		new back_expr_t(Scanner->Token.LineNo),
		new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil)
	);
	expr_t *Term = parse_term(Scanner);
	if (Term == 0) return 0;

	while (Scanner->parse(tkRASSIGN)) Term = new rassign_expr_t(Scanner->Token.LineNo, Term, accept_term(Scanner));
	if (Scanner->parse(tkASSIGN)) return new assign_expr_t(Scanner->Token.LineNo, Term, accept_expr(Scanner));

	start: switch(Precedence) {
	case 0:
		if (Scanner->parse(tkAND)) {
			Term = new right_expr_t(Scanner->Token.LineNo, Term, accept_expr2(Scanner, 1));
			goto start;
		};
	case 1:
		if (Scanner->parse(tkBACKSLASH)) {
			Term = new left_expr_t(Scanner->Token.LineNo, Term, accept_expr2(Scanner, 2));
			goto start;
		};
	case 2:
		if (Scanner->parse(tkEXACTLY)) {
			Term = new comp_expr_t(Scanner->Token.LineNo, true, Term, accept_expr2(Scanner, 3));
			goto start;
		};
		if (Scanner->parse(tkNOTEXACTLY)) {
			Term = new comp_expr_t(Scanner->Token.LineNo, false, Term, accept_expr2(Scanner, 3));
			goto start;
		};
	case 3:
		INFIX(EQUAL, 3);
		INFIX(NOTEQ, 3);
		INFIX(LESS, 3);
		INFIX(GREATER, 3);
		INFIX(LSEQ, 3);
		INFIX(GREQ, 3);
		INFIX(IN, 3);
		INFIX(SUBTYPE, 3);
		INFIX(INVERSE, 3);
	case 4:
		INFIX(PLUS, 4);
		INFIX(MINUS, 4);
		INFIX(LOGICALOR, 4);
	case 5:
		INFIX(MULTIPLY, 5);
		INFIX(DIVIDE, 5);
		INFIX(MODULO, 5);
		INFIX(LOGICALAND, 5);
	case 6:
		INFIX(POWER, 6);
	case 7:
		INFIX(PARTIAL, 7);
		INFIX(QUERY, 7);
	case 8:
		if (Scanner->parse(tkOR)) {
			expr_t *Last = accept_expr2(Scanner, 9);
			Term->Next = Last;
			while (Scanner->parse(tkOR)) {
				Last->Next = accept_expr2(Scanner, 9);
				Last = Last->Next;
			};
			Term = new sequence_expr_t(Scanner->Token.LineNo, Term);
			goto start;
		};
		if (Scanner->parse(tkEXCLAIM)) {
			expr_t *Last = accept_expr2(Scanner, 9);
			Term->Next = Last;
			while (Scanner->parse(tkEXCLAIM)) {
				Last->Next = accept_expr2(Scanner, 9);
				Last = Last->Next;
			};
			Term = new parallel_expr_t(Scanner->Token.LineNo, Term);
			goto start;
		};
		if (Scanner->parse(tkHASH)) {
			expr_t *Last = accept_expr2(Scanner, 9);
			Term->Next = Last;
			while (Scanner->parse(tkHASH)) {
				Last->Next = accept_expr2(Scanner, 9);
				Last = Last->Next;
			};
			Term = new interleave_expr_t(Scanner->Token.LineNo, Term);
			goto start;
		};
	case 9:
		//INFIX(AT, 9);
		if (Scanner->parse(tkAT)) {
			expr_t *Function = new const_expr_t(Scanner->Token.LineNo, $AT);
			if (Scanner->parse(tkLPAREN)) {
				Term->Next = accept_arguments(Scanner);
				Scanner->accept(tkRPAREN);
			} else {
				Term->Next = accept_factor(Scanner);
			};
			Term = new invoke_expr_t(Scanner->Token.LineNo, Function, Term);
			goto start;
		};
	case 10:
		if (Scanner->parse(tkOF)) {
			Term = new limit_expr_t(Scanner->Token.LineNo, Term, accept_expr2(Scanner, 10));
			goto start;
		};
		if (Scanner->parse(tkSKIP)) {
			Term = new skip_expr_t(Scanner->Token.LineNo, Term, accept_expr2(Scanner, 10));
			goto start;
		};
	};
	return Term;
};

static expr_t *accept_expr2(scanner_t *Scanner, int Precedence) {
	expr_t *Expr = parse_expr2(Scanner, Precedence);
	if (Expr) return Expr;
	Scanner->raise_error(Scanner->Token.LineNo, ParseErrorMessageT, "Error: expected expression not %s", Tokens[Scanner->NextToken.Type]);
};

static expr_t *parse_expr(scanner_t *Scanner) {
	expr_t *Expr = parse_expr2(Scanner);
	if (Expr == 0) return 0;
	while (Scanner->parse(tkTHEN)) {
		expr_t *Expr2 = accept_expr2(Scanner);
		if (Scanner->parse(tkELSE)) {
			Expr = new cond_expr_t(Expr->LineNo, Expr, Expr2, accept_expr(Scanner));
		} else {
			Expr = new cond_expr_t(Expr->LineNo, Expr, Expr2, 0);
		};
	};
	if (Scanner->parse(tkELSE)) return new cond_expr_t(Expr->LineNo, Expr, 0, accept_expr(Scanner));
// 	if (Scanner->parse(tkELSE)) return new cond_expr_t(Expr->LineNo, Expr, new const_expr_t(Scanner->Token.LineNo, Std$Object$Nil), accept_expr(Scanner));
	if (Scanner->parse(tkRECV)) {
		int LineNo = Scanner->Token.LineNo;
		Scanner->accept(tkIDENT);
		const char *Var = Scanner->Token.Ident;
		expr_t *Body;
		if (Scanner->parse(tkAS)) {
			int LineNo = Scanner->Token.LineNo;
			whentype_expr_t *Expr = accept_whentype_expr_case(Scanner, true);
			Expr->Condition = new ident_expr_t(LineNo, Var);
			Expr->LineNo = LineNo;
			Body = Expr;
		} else if (Scanner->parse(tkIS)) {
			int LineNo = Scanner->Token.LineNo;
			when_expr_t *Expr = accept_when_expr_case(Scanner, true);
			Expr->Condition = new ident_expr_t(LineNo, Var);
			Expr->LineNo = LineNo;
			Body = Expr;
		} else {
			Scanner->accept(tkDO);
			Body = accept_expr(Scanner);
		};
		block_expr_t *Block = new block_expr_t;
		Block->Receiver.Var = Var;
		Block->Receiver.Body = Body;
		Block->Final = Expr;
		Expr = Block;
	};
	if (Scanner->parse(tkMUST)) {
		block_expr_t *Block = new block_expr_t;
		Block->Must = accept_expr(Scanner);
		Block->Final = Expr;
		Expr = Block;
	};
	return Expr;
};

expr_t *accept_expr(scanner_t *Scanner) {
	expr_t *Expr = parse_expr(Scanner);
	if (Expr) return Expr;
	Scanner->raise_error(Scanner->Token.LineNo, ParseErrorMessageT, "Error: expected expression not %s", Tokens[Scanner->NextToken.Type]);
};

static module_expr_t *accept_globalstatement(scanner_t *Scanner);

static module_expr_t *accept_globalvar(scanner_t *Scanner) {
	module_expr_t::globalvar_t *Var = new module_expr_t::globalvar_t;
	Var->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Var->Name = Scanner->Token.Ident;
	Var->Exported = Scanner->parse(tkEXCLAIM);
	if (Scanner->parse(tkASSIGN)) {
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, accept_expr(Scanner));
		if (Scanner->parse(tkCOMMA)) {
			module_expr_t *Module = accept_globalvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			module_expr_t *Module = accept_globalstatement(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		};
	} else if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		func_expr_t *Func = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, Func);
		if (Scanner->parse(tkCOMMA)) {
			module_expr_t *Module = accept_globalvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			module_expr_t *Module = accept_globalstatement(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		};
	} else {
		if (Scanner->parse(tkCOMMA)) {
			module_expr_t *Module = accept_globalvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			module_expr_t *Module = accept_globalstatement(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			return Module;
		};
	};
};

static module_expr_t *accept_globaldef(scanner_t *Scanner) {
	module_expr_t::globaldef_t *Def = new module_expr_t::globaldef_t;
	Def->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Def->Name = Scanner->Token.Ident;
	Def->Exported = Scanner->parse(tkEXCLAIM);
	if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		Def->Value = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
	} else {
//		Scanner->accept(tkASSIGN);
		Scanner->parse(tkASSIGN); // Allow <- to be optional
		Def->Value = accept_expr(Scanner);
	};
	module_expr_t *Module;
	if (Scanner->parse(tkCOMMA)) {
		Module = accept_globaldef(Scanner);
	} else {
		Scanner->accept(tkSEMICOLON);
		Module = accept_globalstatement(Scanner);
	};
	Def->Next = Module->Defs;
	Module->Defs = Def;
	return Module;
};

static module_expr_t::globalimp_t::uselist_t *accept_globalimp_uses(scanner_t *Scanner) {
	module_expr_t::globalimp_t::uselist_t *Uses = new module_expr_t::globalimp_t::uselist_t;
	Scanner->accept(tkIDENT);
	Uses->LineNo = Scanner->Token.LineNo;
	Uses->Name = Scanner->Token.Ident;
	if (Scanner->parse(tkCOMMA)) Uses->Next = accept_globalimp_uses(Scanner);
	return Uses;
};

static module_expr_t::globalimp_t::path_t *accept_globalimp_path(scanner_t *Scanner, bool Relative) {
	module_expr_t::globalimp_t::path_t *Path = new module_expr_t::globalimp_t::path_t;
	if (Scanner->parse(tkCONST)) {
		Path->Part = Std$String$flatten(Scanner->Token.Const);
	} else if (Scanner->parse(tkIDENT)) {;
		Path->Part = Scanner->Token.Ident;
	} else if (!Relative) {
		Scanner->accept(tkIDENT);
	} else {
		Path->Part = "";
	};
	if (Scanner->parse(tkDOT)) Path->Next = accept_globalimp_path(Scanner, false);
	return Path;
};

static module_expr_t *accept_globalimp(scanner_t *Scanner) {
	module_expr_t::globalimp_t *Imp = new module_expr_t::globalimp_t;
	Imp->LineNo = Scanner->Token.LineNo;
	Imp->Relative = Scanner->parse(tkDOT);
	Imp->Path = accept_globalimp_path(Scanner, Imp->Relative);
	if (Scanner->parse(tkAS)) {
		Scanner->accept(tkIDENT);
		Imp->Alias = Scanner->Token.Ident;
	} else {
		module_expr_t::globalimp_t::path_t *Path = Imp->Path;
		while (Path->Next) Path = Path->Next;
		Imp->Alias = Path->Part;
	};
	if (Scanner->parse(tkEXCLAIM)) Imp->Exported = true;
	if (Scanner->parse(tkUSE)) Imp->Uses = accept_globalimp_uses(Scanner);
	if (Scanner->parse(tkCOMMA)) {
		module_expr_t *Module = accept_globalimp(Scanner);
		Imp->Next = Module->Imps;
		Module->Imps = Imp;
		return Module;
	} else {
		Scanner->accept(tkSEMICOLON);
		module_expr_t *Module = accept_globalstatement(Scanner);
		Imp->Next = Module->Imps;
		Module->Imps = Imp;
		return Module;
	};
};

static module_expr_t *accept_globalstatement(scanner_t *Scanner) {
	if (Scanner->parse(tkIMP)) return accept_globalimp(Scanner);
	if (Scanner->parse(tkDEF)) return accept_globaldef(Scanner);
	if (Scanner->parse(tkVAR)) return accept_globalvar(Scanner);
	expr_t *Expr = parse_expr(Scanner);
	if (Expr) {
		Scanner->accept(tkSEMICOLON);
		module_expr_t *Module = accept_globalstatement(Scanner);
		Expr->Next = Module->Body;
		Module->Body = Expr;
		return Module;
	};
	return new module_expr_t;
};

module_expr_t *accept_module(scanner_t *Scanner, Riva$Module$provider_t *Provider) {
	bool Nested = (Provider == 0);
	const char *Name;
	if (Nested) {
		Riva$Module$t *Module = Sys$Module$new(0);
		Provider = Riva$Module$get_default_provider(Module);
		Name = "<anonymous>";
	} else {
		Scanner->accept(tkMOD);
		Scanner->accept(tkIDENT);
		Name = Scanner->Token.Ident;
		Scanner->accept(tkSEMICOLON);
	};

	int LineNo = Scanner->Token.LineNo;
	module_expr_t *Expr = accept_globalstatement(Scanner);
	Expr->LineNo = LineNo;
	Expr->Provider = Provider;
	Expr->Name = Name;

	Scanner->accept(tkEND);
	if (!Nested) {
		Scanner->accept(tkIDENT);
		Scanner->accept(tkDOT);
	};

	return Expr;
};

module_expr_t *parse_module(scanner_t *Scanner, Riva$Module$provider_t *Provider) {
	const char *Name;
	if (Scanner->parse(tkMOD)) {
		Scanner->accept(tkIDENT);
		Name = Scanner->Token.Ident;
		Scanner->accept(tkSEMICOLON);

		int LineNo = Scanner->Token.LineNo;
		module_expr_t *Expr = accept_globalstatement(Scanner);
		Expr->LineNo = LineNo;
		Expr->Provider = Provider;
		Expr->Name = Name;

		Scanner->accept(tkEND);
		Scanner->accept(tkIDENT);
		Scanner->accept(tkDOT);
		return Expr;
	} else {
		return 0;
	};
};

static command_expr_t *accept_commandvar(scanner_t *Scanner) {
	command_expr_t::globalvar_t *Var = new command_expr_t::globalvar_t;
	Var->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Var->Name = Scanner->Token.Ident;
	if (Scanner->parse(tkASSIGN)) {
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, accept_expr(Scanner));
		if (Scanner->parse(tkCOMMA)) {
			command_expr_t *Module = accept_commandvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			command_expr_t *Module = new command_expr_t;
			Module->Vars = Var;
			Module->Body = Assign;
			return Module;
		};
	} else if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		func_expr_t *Func = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
		ident_expr_t *Ident = new ident_expr_t(Scanner->Token.LineNo, Var->Name);
		assign_expr_t *Assign = new assign_expr_t(Scanner->Token.LineNo, Ident, Func);
		if (Scanner->parse(tkCOMMA)) {
			command_expr_t *Module = accept_commandvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			Assign->Next = Module->Body;
			Module->Body = Assign;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			command_expr_t *Module = new command_expr_t;
			Module->Vars = Var;
			Module->Body = Assign;
			return Module;
		};
	} else {
		if (Scanner->parse(tkCOMMA)) {
			command_expr_t *Module = accept_commandvar(Scanner);
			Var->Next = Module->Vars;
			Module->Vars = Var;
			return Module;
		} else {
			Scanner->accept(tkSEMICOLON);
			command_expr_t *Module = new command_expr_t;
			Module->Vars = Var;
			return Module;
		};
	};
};

static command_expr_t *accept_commanddef(scanner_t *Scanner) {
	command_expr_t::globaldef_t *Def = new command_expr_t::globaldef_t;
	Def->LineNo = Scanner->Token.LineNo;
	Scanner->accept(tkIDENT);
	Def->Name = Scanner->Token.Ident;
	if (Scanner->parse(tkLPAREN)) {
		int LineNo = Scanner->Token.LineNo;
		func_expr_t::parameter_t *Parameters= accept_typed_parameters(Scanner).Parameters;
		Scanner->accept(tkRPAREN);
		Def->Value = new func_expr_t(LineNo, Parameters, accept_expr(Scanner));
	} else {
//		Scanner->accept(tkASSIGN);
		Scanner->parse(tkASSIGN);
		Def->Value = accept_expr(Scanner);
	};
	if (Scanner->parse(tkCOMMA)) {
		command_expr_t *Module = accept_commanddef(Scanner);
		Def->Next = Module->Defs;
		Module->Defs = Def;
		return Module;
	} else {
		Scanner->accept(tkSEMICOLON);
		command_expr_t *Module = new command_expr_t;
		Module->Defs = Def;
		return Module;
	};
};

static command_expr_t::globalimp_t::uselist_t *accept_commandimp_uses(scanner_t *Scanner) {
	command_expr_t::globalimp_t::uselist_t *Uses = new command_expr_t::globalimp_t::uselist_t;
	Scanner->accept(tkIDENT);
	Uses->LineNo = Scanner->Token.LineNo;
	Uses->Name = Scanner->Token.Ident;
	if (Scanner->parse(tkCOMMA)) Uses->Next = accept_commandimp_uses(Scanner);
	return Uses;
};

static command_expr_t::globalimp_t::path_t *accept_commandimp_path(scanner_t *Scanner, bool Relative) {
	command_expr_t::globalimp_t::path_t *Path = new command_expr_t::globalimp_t::path_t;
	if (Scanner->parse(tkCONST)) {
		Path->Part = Std$String$flatten(Scanner->Token.Const);
	} else if (Scanner->parse(tkIDENT)) {
		Path->Part = Scanner->Token.Ident;
	} else if (!Relative) {
		Scanner->accept(tkIDENT);
	} else {
		Path->Part = "";
	}
	if (Scanner->parse(tkDOT)) Path->Next = accept_commandimp_path(Scanner, false);
	return Path;
};

static command_expr_t *accept_commandimp(scanner_t *Scanner) {
	command_expr_t::globalimp_t *Imp = new command_expr_t::globalimp_t;
	Imp->LineNo = Scanner->Token.LineNo;
	Imp->Relative = Scanner->parse(tkDOT);
	Imp->Path = accept_commandimp_path(Scanner, Imp->Relative);
	if (Scanner->parse(tkAS)) {
		Scanner->accept(tkIDENT);
		Imp->Alias = Scanner->Token.Ident;
	} else {
		command_expr_t::globalimp_t::path_t *Path = Imp->Path;
		while (Path->Next) Path = Path->Next;
		Imp->Alias = Path->Part;
	};
	if (Scanner->parse(tkUSE)) Imp->Uses = accept_commandimp_uses(Scanner);
	if (Scanner->parse(tkCOMMA)) {
		command_expr_t *Module = accept_commandimp(Scanner);
		Imp->Next = Module->Imps;
		Module->Imps = Imp;
		return Module;
	} else {
		Scanner->accept(tkSEMICOLON);
		command_expr_t *Module = new command_expr_t;
		Module->Imps = Imp;
		return Module;
	};
};

command_expr_t *accept_command(scanner_t *Scanner) {
	if (Scanner->parse(tkIMP)) return accept_commandimp(Scanner);
	if (Scanner->parse(tkDEF)) return accept_commanddef(Scanner);
	if (Scanner->parse(tkVAR)) return accept_commandvar(Scanner);
	command_expr_t *Module = new command_expr_t;
	Module->Body = accept_expr(Scanner);
	Module->LineNo = Module->Body->LineNo;
	Scanner->accept(tkSEMICOLON);
	return Module;
};

