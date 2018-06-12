#include <Std.h>
#include <Riva.h>
#include <Wrapl/Loader.h>
#include <pthread.h>
#include <Sys/Service.h>
#include <Riva/Module.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Agg/StringTable.h>
#include <string.h>
#include <stdio.h>

#define DEBUG 0;//printf("%s:%d\n", __func__, __LINE__);

typedef struct variable_t variable_t;
typedef struct thread_t thread_t;
typedef struct Wrapl$Loader_debug_module module_t;
typedef struct Wrapl$Loader_debug_state state_t;
typedef struct Wrapl$Loader_debug_function function_t;
typedef struct Wrapl$Loader_debug_instance instance_t;

struct variable_t {
	const char *Name;
	int Index;
	variable_t *Next;
};

struct Wrapl$Loader_debug_module {
	Std$Type$t *Type;
	const char *Name;
	int Index;
	int NoOfLines;
	Agg$StringTable$t Globals[1];
	Std$Object$t *Source;
	int MaxBreakpoint;
	char *Breakpoints;
};

struct Wrapl$Loader_debug_function {
	Std$Type$t *Type;
	const char *Name;
	module_t *Module;
	int NoOfLocals;
	int LocalsOffset;
	int LineNo;
	variable_t *Locals, *LastLocal;
};

struct Wrapl$Loader_debug_instance {
	Std$Type$t *Type;
	instance_t *Up, *Down;
	struct thread_t *Thread;
	function_t *Function;
	Std$Object_t **Locals;
};

struct thread_t {
	Std$Type$t *Type;
	int Index;
	instance_t *Instance;
	pthread_cond_t Paused;
	pthread_mutex_t Lock;
	Std$Object$t *Value;
	int LineNo;
	char Breakinsts[NUM_INSTTYPES];
	struct {
		enum {RUNNING, STEP_IN, STEP_OVER, STEP_OUT, RUN_TO, PAUSED} Mode;
		union {instance_t *Instance; module_t *Module;};
		int LineNo;
	} Step;
};

static pthread_key_t ThreadKey;
static int BreakOnNewModule = 1;

static thread_t *SelectedThread = 0;
static instance_t *SelectedInstance = 0;
static module_t *SelectedModule = 0;

#define MAX_THREADS 256
static thread_t *Threads[MAX_THREADS];
static pthread_mutex_t GlobalsLock = PTHREAD_MUTEX_INITIALIZER;

#define MAX_MODULES 1024
static module_t *Modules[MAX_MODULES];

static module_t *add_module(const char *Name) {
	module_t *Module = new(module_t);
	Module->Name = Name;
	Module->Source = Agg$List$new0();
	pthread_mutex_lock(&GlobalsLock);
	for (int I = 0; I < MAX_MODULES; ++I) {
		if (Modules[I] == 0) {
			Modules[I] = Module;
			printf("Module %d added => %s\n", I, Name);
			break;
		};
	};
	pthread_mutex_unlock(&GlobalsLock);
	return Module;
};

static void add_line(module_t *Module, const char *Line) {
// 	printf("Adding line to %s: %s\n", Module->Name, Line);
	Agg$List$put(Module->Source, Std$String$copy(Line));
	Module->NoOfLines++;
};

static void add_global(module_t *Module, const char *Name, Std$Object_t **Address) {
//	printf("Adding global variable to %s: %s @ %x\n", Module->Name, Name, Address);
	Agg$StringTable$put(Module->Globals, Name, strlen(Name), Address);
};

static function_t *add_function(module_t *Module, int LineNo) {
//	printf("Adding function to %s @ %d\n", Module->Name, LineNo);
	function_t *Function = new(function_t);
	Function->Module = Module;
	Function->LineNo = LineNo;
	asprintf(&Function->Name, "%s:%d", Module->Name, LineNo);
	return Function;
};

