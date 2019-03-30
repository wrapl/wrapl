#include "assembler.h"
#include "missing.h"
#include "system.h"
#include <Riva/Log.h>
#include <Riva/Config.h>
#include <Std.h>
#include <Agg.h>
#include <string.h>
#include <Riva/Debug.h>
#include <stdlib.h>

#if 0
#define DEBUG printf("%s.%d\n", __FILE__, __LINE__);
#else
#define DEBUG
#endif

struct bstate_t {
	void *Run;
	Std$Function$state_t *Chain;
	void *Resume;
	Std$Object_t *Val;
	Std$Object_t **Ref;
	void *Code;
	void *Handler;
};

struct variadic_t {
	Std$Type_t *Type;
	Std$Integer_smallt Length;
	Std$Function_argument Args[];
};

struct trap_t {
	void *State;
	void *Run;
};

operand_t Register[] = {{
	0, operand_t::REGR
}};

#ifdef ASSEMBLER_LISTING

#include <udis86.h>

static const char *listop(operand_t *Operand) {DEBUG
	char *List;
	switch (Operand->Type) {
	case operand_t::CNST: {
		const char *Module, *Import;
		if (Riva$Module$lookup(Operand->Value, &Module, &Import)) {
			asprintf(&List, "CNST:%s.%s", Module, Import);
		} else {
			asprintf(&List, "CNST:%x", Operand->Value);
		};
		return List;
	};
	case operand_t::GVAR: asprintf(&List, "GVAR:%x", Operand->Address); return List;
	case operand_t::LVAR: asprintf(&List, "LVAR:%d", Operand->Index); return List;
	case operand_t::LREF: asprintf(&List, "LREF:%d", Operand->Index); return List;
	case operand_t::TVAR: asprintf(&List, "TVAR:%d", Operand->Index); return List;
	case operand_t::TREF: asprintf(&List, "TREF:%d", Operand->Index); return List;
	case operand_t::TREG: asprintf(&List, "TREG:%d", Operand->Index); return List;
	case operand_t::CLSR: asprintf(&List, "CLSR:%x", Operand->Value); return List;
	case operand_t::FUTR: asprintf(&List, "FUTR"); return List;
	case operand_t::REGR: asprintf(&List, "REGR"); return List;
	};
};

#endif

void inst_t::find_breakpoints() {DEBUG
	if (type() != INSTTYPE_DEFAULT) IsPotentialBreakpoint = true;
	if (Next && (Next->LineNo != LineNo)) Next->IsPotentialBreakpoint = true;
};

struct link_inst_t : inst_t {
	label_t *Link;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: link %x\n", LineNo, Link->final());
	};
#endif
	void add_source(load_inst_t *Load) {Link->add_source(Load);};
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
};

static void use_label(label_t *Start, label_t *Next, bool Follow) {DEBUG
	Next = Next->final();
	if (!Follow) Next->Referenced = true;
	if (Next->Done) {DEBUG
		if (Follow) {DEBUG
			link_inst_t *Inst = new link_inst_t;
			Inst->Link = Next;
			Inst->LineNo = Next->LineNo;
			Next->Referenced = true;
			Start->append(Inst);
		};
	} else {DEBUG
		Next->Done = true;
		if (Start->Tail) {
			Start->Tail->Next = Next;
		} else {
			Start->Next = Next;
		};
		inst_t *Tail = Start->Tail = Next->Tail;
		label_t *Link = Next->Link;
		if (!Link && Tail->needs_link()) {
			printf("Internal error: instruction can not terminate block.\n");
			exit(1);
		};
		if (Link) {DEBUG
			use_label(Start, Link, true);
		};
		inst_t *Inst = Next;
		do {
			Inst = Inst->Next;
			Inst->append_links(Start);
		} while (Inst != Tail);
		DEBUG
	};
};

struct test_param_inst_t : inst_t {
	uint32_t Index;
	label_t *Continue;
	bool Reference;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: test_param [%s] %d -> %x\n", LineNo, Reference ? "REF" : "VAL", Index, Continue->final());
	};
#endif
	void append_links(label_t *Start) {
		use_label(Start, Continue, false);
	};
	void encode(assembler_t *Assembler);
};

void label_t::test_param(uint32_t LineNo, bool Reference, uint32_t Index, label_t *Continue) {DEBUG
	test_param_inst_t *Inst = new test_param_inst_t;
	Inst->Index = Index;
	Inst->Reference = Reference;
	Inst->Continue = Continue;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct default_param_inst_t : inst_t {
	uint32_t Index;
	bool Reference;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: default_param [%s] %d\n", LineNo, Reference ? "REF" : "VAL", Index);
	};
#endif
	void add_source(load_inst_t *Load) {
		if (Reference) Load->load_ref(); else Load->load_val();
	};
	void encode(assembler_t *Assembler);
};

