#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <Wrapl/Loader.h>
#include <Riva/Thread.h>
#include <Std/Function.h>
#include <pthread.h>

struct dstate_t;
struct debugger_t;
struct debug_module_t;
struct debug_function_t;

struct debug_thread_t {
	debug_thread_t *Prev, *Next;
	uint32_t Id;
	uint32_t StepIn;
	uint32_t RunTo;
	dstate_t *StepOverInstance;
	dstate_t *StepOutInstance;
	dstate_t *State;
	pthread_cond_t Resume[1];
	bool Paused;
	uint32_t Enters, Exits;
};

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

debug_module_t *debug_module(const char *Name);
uint8_t *debug_breakpoints(debug_function_t *Function, uint32_t LineNo);
void debug_add_line(debug_module_t *Module, const char *Line);
void debug_add_global(debug_module_t *Module, const char *Name, Std$Object$t **Address);
debug_function_t *debug_function(debug_module_t *Module, uint32_t LineNo);
int debug_module_id(debug_function_t *Function);
void debug_add_local(debug_function_t *Function, const char *Name, uint32_t Index);
void debug_set_locals(debug_function_t *Function, uint32_t LocalsOffset, uint32_t NoOfLocals);
void debug_enable(const char *SocketPath, bool ClientMode);

extern debugger_t *Debugger;

extern "C" {
	
	void debug_break_impl(dstate_t *State, uint32_t LineNo);
	void debug_enter_impl(dstate_t *State);
	void debug_exit_impl(dstate_t *State);
}


#endif
