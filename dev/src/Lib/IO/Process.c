#include <Std.h>
#include <Agg.h>
#include <Sys/Module.h>
#include <Sys/Signal.h>
#include <IO/Socket.h>
#include <Riva/Memory.h>
#include <Riva/Thread.h>
#include <Riva/System.h>

#if defined(LINUX) || defined(CYGWIN)

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct process_t {
	const Std$Type_t *Type;
	int Handle;
	pid_t Pid;
} process_t;

TYPE(T, IO$Socket$T, NATIVE($TextReaderT), NATIVE($TextWriterT), NATIVE($ReaderT), NATIVE($WriterT), NATIVE($SeekerT), NATIVE($T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(PipeMessageT, IO$Stream$MessageT);
TYPE(ForkMessageT, IO$Stream$MessageT);
TYPE(WaitMessageT, IO$Stream$MessageT);

IO$Stream_messaget PipeMessage[] = {{IO$Stream$MessageT, "Pipe Error"}};
IO$Stream_messaget ForkMessage[] = {{IO$Stream$MessageT, "Fork Error"}};
IO$Stream_messaget WaitMessage[] = {{IO$Stream$MessageT, "Wait Error"}};

CONSTANT(Message, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("Message");
	Sys$Module$export(Module, "PipeError", 0, PipeMessage);
	Sys$Module$export(Module, "ForkError", 0, ForkMessage);
	Sys$Module$export(Module, "WaitError", 0, WaitMessage);
	return Module;
};

static void finalize_process(process_t *Process, void *Data) {
	close(Process->Handle);
};

#ifdef DOCUMENTING
GLOBAL_FUNCTION(Open, 2) {
//@prog:Std$String$T
//@args...
//:T
// Spawns a new process <code>prog args</code> and returns a handle to it. <var>args</var> should either be
// a) a list or arguments (including the name of exectuable as the first argument)
// b) any number of strings arguments (in which case the name of the executable is taken from <var>prog</var>
// The returned handle is a <id>IO/Socket/T</id>, connecting to the input and output of the spawned process.
#else
GLOBAL_FUNCTION(Open, 1) {
#endif
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	int Pair[2];
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, Pair) == -1) {
		Result->Val = PipeMessage;
		return MESSAGE;
	};
	//fcntl(Pair[0], F_SETFL, fcntl(Pair[0], F_GETFL, 0) | O_NONBLOCK);
	//fcntl(Pair[1], F_SETFL, fcntl(Pair[1], F_GETFL, 0) | O_NONBLOCK);
	pid_t Pid = fork();
	if (Pid == -1) {
		close(Pair[0]);
		close(Pair[1]);
		Result->Val = IO$Stream$Message$from_errno(ForkMessageT);
		return MESSAGE;
	};
	if (Pid == 0) {
		close(Pair[1]);
		dup2(Pair[0], STDIN_FILENO);
		dup2(Pair[0], STDOUT_FILENO);
		dup2(Pair[0], STDERR_FILENO);
		int NoOfArgs;
		if (Count > 1 && Args[1].Val->Type == Agg$List$T) {
			NoOfArgs = ((Agg$List_t *)Args[1].Val)->Length + 1;
		} else {
			NoOfArgs = Count + 1;
		};
		const char *Prog = Std$String$flatten(Args[0].Val);
		const char *Argv[NoOfArgs];
		if (Count > 1 && Args[1].Val->Type == Agg$List$T) {
			Agg$List_t *List = Args[1].Val;
			char **Arg = Argv;
			for (Agg$List_node *Node = List->Head; Node; Node = Node->Next) *(Arg++) = Std$String$flatten(Node->Value);
			*Arg = 0;
		} else {
			char **Arg = Argv;
			*(Arg++) = Prog;
			for (int I = 1; I < Count; ++I) {
				CHECK_EXACT_ARG_TYPE(I, Std$String$T);
				*(Arg++) = Std$String$flatten(Args[I].Val);
			};
			*Arg = 0;
		};
		if (execvp(Prog, Argv) == -1) {
			write(Pair[0], "command not found", 17);
			close(Pair[0]);
			_exit(-1);
		};
	} else {
		close(Pair[0]);
		process_t *Process = new(process_t);
		Process->Type = T;
		Process->Handle = Pair[1];
		Process->Pid = Pid;
		Riva$Memory$register_finalizer(Process, finalize_process, 0, 0, 0);
		Result->Val = Process;
		return SUCCESS;
	};
};

GLOBAL_METHOD(Result, 1, "result", TYP, T) {
//@proc
//:Std$Integer$T
// Returns the exit status of the process referred to by <var>proc</var> if it has finished executing. Fails otherwise.
	process_t *Process = Args[0].Val;
	int Status;
	if (waitpid(Process->Pid, &Status, WNOHANG) == Process->Pid) {
		Result->Val = Std$Integer$new_small(Status);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

GLOBAL_METHOD(Wait, 1, "wait", TYP, T) {
//@proc
//:Std$Integer$T
// Returns the exit status of the process referred to by <var>proc</var>, waiting if necessary for the process to finish. executing. Fails otherwise.
	process_t *Process = Args[0].Val;
	int Status;
	if (waitpid(Process->Pid, &Status, 0) == Process->Pid) {
		Result->Val = Std$Integer$new_small(Status);
		return SUCCESS;
	} else {
		Result->Val = IO$Stream$Message$from_errno(WaitMessageT);
		return MESSAGE;
	};
};

GLOBAL_METHOD(Signal, 2, "signal", TYP, T, TYP, Std$Integer$SmallT) {
	process_t *Process = Args[0].Val;
	kill(Process->Pid, Std$Integer$get_small(Args[1].Val));
};

#else

typedef struct process_t {
	Std$Type_t *Type;
	//IO$Windows_t *Stream;
} process_t;

TYPE(T);

GLOBAL_FUNCTION(Open, 2) {
	return FAILURE;
};

METHOD("result", TYP, T) {
	process_t *Process = Args[0].Val;
};

METHOD("wait", TYP, T) {
	process_t *Process = Args[0].Val;
};

#endif