static void *set_locals_offset(function_t *Function, int Offset) {
	Function->LocalsOffset = Offset;
};

static void add_local(function_t *Function, const char *Name, int Index) {
//	printf("Adding local variable to %s.%x: %s @ %d\n", Function->Module->Name, Function, Name, Index);
	variable_t *Variable = new(variable_t);
	Variable->Name = Name;
	Variable->Index = Index;
	if (Function->LastLocal) {
		Function->LastLocal->Next = Variable;
	} else {
		Function->Locals = Variable;
	};
	Function->LastLocal = Variable;
};

static void unpause_thread(thread_t *Thread) {DEBUG
	pthread_cond_signal(&Thread->Paused);
	pthread_mutex_unlock(&Thread->Lock);
};

SYMBOL($AS, "@");

static void thread_exit(thread_t *Thread) {
	if (Thread == (void *)0xFFFFFFFF) return;
	pthread_mutex_lock(&GlobalsLock);
	Threads[Thread->Index] = 0;
	printf("Thread %d exited\n", Thread->Index);
	if (SelectedThread == Thread) {
		SelectedThread = 0;
		SelectedInstance = 0;
	};
	pthread_mutex_unlock(&GlobalsLock);
};

static void break_thread(thread_t *Thread, Std$Object$t *Value) {DEBUG
	pthread_mutex_lock(&Thread->Lock);
	Thread->Step.Mode = PAUSED;
	Thread->Value = Value;
	if (Thread == SelectedThread) {
		if (Thread->Instance != SelectedInstance) {
			SelectedInstance = Thread->Instance;
			printf("%s\n", SelectedInstance->Function->Name);
		};
		Std$Object$t *Line = Agg$List$find_node(SelectedInstance->Function->Module->Source, Thread->LineNo)->Value;
		printf("%d: %s", Thread->LineNo, Std$String$flatten(Line));
	} else {
		printf("Thread %d paused\n", Thread->Index);
	};
	pthread_cond_wait(&Thread->Paused, &Thread->Lock);
	pthread_mutex_unlock(&Thread->Lock);
};

static void *enter_function(function_t *Function, state_t *State) {DEBUG
//	printf("Module = %x\n", Function->Module);
//	printf("Entering function: %s.%x @ %x\n", Function->Module->Name, Function, State);
	thread_t *Thread = pthread_getspecific(ThreadKey);
//	printf("Displayed thread = %x, thread = %x\n", DisplayedThread, Thread);
	if (Thread == (thread_t *)0xFFFFFFFF) return (instance_t *)0xFFFFFFFF;
	if (Thread == 0) {
		Thread = new(thread_t);
		Thread->LineNo = Function->LineNo;
		pthread_setspecific(ThreadKey, Thread);
		pthread_cond_init(&Thread->Paused, 0);
		pthread_mutex_init(&Thread->Lock, 0);
		Thread->Breakinsts[INSTTYPE_SEND] = 1;
		pthread_mutex_lock(&GlobalsLock);
		for (int I = 0; I < MAX_THREADS; ++I) {
			if (Threads[I] == 0) {
				Threads[I] = Thread;
				Thread->Index = I;
				break;
			}
		};
		if (SelectedThread == 0) {
			SelectedThread = Thread;
			Thread->Step.Mode = STEP_IN;
		};
		pthread_mutex_unlock(&GlobalsLock);
		printf("Thread %d created\n", Thread->Index);
	};
	instance_t *Instance = new(instance_t);
	Instance->Function = Function;
	Instance->Thread = Thread;
	pthread_mutex_lock(&Thread->Lock);
	Instance->Up = Thread->Instance;
	Instance->Locals = (char *)State + Function->LocalsOffset;
	if (Thread->Instance) Thread->Instance->Down = Instance;
	Thread->Instance = Instance;
	pthread_mutex_unlock(&Thread->Lock);
	return Instance;
};

