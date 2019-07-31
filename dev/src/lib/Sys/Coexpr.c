#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <string.h>

TYPE(T);
// A coexpression type for cooperative multitasking or complex program flow control.
// Whenever a coexpression switches control to a paused coexpression, it passes a transfer value to the activated expression. The method/function which paused the coexpression will return the transfer value when the coexpression is reactivated.
// Coexpressions are protected by mutexes; attempting to activate a running coexpression from a second thread will cause the second thread to block until the coexpression is paused in the first thread.

#ifdef WINDOWS

#include <windows.h>

#else

#include <ucontext.h>
#include <pthread.h>

static pthread_key_t CoexprKey;

#endif

typedef struct coexpr_t {
	const Std$Type$t *Type;
	struct coexpr_t *Caller;
	Std$Function$argument Transfer;
	int Status;
#ifdef WINDOWS
    void *Fiber;
    Std$Object$t *Func;
#else
	pthread_mutex_t Mutex[1];
	ucontext_t Context;
#endif
} coexpr_t;

static inline coexpr_t *switch_coexpr(coexpr_t *Old, coexpr_t *New, int Status, Std$Function$argument Transfer) {
    New->Transfer = Transfer;
	New->Status = Status;
	New->Caller = Old;
#ifdef WINDOWS
    SwitchToFiber(New->Fiber);
#else
	//if (pthread_mutex_trylock(New->Mutex)) {};
	pthread_mutex_lock(New->Mutex);
	pthread_setspecific(CoexprKey, New);
	//pthread_mutex_unlock(Old->Mutex);
	swapcontext(&Old->Context, &New->Context);
#endif
	coexpr_t *Caller = Old->Caller;
#ifdef WINDOWS
#else
	pthread_mutex_unlock(Caller->Mutex);
#endif
	return Caller;
};

#ifdef WINDOWS

static void __stdcall coexpr_func(coexpr_t *Callee) {
	coexpr_t *Caller = Callee->Caller;
	Std$Function$result Result;
	int Status = Std$Function$invoke(Callee->Func, 1, &Result, &Callee->Transfer);
	for (;;) switch (Status) {
	case SUSPEND:
		Caller = switch_coexpr(Callee, Caller, SUCCESS, Result.Arg);
		Result.Arg = Callee->Transfer;
		Status = Std$Function$resume(&Result);
		continue;
	case SUCCESS:
		Caller = switch_coexpr(Callee, Caller, SUCCESS, Result.Arg);
	case FAILURE:
		for (;;) Caller = switch_coexpr(Callee, Caller, FAILURE, (Std$Function$argument){0, 0});
	case MESSAGE:
		for (;;) Caller = switch_coexpr(Callee, Caller, MESSAGE, Result.Arg);
	};
};

GLOBAL_FUNCTION(New, 1) {
//@func:Std$Function$T
// Returns a new coexpression with entry point <var>func</var>.
	coexpr_t *Coexpr = new(coexpr_t);
	Coexpr->Type = T;
	Coexpr->Fiber = CreateFiber(0, coexpr_func, Coexpr);
	Coexpr->Func = Args[0].Val;
	Result->Val = Coexpr;
	return SUCCESS;
};

#else

static void coexpr_func(coexpr_t *Callee, Std$Object$t *Fun) {
	coexpr_t *Caller = Callee->Caller;
	pthread_mutex_unlock(Caller->Mutex);
	Std$Function$result Result;
	int Status = Std$Function$invoke(Fun, 1, &Result, &Callee->Transfer);
	for (;;) switch (Status) {
	case SUSPEND:
		Caller = switch_coexpr(Callee, Caller, SUCCESS, Result.Arg);
		Result.Arg = Callee->Transfer;
		Status = Std$Function$resume(&Result);
		continue;
	case SUCCESS:
		Caller = switch_coexpr(Callee, Caller, SUCCESS, Result.Arg);
	case FAILURE:
		for (;;) Caller = switch_coexpr(Callee, Caller, FAILURE, (Std$Function$argument){0, 0});
	case MESSAGE:
		for (;;) Caller = switch_coexpr(Callee, Caller, MESSAGE, Result.Arg);
	};
};

GLOBAL_FUNCTION(New, 1) {
//@func:Std$Function$T
//@size:Std$Integer$SmallT=16352
// Returns a new coexpression with entry point <var>func</var> and a stack of size <var>size</var> bytes.
	size_t StackSize = 16384 - 32;
	if (Count > 1) {
		CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
		StackSize = Std$Integer$get_small(Args[1].Val);
	};
	coexpr_t *Coexpr = Riva$Memory$alloc(StackSize);
	Coexpr->Type = T;
	getcontext(&Coexpr->Context);
	Coexpr->Context.uc_stack.ss_sp = Coexpr;
	Coexpr->Context.uc_stack.ss_size = Riva$Memory$size(Coexpr) - sizeof(coexpr_t);
	makecontext(&Coexpr->Context, coexpr_func, 2, Coexpr, Args[0].Val);
	pthread_mutex_init(Coexpr->Mutex, 0);
	Result->Val = Coexpr;
	return SUCCESS;
};

#endif

