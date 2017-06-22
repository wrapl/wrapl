#include <Riva/Memory.h>
#include <Riva/Config.h>
#include <Riva/Module.h>
#include <Agg/Table.h>
#include <Agg/List.h>
#include <IO/Decoder.h>
#include <IO/Encoder.h>
#include <IO/Posix.h>
#include <IO/Socket.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <jansson.h>
#include "debugger.h"
#include "stringtable.h"
#include "compiler.h"
#include "scanner.h"
#include "parser.h"

const int BREAKPOINT_BUFFER_SIZE = 128;

struct debug_local_t;
struct debug_global_t;

struct debugger_t {
	const Std$Type$t *Type;
	pthread_key_t ThreadKey;
	pthread_mutex_t Lock[1];
	debug_thread_t Threads[1];
	debug_module_t *Modules;
	stringtable_t Commands;
	int Socket;
	uint32_t NextThreadId, NextModuleId;
	const char *SocketPath;
	const char *EvaluateBuffer;
	dstate_t *EvaluateState;
	scanner_t *Scanner;
	compiler_t *Compiler;
};

TYPE(T, IO$Stream$ReaderT);

TYPED_INSTANCE(const char *, IO$Stream$readi, T, debugger_t *Debugger, int Length, const char *Term, int Blocking) {
	const char *Line = Debugger->EvaluateBuffer;
	Debugger->EvaluateBuffer = 0;
	return Line;
}

struct debug_global_t {
	debug_global_t *Next;
	Std$Object$t **Address;
	Std$Object$t *Value;
	const char *Name;
};

struct debug_module_t {
	debug_module_t *Next;
	uint32_t Id, NumBreakpointBuffers;
	uint8_t **BreakpointBuffers;
	const char *Name;
	debug_global_t *Globals;
};

struct debug_local_t {
	debug_local_t *Next;
	uint32_t Index;
	Std$Object$t *Value;
	const char *Name;
};

struct debug_function_t {
	debug_module_t *Module;
	uint32_t LineNo, LocalsOffset, NoOfLocals;
	debug_local_t *Locals;
};

typedef json_t *debugger_command_func(json_t *);

static debug_thread_t *debugger_get_thread(json_t *Args) {
	uint32_t Id;
	if (json_unpack(Args, "{si}", "thread", &Id)) return 0;
	for (debug_thread_t *Thread = Debugger->Threads->Next; Thread != Debugger->Threads; Thread = Thread->Next) {
		if (Thread->Id == Id) return Thread;
	}
	return 0;
}

static debug_module_t *debugger_get_module(json_t *Args) {
	uint32_t Id;
	if (json_unpack(Args, "{si}", "module", &Id)) return 0;
	for (debug_module_t *Module = Debugger->Modules; Module; Module = Module->Next) {
		if (Module->Id == Id) return Module;
	}
	return 0;
}