static void exit_function(instance_t *Instance) {DEBUG
	if (Instance == (instance_t *)0xFFFFFFFF) return;
 	thread_t *Thread = Instance->Thread;
	pthread_mutex_lock(&Thread->Lock);
	Thread->Instance = Instance->Up;
	if (Thread->Instance) Thread->Instance->Down = 0;
	pthread_mutex_unlock(&Thread->Lock);
};

static void alloc_local(instance_t *Instance, int Index, Std$Object_t **Address) {DEBUG
	if (Instance == (instance_t *)0xFFFFFFFF) return;
	thread_t *Thread = Instance->Thread;
};

static inline int is_breakinst(thread_t *Thread, insttype_t InstType) {
	return Thread->Breakinsts[InstType];
};

static inline int is_breakpoint(module_t *Module, int LineNo) {
	if (Module->Breakpoints == 0) return 0;
	if (Module->MaxBreakpoint <= LineNo) return 0;
	return Module->Breakpoints[LineNo - 1];
};

static inline void set_breakpoint(module_t *Module, int LineNo) {
	if (Module->MaxBreakpoint <= LineNo) {
		char *Breakpoints = Riva$Memory$alloc_atomic(Module->NoOfLines);
		memset(Breakpoints, 0, Module->NoOfLines);
		memcpy(Breakpoints, Module->Breakpoints, Module->MaxBreakpoint);
		Module->Breakpoints = Breakpoints;
		Module->MaxBreakpoint = Module->NoOfLines;
	};
	++Module->Breakpoints[LineNo - 1];
};

static inline void clear_breakpoint(module_t *Module, int LineNo) {
	--Module->Breakpoints[LineNo - 1];
};

static void break_line(instance_t *Instance, int LineNo, insttype_t InstType, Std$Object$t *Value) {DEBUG
	if (Instance == (instance_t *)0xFFFFFFFF) return;
	//printf("Break line: %d, %d in %x\n", LineNo, InstType, Instance);
	thread_t *Thread = Instance->Thread;
	if (LineNo) Thread->LineNo = LineNo;
	DEBUG
	if (Thread->Step.Mode == STEP_IN) goto breakpoint;
	DEBUG
	function_t *Function = Instance->Function;
	module_t *Module = Function->Module;
	DEBUG
	if (LineNo == Thread->Step.LineNo) {
		DEBUG
		if ((Thread->Step.Mode == RUN_TO) && (Module == Thread->Step.Module)) goto breakpoint;
	} else {
		DEBUG
		if ((Thread->Step.Mode == STEP_OVER) && (Instance == Thread->Step.Instance)) goto breakpoint;
	};
	DEBUG
	if (InstType != INSTTYPE_DEFAULT) {
		if (is_breakinst(Thread, InstType)) goto breakpoint;
	} else {
		if (is_breakpoint(Module, LineNo)) goto breakpoint;
	};
	DEBUG
	return;
breakpoint:
	if (InstType == INSTTYPE_DEFAULT) Value = 0;
	break_thread(Thread, Value);
};

static void module_selection_changed(GtkTreeSelection *Selection, void *User) {
};

GLOBAL_FUNCTION(Break, 0) {
	thread_t *Thread = pthread_getspecific(ThreadKey);
	if (Thread == (thread_t *)0xFFFFFFFF) return SUCCESS;
	if (Thread == 0) return SUCCESS;
	Thread->Step.Mode = STEP_IN;
	return SUCCESS;
};

GLOBAL_FUNCTION(ResumeThread, 0) {
	if (SelectedThread) {
		if (SelectedThread->Step.Mode == PAUSED) {
			SelectedThread->Step.Mode = RUNNING;
			pthread_cond_signal(&SelectedThread->Paused);
			//pthread_mutex_unlock(&SelectedThread->Lock);
		};
	};
};

