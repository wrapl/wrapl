#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <Riva/Memory.h>

#define TRACE {printf("%s:%d\n", __FILE__, __LINE__);}
#define ASSEMBLER_LISTING

#include <stdint.h>
#include <stdio.h>
#include <Sys/Module.h>
#include "stringtable.h"
#include "debugger.h"

struct assembler_t;

struct closure_t {
	const Std$Type$t *Type;
	void *Entry;
	void *Values[];
};

struct code_t {
	const Std$Type$t *Type;
	void *Entry;
	void *Frame;
};

struct upvalue_t {
	upvalue_t *Next;
	uint32_t Index;
};

struct uptemp_t {
	uptemp_t *Next;
	uint32_t Index;
};

struct load_inst_t;
struct label_t;

struct parameter_t {
	parameter_t *Next;
	bool Indirect;
	bool Default;
};

struct frame_t {
	uint32_t NoOfLocals;
	uint32_t NoOfTemps;
	uint32_t NoOfScopes;
	uint32_t NoOfUpValues;
	uint32_t NoOfUpTemps;
	uint32_t NoOfTraps;
	uint32_t NoOfParams;
	bool Variadic;
	upvalue_t *UpValues;
	uptemp_t *UpTemps;
	parameter_t *Params;
	label_t *Resend;
	debug_function_t *DebugInfo;
};

struct operand_t {
	operand_t *Next;
	enum {CNST = 0, GVAR = 1, LVAR, LREF, TVAR, TREF, TREG, CLSR, FUTR, REGR} Type;
	union {
		Std$Object$t *Value;
		Std$Object$t **Address;
		struct {uint32_t Index;};
		struct {void *Entry; upvalue_t *UpValues; uptemp_t *UpTemps;};
		struct future_t *Future;
	};
};

extern operand_t Register[];

struct inst_t {
	inst_t *Next;
	//operand_t *Ecx, *Edx;
	int LineNo;
	bool IsPotentialBreakpoint;

#ifdef ASSEMBLER_LISTING
	virtual void list() {printf("\tunknown\n");};
#endif
	virtual void add_sources() {};
	virtual void add_source(load_inst_t *Load) {};
	virtual void append_links(label_t *Start) {};
	virtual int noof_consts() {return 0;};
	virtual const void **get_consts(const void **Ptr) {return Ptr;};
	virtual void encode(assembler_t *Assembler) {};
	virtual void find_breakpoints();
	virtual bool can_suspend() {return false;};
	virtual bool needs_link() {return true;};
	virtual insttype_t type() {return INSTTYPE_DEFAULT;};
};

struct load_inst_t : inst_t {
	enum {LOAD_NONE, LOAD_VAL, LOAD_REF, LOAD_BOTH, LOAD_ARG} Type;
	operand_t *Operand;

#ifdef ASSEMBLER_LISTING
	void list();
#endif

	void add_sources() {Next->add_source(this);};
	int noof_consts();
	const void **get_consts(const void **);
	void encode(assembler_t *Assembler);

	void load_val();
	void load_ref();
	void load_both();
	void load_arg();
};

#include <stdio.h>

struct select_integer_case_t {
	select_integer_case_t *Next;
	label_t *Body;
	int32_t Min, Max;
};

struct select_string_case_t {
	select_string_case_t *Next;
	label_t *Body;
	const char *Key;
	uint32_t Length;
};

struct select_real_case_t {
	select_real_case_t *Next;
	label_t *Body;
	double Min, Max;
};

struct select_object_case_t {
	select_object_case_t *Next;
	label_t *Body;
	Std$Object$t *Object;
};

struct select_type_case_t {
	select_type_case_t *Next;
	label_t *Body;
	Std$Object$t *Type;
};

struct label_t : inst_t {
	label_t *Link;
	inst_t *Tail;
	bool Done, Referenced;
	int Label;

	label_t *final();

#ifdef ASSEMBLER_LISTING
	void list();
#endif

	void append(inst_t *Inst);

	void link(uint32_t LineNo, label_t *U) {Link = U;};
	void test_param(uint32_t LineNo, bool Reference, uint32_t Index, label_t *Continue);
	void default_param(uint32_t LineNo, bool Reference, uint32_t Index);
	void local(uint32_t LineNo, uint32_t Index);
	void store_local(uint32_t LineNo, uint32_t Index);
	void init_trap(uint32_t LineNo, uint32_t Trap, label_t *Failure);
	void push_trap(uint32_t LineNo, uint32_t Trap, label_t *Failure, uint32_t Temp);
	void store_trap(uint32_t LineNo, uint32_t Temp, label_t *Failure);
	void store_link(uint32_t LineNo, uint32_t Temp, label_t *Target);
	void jump_link(uint32_t LineNo, uint32_t Temp);
	void resume(uint32_t LineNo);
	void load(uint32_t LineNo, operand_t *Operand);
	void store_reg(uint32_t LineNo, operand_t *Operand);
	void store_val(uint32_t LineNo, operand_t *Operand);
	void store_ref(uint32_t LineNo, operand_t *Operand);
	void store_con(uint32_t LineNo, operand_t *Operand, Std$Object$t *Value);
	void flush(uint32_t LineNo);
	void store_arg(uint32_t LineNo, uint32_t Index, operand_t *Operand);
	void fixup_arg(uint32_t LineNo, uint32_t Index, operand_t *Operand);
	void invoke(uint32_t LineNo, uint32_t Trap, uint32_t Args, uint32_t Count, label_t *Fixup);
	void back(uint32_t LineNo, uint32_t Trap);
	void fail(uint32_t LineNo, bool CodeBlock = false);
	void ret(uint32_t LineNo);
	void susp(uint32_t LineNo, bool CodeBlock);
	void recv(uint32_t LineNo, label_t *Handler);
	void send(uint32_t LineNo);
	void resend(uint32_t LineNo);
	void comp(uint32_t LineNo, bool Equal, operand_t *Operand, label_t *Failure);
	void limit(uint32_t LineNo, uint32_t Trap, uint32_t Temp);
	void test_limit(uint32_t LineNo, uint32_t Temp, label_t *Failure);
	void skip(uint32_t LineNo, uint32_t Temp);
	void test_skip(uint32_t LineNo, uint32_t Trap, uint32_t Temp);
	void test_unique(uint32_t LineNo, uint32_t Trap, uint32_t Temp);
	void select_integer(uint32_t LineNo, select_integer_case_t *Cases, label_t *Default);
	void select_string(uint32_t LineNo, select_string_case_t *Cases, label_t *Default);
	void select_real(uint32_t LineNo, select_real_case_t *Cases, label_t *Default);
	void select_object(uint32_t LineNo, select_object_case_t *Cases, label_t *Default);
	void select_type(uint32_t LineNo, select_type_case_t *Cases, label_t *Default);
	void type_of(uint32_t LineNo);
	void value_of(uint32_t LineNo);
	void new_list(uint32_t LineNo, uint32_t Index);
	void store_list(uint32_t LineNo, uint32_t Index);
	void new_table(uint32_t LineNo, uint32_t Index);
	void store_table(uint32_t LineNo, uint32_t Index);
	void store_table2(uint32_t LineNo, uint32_t Index, uint32_t Key, int Reverse);
	void new_count(uint32_t LineNo, uint32_t Index);
	void inc_count(uint32_t LineNo, uint32_t Index);
	void load_code(uint32_t LineNo, label_t *Code);

	operand_t *assemble(const frame_t *Function, const char *StrInfo, int IntInfo, operand_t *Closure = 0);

	void add_source(load_inst_t *Load);
	void encode(assembler_t *Assembler);
};

#endif
