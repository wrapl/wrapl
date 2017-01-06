#include <Riva/Memory.h>
#include <IO/Decoder.h>
#include <IO/Encoder.h>
#include <IO/Posix.h>
#include <IO/Socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "debugger.h"

const int BREAKPOINT_BUFFER_SIZE = 128;

struct debugger_t {
	pthread_key_t ThreadKey;
	pthread_mutex_t Lock[1];
	debug_thread_t Threads[1];
	int Socket;
	debug_thread_t *CommandThread;
	debug_module_t *Modules;
	IO$Decoder$t *Decoder;
	IO$Encoder$t *Encoder;
};

struct debug_module_t {
	debug_module_t *Next;
	void **BreakpointBuffers;
	int NumBreakpointBuffers;
	const char *Name;
};

bool debugger_recv(int Size, char *Buffer) {
	char *Tmp = Buffer;
	while (Size) {
		int Read = read(Debugger->Socket, Tmp, Size);
		if (Read == -1) return false;
		Size -= Read;
		Tmp += Read;
	}
	return true;
}

bool debugger_send(int Size, const char *Buffer) {
	char *Tmp = Buffer;
	while (Size) {
		int Written = write(Debugger->Socket, Tmp, Size);
		if (Written == -1) return false;
		Size -= Written;
		Tmp += Written;
	}
	return true;
}

enum {
	CMD_SET_THREAD,
	CMD_CONTINUE,
	CMD_PAUSE,
	CMD_STEP_IN,
	CMD_STEP_OVER,
	CMD_STEP_OUT,
	CMD_RUN_TO,
	CMD_BACKTRACE,
	CMD_MODULES,
	CMD_MODULE,
	CMD_FUNCTION,
	CMD_LOCAL,
	CMD_GLOBAL
};

enum {
	MSG_RESPONSE,
	MSG_BREAK,
	MSG_THREAD_START,
	MSG_THREAD_END,
	MSG_LINE_ADD,
	MSG_GLOBAL_ADD,
	MSG_LOCAL_ADD
};