GLOBAL_FUNCTION(StepInThread, 0) {
	if (SelectedThread) {
		if (SelectedThread->Step.Mode == PAUSED) {
			SelectedThread->Step.Mode = STEP_IN;
			pthread_cond_signal(&SelectedThread->Paused);
			//pthread_mutex_unlock(&SelectedThread->Lock);
		};
	};
};

GLOBAL_FUNCTION(StepOverThread, 0) {
	if (SelectedThread) {
		if (SelectedThread->Step.Mode == PAUSED) {
			SelectedThread->Step.Mode = STEP_OVER;
			SelectedThread->Step.Instance = SelectedThread->Instance;
			pthread_cond_signal(&SelectedThread->Paused);
			//pthread_mutex_unlock(&SelectedThread->Lock);
		};
	};
};

GLOBAL_FUNCTION(PauseThread, 0) {
	if (SelectedThread) {
		if (SelectedThread->Step.Mode != PAUSED) {
			SelectedThread->Step.Mode = STEP_IN;
		};
	};
};

GLOBAL_FUNCTION(UpStack, 0) {
	if (SelectedThread) {
		pthread_mutex_lock(&SelectedThread->Lock);
		if (SelectedInstance && SelectedInstance->Up) {
			SelectedInstance = SelectedInstance->Up;
			printf("%s\n", SelectedInstance->Function->Name);
			pthread_mutex_unlock(&SelectedThread->Lock);
			return SUCCESS;
		} else {
			pthread_mutex_unlock(&SelectedThread->Lock);
			return FAILURE;
		};
	};
};

GLOBAL_FUNCTION(DownStack, 0) {
	if (SelectedThread) {
		pthread_mutex_lock(&SelectedThread->Lock);
		if (SelectedInstance && SelectedInstance->Down) {
			SelectedInstance = SelectedInstance->Down;
			printf("%s\n", SelectedInstance->Function->Name);
			pthread_mutex_unlock(&SelectedThread->Lock);
			return SUCCESS;
		} else {
			pthread_mutex_unlock(&SelectedThread->Lock);
			return FAILURE;
		};
	};
};

GLOBAL_FUNCTION(FindLocal, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	if (SelectedThread) {
		pthread_mutex_lock(&SelectedThread->Lock);
		if ((SelectedInstance == 0) || (SelectedThread->Step.Mode != PAUSED)) {
			pthread_mutex_unlock(&SelectedThread->Lock);
			return FAILURE;
		};
		const char *Name = Std$String$flatten(Args[0].Val);
		for (variable_t *Local = SelectedInstance->Function->Locals; Local; Local = Local->Next) {
			if (!strcmp(Local->Name, Name)) {
				Std$Object$t **Address = SelectedInstance->Locals[Local->Index];
				if (Address[0]) {
					Result->Val = *(Result->Ref = Address);
					pthread_mutex_unlock(&SelectedThread->Lock);
					return SUCCESS;
				};
			};
		};
		module_t *Module = SelectedInstance->Function->Module;
		Std$Object$t **Address = Agg$StringTable$get(Module->Globals, Name, strlen(Name));
		if (Address) {
			Result->Val = *(Result->Ref = Address);
			pthread_mutex_unlock(&SelectedThread->Lock);
			return SUCCESS;
		};
		pthread_mutex_unlock(&SelectedThread->Lock);
	};
	return FAILURE;
};

GLOBAL_FUNCTION(ListThreads, 0) {
	pthread_mutex_lock(&GlobalsLock);
	for (int I = 0; I < MAX_THREADS; ++I) {
		thread_t *Thread = Threads[I];
		if (Thread) {
			pthread_mutex_lock(&Thread->Lock);
			printf("Thread %d [%d]", Thread->Index, Thread->Step.Mode);
			if (Thread->Instance && Thread->Instance->Function) {
				printf(" %s", Thread->Instance->Function->Name);
			};
			printf("\n");
			pthread_mutex_unlock(&Thread->Lock);
		}
	};
	pthread_mutex_unlock(&GlobalsLock);
	return SUCCESS;
};