static inline coexpr_t *self(void) {
#ifdef WINDOWS
	coexpr_t *Coexpr = GetFiberData();
#else
	coexpr_t *Coexpr = pthread_getspecific(CoexprKey);
#endif
	if (Coexpr == 0) {
		Coexpr = new(coexpr_t);
		Coexpr->Type = T;
#ifdef WINDOWS
	    Coexpr->Fiber = ConvertThreadToFiber(Coexpr);
#else
		pthread_mutex_init(Coexpr->Mutex, 0);
		pthread_mutex_lock(Coexpr->Mutex);
		pthread_setspecific(CoexprKey, Coexpr);
#endif
		Coexpr->Caller = Coexpr;
	};
	return Coexpr;
};

GLOBAL_FUNCTION(Self, 0) {
//:T
// Returns the current coexpression.
	Result->Val = self();
	return SUCCESS;
};

GLOBAL_FUNCTION(Yield, 1) {
//@value
// Transfers control to the coexpression that activated this one, with <var>value</var> as the transfer value. When control passes back to this coexpression, returns the transfer from the activating coexpression.
    coexpr_t *Caller = self();
	coexpr_t *Callee = Caller->Caller;
	switch_coexpr(Caller, Callee, SUCCESS, Args[0]);
	Result->Arg = Caller->Transfer;
	return SUCCESS;
};

GLOBAL_FUNCTION(Switch, 2) {
//@coexpr:T
//@value
// Transfers control to <var>coexpr</var> with <var>value</var> as the transfer value. When control passes back to this coexpression, returns the transfer from the activating coexpression.
	coexpr_t *Caller = self();
	coexpr_t *Callee = Args[0].Val;
	if (Caller == Callee) {
		Result->Arg = Args[1];
		return SUCCESS;
	};
	switch_coexpr(Caller, Callee, SUCCESS, Args[1]);
	Result->Arg = Caller->Transfer;
	return Caller->Status;
};

METHOD("^", SKP, TYP, T) {
//@value
//@coexpr:T
// Transfers control to <var>coexpr</var> with <var>value</var> as the transfer value. When control passes back to this coexpression, returns the transfer from the activating coexpression.
	coexpr_t *Caller = self();
	coexpr_t *Callee = Args[1].Val;
	if (Caller == Callee) {
		Result->Arg = Args[0];
		return SUCCESS;
	};
	switch_coexpr(Caller, Callee, SUCCESS, Args[0]);
	Result->Arg = Caller->Transfer;
	return Caller->Status;
};

METHOD("^", TYP, T) {
//@value
//@coexpr:T
// Transfers control to <var>coexpr</var> with <id>NIL</id> as the transfer value. When control passes back to this coexpression, returns the transfer from the activating coexpression.
	coexpr_t *Caller = self();
	coexpr_t *Callee = Args[0].Val;
	if (Caller == Callee) return SUCCESS;
	switch_coexpr(Caller, Callee, SUCCESS, (Std$Function$argument){Std$Object$Nil, 0});
	Result->Arg = Caller->Transfer;
	return Caller->Status;
};

GLOBAL_FUNCTION(Caller, 0) {
//:T
// Returns the coexpression that activated the current coexpression. If the current coexpression is the initial coexpression in a thread, then returns <code>Self()</code>.
	coexpr_t *Coexpr = self();
	Result->Val = Coexpr->Caller;
	return SUCCESS;
};

METHOD("collect", TYP, T) {
//@co:T
//:Agg$List$T
// Returns the values produced by <var>co</var> as a list.
	coexpr_t *Caller = self();
	coexpr_t *Callee = Args[0].Val;
	Agg$List$t *List = new(Agg$List$t);
	List->Type = Agg$List$T;
	List->Lower = List->Upper = 0;
	List->Access = 4;
	Result->Val = List;
	Agg$List$node *Node, *Prev;
	unsigned long NoOfElements;
	Callee = switch_coexpr(Caller, Callee, SUCCESS, Args[0]);
	switch (Caller->Status) {
	case SUSPEND:
	case SUCCESS:
		Node = new(Agg$List$node);
		NoOfElements = 1;
		Node->Value = Caller->Transfer.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (;;) {
			Callee = switch_coexpr(Caller, Callee, SUCCESS, Args[0]);
			switch (Caller->Status) {
			case SUCCESS:
			case SUSPEND:
				++NoOfElements;
				Prev = Node;
				Node = new(Agg$List$node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Caller->Transfer.Val;
				break;
			case MESSAGE:
				Result->Val = Caller->Transfer.Val;
				return MESSAGE;
			case FAILURE:
				List->Tail = Node;
				List->Length = NoOfElements;
				return SUCCESS;
			};
		};
	case MESSAGE:
		Result->Val = Caller->Transfer.Val;
		return MESSAGE;
	case FAILURE:
		return SUCCESS;
	};
};

INITIAL() {
	coexpr_t *Coexpr = new(coexpr_t);
	Coexpr->Type = T;
#ifdef WINDOWS
    Coexpr->Fiber = ConvertThreadToFiber(Coexpr);
#else
	pthread_key_create(&CoexprKey, 0);
	pthread_setspecific(CoexprKey, Coexpr);
	pthread_mutex_init(Coexpr->Mutex, 0);
	pthread_mutex_lock(Coexpr->Mutex);
#endif
	//switch_coexpr(Coexpr, Coexpr, SUCCESS, (Std$Function$argument){Std$Object$Nil, 0});
	Coexpr->Caller = Coexpr;
};