static json_t *debugger_command_pause(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	Thread->StepIn = 1;
	Thread->RunTo = 0;
	Thread->StepOverInstance = 0;
	Thread->StepOutInstance = 0;
	if (Thread->Paused) {
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}


static json_t *debugger_command_continue(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	Thread->StepIn = 0;
	Thread->RunTo = 0;
	Thread->StepOverInstance = 0;
	Thread->StepOutInstance = 0;
	if (Thread->Paused) {
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_in(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	Thread->StepIn = 1;
	Thread->RunTo = 0;
	Thread->StepOverInstance = 0;
	Thread->StepOutInstance = 0;
	if (Thread->Paused) {
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_out(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	if (Thread->Paused) {
		Thread->StepIn = 0;
		Thread->RunTo = 0;
		Thread->StepOverInstance = 0;
		Thread->StepOutInstance = Thread->State;
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_over(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	if (Thread->Paused) {
		Thread->StepIn = 0;
		Thread->RunTo = 0;
		Thread->StepOverInstance = Thread->State;
		Thread->StepOutInstance = Thread->State;
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}

static json_t *debugger_command_run_to(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread) return json_pack("{ss}", "error", "invalid thread");
	debug_module_t *Module = debugger_get_module(Args);
	if (!Module) return json_pack("{ss}", "error", "invalid module");
	int Line;
	if (json_unpack(Args, "{si}", "line", &Line)) return json_pack("{ss}", "error", "invalid arguments");
	if (Thread->Paused) {
		Thread->StepIn = 0;
		Thread->RunTo = Module->Id << 16 + Line;
		Thread->StepOverInstance = 0;
		Thread->StepOutInstance = 0;
		Thread->Paused = false;
		pthread_cond_signal(Thread->Resume);
	}
	return json_true();
}

static json_t *debugger_command_list_modules(json_t *Command) {
	return json_true();
}

static json_t *debugger_command_list_threads(json_t *Command) {
	json_t *Threads = json_array();
	for (debug_thread_t *Thread = Debugger->Threads->Next; Thread != Debugger->Threads; Thread = Thread->Next) {
		json_array_append(Threads, json_pack("{siss}",
			"id", Thread->Id,
			"state", Thread->Paused ? "paused" : "running"
		));
	}
	return Threads;
};

static void debugger_describe_variable(json_t *Result, Std$Object$t *Value) {
	Std$Type$t *Type = Value->Type;
	if (Value == Std$Object$Nil) {
		json_object_set(Result, "type", json_string("nil"));
		json_object_set(Result, "value", json_null());
	} else if (Type == Std$Integer$SmallT) {
		json_object_set(Result, "type", json_string("integer"));
		json_object_set(Result, "value", json_integer(Std$Integer$get_small(Value)));
	} else if (Type == Std$Real$T) {
		json_object_set(Result, "type", json_string("real"));
		json_object_set(Result, "value", json_real(Std$Real$get_value(Value)));
	} else if (Type == Std$String$T) {
		json_object_set(Result, "type", json_string("string"));
		json_object_set(Result, "value", json_string(Std$String$flatten(Value)));
	} else if (Type == Std$Symbol$T) {
		json_object_set(Result, "type", json_string("symbol"));
		json_object_set(Result, "value", json_string(Std$String$flatten((Std$Object$t *)Std$Symbol$get_name(Value))));
	} else if (Type == Agg$Table$T) {
		json_object_set(Result, "type", json_string("table"));
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(2 * Agg$Table$size(Value)));
	} else if (Type == Agg$List$T) {
		json_object_set(Result, "type", json_string("list"));
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(Agg$List$length(Value)));
	} else {
		json_object_set(Result, "type", json_string("object"));
		const char *Description, *ModuleName, *SymbolName;
		if (!Riva$Module$lookup(Value->Type, &ModuleName, &SymbolName)) {
			json_object_set(Result, "value", json_string("object"));
		} else {
			const char *ShortName = strrchr(ModuleName, '/');
			if (ShortName) {
				++ShortName;
			} else {
				ShortName = ModuleName;
			}
			asprintf(&Description, "%s.%s", ShortName, SymbolName);
			json_object_set(Result, "value", json_string(Description));
		}
		const Std$Array$t *Fields = Value->Type->Fields;
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(Fields->Length.Value));
	}
}

static void debugger_append_variable(json_t *Variables, const char *Name, Std$Object$t *Value) {
	json_t *Description = json_pack("{ss}", "name", Name);
	debugger_describe_variable(Description, Value);
	json_array_append(Variables, Description);
}

static int debugger_append_table_func(Std$Object$t *Key, Std$Object$t *Value, json_t *Variables) {
	debugger_append_variable(Variables, "key", Key);
	debugger_append_variable(Variables, "value", Value);
	return 0;
}

static json_t *debugger_command_get_variable(json_t *Args) {
	int Count = json_integer_value(json_object_get(Args, "count"));
	int Start = json_integer_value(json_object_get(Args, "start"));
	json_t *Variables = json_array();
	if (json_object_get(Args, "state")) {
		dstate_t *State = (dstate_t *)(size_t)json_number_value(json_object_get(Args, "state"));
		debug_function_t *Function = State->Function;
		Std$Object$t ***Locals = (void *)State + Function->LocalsOffset;
		for (debug_local_t *Local = State->Function->Locals; Local; Local = Local->Next) {
			if (Local->Value) {
				debugger_append_variable(Variables, Local->Name, Local->Value);
			} else {
				Std$Object$t **Ref = Locals[Local->Index];
				if (Ref) {
					debugger_append_variable(Variables, Local->Name, *Ref);
				} else {
					json_array_append(Variables, json_pack("{ssss}", "name", Local->Name, "type", "none"));
				}
			}
		}
		return Variables;
	}
	if (json_object_get(Args, "module")) {
		debug_module_t *Module = debugger_get_module(Args);
		if (!Module) return json_pack("{ss}", "error", "invalid module");
		for (debug_global_t *Global = Module->Globals; Global; Global = Global->Next) {
			if (Global->Value) {
				debugger_append_variable(Variables, Global->Name, Global->Value);
			} else {
				Std$Object$t *Value = Global->Address[0];
				if (Value) {
					debugger_append_variable(Variables, Global->Name, Value);
				} else {
					json_array_append(Variables, json_pack("{ssss}", "name", Global->Name, "type", "none"));
				}
			}
		}
		return Variables;
	}
	Std$Object$t *Value = (Std$Object$t *)(size_t)json_number_value(json_object_get(Args, "ref"));
	Std$Type$t *Type = Value->Type;
	if (Type == Agg$Table$T) {
		Agg$Table$foreach(Value, debugger_append_table_func, Variables);
	} else if (Type == Agg$List$T) {
		int Index = 0;
		for (Agg$List$node *Node = ((Agg$List$t *)Value)->Head; Node; Node = Node->Next) {
			char *Name;
			asprintf(&Name, "%d", ++Index);
			debugger_append_variable(Variables, Name, Node->Value);
		}
	} else {
		const Std$Array$t *Fields = Value->Type->Fields;
		for (int I = 0; I < Fields->Length.Value; ++I) {
			debugger_append_variable(Variables, Std$String$flatten((Std$Object$t *)Std$Symbol$get_name(Fields->Values[I])), *((Std$Object$t **)Value + I + 1));
		}
	}
	return Variables;
}

static uint8_t *debug_module_breakpoints(debug_module_t *Module, uint32_t LineNo) {
	int Index = LineNo / BREAKPOINT_BUFFER_SIZE;
	if (Index >= Module->NumBreakpointBuffers) {
		uint8_t **BreakpointBuffers = (void **)Riva$Memory$alloc((Index + 1) * sizeof(void *));
		for (int I = 0; I < Module->NumBreakpointBuffers; ++I) BreakpointBuffers[I] = Module->BreakpointBuffers[I];
		Module->BreakpointBuffers = BreakpointBuffers;
		Module->NumBreakpointBuffers = Index + 1;
	}
	if (Module->BreakpointBuffers[Index]) return Module->BreakpointBuffers[Index] + (LineNo % BREAKPOINT_BUFFER_SIZE) / 8;
	uint8_t *BreakpointBuffer = (uint8_t *)Riva$Memory$alloc_atomic(BREAKPOINT_BUFFER_SIZE / 8);
	memset(BreakpointBuffer, 0, BREAKPOINT_BUFFER_SIZE / 8);
	Module->BreakpointBuffers[Index] = BreakpointBuffer;
	return BreakpointBuffer + (LineNo % BREAKPOINT_BUFFER_SIZE) / 8;
}

static json_t *debugger_command_set_breakpoints(json_t *Args) {
	debug_module_t *Module = debugger_get_module(Args);
	if (!Module) return json_pack("{ss}", "error", "invalid module");
	json_t *Enable = json_object_get(Args, "enable");
	json_t *Disable = json_object_get(Args, "disable");
	if (!Enable || !Disable) return json_pack("{ss}", "error", "invalid arguments");
	for (int I = 0; I < json_array_size(Enable); ++I) {
		uint32_t LineNo = json_integer_value(json_array_get(Enable, I));
		debug_module_breakpoints(Module, LineNo)[0] |= 1 << (LineNo % 8);
	}
	for (int I = 0; I < json_array_size(Disable); ++I) {
		uint32_t LineNo = json_integer_value(json_array_get(Disable, I));
		debug_module_breakpoints(Module, LineNo)[0] &= ~(1 << (LineNo % 8));
	}
	return json_true();
}

LOCAL_FUNCTION(DebuggerIDFunc) {
	const char *Name = Std$String$flatten(Args[0].Val);
	printf("Looking up variable %s\n", Name);
	debug_function_t *Function = Debugger->EvaluateState->Function;
	Std$Object$t ***Locals = (void *)Debugger->EvaluateState + Function->LocalsOffset;
	for (debug_local_t *Local = Function->Locals; Local; Local = Local->Next) {
		if (!strcmp(Local->Name, Name)) {
			if (Local->Value) {
				Result->Val = Local->Value;
				return SUCCESS;
			}
			Std$Object$t **Ref = Locals[Local->Index];
			if (Ref) {
				Result->Val = Ref[0];
				return SUCCESS;
			}
		}
	}
	debug_module_t *Module = Function->Module;
	for (debug_global_t *Global = Module->Globals; Global; Global = Global->Next) {
		if (!strcmp(Global->Name, Name)) {
			if (Global->Value) {
				Result->Val = Global->Value;
				return SUCCESS;
			}
			Result->Val = Global->Address[0];
			return SUCCESS;
		}
	}
	return FAILURE;
}

static json_t *debugger_evaluate(json_t *Args) {
	debug_thread_t *Thread = debugger_get_thread(Args);
	if (!Thread || !Thread->Paused) return json_pack("{ss}", "error", "invalid thread");
	if (json_object_get(Args, "state")) {
		Debugger->EvaluateState = (dstate_t *)(size_t)json_number_value(json_object_get(Args, "state"));
	} else {
		Debugger->EvaluateState = Thread->State;
	}
	if (json_unpack(Args, "{ss}", "expression", &Debugger->EvaluateBuffer)) return json_pack("{ss}", "error", "missing expression");
	if (setjmp(Debugger->Scanner->Error.Handler)) {
		Debugger->Scanner->flush();
		return json_pack("{sssi}", "error", Debugger->Scanner->Error.Message, "line", Debugger->Scanner->Error.LineNo);
	}
	command_expr_t *Command = accept_command(Debugger->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	if (setjmp(Debugger->Compiler->Error.Handler)) {
		Debugger->Compiler->flush();
		return json_pack("{sssi}", "error", Debugger->Compiler->Error.Message, "line", Debugger->Compiler->Error.LineNo);
	}
	Std$Function$result Result[0];
	switch (Command->compile(Debugger->Compiler, Result)) {
	case SUSPEND: case SUCCESS: {
		json_t *Output = json_object();
		debugger_describe_variable(Output, Result->Val);
		return Output;
	}
	case FAILURE: return json_pack("{sb}", "failed", 1);
	case MESSAGE: {
		json_t *Output = json_pack("{sb}", "message", 1);
		debugger_describe_variable(Output, Result->Val);
		return Output;
	}
	}
}

static void *debugger_thread_func(debugger_t *Debugger) {
	json_t *CommandJson;
	json_error_t JsonError;
	while (CommandJson = json_loadfd(Debugger->Socket, JSON_DISABLE_EOF_CHECK, &JsonError)) {
		int Index;
		const char *Command;
		json_t *Args;
		json_t *ResultJson;
		pthread_mutex_lock(Debugger->Lock);
			//printf("Received: %s\n", json_dumps(CommandJson, JSON_INDENT(4)));
			if (!json_unpack(CommandJson, "[iso]", &Index, &Command, &Args)) {
				debugger_command_func *CommandFunc = (debugger_command_func *)stringtable_get(Debugger->Commands, Command);
				if (CommandFunc) {
					ResultJson = json_pack("[io?]", Index, CommandFunc(Args));
				} else {
					ResultJson = json_pack("[i{ss}]", Index, "error", "invalid command");
				}
			} else {
				ResultJson = json_pack("[i{sssssi}]", Index, "error", "parse error", "message", JsonError.text, "position", JsonError.position);
			}
			json_dumpfd(ResultJson, Debugger->Socket, JSON_COMPACT);
			write(Debugger->Socket, "\n", strlen("\n"));
			//printf("Reply: %s\n", json_dumps(ResultJson, JSON_INDENT(4)));
		pthread_mutex_unlock(Debugger->Lock);
	}
	// For now, just kill the program once the debugger disconnects
	exit(1);
}

static void debugger_update(json_t *UpdateJson) {
	json_dumpfd(UpdateJson, Debugger->Socket, JSON_COMPACT);
	write(Debugger->Socket, "\n", strlen("\n"));
	//printf("Update: %s\n", json_dumps(UpdateJson, JSON_INDENT(4)));
}

debug_module_t *debug_module(const char *Name) {
	debug_thread_t *Thread = pthread_getspecific(Debugger->ThreadKey);
	if (!Thread) {
		pthread_setspecific(Debugger->ThreadKey, Thread = new debug_thread_t());
		Thread->Paused = false;
		Thread->Enters = 0;
		Thread->Exits = 0;
		pthread_cond_init(Thread->Resume, 0);
		pthread_mutex_lock(Debugger->Lock);
			Thread->Id = ++Debugger->NextThreadId;
			debug_thread_t *Prev = Debugger->Threads->Prev;
			if (Prev->Next == Prev) Thread->StepIn = 1;
			Prev->Next = Thread;
			Thread->Prev = Prev;
			Thread->Next = Debugger->Threads;
			Debugger->Threads->Prev = Thread;
			debugger_update(json_pack("[is{si}]", 0, "thread_enter", "thread", Thread->Id));
		pthread_mutex_unlock(Debugger->Lock);
	}
	debug_module_t *Module = new(debug_module_t);
	Module->Name = Name;
	pthread_mutex_lock(Debugger->Lock);
		Module->Id = ++Debugger->NextModuleId;
		Module->Next = Debugger->Modules;
		Debugger->Modules = Module;
		debugger_update(json_pack("[is{sisssi}]", 0, "module_add", "module", Module->Id, "name", Name, "thread", Thread->Id));
		Thread->Paused = true;
		do pthread_cond_wait(Thread->Resume, Debugger->Lock); while (Thread->Paused);
	pthread_mutex_unlock(Debugger->Lock);
	return Module;
}

uint8_t *debug_breakpoints(debug_function_t *Function, uint32_t LineNo) {
	return debug_module_breakpoints(Function->Module, LineNo);
}

void debug_add_line(debug_module_t *Module, const char *Line) {
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "line_add", "module", Module->Id, "line", Line));
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_add_global_variable(debug_module_t *Module, const char *Name, Std$Object$t **Address) {
	debug_global_t *Global = new(debug_global_t);
	Global->Name = Name;
	Global->Address = Address;
	debug_global_t **Slot = &Module->Globals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Global;
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "global_add", "module", Module->Id, "name", Name));
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_add_global_constant(debug_module_t *Module, const char *Name, Std$Object$t *Value) {
	debug_global_t *Global = new(debug_global_t);
	Global->Name = Name;
	Global->Value = Value;
	debug_global_t **Slot = &Module->Globals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Global;
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "global_add", "module", Module->Id, "name", Name));
	pthread_mutex_unlock(Debugger->Lock);
}

debug_function_t *debug_function(debug_module_t *Module, uint32_t LineNo) {
	debug_function_t *Function = new(debug_function_t);
	Function->Module = Module;
	Function->LineNo = LineNo;
	return Function;
}

int debug_module_id(debug_function_t *Function) {
	return Function->Module->Id;
}

void debug_add_local_variable(debug_function_t *Function, const char *Name, uint32_t Index) {
	debug_local_t *Local = new(debug_local_t);
	Local->Name = Name;
	Local->Index = Index;
	debug_local_t **Slot = &Function->Locals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Local;
}

void debug_add_local_constant(debug_function_t *Function, const char *Name, Std$Object$t *Value) {
	debug_local_t *Local = new(debug_local_t);
	Local->Name = Name;
	Local->Value = Value;
	debug_local_t **Slot = &Function->Locals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Local;
}

void debug_add_local_constant(debug_function_t *Function, const char *Name, uint32_t Index) {
	debug_local_t *Local = new(debug_local_t);
	Local->Name = Name;
	Local->Index = Index;
	debug_local_t **Slot = &Function->Locals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Local;
}

void debug_set_locals(debug_function_t *Function, uint32_t LocalsOffset, uint32_t NoOfLocals) {
	Function->LocalsOffset = LocalsOffset;
	Function->NoOfLocals = NoOfLocals;
}

debugger_t *Debugger = 0;

void debug_break_impl(dstate_t *State, uint32_t LineNo) {
	debug_thread_t *Thread = State->Thread;
	pthread_mutex_lock(Debugger->Lock);
		Thread->Paused = true;
		json_t *Enters = json_array();
		dstate_t *EnterState = State;
		for (int I = 0; I < Thread->Enters; ++I) {
			debug_function_t *Function = EnterState->Function;
			json_array_insert_new(Enters, 0, json_pack("{sisisfsi}",
				"module", Function->Module->Id,
				"line", Function->LineNo,
				"state", (double)(size_t)EnterState,
				"size", Function->NoOfLocals
			));
			EnterState = EnterState->UpState;
		}
		debugger_update(json_pack("[is{sisisisO}]", 0, "break",
			"thread", Thread->Id,
			"exits", Thread->Exits,
			"line", LineNo,
			"enters", Enters
		));
		Thread->Exits = 0;
		Thread->Enters = 0;
		do pthread_cond_wait(Thread->Resume, Debugger->Lock); while (Thread->Paused);
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_enter_impl(dstate_t *State) {
	debug_thread_t *Thread = pthread_getspecific(Debugger->ThreadKey);
	if (!Thread) {
		pthread_setspecific(Debugger->ThreadKey, Thread = new debug_thread_t());
		Thread->Paused = false;
		Thread->Enters = 1;
		Thread->Exits = 0;
		pthread_cond_init(Thread->Resume, 0);
		pthread_mutex_lock(Debugger->Lock);
			Thread->Id = ++Debugger->NextThreadId;
			debug_thread_t *Prev = Debugger->Threads->Prev;
			if (Prev->Next == Prev) Thread->StepIn = 1;
			Prev->Next = Thread;
			Thread->Prev = Prev;
			Thread->Next = Debugger->Threads;
			Debugger->Threads->Prev = Thread;
			debugger_update(json_pack("[is{si}]", 0, "thread_enter", "thread", Thread->Id));
		pthread_mutex_unlock(Debugger->Lock);
	} else {
		++Thread->Enters;
	}
	dstate_t *UpState = State->UpState = Thread->State;
	State->Thread = Thread;
	Thread->State = State;
}

void debug_exit_impl(dstate_t *State) {
	debug_thread_t *Thread = State->Thread;
	dstate_t *UpState = State->UpState;
	Thread->State = UpState;
	if (Thread->Enters > 0) {
		--Thread->Enters;
	} else {
		++Thread->Exits;
	}
	if (State == Thread->StepOverInstance || State == Thread->StepOutInstance) {
		Thread->StepIn = 1;
	}
	/*} else {
		pthread_mutex_lock(Debugger->Lock);
			Thread->Next->Prev = Thread->Prev;
			Thread->Prev->Next = Thread->Next;
			json_t *UpdateJson = json_pack("[is{si}]", 0, "thread_exit", "thread", Thread->Id);
			json_dumpfd(UpdateJson, Debugger->Socket, JSON_COMPACT);
			write(Debugger->Socket, "\n", strlen("\n"));
		pthread_mutex_unlock(Debugger->Lock);
	}*/
}

static void debug_shutdown() {
	if (Debugger->SocketPath) unlink(Debugger->SocketPath);
	debugger_update(json_pack("[is]", 0, "shutdown"));
}

void debug_enable(const char *SocketPath, bool ClientMode) {
	int Socket = socket(PF_UNIX, SOCK_STREAM, 0);
	Debugger = new debugger_t();
	Debugger->Type = T;
	Debugger->Scanner = new scanner_t((IO$Stream$t *)Debugger);
	Debugger->Compiler = new compiler_t("debugger");
	Debugger->Compiler->MissingIDFunc = (Std$Object$t *)DebuggerIDFunc;
	stringtable_put(Debugger->Commands, "pause", debugger_command_pause);
	stringtable_put(Debugger->Commands, "continue", debugger_command_continue);
	stringtable_put(Debugger->Commands, "step_in", debugger_command_step_in);
	stringtable_put(Debugger->Commands, "step_out", debugger_command_step_out);
	stringtable_put(Debugger->Commands, "step_over", debugger_command_step_over);
	stringtable_put(Debugger->Commands, "run_to", debugger_command_run_to);
	stringtable_put(Debugger->Commands, "list_modules", debugger_command_list_modules);
	stringtable_put(Debugger->Commands, "list_threads", debugger_command_list_threads);
	stringtable_put(Debugger->Commands, "get_variable", debugger_command_get_variable);
	stringtable_put(Debugger->Commands, "set_breakpoints", debugger_command_set_breakpoints);
	stringtable_put(Debugger->Commands, "evaluate", debugger_evaluate);
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	strcpy(Name.sun_path, SocketPath);
	if (ClientMode) {
		if (connect(Socket, (sockaddr *)&Name, SUN_LEN(&Name))) {
			printf("Error connecting debugger socket to %s\n", SocketPath);
			exit(1);
		}
		Debugger->Socket = Socket;
		printf("Connected debugger socket to %s\n", SocketPath);
	} else {
		Debugger->SocketPath = SocketPath;
		if (bind(Socket, (sockaddr *)&Name, SUN_LEN(&Name))) {
			printf("Error binding debugger socket on %s\n", SocketPath);
			exit(1);
		}
		listen(Socket, 1);
		struct sockaddr Addr;
		socklen_t Length = sizeof(Addr);
		printf("Started debugger socket on %s\n", SocketPath);
		Debugger->Socket = accept(Socket, &Addr, &Length);
		printf("Debugger connected\n");
	}
	pthread_key_create(&Debugger->ThreadKey, 0);
	pthread_mutex_init(Debugger->Lock, 0);
	Debugger->Threads->Prev = Debugger->Threads->Next = Debugger->Threads;
	pthread_t Thread;
	pthread_create(&Thread, 0, debugger_thread_func, Debugger);
	atexit(debug_shutdown);
}