GLOBAL_FUNCTION(SelectThread, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	int Index = Std$Integer$get_small(Args[0].Val);
	if ((Index < 0) || (Index >= MAX_THREADS)) {
		Result->Val = Std$String$new("No such thread");
		return MESSAGE;
	};
	pthread_mutex_lock(&GlobalsLock);
	thread_t *Thread = Threads[Index];
	if (Thread == 0) {
		pthread_mutex_unlock(&GlobalsLock);
		Result->Val = Std$String$new("No such thread");
		return MESSAGE;
	} else if (Thread == SelectedThread) {
		pthread_mutex_unlock(&GlobalsLock);
		return SUCCESS;
	} else {
		SelectedThread = Thread;
		pthread_mutex_lock(&Thread->Lock);
		SelectedInstance = Thread->Instance;
		Std$Object$t *Line = Agg$List$find_node(SelectedInstance->Function->Module->Source, Thread->LineNo)->Value;
		printf("Thread %d [%d]\n", Thread->Index, Thread, Thread->Step.Mode);
		printf("%d: %s\n", Thread->LineNo, Std$String$flatten(Line));
		pthread_mutex_unlock(&Thread->Lock);
		pthread_mutex_unlock(&GlobalsLock);
		return SUCCESS;
	};
};

GLOBAL_FUNCTION(ListModules, 0) {
	pthread_mutex_lock(&GlobalsLock);
	for (int I = 0; I < MAX_MODULES; ++I) {
		module_t *Module = Modules[I];
		if (Module) {
			printf("Module %d => %s\n", I, Module->Name);
		} else {
			break;
		};
	};
	pthread_mutex_unlock(&GlobalsLock);
	return SUCCESS;
};

GLOBAL_FUNCTION(ModuleSource, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	int I = Std$Integer$get_small(Args[0].Val);
	if ((I < 0) || (I >= MAX_MODULES) || (Modules[I] == 0)) return FAILURE;
	module_t *Module = Modules[I];
	Result->Val = Module->Source;
	return SUCCESS;
};

GLOBAL_FUNCTION(SetBreakpoint, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	int I = Std$Integer$get_small(Args[0].Val);
	if ((I < 0) || (I >= MAX_MODULES) || (Modules[I] == 0)) return FAILURE;
	module_t *Module = Modules[I];
	int Line = Std$Integer$get_small(Args[1].Val);
	set_breakpoint(Module, Line);
	printf("Breakpoint @ %s:%d = %d\n", Module->Name, Line, Module->Breakpoints[Line - 1]);
	return SUCCESS;
};

GLOBAL_FUNCTION(ClearBreakpoint, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	int I = Std$Integer$get_small(Args[0].Val);
	if ((I < 0) || (I >= MAX_MODULES) || (Modules[I] == 0)) return FAILURE;
	module_t *Module = Modules[I];
	int Line = Std$Integer$get_small(Args[1].Val);
	clear_breakpoint(Module, Line);
	printf("Breakpoint @ %s:%d\n", Module->Name, Line, Module->Breakpoints[Line - 1]);
	return SUCCESS;
};

GLOBAL_FUNCTION(Init, 0) {
	static Wrapl$Loader_debugger Debugger[1] = {{
		.add_module = add_module,
		.add_line = add_line,
		.add_global = add_global,
		.add_function = add_function,
		.set_locals_offset = set_locals_offset,
		.add_local = add_local,
		.enter_function = enter_function,
		.exit_function = exit_function,
		.alloc_local = alloc_local,
		.break_line = break_line
	}};
	memset(Threads, 0, sizeof(Threads));
	pthread_key_create(&ThreadKey, thread_exit);
	pthread_setspecific(ThreadKey, (void *)0xFFFFFFFF);
	Wrapl$Loader$enable_debug(Debugger);
	return SUCCESS;
};
