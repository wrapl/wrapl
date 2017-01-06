#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <Wrapl/Loader.h>
#include <Riva/Thread.h>
#include <Std/Function.h>
#include <pthread.h>

struct dstate_t;
struct debugger_t;
struct debug_module_t;

struct debug_thread_t {
	int StepIn;
	dstate_t *StepOverInstance;
	dstate_t *StepOutInstance;
	debug_module_t *RunToModule;
	int RunToLine;
	debug_thread_t *Prev, *Next;
	dstate_t *State;
	pthread_cond_t Resume[1];
	bool Paused;
};

struct debug_function_t {
	debug_module_t *Module;
	int LineNo;
	int LocalsOffset;
};

debug_module_t *debug_module(const char *Name);
void *debug_breakpoints(debug_module_t *Module, int LineNo);
void debug_add_line(debug_module_t *Module, const char *Line);
void debug_add_global(debug_module_t *Module, const char *Name, Std$Object$t **Address);
debug_function_t *debug_function(debug_module_t *Module, int LineNo);
void debug_add_local(debug_function_t *Function, const char *Name, int Index);

struct dstate_t {
	void *Run;
	Std$Function$state_t *Chain;
	void *Resume;
	Std$Object_t *Val;
	Std$Object_t **Ref;
	void *Code;
	void *Handler;
	void *UpState;
	debug_function_t *Function;
	debug_thread_t *Thread;
};

extern debugger_t *Debugger;

extern "C" {
	void debug_enable(int Port);
	void debug_break(dstate_t *State, int LineNo);
	void debug_enter(dstate_t *State);
	void debug_exit(dstate_t *State);
}


#endif
