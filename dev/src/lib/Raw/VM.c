#include <Std.h>
#include <Riva.h>
#include <Agg.h>

typedef union {
	struct {
		Std$Object$t *Val;
		Std$Object$t **Ref;
	};
	struct {
		uint8_t *Run;
		Std$Function$state_t *State;
	};
} slot_t;

typedef struct {
	void *Run;
	Std$Function$state_t *Chain;
	void *Resume;
	uint8_t *Code;
	slot_t Slots[];
} state_t;

typedef struct {
	Std$Type$t *Type;
	uint8_t *Code;
	int NumSlots;
	Std$Function$argument UpValues[];
} closure_t;

extern const Std$Type$T T[];

state_t *entry(closure_t *Closure, unsigned long Count, const Std$Function$argument *Args) {
	state_t *State = Riva$Memory$alloc(sizeof(state_t) + NumSlots * sizeof(slot_t));
	State->Code = Closure->Code;
	// copy upvalues
	// set up arguments
	return State;
}

enum {
	NO_OPERATION,
	TRAP_INIT,
	TRAP_PUSH,
	TRAP_STORE,
	BACKTRACK,
	RESUME,
	LINK_STORE,
	LINK_JUMP,
	JUMP,
	LOCAL_INIT,
	LOCAL_LOAD,
	LOCAL_STORE,
	LOCAL_STORE_CONSTANT,
	REFERENCE_STORE,
	GLOBAL_LOAD,
	GLOBAL_STORE,
	GLOBAL_STORE_CONSTANT,
	LOAD_CONSTANT,
	LOAD_VALUE,
	LOAD_TYPE,
	FLUSH,
	INVOKE,
	FAIL,
	RETURN,
	SUSPEND,
	RECEIVE,
	SEND,
	RESEND,
	COMPARE,
	LIMIT_INIT,
	LIMIT_TEST,
	SKIP_INIT,
	SKIP_TEST,
	SELECT_INTEGER,
	SELECT_STRING,
	SELECT_REAL,
	SELECT_OBJECT,
	SELECT_TYPE,
	LIST_INIT,
	LIST_ADD,
	TABLE_INIT,
	TABLE_ADD,
	COUNT_INIT,
	COUNT_ADD,
	CODE_LOAD
};

Std$Function$status run(Std$Function$result *Result) {
	state_t *State = (state_t *)Result->State;
	uint8_t *Code = State->Code;
	for (;;) {
		
	}
}