static void *debugger_thread_func(debugger_t *Debugger) {
	char Buffer[32];
	while (debugger_recv(Buffer, 1)) {
		switch (Buffer[0]) {
		case CMD_SET_THREAD: {
			debugger_recv(Buffer, 4);
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *Thread = *(debug_thread_t **)Buffer;
			for (debug_thread_t *Test = Debugger->Threads->Next; Test != Debugger->Threads; Test = Test->Next) {
				if (Test == Thread) {
					Debugger->CommandThread = Thread;
					break;
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		case CMD_CONTINUE: {
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *CommandThread = Debugger->CommandThread;
			if (CommandThread) {
				CommandThread->StepIn = 0;
				CommandThread->RunToModule = 0;
				CommandThread->StepOverInstance = 0;
				CommandThread->StepOutInstance = 0;
				if (CommandThread->Paused) {
					CommandThread->Paused = false;
					pthread_cond_signal(CommandThread->Resume);
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		case CMD_PAUSE:
		case CMD_STEP_IN: {
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *CommandThread = Debugger->CommandThread;
			if (CommandThread) {
				CommandThread->StepIn = 1;
				CommandThread->RunToModule = 0;
				CommandThread->StepOverInstance = 0;
				CommandThread->StepOutInstance = 0;
				if (CommandThread->Paused) {
					CommandThread->Paused = false;
					pthread_cond_signal(CommandThread->Resume);
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		case CMD_STEP_OUT: {
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *CommandThread = Debugger->CommandThread;
			if (CommandThread) {
				if (CommandThread->Paused) {
					CommandThread->StepIn = 0;
					CommandThread->RunToModule = 0;
					CommandThread->StepOverInstance = 0;
					CommandThread->StepOutInstance = CommandThread->State;
					CommandThread->Paused = false;
					pthread_cond_signal(CommandThread->Resume);
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		case CMD_STEP_OVER: {
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *CommandThread = Debugger->CommandThread;
			if (CommandThread) {
				if (CommandThread->Paused) {
					CommandThread->StepIn = 0;
					CommandThread->RunToModule = 0;
					CommandThread->StepOverInstance = CommandThread->State;
					CommandThread->StepOutInstance = 0;
					CommandThread->Paused = false;
					pthread_cond_signal(CommandThread->Resume);
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		case CMD_RUN_TO: {
			debugger_recv(Buffer, 8);
			pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *CommandThread = Debugger->CommandThread;
			if (CommandThread) {
				if (CommandThread->Paused) {
					CommandThread->StepIn = 0;
					CommandThread->RunToModule = *(debug_module_t **)Buffer;
					CommandThread->RunToLine = *(int *)(Buffer + 4);
					CommandThread->StepOverInstance = 0;
					CommandThread->StepOutInstance = 0;
					CommandThread->Paused = false;
					pthread_cond_signal(CommandThread->Resume);
				}
			}
			pthread_mutex_unlock(Debugger->Lock);
			break;
		}
		CMD_MODULES: {
			pthread_mutex_lock(Debugger->Lock);
			
			pthread_mutex_unlock(Debugger->Lock);
		}
		}
	}
}

debug_module_t *debug_module(const char *Name) {
	debug_module_t *Module = new(debug_module_t);
	Module->Name = Name;
	pthread_mutex_lock(Debugger->Lock);
		Module->Next = Debugger->Modules;
		Debugger->Modules = Module;
	pthread_mutex_unlock(Debugger->Lock);
	return Module;
}

void *debug_breakpoints(debug_module_t *Module, int LineNo) {
	int Index = LineNo / BREAKPOINT_BUFFER_SIZE;
	if (Index >= Module->NumBreakpointBuffers) {
		void **BreakpointBuffers = (void **)Riva$Memory$alloc((Index + 1) * sizeof(void *));
		for (int I = 0; I < Module->NumBreakpointBuffers; ++I) BreakpointBuffers[I] = Module->BreakpointBuffers[I];
		Module->BreakpointBuffers = BreakpointBuffers;
		Module->NumBreakpointBuffers = Index + 1;
	}
	if (Module->BreakpointBuffers[Index]) return Module->BreakpointBuffers[Index];
	void *BreakpointBuffer = (void *)Riva$Memory$alloc_atomic(BREAKPOINT_BUFFER_SIZE / 8);
	memset(BreakpointBuffer, 0, BREAKPOINT_BUFFER_SIZE / 8);
	Module->BreakpointBuffers[Index] = BreakpointBuffer;
	return BreakpointBuffer + (LineNo % BREAKPOINT_BUFFER_SIZE) / 8;
}

void debug_add_line(debug_module_t *Module, const char *Line) {
	size_t Length = strlen(Line);
	char Buffer[9];
	Buffer[0] = MSG_LINE_ADD;
	*(debug_module_t **)(Buffer + 1) = Module;
	*(size_t *)(Buffer + 5) = Length;
	debugger_send(9, Buffer);
	debugger_send(Length, Line);
}

void debug_add_global(debug_module_t *Module, const char *Name, Std$Object$t **Address) {
	
}

debug_function_t *debug_function(debug_module_t *Module, int LineNo) {
	debug_function_t *Function = new(debug_function_t);
	Function->Module = Module;
	Function->LineNo = LineNo;
	return Function;
}

void debug_add_local(debug_function_t *Function, const char *Name, int Index) {
	
}

debugger_t *Debugger = 0;

void debug_break(dstate_t *State, int LineNo) {
	debug_thread_t *Thread = State->Thread;
	pthread_mutex_lock(Debugger->Lock);
		Thread->Paused = true;
		char Buffer[9];
		Buffer[0] = MSG_BREAK;
		*(debug_thread_t **)(Buffer + 1) = Thread;
		*(int *)(Buffer + 5) = LineNo;
		debugger_send(9, Buffer);
		do pthread_cond_wait(Thread->Resume, Debugger->Lock); while (Thread->Paused);
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_enter(dstate_t *State) {
	debug_thread_t *Thread = pthread_getspecific(Debugger->ThreadKey);
	if (!Thread) {
		pthread_setspecific(Debugger->ThreadKey, Thread = new debug_thread_t());
		Thread->Paused = false;
		pthread_cond_init(Thread->Resume, 0);
		pthread_mutex_lock(Debugger->Lock);
			debug_thread_t *Prev = Debugger->Threads->Prev;
			Prev->Next = Thread;
			Thread->Prev = Prev;
			Thread->Next = Debugger->Threads;
			Debugger->Threads->Prev = Thread;
			char Buffer[5];
			Buffer[0] = MSG_THREAD_START;
			*(debug_thread_t **)(Buffer + 1) = Thread;
			debugger_send(5, Buffer);
		pthread_mutex_unlock(Debugger->Lock);
	}
	dstate_t *UpState = State->UpState = Thread->State;
	State->Thread = Thread;
	Thread->State = State;
}

void debug_exit(dstate_t *State) {
	debug_thread_t *Thread = State->Thread;
	dstate_t *UpState = State->UpState;
	Thread->State = UpState;
	if (!UpState) {
		pthread_mutex_lock(Debugger->Lock);
			Thread->Next->Prev = Thread->Prev;
			Thread->Prev->Next = Thread->Next;
			if (Debugger->CommandThread == Thread) Debugger->CommandThread = 0;
			char Buffer[5];
			Buffer[0] = MSG_THREAD_END;
			*(debug_thread_t **)(Buffer + 1) = Thread;
			debugger_send(5, Buffer);
		pthread_mutex_unlock(Debugger->Lock);
	}
}

void debug_enable(int Port) {
	printf("Starting debugger on port %d\n", Port);
	int Socket = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(Port);
	Name.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(Socket, (sockaddr *)&Name, sizeof(Name));
	listen(Socket, 1);
	struct sockaddr Addr;
	socklen_t Length = sizeof(Addr);
	Debugger = new debugger_t();
	printf("Waiting for connection\n");
	Debugger->Socket = accept(Socket, &Addr, &Length);
	Std$Object$t *Stream = IO$Posix$new(IO$Socket$T, Debugger->Socket);
	pthread_key_create(&Debugger->ThreadKey, 0);
	pthread_mutex_init(Debugger->Lock, 0);
	Debugger->Threads->Prev = Debugger->Threads->Next = Debugger->Threads;
	pthread_t Thread;
	pthread_create(&Thread, 0, debugger_thread_func, Debugger);
	Debugger->Decoder = IO$Decoder$new(Stream);
	Debugger->Encoder = IO$Encoder$new(Stream);
}