void label_t::default_param(uint32_t LineNo, bool Reference, uint32_t Index) {DEBUG
	default_param_inst_t *Inst = new default_param_inst_t;
	Inst->Index = Index;
	Inst->Reference = Reference;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct local_inst_t : inst_t {
	uint32_t Index;
	const char *Name;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: local %d\n", LineNo, Index);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::local(uint32_t LineNo, uint32_t Index) {DEBUG
	local_inst_t *Inst = new local_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_local_inst_t : inst_t {
	uint32_t Index;
	void *Debug;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_local %d\n", LineNo, Index);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::store_local(uint32_t LineNo, uint32_t Index) {DEBUG
	store_local_inst_t *Inst = new store_local_inst_t;
	Inst->Index = Index;
	//Inst->Debug = Debug;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct init_trap_inst_t : inst_t {
	uint32_t Trap;
	label_t *Failure;
	//state_t *State;

	int noof_consts() {return 1;};
	const void **get_consts(const void **);


	void append_links(label_t *Start) {
		use_label(Start, Failure, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Failure->LineNo != LineNo) Failure->IsPotentialBreakpoint = true;
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: zero %d <- %x\n", LineNo, Trap, Failure->final());
	};
#endif
	void encode(assembler_t *Assembler);
};

const void **init_trap_inst_t::get_consts(const void **Ptr) {DEBUG
	//Ptr[0] = State;
	//return Ptr + 1;
	return Ptr;
};

void label_t::init_trap(uint32_t LineNo, uint32_t Trap, label_t *Failure) {DEBUG
	init_trap_inst_t *Inst = new init_trap_inst_t;
	Inst->Trap = Trap;
	
	//TRAP TEST!!!
	//label_t *Failure0 = new label_t();
	//Failure0->resume(LineNo);
	//Failure0->link(LineNo, Failure);
	//Inst->Failure = Failure0;
	Inst->Failure = Failure;
	
	//Inst->State = new state_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct push_trap_inst_t : inst_t {
	uint32_t Trap;
	label_t *Failure;
	uint32_t Temp;

	void append_links(label_t *Start) {
		use_label(Start, Failure, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Failure->LineNo != LineNo) Failure->IsPotentialBreakpoint = true;
	};
	void add_source(load_inst_t *Load) {Next->add_source(Load);};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: push_trap %d <- [%d, %x]\n", LineNo, Trap, Temp, Failure);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::push_trap(uint32_t LineNo, uint32_t Trap, label_t *Failure, uint32_t Temp) {DEBUG
	push_trap_inst_t *Inst = new push_trap_inst_t;
	Inst->Trap = Trap;
	Inst->Temp = Temp;
	
	label_t *Failure0 = new label_t();
	Failure0->resume(LineNo);
	Failure0->link(LineNo, Failure);
	Inst->Failure = Failure0;
	
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_trap_inst_t : inst_t {
	label_t *Failure;
	uint32_t Temp;

	void append_links(label_t *Start) {
		use_label(Start, Failure, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Failure->LineNo != LineNo) Failure->IsPotentialBreakpoint = true;
	};
	void add_source(load_inst_t *Load) {Next->add_source(Load);};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_trap [%d] <- %x\n", LineNo, Temp, Failure);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::store_trap(uint32_t LineNo, uint32_t Temp, label_t *Failure) {DEBUG
	store_trap_inst_t *Inst = new store_trap_inst_t;
	Inst->Temp = Temp;
	
	label_t *Failure0 = new label_t();
	Failure0->resume(LineNo);
	Failure0->link(LineNo, Failure);
	Inst->Failure = Failure0;

	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct resume_inst_t : inst_t {
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: resume\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::resume(uint32_t LineNo) {DEBUG
	resume_inst_t *Inst = new resume_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

void load_inst_t::load_val() {DEBUG
	switch (Type) {
	case LOAD_NONE: Type = LOAD_VAL; return;
	case LOAD_VAL: return;
	case LOAD_REF: Type = LOAD_BOTH; return;
	case LOAD_BOTH: return;
	case LOAD_ARG: Type = LOAD_BOTH; return;
	};
};

void load_inst_t::load_ref() {DEBUG
	switch (Type) {
	case LOAD_NONE: Type = LOAD_REF; return;
	case LOAD_VAL: Type = LOAD_BOTH; return;
	case LOAD_REF: return;
	case LOAD_BOTH: return;
	case LOAD_ARG: return;
	};
};

void load_inst_t::load_both() {DEBUG
	Type = LOAD_BOTH;
};

void load_inst_t::load_arg() {DEBUG
	switch (Type) {
	case LOAD_NONE: Type = LOAD_ARG; return;
	case LOAD_VAL: Type = LOAD_BOTH; return;
	case LOAD_REF: Type = LOAD_ARG; return;
	case LOAD_BOTH: return;
	case LOAD_ARG: return;
	};
};

int load_inst_t::noof_consts() {DEBUG
	switch (Operand->Type) {
	case operand_t::CNST: return 1;
	case operand_t::GVAR: return 1;
	case operand_t::CLSR: return 1;
	default: return 0;
	};
};

const void **load_inst_t::get_consts(const void **Ptr) {DEBUG
	switch (Operand->Type) {
	case operand_t::CNST:
		Ptr[0] = Operand->Value;
		return Ptr + 1;
	case operand_t::GVAR:
		Ptr[0] = Operand->Address;
		return Ptr + 1;
	case operand_t::CLSR:
		Ptr[0] = Operand->Entry;
		return Ptr + 1;
	default: return Ptr;
	};
};

#ifdef ASSEMBLER_LISTING
void load_inst_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	static const char *Types[] = {
		"_none", "_val", "_ref", "_both", "_arg"
	};
	printf("%4d: load%s %s\n", LineNo, Types[Type], listop(Operand));
};
#endif

void label_t::load(uint32_t LineNo, operand_t *Operand) {DEBUG
	if (Operand == Register) return;
	load_inst_t *Inst = new load_inst_t;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_reg_inst_t : inst_t {
	operand_t *Operand;
	void add_source(load_inst_t *Load) {
		Load->load_both();
		Next->add_source(Load);
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_reg %s\n", LineNo, listop(Operand));
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::store_reg(uint32_t LineNo, operand_t *Operand) {DEBUG
	store_reg_inst_t *Inst = new store_reg_inst_t;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_con_inst_t : load_inst_t {
	Std$Object_t *Value;
	void add_source(load_inst_t *Load) {
		if (Operand == Register) Load->load_ref();
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		static const char *Types[] = {
			"_none", "_val", "_ref", "_both", "_arg"
		};
		printf("%4d: store_con%s %s <- %x\n", LineNo, Types[Type], listop(Operand), Value);
	};
#endif
	int noof_consts() {return 1;};
	const void **get_consts(const void **Ptr) {Ptr[0] = Value; return Ptr + 1;};
	void encode(assembler_t *Assembler);
};

void label_t::store_con(uint32_t LineNo, operand_t *Operand, Std$Object_t *Value) {DEBUG
	store_con_inst_t *Inst = new store_con_inst_t;
	Inst->Operand = Operand;
	Inst->Value = Value;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_val_inst_t : inst_t {
	operand_t *Operand;
	void add_source(load_inst_t *Load) {
		Load->load_val();
		Next->add_source(Load);
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_val %s\n", LineNo, listop(Operand));
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::store_val(uint32_t LineNo, operand_t *Operand) {DEBUG
	store_val_inst_t *Inst = new store_val_inst_t;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_ref_inst_t : inst_t {
	operand_t *Operand;
	void add_source(load_inst_t *Load) {
		Load->load_ref();
		Next->add_source(Load);
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_ref %s\n", LineNo, listop(Operand));
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::store_ref(uint32_t LineNo, operand_t *Operand) {DEBUG
	store_ref_inst_t *Inst = new store_ref_inst_t;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct flush_inst_t : inst_t {
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: flush\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler) {};
};

void label_t::flush(uint32_t LineNo) {DEBUG
	flush_inst_t *Inst = new flush_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_arg_inst_t : inst_t {
	uint32_t Index;
	operand_t *Operand;
	void add_source(load_inst_t *Load) {
		if (Operand == Register) Load->load_arg();
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_arg %d <- %s\n", LineNo, Index, listop(Operand));
	};
#endif
	int noof_consts();
	const void **get_consts(const void **);
	void encode(assembler_t *Assembler);
};

int store_arg_inst_t::noof_consts() {
	switch (Operand->Type) {
	case operand_t::CNST: return 1;
	case operand_t::GVAR: return 1;
	case operand_t::CLSR: return 1;
	default: return 0;
	};
};

const void **store_arg_inst_t::get_consts(const void **Ptr) {DEBUG
	switch (Operand->Type) {
	case operand_t::CNST:
		Ptr[0] = Operand->Value;
		return Ptr + 1;
	case operand_t::GVAR:
		Ptr[0] = Operand->Address;
		return Ptr + 1;
	case operand_t::CLSR:
		Ptr[0] = Operand->Entry;
		return Ptr + 1;
	default: return Ptr;
	};
};

void label_t::store_arg(uint32_t LineNo, uint32_t Index, operand_t *Operand) {DEBUG
	store_arg_inst_t *Inst = new store_arg_inst_t;
	Inst->Index = Index;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct fixup_arg_inst_t : inst_t {
	uint32_t Index;
	operand_t *Operand;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: fixup_arg %d <- %s\n", LineNo, Index, listop(Operand));
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::fixup_arg(uint32_t LineNo, uint32_t Index, operand_t *Operand) {DEBUG
	fixup_arg_inst_t *Inst = new fixup_arg_inst_t;
	Inst->Index = Index;
	Inst->Operand = Operand;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct invoke_inst_t : inst_t {
	uint32_t Trap, Args, Count;
	//label_t *Fixup;
	void add_source(load_inst_t *Load) {Load->load_val();};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: invoke %d, %d, %d\n", LineNo, Trap, Args, Count);
	};
#endif
	void encode(assembler_t *Assembler);
	void append_links(label_t *Start) {
		//use_label(Start, Fixup, false);
	};
};

void label_t::invoke(uint32_t LineNo, uint32_t Trap, uint32_t Args, uint32_t Count, label_t *Fixup) {DEBUG
	invoke_inst_t *Inst = new invoke_inst_t;
	Inst->Trap = Trap;
	Inst->Args = Args;
	Inst->Count = Count;
	//Inst->Fixup = Fixup;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct back_inst_t : inst_t {
	uint32_t Trap;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: back %d\n", LineNo, Trap);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
};

void label_t::back(uint32_t LineNo, uint32_t Trap) {DEBUG
	back_inst_t *Inst = new back_inst_t;
	Inst->Trap = Trap;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct fail_inst_t : inst_t {
	bool CodeBlock;

#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: fail\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
	insttype_t type() {return INSTTYPE_FAIL;};
};

void label_t::fail(uint32_t LineNo, bool CodeBlock) {DEBUG
	fail_inst_t *Inst = new fail_inst_t;
	Inst->LineNo = LineNo;
	Inst->CodeBlock = CodeBlock;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct ret_inst_t : inst_t {
	void add_source(load_inst_t *Load) {
		Load->load_both();
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: ret\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
	insttype_t type() {return INSTTYPE_RETURN;};
};

void label_t::ret(uint32_t LineNo) {DEBUG
	ret_inst_t *Inst = new ret_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct susp_inst_t : inst_t {
	bool CodeBlock;

	void add_source(load_inst_t *Load) {
		Load->load_both();
		Next->add_source(Load);
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: susp\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
	bool can_suspend() {return true;};
	insttype_t type() {return INSTTYPE_SUSPEND;};
};

void label_t::susp(uint32_t LineNo, bool CodeBlock) {DEBUG
	susp_inst_t *Inst = new susp_inst_t;
	Inst->LineNo = LineNo;
	Inst->CodeBlock = CodeBlock;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct recv_inst_t : inst_t {
	label_t *Handler;

	void append_links(label_t *Start) {
		if (Handler) {
			use_label(Start, Handler, false);
		};
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Handler) Handler->IsPotentialBreakpoint = true;
	};
	//void add_source(load_inst_t *Load) {Next->add_source(Load);};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: recv %x\n", LineNo, Handler ? Handler->final() : 0);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::recv(uint32_t LineNo, label_t *Handler) {DEBUG
	recv_inst_t *Inst = new recv_inst_t;
	Inst->Handler = Handler;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct send_inst_t : inst_t {
	void add_source(load_inst_t *Load) {Load->load_val();};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: send\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
	insttype_t type() {return INSTTYPE_SEND;};
};

void label_t::send(uint32_t LineNo) {DEBUG
	send_inst_t *Inst = new send_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct resend_inst_t : inst_t {
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: resend\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
	insttype_t type() {return INSTTYPE_SEND;};
};

void label_t::resend(uint32_t LineNo) {DEBUG
	resend_inst_t *Inst = new resend_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_link_inst_t : inst_t {
	uint32_t Temp;
	label_t *Target;
	void add_source(load_inst_t *Load) {Next->add_source(Load);};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_link %d, %x\n", LineNo, Temp, Target);
	};
#endif
	void append_links(label_t *Start) {
		use_label(Start, Target, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Target->LineNo != LineNo) Target->IsPotentialBreakpoint = true;
	};
	void encode(assembler_t *Assembler);
};

void label_t::store_link(uint32_t LineNo, uint32_t Temp, label_t *Target) {DEBUG
	store_link_inst_t *Inst = new store_link_inst_t;
	Inst->Temp = Temp;
	Inst->Target = Target;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct jump_link_inst_t : inst_t {
	uint32_t Temp;
	void add_source(load_inst_t *Load) {Load->load_both();};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: jump_link %d\n", LineNo, Temp);
	};
#endif
	void encode(assembler_t *Assembler);
	bool needs_link() {return false;};
};

void label_t::jump_link(uint32_t LineNo, uint32_t Temp) {DEBUG
	jump_link_inst_t *Inst = new jump_link_inst_t;
	Inst->Temp = Temp;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct limit_inst_t : inst_t {
	uint32_t Trap, Temp;
	void add_source(load_inst_t *Load) {Load->load_val();};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: limit %d, %d\n", LineNo, Trap, Temp);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::limit(uint32_t LineNo, uint32_t Trap, uint32_t Temp) {DEBUG
	limit_inst_t *Inst = new limit_inst_t;
	Inst->Trap = Trap;
	Inst->Temp = Temp;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct test_limit_inst_t : inst_t {
	uint32_t Temp;
	label_t *Target;
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start) {
		use_label(Start, Target, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Target->LineNo != LineNo) Target->IsPotentialBreakpoint = true;
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: test_limit %d, %x\n", LineNo, Temp, Target);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::test_limit(uint32_t LineNo, uint32_t Temp, label_t *Target) {DEBUG
	test_limit_inst_t *Inst = new test_limit_inst_t;
	Inst->Target = Target;
	Inst->Temp = Temp;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct skip_inst_t : inst_t {
	uint32_t Temp;
	void add_source(load_inst_t *Load) {Load->load_val();};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: skip %d\n", LineNo, Temp);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::skip(uint32_t LineNo, uint32_t Temp) {DEBUG
	skip_inst_t *Inst = new skip_inst_t;
	Inst->Temp = Temp;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct test_skip_inst_t : inst_t {
	uint32_t Temp;
	uint32_t Trap;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: test_skip %d, %d\n", LineNo, Temp, Trap);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::test_skip(uint32_t LineNo, uint32_t Trap, uint32_t Temp) {DEBUG
	test_skip_inst_t *Inst = new test_skip_inst_t;
	Inst->Trap = Trap;
	Inst->Temp = Temp;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct comp_inst_t : inst_t {
	operand_t *Operand;
	label_t *Failure;
	bool Equal;
	void append_links(label_t *Start) {
		use_label(Start, Failure, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Failure->LineNo != LineNo) Failure->IsPotentialBreakpoint = true;
	};
	void add_source(load_inst_t *Load) {
		Load->load_val();
		Next->add_source(Load);
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: comp %s %s // %x\n", LineNo, Equal ? "==" : "~==", listop(Operand), Failure);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::comp(uint32_t LineNo, bool Equal, operand_t *Operand, label_t *Failure) {DEBUG
	comp_inst_t *Inst = new comp_inst_t;
	Inst->Operand = Operand;
	Inst->Failure = Failure;
	Inst->Equal = Equal;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct select_integer_inst_t : inst_t {
	select_integer_case_t *Cases;
	label_t *Default;

#ifdef ASSEMBLER_LISTING
	void list();
#endif
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start);
	void encode(assembler_t *Assembler);
	void find_breakpoints();
	bool needs_link() {return false;};
};

#ifdef ASSEMBLER_LISTING
void select_integer_inst_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	printf("%4d: select_integer\n", LineNo);
	for (select_integer_case_t *Case = Cases; Case; Case = Case->Next) {
		printf("\t\t%x .. %x => %x\n", Case->Min, Case->Max, Case->Body->final());
	};
	printf("\t\telse %x\n", Default->final());
};
#endif

void select_integer_inst_t::append_links(label_t *Start) {DEBUG
	for (select_integer_case_t *Case = Cases; Case; Case = Case->Next) use_label(Start, Case->Body, false);
	use_label(Start, Default, false);
};

void select_integer_inst_t::find_breakpoints() {DEBUG
	for (select_integer_case_t *Case = Cases; Case; Case = Case->Next) Case->Body->IsPotentialBreakpoint = true;
	Default->IsPotentialBreakpoint = true;
};

void label_t::select_integer(uint32_t LineNo, select_integer_case_t *Cases, label_t *Default) {DEBUG
	select_integer_inst_t *Inst = new select_integer_inst_t;
	Inst->Cases = Cases;
	Inst->Default = Default;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct select_string_inst_t : inst_t {
	select_string_case_t *Cases;
	label_t *Default;

#ifdef ASSEMBLER_LISTING
	void list();
#endif
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start);
	int noof_consts();
	const void **get_consts(const void **Ptr);
	void encode(assembler_t *Assembler);
	void find_breakpoints();
	bool needs_link() {return false;};
};

#ifdef ASSEMBLER_LISTING
void select_string_inst_t::list() {DEBUG
	if (IsPotentialBreakpoint) printf("*");
	printf("%4d: select_string\n", LineNo);
	for (select_string_case_t *Case = Cases; Case; Case = Case->Next) {
		printf("\t\t%.*s => %x\n", Case->Length, Case->Key, Case->Body->final());
	};
	printf("\t\telse %x\n", Default->final());
};
#endif

void select_string_inst_t::append_links(label_t *Start) {DEBUG
	for (select_string_case_t *Case = Cases; Case; Case = Case->Next) use_label(Start, Case->Body, false);
	use_label(Start, Default, false);
};

void select_string_inst_t::find_breakpoints() {DEBUG
	for (select_string_case_t *Case = Cases; Case; Case = Case->Next) Case->Body->IsPotentialBreakpoint = true;
	Default->IsPotentialBreakpoint = true;
};

int select_string_inst_t::noof_consts() {DEBUG
	int Count = 0;
	for (select_string_case_t *Case = Cases; Case; Case = Case->Next) ++Count;
	return Count;
};

const void **select_string_inst_t::get_consts(const void **Ptr) {DEBUG
	for (const select_string_case_t *Case = Cases; Case; Case = Case->Next) *(Ptr++) = Case->Key;
	return Ptr;
};

static select_string_case_t *sort_string_cases(select_string_case_t *Cases) {DEBUG
	if (Cases->Next == 0) return Cases;
	select_string_case_t *A = Cases;
	Cases = Cases->Next;
	select_string_case_t *S = 0, *T = 0;
	do {
		select_string_case_t *B = Cases;
		Cases = Cases->Next;
		if (A->Length < B->Length) {
			B->Next = S;
			S = B;
		} else {
			B->Next = T;
			T = B;
		};
	} while (Cases);
	if (S) S = sort_string_cases(S);
	if (T) T = sort_string_cases(T);
	select_string_case_t **C = &S;
	while (*C) C = &(*C)->Next;
	*C = A;
	A->Next = T;
	return S;
};

void label_t::select_string(uint32_t LineNo, select_string_case_t *Cases, label_t *Default) {DEBUG
	// Should sort strings into increasing size, and ensure there is at most one empty string case
	select_string_inst_t *Inst = new select_string_inst_t;
	Inst->Cases = sort_string_cases(Cases);
	Inst->Default = Default;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct select_real_inst_t : inst_t {
	select_real_case_t *Cases;
	int Count;
	double *Constants;
	label_t *Default;

#ifdef ASSEMBLER_LISTING
	void list();
#endif
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start);
	int noof_consts() {return 1;};
	const void **get_consts(const void **Ptr) {
		Ptr[0] = Constants;
		return ++Ptr;
	};
	void encode(assembler_t *Assembler);
	void find_breakpoints();
	bool needs_link() {return false;};
};

#ifdef ASSEMBLER_LISTING
void select_real_inst_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	printf("%4d: select_real\n", LineNo);
	for (select_real_case_t *Case = Cases; Case; Case = Case->Next) {
		printf("\t\t%f .. %f => %x\n", Case->Min, Case->Max, Case->Body->final());
	};
	printf("\t\telse %x\n", Default->final());
};
#endif

void select_real_inst_t::append_links(label_t *Start) {DEBUG
	for (select_real_case_t *Case = Cases; Case; Case = Case->Next) use_label(Start, Case->Body, false);
	use_label(Start, Default, false);
};

void select_real_inst_t::find_breakpoints() {DEBUG
	for (select_real_case_t *Case = Cases; Case; Case = Case->Next) Case->Body->IsPotentialBreakpoint = true;
	Default->IsPotentialBreakpoint = true;
};

void label_t::select_real(uint32_t LineNo, select_real_case_t *Cases, label_t *Default) {DEBUG
	select_real_inst_t *Inst = new select_real_inst_t;
	Inst->Cases = Cases;
	int Count = 0;
	for (select_real_case_t *Case = Cases; Case; Case = Case->Next) Count++;
	double *Constants = (double *)Riva$Memory$alloc_atomic(sizeof(double) * Count * 2);
	double *Ptr = Constants;
	for (select_real_case_t *Case = Cases; Case; Case = Case->Next) {
		*(Ptr++) = Case->Min;
		*(Ptr++) = Case->Max;
	};
	Inst->Count = Count;
	Inst->Constants = Constants;
	Inst->Default = Default;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct select_object_inst_t : inst_t {
	select_object_case_t *Cases;
	label_t *Default;

#ifdef ASSEMBLER_LISTING
	void list();
#endif
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start);
	int noof_consts();
	const void **get_consts(const void **Ptr);
	void encode(assembler_t *Assembler);
	void find_breakpoints();
	bool needs_link() {return false;};
};

#ifdef ASSEMBLER_LISTING
void select_object_inst_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	printf("%4d: select_object\n", LineNo);
	for (select_object_case_t *Case = Cases; Case; Case = Case->Next) {
		printf("\t\t%x => %x\n", Case->Object, Case->Body->final());
	};
	printf("\t\telse %x\n", Default->final());
};
#endif

void select_object_inst_t::append_links(label_t *Start) {DEBUG
	for (select_object_case_t *Case = Cases; Case; Case = Case->Next) use_label(Start, Case->Body, false);
	use_label(Start, Default, false);
};

void select_object_inst_t::find_breakpoints() {DEBUG
	for (select_object_case_t *Case = Cases; Case; Case = Case->Next) Case->Body->IsPotentialBreakpoint = true;
	Default->IsPotentialBreakpoint = true;
};

int select_object_inst_t::noof_consts() {DEBUG
	int Count = 0;
	for (select_object_case_t *Case = Cases; Case; Case = Case->Next) ++Count;
	return Count;
};

const void **select_object_inst_t::get_consts(const void **Ptr) {DEBUG
	for (const select_object_case_t *Case = Cases; Case; Case = Case->Next) *(Ptr++) = Case->Object;
	return Ptr;
};

void label_t::select_object(uint32_t LineNo, select_object_case_t *Cases, label_t *Default) {DEBUG
	select_object_inst_t *Inst = new select_object_inst_t;
	Inst->Cases = Cases;
	Inst->Default = Default;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct select_type_inst_t : inst_t {
	select_type_case_t *Cases;
	label_t *Default;

#ifdef ASSEMBLER_LISTING
	void list();
#endif
	void add_source(load_inst_t *Load) {Load->load_val();};
	void append_links(label_t *Start);
	int noof_consts();
	const void **get_consts(const void **Ptr);
	void encode(assembler_t *Assembler);
	void find_breakpoints();
	bool needs_link() {return false;};
};

#ifdef ASSEMBLER_LISTING
void select_type_inst_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	printf("%4d: select_type\n", LineNo);
	for (select_type_case_t *Case = Cases; Case; Case = Case->Next) {
		const char *Module, *Import;
		if (Riva$Module$lookup(Case->Type, &Module, &Import)) {
			printf("\t\t%s.%s => %x\n", Module, Import, Case->Body->final());
		} else {
			printf("\t\t%x => %x\n", Case->Type, Case->Body->final());
		};		
	};
	printf("\t\telse %x\n", Default->final());
};
#endif

void select_type_inst_t::append_links(label_t *Start) {DEBUG
	for (select_type_case_t *Case = Cases; Case; Case = Case->Next) use_label(Start, Case->Body, false);
	use_label(Start, Default, false);
};

void select_type_inst_t::find_breakpoints() {DEBUG
	for (select_type_case_t *Case = Cases; Case; Case = Case->Next) Case->Body->IsPotentialBreakpoint = true;
	Default->IsPotentialBreakpoint = true;
};

int select_type_inst_t::noof_consts() {DEBUG
	int Count = 0;
	for (select_type_case_t *Case = Cases; Case; Case = Case->Next) ++Count;
	return Count;
};

const void **select_type_inst_t::get_consts(const void **Ptr) {DEBUG
	for (const select_type_case_t *Case = Cases; Case; Case = Case->Next) *(Ptr++) = Case->Type;
	return Ptr;
};

void label_t::select_type(uint32_t LineNo, select_type_case_t *Cases, label_t *Default) {DEBUG
	select_type_inst_t *Inst = new select_type_inst_t;
	Inst->Cases = Cases;
	Inst->Default = Default;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct type_of_inst_t : load_inst_t {
	int noof_consts() {return 0;};
	const void **get_consts(const void **Ptr) {return Ptr;};
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: type_of\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::type_of(uint32_t LineNo) {DEBUG
	type_of_inst_t *Inst = new type_of_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct value_of_inst_t : load_inst_t {
	int noof_consts() {return 0;};
	const void **get_consts(const void **Ptr) {return Ptr;};
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: value_of\n", LineNo);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::value_of(uint32_t LineNo) {DEBUG
	value_of_inst_t *Inst = new value_of_inst_t;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct new_list_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: new_list %d\n", LineNo, Index);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::new_list(uint32_t LineNo, uint32_t Index) {DEBUG
	new_list_inst_t *Inst = new new_list_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_list_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_list %d\n", LineNo, Index);
	};
#endif
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
	void encode(assembler_t *Assembler);
};

void label_t::store_list(uint32_t LineNo, uint32_t Index) {DEBUG
	store_list_inst_t *Inst = new store_list_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct new_table_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: new_table %d\n", LineNo, Index);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::new_table(uint32_t LineNo, uint32_t Index) {DEBUG
	new_table_inst_t *Inst = new new_table_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_table_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_table %d\n", LineNo, Index);
	};
#endif
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
	void encode(assembler_t *Assembler);
};

void label_t::store_table(uint32_t LineNo, uint32_t Index) {DEBUG
	store_table_inst_t *Inst = new store_table_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct store_table2_inst_t : inst_t {
	uint32_t Index;
	uint32_t Key;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: store_table %d\n", LineNo, Index);
	};
#endif
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
	void encode(assembler_t *Assembler);
};

void label_t::store_table2(uint32_t LineNo, uint32_t Index, uint32_t Key) {DEBUG
	store_table2_inst_t *Inst = new store_table2_inst_t;
	Inst->Index = Index;
	Inst->Key = Key;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct new_count_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: new_count %d\n", LineNo, Index);
	};
#endif
	void encode(assembler_t *Assembler);
};

void label_t::new_count(uint32_t LineNo, uint32_t Index) {DEBUG
	new_count_inst_t *Inst = new new_count_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct inc_count_inst_t : inst_t {
	uint32_t Index;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: inc_count %d\n", LineNo, Index);
	};
#endif
	void add_source(load_inst_t *Load) {
		Load->load_val();
	};
	void encode(assembler_t *Assembler);
};

void label_t::inc_count(uint32_t LineNo, uint32_t Index) {DEBUG
	inc_count_inst_t *Inst = new inc_count_inst_t;
	Inst->Index = Index;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

struct load_code_inst_t : inst_t {
	label_t *Code;
#ifdef ASSEMBLER_LISTING
	void list() {
		if (IsPotentialBreakpoint) printf("*");
		printf("%4d: load_code %x\n", LineNo, Code);
	};
#endif
	void append_links(label_t *Start) {
		use_label(Start, Code, false);
	};
	void find_breakpoints() {
		if (Next->LineNo != LineNo) Next->IsPotentialBreakpoint = true;
		if (Code->LineNo != LineNo) Code->IsPotentialBreakpoint = true;
	};
	void encode(assembler_t *Assembler);
};

void label_t::load_code(uint32_t LineNo, label_t *Code) {DEBUG
	load_code_inst_t *Inst = new load_code_inst_t;
	Inst->Code = Code;
	Inst->LineNo = LineNo;
	Inst->IsPotentialBreakpoint = false;
	append(Inst);
};

label_t *label_t::final() {DEBUG
	if (Next) return this;
	if (Link) return Link->final();
	printf("Internal error: empty code block produced.\n");
	exit(1);
};

void label_t::append(inst_t *Inst) {DEBUG
	if (Tail) {
		Tail->Next = Inst;
	} else {
		Next = Inst;
		LineNo = Inst->LineNo;
	};
	Tail = Inst;
};

void label_t::add_source(load_inst_t *Load) {DEBUG
	if (Next) {
		Next->add_source(Load);
	} else {
		Link->add_source(Load);
	};
};

#ifdef ASSEMBLER_LISTING
void label_t::list() {
	if (IsPotentialBreakpoint) printf("*");
	//if (Referenced) {
		printf("%4d: %08x:\n", LineNo, this);
	//};
};
#endif

struct assembler_t {
	struct dasm_State *Dynasm;
	uint32_t Size;
	uint32_t Temps;
	uint32_t Locals;
	uint32_t UpValues;
	uint32_t UpTemps;
	uint32_t NoOfDefaults;
	uint32_t NoOfUpValues;
	uint32_t NoOfUpTemps;
	uint32_t NoOfParams;
	uint32_t NoOfLocals;
	label_t *Resend;
	parameter_t *Params;
	bool CanSuspend;
	bool Variadic;
	debug_function_t *DebugInfo;
};

#define Dst			Assembler
#define Dst_DECL	assembler_t *Assembler
#define Dst_REF		Assembler->Dynasm

static void dasm_m_grow(Dst_DECL, void **pp, size_t *szp, int need) {
	size_t sz = *szp;
	if (sz >= need) return;
	if (sz < 16) sz = 16;
	while (sz < need) sz += (sz >> 1);
	*pp = Riva$Memory$realloc(*pp, sz);
	*szp = sz;
};

static void dasm_m_free(Dst_DECL, void *p, size_t sz) {
};

typedef struct code_header_t {
	Riva$Debug_hdr Hdr;
	const void **Consts;
	unsigned long Size;
} code_header_t;

#include "dasm_proto.h"
#include "dasm_x86.h"
#include "assembler-internal.c"

operand_t *label_t::assemble(const frame_t *Frame, const char *StrInfo, int IntInfo, operand_t *Operand) {DEBUG
	label_t Assembly;
	memset(&Assembly, 0, sizeof(Assembly));
	use_label(&Assembly, this, true);
	use_label(&Assembly, Frame->Resend, false);
	for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) Inst->add_sources();

	debug_function_t *DebugInfo = Frame->DebugInfo;

	int NoOfConsts = 0;
	if (DebugInfo) NoOfConsts += 1;
	bool CanSuspend = false;
	for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) NoOfConsts += Inst->noof_consts();
	if (DebugInfo || Riva$Config$get("Wrapl/AlwaysHeapFrame")) {
		CanSuspend = true;
	} else {
		for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) CanSuspend |= Inst->can_suspend();
	};

	if (DebugInfo) {
		Assembly.Next->IsPotentialBreakpoint = true;
		for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) Inst->find_breakpoints();
	};

	assembler_t *Assembler = new assembler_t;
	if (DebugInfo) {
		Assembler->UpValues = sizeof(dstate_t);
	} else {
		Assembler->UpValues = sizeof(bstate_t);
	};
	Assembler->Locals = Assembler->UpValues + 4 * Frame->NoOfUpValues;
	Assembler->UpTemps = Assembler->Locals + 4 * Frame->NoOfLocals;
	Assembler->Temps = Assembler->UpTemps + 8 * Frame->NoOfUpTemps;
	Assembler->Size = Assembler->Temps + 8 * Frame->NoOfTemps;
	Assembler->NoOfParams = Frame->NoOfParams;
	Assembler->Params = Frame->Params;
	Assembler->NoOfUpValues = Frame->NoOfUpValues;
	Assembler->NoOfUpTemps = Frame->NoOfUpTemps;
	Assembler->NoOfLocals = Frame->NoOfLocals;
	Assembler->Resend = Frame->Resend;
	Assembler->CanSuspend = CanSuspend || (Assembler->Size > 256);
	Assembler->Variadic = Frame->Variadic;
	Assembler->DebugInfo = DebugInfo;

	if (DebugInfo) debug_set_locals(DebugInfo, Assembler->Locals, Frame->NoOfLocals);

#ifdef ASSEMBLER_LISTING
	if (Riva$Config$get("Wrapl/Disassemble")) {
		printf("\n\nASSEMBLY\n");
			for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) Inst->list();
		printf("END\n");
		printf("NoOfTemps = %d\n", Frame->NoOfTemps);
		printf("UpValues = %d\n", Assembler->UpValues);
		printf("Locals = %d\n", Assembler->Locals);
		printf("UpTemps = %d\n", Assembler->UpTemps);
		printf("Temps = %d\n", Assembler->Temps);
		printf("Size = %d\n", Assembler->Size);
		printf("NoOfParams = %d\n", Assembler->NoOfParams);
		printf("NoOfUpValues = %d\n", Assembler->NoOfUpValues);
		printf("NoOfUpTemps = %d\n", Assembler->NoOfUpTemps);
		printf("NoOfLocals = %d\n", Assembler->NoOfLocals);
	};
#endif

	dasm_init(Dst, DASM_MAXSECTION);
	void **Globals = new void *[20];
	dasm_setupglobal(Dst, Globals, 20);
	dasm_setup(Dst, ActionList);
	encode_entry(Assembler);
	for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) {
		if (DebugInfo) {
			if (Inst->IsPotentialBreakpoint) {
				encode_potential_breakpoint(Assembler, Inst->LineNo, Inst->type());
			};
		};
		Inst->encode(Assembler);
	};
	uint32_t Size;
	dasm_link(Dst, &Size);

	code_header_t **Code0 = (code_header_t **)Riva$Memory$alloc_code(Size);
	uint8_t *Code = (uint8_t *)(Code0 + 1);

	code_header_t *Header = Code0[0] = new code_header_t;
	Header->Hdr.StrInfo = StrInfo;
	Header->Hdr.IntInfo = IntInfo;


	const void **ConstPtr = new const void *[NoOfConsts];
	if (DebugInfo) *(ConstPtr++) = DebugInfo;
	Header->Consts = ConstPtr;
	Header->Size = Size;

	for (inst_t *Inst = Assembly.Next; Inst; Inst = Inst->Next) ConstPtr = Inst->get_consts(ConstPtr);
	dasm_encode(Assembler, Code);

#ifdef ASSEMBLER_LISTING
	if (Riva$Config$get("Wrapl/Disassemble")) {
		ud_t UD;
		ud_init(&UD);
		ud_set_input_buffer(&UD, Code, Size);
		ud_set_mode(&UD, 32);
		ud_set_pc(&UD, (uint64_t)Code);
		ud_set_syntax(&UD, UD_SYN_INTEL);
		while (ud_disassemble(&UD)) {
			if (ud_insn_mnemonic(&UD) == UD_Icall) {
				ud_operand_t *Operand = ud_insn_opr(&UD, 0);
				void *Address = ud_insn_off(&UD) + Operand->lval.udword + 5;
				if (Address == &debug_exit) {
					printf("%8x: call debug_exit\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &debug_break) {
					printf("%8x: call debug_break\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &debug_enter) {
					printf("%8x: call debug_enter\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &alloc_local) {
					printf("%8x: call alloc_local\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &Riva$Memory$alloc) {
					printf("%8x: call Riva$Memory$alloc\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &Std$Type$is) {
					printf("%8x: call Std$Type$is\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &Agg$Table$new) {
					printf("%8x: call Agg$Table$new\n", (uint32_t)ud_insn_off(&UD));
				} else if (Address == &Agg$Table$insert) {
					printf("%8x: call Agg$Table$insert\n", (uint32_t)ud_insn_off(&UD));
				} else {
					printf("%8x: %s\n", (uint32_t)ud_insn_off(&UD), ud_insn_asm(&UD));
				}
			} else {
				printf("%8x: %s\n", (uint32_t)ud_insn_off(&UD), ud_insn_asm(&UD));
			}
		}
	}
#endif

	if (Operand) {
		//Make sure Operand->Value doesn't use surrounding dynamic scopes
		if (Frame->UpValues) return 0;
		if (Frame->UpTemps) return 0;
		closure_t *Closure = (closure_t *)Operand->Value;
		Closure->Type = WraplT;
		Closure->Entry = Code;
	} else if ((Assembler->NoOfUpValues == 0) && (Assembler->NoOfUpTemps == 0) && (Assembler->NoOfDefaults == 0)) {
		closure_t *Closure = new closure_t;
		Closure->Type = WraplT;
		Closure->Entry = Code;
		Operand = new operand_t;
		Operand->Type = operand_t::CNST;
		Operand->Value = (Std$Object_t *)Closure;
	} else {
		Operand = new operand_t;
		Operand->Type = operand_t::CLSR;
		Operand->Entry = Code;
		Operand->UpValues = Frame->UpValues;
		Operand->UpTemps = Frame->UpTemps;
	};
	return Operand;
};
