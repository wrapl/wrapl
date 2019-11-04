#include <Sys/Program.h>
#include <Std.h>
#include <Agg.h>
#include <IO/Socket.h>
#include <Riva/Memory.h>
#include <Riva/Thread.h>
#include <Riva/System.h>
#include <Riva/Debug.h>
#include <Util/TypedFunction.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

#ifdef LINUX
#include <unistd.h>
#else
#include <windows.h>
#endif

TYPE(ErrorT);

TYPED_FUNCTION(const char *, _error_name, Sys$Program$error_t *Error) {
	return "Generic error";
}

AMETHOD(Std$String$Of, TYP, ErrorT) {
	Sys$Program$error_t *Error = (Sys$Program$error_t *)Args[0].Val;
	RETURN(Std$String$new_format("%s: %s", _error_name(Error), Error->Message));
}

#ifdef LINUX

Std$Object$t *IntHandler;

void inthandler(int Signal) {
	Std$Function$result Result;
	Std$Function$status Status = Std$Function$call(IntHandler, 0, &Result);
};

GLOBAL_FUNCTION(OnInt, 1) {
//@handler : Std$Function$T
// Installs <var>handler</var> to handler Ctrl-C presses at the terminal
// This function is kinda hacky, try not to use it :)
	IntHandler = Args[0].Val;
	struct sigaction Action;
	Action.sa_handler = inthandler;
	Action.sa_flags = SA_RESTART | SA_NODEFER;
	sigaction(SIGINT, &Action, 0);
	return SUCCESS;
};

Std$Object$t *UrgHandler;

void urghandler(int Signal) {
	Std$Function$result Result;
	Std$Function$status Status = Std$Function$call(UrgHandler, 0, &Result);
};

GLOBAL_FUNCTION(OnUrg, 1) {
//@handler : Std$Function$T
// Installs <var>handler</var> to handler OOB data
// This function is kinda hacky, try not to use it :)
	UrgHandler = Args[0].Val;
	struct sigaction Action;
	Action.sa_handler = urghandler;
	Action.sa_flags = SA_RESTART | SA_NODEFER;
	sigaction(SIGURG, &Action, 0);
	return SUCCESS;
};

Sys$Program$stack_trace_t *_stack_trace(int MaxDepth) {
	Sys$Program$stack_trace_t *StackTrace = Riva$Memory$alloc(sizeof(Sys$Program$stack_trace_t) + MaxDepth * sizeof(const char *));
	StackTrace->Depth = Riva$Debug$stack_trace((void **)&MaxDepth, StackTrace->Trace, MaxDepth);
	return StackTrace;
}

#endif

GLOBAL_FUNCTION(Exit, 0) {
//@code : Std$Integer$SmallT = 0
// Terminates the program with exit code <var>code</var>.
	exit(Count ? ((Std$Integer$smallt *)Args[0].Val)->Value : 0);
};

#ifdef LINUX

GLOBAL_FUNCTION(Sleep, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	return sleep(((Std$Integer$smallt *)Args[0].Val)->Value) ? FAILURE : SUCCESS;
};

#else

GLOBAL_FUNCTION(_Sleep, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	Sleep(((Std$Integer$smallt *)Args[0].Val)->Value * 1000);
	return SUCCESS;
};

#endif

GLOBAL_FUNCTION(Break, 0) {
	asm("int3");
	return SUCCESS;
};

GLOBAL_FUNCTION(Restart, 0) {
	
};

GLOBAL_FUNCTION(CurrentDir, 0) {
	RETURN(Std$String$new(getcwd(0, 0)));
}

GLOBAL(Agg$List$T, Agg$List$t, Args)[] = {{
// The command line arguments.
	Agg$List$T,
	0, 0, 0, 0,
	0,
	0, 0, 0, 4
}};

INITIAL() {
	if (Riva$System$_NoOfArgs > 0) {
		Agg$List$node *Node = new(Agg$List$node);
		Node->Value = Std$String$new(Riva$System$_Args[0]);
		Args->Head = Node;
		Args->Cache = Node;
		Args->Index = 1;
		for (int I = 1; I < Riva$System$_NoOfArgs; ++I) {
			Agg$List$node *Prev = Node;
			Node = new(Agg$List$node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Std$String$new(Riva$System$_Args[I]);
		};
		Args->Tail = Node;
		Node->Next = 0;
		Args->Length = Riva$System$_NoOfArgs;
	};
};
