#include <Std.h>
#include <Riva.h>
#include <Wrapl/Loader.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <Sys/Service.h>
#include <Riva/Module.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <string.h>
#include <stdio.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#define DEBUG 0;//printf("%s:%d\n", __func__, __LINE__);

typedef struct variable_t variable_t;
typedef struct thread_t thread_t;
typedef struct Wrapl$Loader$debug_module module_t;
typedef struct Wrapl$Loader$debug_state state_t;
typedef struct Wrapl$Loader$debug_function function_t;
typedef struct Wrapl$Loader$debug_instance instance_t;
typedef struct watch_t watch_t;

struct variable_t {
	const char *Name;
	int Index;
	variable_t *Next;
};

struct Wrapl$Loader$debug_module {
	const char *Name;
	int Index;
	int NoOfLines;
	GtkSourceBuffer *Source;
	GtkTreePath *ModulePath, *GlobalsPath;
	int MaxBreakpoint;
	char *Breakpoints;
};

struct Wrapl$Loader$debug_function {
	const char *Name;
	module_t *Module;
	int NoOfLocals;
	int LocalsOffset;
	int LineNo;
	variable_t *Locals, *LastLocal;
};

struct Wrapl$Loader$debug_instance {
	instance_t *Up;
	struct thread_t *Thread;
	function_t *Function;
	GtkTreePath *Path;
	Std$Object$t **Locals;
};

struct thread_t {
	instance_t *Instance;
	pthread_cond_t Paused;
	pthread_mutex_t Lock;
	GtkTreeStore *Locals;
	GtkTreePath *Path;
	Std$Object$t *Value;
	int LineNo;
	char Breakinsts[NUM_INSTTYPES];
	struct {
		enum {RUNNING, STEP_IN, STEP_OVER, STEP_OUT, RUN_TO, PAUSED} Mode;
		union {instance_t *Instance; module_t *Module;};
		int LineNo;
	} Step;
};

struct watch_t {
	GtkTreeStore *Values;
};

static pthread_key_t ThreadKey;

static GtkSourceView *SourceView;
static GtkSourceLanguage *SourceLanguage;
static GtkSourceStyleScheme *StyleScheme;
static GtkTreeStore *Globals;
static GtkTreeView *ModulesView, *GlobalsView, *LocalsView;
static GtkTreeSelection *ModulesSelection;
static GtkListStore *Modules, *Threads;
static GtkTextTagTable *SourceTags;
static GtkTextTag *SourceTag, *PausedTag;
static GtkStatusbar *StatusBar;

static thread_t *DisplayedThread = 0;
static module_t *DisplayedModule = 0;
static int BreakOnNewModule = 1;

enum {COL_LINENO, COL_BREAK, COL_TEXT};

static GdkPixbuf *BreakpointPixbuf;
static GdkPixbuf *NoBreakpointPixbuf;

static void thread_exit(thread_t *Thread) {
	GtkTreeIter Iter;
	gtk_tree_model_get_iter(Threads, &Iter, Thread->Path);
	gtk_list_store_set(Threads, &Iter, 0, GTK_STOCK_MEDIA_STOP, -1);
};

static module_t *add_module(const char *Name) {
//	printf("Adding module: %s\n", Name);
	gdk_threads_enter();
	module_t *Module = new(module_t);
	Module->Name = Name;
	Module->Source = gtk_source_buffer_new(SourceTags);
	gtk_source_buffer_set_language(Module->Source, SourceLanguage);
	gtk_source_buffer_set_style_scheme(Module->Source, StyleScheme);
	gtk_text_view_set_buffer(SourceView, Module->Source);
	GtkTreeIter Iter;
	gtk_list_store_append(Modules, &Iter);
	gtk_list_store_set(Modules, &Iter, 0, Name, 1, Module, -1);
	Module->ModulePath = gtk_tree_model_get_path(Modules, &Iter);
	DisplayedModule = Module;
	gtk_tree_store_append(Globals, &Iter, 0);
	gtk_tree_store_set(Globals, &Iter, 0, Module->Name, 1, 0, 2, 0, 3, 0, 5, -1, -1);
	Module->GlobalsPath = gtk_tree_model_get_path(Globals, &Iter);
	//Module->Breakpoints = Riva$Memory$alloc_atomic(1);
	//Module->Breakpoints[0] = BreakOnNewModule;
	//Module->MaxBreakpoint = 1;
	gdk_threads_leave();
	return Module;
};

static void add_line(module_t *Module, const char *Line) {
// 	printf("Adding line to %s: %s\n", Module->Name, Line);
	gdk_threads_enter();
	GtkTextIter Iter;
	gtk_text_buffer_get_end_iter(Module->Source, &Iter);
	gtk_text_buffer_insert_pixbuf(Module->Source, &Iter, NoBreakpointPixbuf);
	gtk_text_buffer_insert_with_tags(Module->Source, &Iter, Line, strlen(Line), SourceTag, 0);
	Module->NoOfLines++;
	gdk_threads_leave();
};

static void add_global(module_t *Module, const char *Name, Std$Object$t **Address) {
//	printf("Adding global variable to %s: %s @ %x\n", Module->Name, Name, Address);
	gdk_threads_enter();
	GtkTreeIter Parent, Iter;
	gtk_tree_model_get_iter(Globals, &Parent, Module->GlobalsPath);
	gtk_tree_store_append(Globals, &Iter, &Parent);
	gtk_tree_store_set(Globals, &Iter, 0, Name, 1, "NIL", 2, Address, 3, Std$Object$Nil, 5, -1, -1);
	gdk_threads_leave();
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

static GAsyncQueue *PauseQueue;

static void unpause_thread(thread_t *Thread) {DEBUG
	pthread_cond_signal(&Thread->Paused);
	pthread_mutex_unlock(&Thread->Lock);
};

SYMBOL($AS, "@");
/*static const char *to_string(Std$Object$t *Value) {
	Std$Function$result Result;
	const char *Module, *Import;
	if (Std$Function$call($AS, 2, &Result, Value, 0, Std$String$T, 0) < FAILURE) {
		return Std$String$flatten(Result.Val);
	} else if (Riva$Module$lookup(Value, &Module, &Import)) {
		char *Result = 0;
		asprintf(&Result, "%s.%s", Module, Import);
		return Result;
	} else if (Riva$Module$lookup(Value->Type, &Module, &Import)) {
		char *Result = 0;
		asprintf(&Result, "<%s.%s @ #%x>", Module, Import, Value);
		return Result;
	} else {
		return "<value>";
	};
};*/

static const char *to_string(Std$Object$t *Value) {
	Std$Function$result Result;
	if (Std$Function$call($AS, 2, &Result, Value, 0, Std$String$T, 0) < FAILURE) {
		return Std$String$flatten(Result.Val);
	} else {
		return 0;
	};
};

static void display_variable(GtkTreeModel *Variables, GtkTreeIter *Iter, Std$Object$t *Value, Std$Object$t **Address) {
	//printf("Address = 0x%x, Value = 0x%x\n", Address, Value);
	const char *String;
	if (Value->Type == Agg$Table$T) {
		size_t OldGeneration, CurrentGeneration = Agg$Table$generation(Value);
		gtk_tree_model_get(Variables, Iter, 5, &OldGeneration, -1);
		//printf("OldGeneration = %d, CurrentGeneration = %d\n", OldGeneration, CurrentGeneration);
		if (OldGeneration != CurrentGeneration) {
			gtk_tree_store_set(Variables, Iter, 1, "{...}", 3, Value, 5, CurrentGeneration, -1);
			GtkTreeIter Child;
			while (gtk_tree_model_iter_children(Variables, &Child, Iter)) gtk_tree_store_remove(Variables, &Child);
			if (Agg$Table$size(Value) < 100) {
				Agg$Table$trav *Trav = Agg$Table$trav_new();
				for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Value); Node; Node = Agg$Table$trav_next(Trav)) {
					gtk_tree_store_append(Variables, &Child, Iter);
					gtk_tree_store_set(Variables, &Child, 0, to_string(Agg$Table$node_key(Node)) ?: "<key>", 5, -1, -1);
					Std$Object$t **NodeAddress = Agg$Table$node_value_ref(Node);
					display_variable(Variables, &Child, *NodeAddress, NodeAddress);
				};
			};
		};
	} else if (Value->Type == Agg$List$T) {
		Agg$List$t *List = (Agg$List$t *)Value;
		GtkTreeIter Child;
		while (gtk_tree_model_iter_children(Variables, &Child, Iter)) gtk_tree_store_remove(Variables, &Child);
		if (List->Length) {
			gtk_tree_store_set(Variables, Iter, 1, "[...]", 3, Value, 5, 0, -1);
			if (List->Length < 20) {
				size_t Index = 0;
				for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
					asprintf(&String, "%d", ++Index);
					gtk_tree_store_append(Variables, &Child, Iter);
					gtk_tree_store_set(Variables, &Child, 0, String, 5, -1, -1);
					Std$Object$t **NodeAddress = &Node->Value;
					display_variable(Variables, &Child, *NodeAddress, NodeAddress);
				};
			};
		} else {
			gtk_tree_store_set(Variables, Iter, 1, "[]", 3, Value, 5, 0, -1);
		};
	} else if (String = to_string(Value)) {
		gtk_tree_store_set(Variables, Iter, 1, String, 2, Address, 3, Value, -1);
	} else if (Address && *Address == Value && gtk_tree_model_iter_has_child(Variables, Iter)) {
	} else {
		//printf("Refreshing object\n");
		gtk_tree_store_set(Variables, Iter, 1, "<...>", 3, Value, -1);
		GtkTreeIter Child;
		while (gtk_tree_model_iter_children(Variables, &Child, Iter)) gtk_tree_store_remove(Variables, &Child);
		const Std$Array$t *Fields = Value->Type->Fields;
		for (int I = 0; I < Fields->Length.Value; ++I) {
			gtk_tree_store_append(Variables, &Child, Iter);
			gtk_tree_store_set(Variables, &Child,
				0, Std$String$flatten(Std$Symbol$get_name(Fields->Values[I])),
				1, "?",
				2, (Std$Object$t **)Value + I + 1,
				3, 0,
				5, -1,
				-1
			);
		};
	};
};

static void variable_expanded(GtkTreeView *View, GtkTreeIter *Iter, GtkTreePath *Path, void *Data) {
	GtkTreeModel *Variables = gtk_tree_view_get_model(View);
	Std$Object$t **Address, *Value;
	const char *Name;
	int IsFunction;
	gtk_tree_model_get(Variables, Iter, 0, &Name, 2, &Address, 3, &Value, 4, &IsFunction, -1);
	//printf("Name, Address, Value = %s, %x, %x, %x\n", Name, Address, Address ? Address[0] : 0, Value);
	GtkTreeIter Child;
	if (gtk_tree_model_iter_children(Variables, &Child, Iter)) do {
		gtk_tree_model_get(Variables, &Child, 0, &Name, 2, &Address, 3, &Value, -1);
		if (Address) {
			if (*Address) display_variable(Variables, &	Child, *Address, Address);
		} else if (Value) {
			display_variable(Variables, &Child, Value, Address);
		};
	} while (gtk_tree_model_iter_next(Variables, &Child));
};

static gboolean refresh_variable(GtkTreeModel *Variables, GtkTreePath *Path, GtkTreeIter *Iter, GtkTreeView *View) {
	GtkTreePath *Parent = gtk_tree_path_copy(Path);
	gtk_tree_path_up(Parent);
	if (!gtk_tree_view_row_expanded(View, Parent)) return FALSE;
	Std$Object$t **Address, *Value;
	const char *Name;
	gtk_tree_model_get(Variables, Iter, 0, &Name, 2, &Address, 3, &Value, -1);
	//printf("Name, Address, Value = %s, %x, %x, %x\n", Name, Address, Address ? Address[0] : 0, Value);
	if (Address) {
		if (*Address) display_variable(Variables, Iter, *Address, Address);
	} else if (Value) {
		display_variable(Variables, Iter, Value, Address);
	};
	return FALSE;
};

static gboolean update_variable(GtkTreeModel *Variables, GtkTreePath *Path, GtkTreeIter *Iter, GtkTreeView *View) {
	GtkTreePath *Parent = gtk_tree_path_copy(Path);
	gtk_tree_path_up(Parent);
	if (!gtk_tree_view_row_expanded(View, Parent)) return FALSE;
	Std$Object$t **Address, *Value;
	const char *Name;
	gtk_tree_model_get(Variables, Iter, 0, &Name, 2, &Address, 3, &Value, -1);
	//printf("Name, Address, Value = %s, %x, %x, %x\n", Name, Address, Address ? Address[0] : 0, Value);
	if (Address) {
		if (*Address && *Address != Value) display_variable(Variables, Iter, *Address, Address);
	} else if (Value) {
		display_variable(Variables, Iter, Value, Address);
	};
	return FALSE;
};

static void update_display(void) {DEBUG
//	printf("Updating display...\n");
	instance_t *Instance = DisplayedThread->Instance;
	function_t *Function = Instance->Function;
	module_t *Module = Function->Module;
	GtkTextIter LineBeg, LineEnd;
	//printf("Applying tag to line %d\n", DisplayedThread->LineNo);
//	printf("Instance = %x\n", Instance);
//	printf("Function = %x\n", Function);
	gtk_text_view_set_buffer(SourceView, Module->Source);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineBeg, DisplayedThread->LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineEnd, DisplayedThread->LineNo);
	gtk_text_buffer_apply_tag(Module->Source, PausedTag, &LineBeg, &LineEnd);	
	gtk_text_view_scroll_to_iter(SourceView, &LineBeg, 0.0, TRUE, 0.0, 0.5);
	gtk_tree_selection_select_path(ModulesSelection, Module->ModulePath);
	gtk_tree_view_scroll_to_cell(ModulesView, Module->ModulePath, 0, TRUE, 0.5, 0.5);
	DisplayedModule = Module;
	gtk_tree_model_foreach(Globals, update_variable, GlobalsView);
	gtk_tree_model_foreach(DisplayedThread->Locals, update_variable, LocalsView);
	gtk_statusbar_remove_all(StatusBar, 0);
	if (DisplayedThread->Value) {
		gtk_statusbar_push(StatusBar, 0, to_string(DisplayedThread->Value));
	};
};

static void display_thread(thread_t *Thread) {DEBUG
//	printf("Thread, Thread->Locals = %x, %x\n", Thread, Thread->Locals);
	gtk_tree_view_set_model(LocalsView, Thread->Locals);
	DisplayedThread = Thread;
};

static void break_thread(thread_t *Thread, Std$Object$t *Value) {DEBUG
	pthread_mutex_lock(&Thread->Lock);
	Thread->Step.Mode = PAUSED;
	Thread->Value = Value;
	GtkTreeIter Iter;
	gtk_tree_model_get_iter(Threads, &Iter, Thread->Path);
	gtk_list_store_set(Threads, &Iter, 0, GTK_STOCK_MEDIA_PAUSE, -1);
//	printf("PauseQueue = %x\n", PauseQueue);
	g_async_queue_push(PauseQueue, Thread);
	g_main_context_wakeup(0);
//	printf("Thread resuming -> %x.\n", Thread->Instance);
	pthread_cond_wait(&Thread->Paused, &Thread->Lock);
	gtk_tree_model_get_iter(Threads, &Iter, Thread->Path);
	gtk_list_store_set(Threads, &Iter, 0, GTK_STOCK_MEDIA_PLAY, -1);
	pthread_mutex_unlock(&Thread->Lock);
};

static void thread_selection_changed(GtkTreeSelection *Selection, void *User) {
	GtkTreeIter Iter;
	if (gtk_tree_selection_get_selected(Selection, 0, &Iter)) {
		thread_t *Thread;
		gtk_tree_model_get(Threads, &Iter, 2, &Thread, -1);
		if (DisplayedThread != Thread) {
			display_thread(Thread);
			if (Thread->Step.Mode == PAUSED) update_display();
		};
	};
};

static void *enter_function(function_t *Function, state_t *State) {DEBUG
//	printf("Module = %x\n", Function->Module);
//	printf("Entering function: %s.%x @ %x\n", Function->Module->Name, Function, State);
	thread_t *Thread = pthread_getspecific(ThreadKey);
//	printf("Displayed thread = %x, thread = %x\n", DisplayedThread, Thread);
	if (Thread == (thread_t *)0xFFFFFFFF) return (instance_t *)0xFFFFFFFF;
	if (Thread == 0) {
		gdk_threads_enter();
		//printf("Discovered new thread\n");
		Thread = new(thread_t);
		Thread->Locals = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_INT);
		Thread->LineNo = Function->LineNo;
		pthread_setspecific(ThreadKey, Thread);
		pthread_cond_init(&Thread->Paused, 0);
		pthread_mutex_init(&Thread->Lock, 0);
		GtkTreeIter Iter;
		gtk_list_store_append(Threads, &Iter);
		Thread->Path = gtk_tree_model_get_path(Threads, &Iter);
		char *ID;
		asprintf(&ID, "0x%8x", Thread);
		gtk_list_store_set(Threads, &Iter, 0, GTK_STOCK_MEDIA_PLAY, 1, ID, 2, Thread, -1);

		Thread->Breakinsts[INSTTYPE_SEND] = 1;

		gdk_threads_leave();
	};
	if (Thread == DisplayedThread) gdk_threads_enter();
	instance_t *Instance = new(instance_t);
	Instance->Function = Function;
	Instance->Thread = Thread;
	Instance->Up = Thread->Instance;
//	printf("LocalsOffset = %d\n", Function->LocalsOffset);
	Instance->Locals = (char *)State + Function->LocalsOffset;
	GtkTreeIter Parent;
	gtk_tree_store_prepend(Thread->Locals, &Parent, 0);
	gtk_tree_store_set(Thread->Locals, &Parent, 0, Function->Name, 3, Instance, 4, -1, -1);
	Instance->Path = gtk_tree_model_get_path(Thread->Locals, &Parent);
	int I = 0;
	for (variable_t *Local = Function->Locals; Local; Local = Local->Next) {
		GtkTreeIter Iter;
		gtk_tree_store_append(Thread->Locals, &Iter, &Parent);
		//printf("Adding local: %s -> %x\n", Local->Name, Instance->Locals[I]);
		gtk_tree_store_set(Thread->Locals, &Iter, 0, Local->Name, 1, "?", 2, Instance->Locals[I++], 3, 0, 5, -1, -1);
	};
	if (Function->Locals) gtk_tree_view_expand_row(LocalsView, gtk_tree_model_get_path(Thread->Locals, &Parent), FALSE);
	Thread->Instance = Instance;
//	printf("Returning instance %x\n", Instance);
	if (Thread == DisplayedThread) gdk_threads_leave();
	if (DisplayedThread == 0) break_thread(Thread, Std$Object$Nil);
	return Instance;
};

static void exit_function(instance_t *Instance) {DEBUG
	if (Instance == (instance_t *)0xFFFFFFFF) return;
//	printf("Leaving function %x @ %x\n", Instance->Function, Instance);
// 	asm("int3");
 	thread_t *Thread = Instance->Thread;
 	if (Thread == DisplayedThread) gdk_threads_enter();
//	printf("Thread = %x, Thread->Instance = %x\n", Thread, Thread->Instance);
	GtkTreeIter Parent, Iter;
	if (gtk_tree_model_get_iter_first(Thread->Locals, &Iter)) {
		instance_t *Instance2;
		do {
			gtk_tree_model_get(Thread->Locals, &Iter, 3, &Instance2, -1);
		} while (gtk_tree_store_remove(Thread->Locals, &Iter) && (Instance2 != Instance));
	};

/*
	gtk_tree_model_get_iter(Thread->Locals, &Parent, Thread->Path);
	while (gtk_tree_model_iter_children(Thread->Locals, &Iter, &Parent)) gtk_tree_store_remove(Thread->Locals, &Iter);
	gtk_tree_store_remove(Thread->Locals, &Parent);
*/
	Thread->Instance = Instance->Up;
	if (Thread == DisplayedThread) gdk_threads_leave();
};

static void alloc_local(instance_t *Instance, int Index, Std$Object$t **Address) {DEBUG
	if (Instance == (instance_t *)0xFFFFFFFF) return;
	thread_t *Thread = Instance->Thread;
	if (Thread == DisplayedThread) gdk_threads_enter();
	GtkTreeIter Parent, Iter;
	if (gtk_tree_model_get_iter(Thread->Locals, &Parent, Instance->Path)) {
	//	gtk_tree_model_iter_nth_child(Thread->Locals, &Parent, 0, Instance->Index);
		gtk_tree_model_iter_nth_child(Thread->Locals, &Iter, &Parent, Index);
		gtk_tree_store_set(Thread->Locals, &Iter, 2, Address, -1);
		
	//	const char *Name;
	//	gtk_tree_model_get(Thread->Locals, &Iter, 0, &Name, -1);
	//	printf("Allocating local: %s -> %x\n", Name, Address);
	} else {
		printf("Could not find locals\n");
	};
	if (Thread == DisplayedThread) gdk_threads_leave();
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
	Module->Breakpoints[LineNo - 1] = 1;
	GtkTextIter StartIter, EndIter;
	gtk_text_buffer_get_iter_at_line(Module->Source, &StartIter, LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &EndIter, LineNo - 1);
	gtk_text_iter_forward_char(&EndIter);
	gtk_text_buffer_delete(Module->Source, &StartIter, &EndIter);
	gtk_text_buffer_insert_pixbuf(Module->Source, &StartIter, BreakpointPixbuf);
};

static inline void clear_breakpoint(module_t *Module, int LineNo) {
	Module->Breakpoints[LineNo - 1] = 0;
	GtkTextIter StartIter, EndIter;
	gtk_text_buffer_get_iter_at_line(Module->Source, &StartIter, LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &EndIter, LineNo - 1);
	gtk_text_iter_forward_char(&EndIter);
	gtk_text_buffer_delete(Module->Source, &StartIter, &EndIter);
	gtk_text_buffer_insert_pixbuf(Module->Source, &StartIter, NoBreakpointPixbuf);
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

extern char UI_Start[];
extern char UI_End[];

static void module_selection_changed(GtkTreeSelection *Selection, void *User) {
	GtkTreeIter Iter;
	if (gtk_tree_selection_get_selected(Selection, 0, &Iter)) {
		module_t *Module;
		gtk_tree_model_get(Modules, &Iter, 1, &Module, -1);
		gtk_text_view_set_buffer(SourceView, Module->Source);
		DisplayedModule = Module;
	};
};

extern guint8 BreakpointData[];
extern guint8 NoBreakpointData[];

static gboolean pause_queue_prepare(GSource *Source, gint *Timeout) {
	Timeout[0] = -1;
//	printf("Queue length = %d\n", g_async_queue_length(PauseQueue));
	return g_async_queue_length(PauseQueue) > 0;
};

static gboolean pause_queue_check(GSource *Source) {
//	printf("Queue length = %d\n", g_async_queue_length(PauseQueue));
	return g_async_queue_length(PauseQueue) > 0;
};

static gboolean pause_queue_dispatch(GSource *Source, GSourceFunc Callback, void *Data) {
	thread_t *Thread = g_async_queue_pop(PauseQueue);
//	printf("Dispatch thread lock signal... %x\n", Thread);
	pthread_mutex_lock(&Thread->Lock);
	if (DisplayedThread == 0) {
		gdk_threads_enter();
		display_thread(Thread);
		gdk_threads_leave();
	};
	if (DisplayedThread == Thread) {
		gdk_threads_enter();
		update_display();
		gdk_threads_leave();
	};
	return TRUE;
};

static void run_clicked(GtkButton *Button, void *User) {
	if (DisplayedThread == 0) return;
	if (DisplayedThread->Step.Mode != PAUSED) return;
	instance_t *Instance = DisplayedThread->Instance;
	function_t *Function = Instance->Function;
	module_t *Module = Function->Module;
	GtkTextIter LineBeg, LineEnd;
//	printf("Removing tag from line %d\n", DisplayedThread->LineNo);
//	printf("Instance = %x\n", Instance);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineBeg, DisplayedThread->LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineEnd, DisplayedThread->LineNo);
	gtk_text_buffer_remove_tag(Module->Source, PausedTag, &LineBeg, &LineEnd);
	DisplayedThread->Step.Mode = RUNNING;
	pthread_cond_signal(&DisplayedThread->Paused);
//	printf("Resuming thread...\n");
	pthread_mutex_unlock(&DisplayedThread->Lock);
};

static void step_into_clicked(GtkButton *Button, void *User) {
	if (DisplayedThread == 0) return;
	if (DisplayedThread->Step.Mode != PAUSED) return;
	instance_t *Instance = DisplayedThread->Instance;
	function_t *Function = Instance->Function;
	module_t *Module = Function->Module;
	GtkTextIter LineBeg, LineEnd;
//	printf("Removing tag from line %d\n", DisplayedThread->LineNo);
//	printf("Instance = %x\n", Instance);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineBeg, DisplayedThread->LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineEnd, DisplayedThread->LineNo);
	gtk_text_buffer_remove_tag(Module->Source, PausedTag, &LineBeg, &LineEnd);
	DisplayedThread->Step.Mode = STEP_IN;
	pthread_cond_signal(&DisplayedThread->Paused);
//	printf("Resuming thread...\n");
	pthread_mutex_unlock(&DisplayedThread->Lock);
};

static void step_over_clicked(GtkButton *Button, void *User) {
	if (DisplayedThread == 0) return;
	if (DisplayedThread->Step.Mode != PAUSED) return;
	instance_t *Instance = DisplayedThread->Instance;
	function_t *Function = Instance->Function;
	module_t *Module = Function->Module;
	GtkTextIter LineBeg, LineEnd;
//	printf("Removing tag from line %d\n", DisplayedThread->LineNo);
//	printf("Instance = %x\n", Instance);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineBeg, DisplayedThread->LineNo - 1);
	gtk_text_buffer_get_iter_at_line(Module->Source, &LineEnd, DisplayedThread->LineNo);
	gtk_text_buffer_remove_tag(Module->Source, PausedTag, &LineBeg, &LineEnd);
	DisplayedThread->Step.Mode = STEP_OVER;
	DisplayedThread->Step.Instance = Instance;
	DisplayedThread->Step.LineNo = DisplayedThread->LineNo;
	pthread_cond_signal(&DisplayedThread->Paused);
//	printf("Resuming thread...\n");
	pthread_mutex_unlock(&DisplayedThread->Lock);
};

static void pause_clicked(GtkButton *Button, void *User) {
	if (DisplayedThread == 0) return;
	if (DisplayedThread->Step.Mode == PAUSED) return;
//	printf("Pausing thread %x\n", DisplayedThread);
	DisplayedThread->Step.Mode = STEP_IN;
};

GLOBAL_FUNCTION(Break, 0) {
	thread_t *Thread = pthread_getspecific(ThreadKey);
	if (Thread == (thread_t *)0xFFFFFFFF) return SUCCESS;
	if (Thread == 0) return SUCCESS;
	Thread->Step.Mode = STEP_IN;
	return SUCCESS;
};

static void breakpoint_clicked(GtkButton *Button, void *User) {
	if (DisplayedModule == 0) return;
	GtkTextIter Iter;
	gtk_text_buffer_get_iter_at_mark(
		DisplayedModule->Source, &Iter,
		gtk_text_buffer_get_insert(DisplayedModule->Source)
	);
	int LineNo = gtk_text_iter_get_line(&Iter) + 1;
	if (is_breakpoint(DisplayedModule, LineNo)) {
		clear_breakpoint(DisplayedModule, LineNo);
	} else {
		set_breakpoint(DisplayedModule, LineNo);
	};
};

static void refresh_clicked(GtkButton *Button, void *User) {
	gtk_tree_model_foreach(Globals, refresh_variable, GlobalsView);
	if (DisplayedThread) gtk_tree_model_foreach(DisplayedThread->Locals, refresh_variable, LocalsView);
};

static void initialize(void) {
	static GSourceFuncs pause_queue_functions = {
		pause_queue_prepare,
		pause_queue_check,
		pause_queue_dispatch,
		0,
	};
	PauseQueue = g_async_queue_new();
	g_source_attach(g_source_new(&pause_queue_functions, sizeof(GSource)), 0);
	GladeXML *Xml = glade_xml_new_from_buffer(UI_Start, UI_End - UI_Start, 0, 0);

	PangoFontDescription *FontDesc = pango_font_description_from_string("Monaco 9");
	//printf("FontDesc = %s\n", pango_font_description_to_string(FontDesc));

	SourceTags = gtk_text_tag_table_new();
	SourceTag = gtk_text_tag_new(0);
	//g_object_set(SourceTag, "family", "Monospace", 0);
	g_object_set(SourceTag, "font-desc", FontDesc, "pixels-above-lines", 0, "pixels-below-lines", 0, 0);
	gtk_text_tag_table_add(SourceTags, SourceTag);
	PausedTag = gtk_text_tag_new(0);
	g_object_set(PausedTag, "font-desc", FontDesc, "background", "yellow", "pixels-above-lines", 0, "pixels-below-lines", 0, 0);
	gtk_text_tag_table_add(SourceTags, PausedTag);
	SourceView = gtk_source_view_new();
	gtk_container_add(glade_xml_get_widget(Xml, "SourceScrolledWindow"), SourceView);
	gtk_text_view_set_editable(SourceView, FALSE);
	gtk_source_view_set_show_line_numbers(SourceView, TRUE);
	gtk_source_view_set_tab_width(SourceView, 4);
	
	GtkSourceLanguageManager *LanguageManager = gtk_source_language_manager_get_default();
	SourceLanguage = gtk_source_language_manager_get_language(LanguageManager, "wrapl");
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();
	StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, "wrapl");
	//StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, "wombat");
	gtk_widget_modify_font(GTK_WIDGET(SourceView), FontDesc);

	BreakpointPixbuf = gdk_pixbuf_new_from_inline(-1, BreakpointData, FALSE, 0);
	NoBreakpointPixbuf = gdk_pixbuf_new_from_inline(-1, NoBreakpointData, FALSE, 0);

	GtkCellRenderer *Renderer;
	GtkTreeViewColumn *Column;
	Modules = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	ModulesView = glade_xml_get_widget(Xml, "ModulesView");
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("File", Renderer, "text", 0, 0);
	gtk_tree_view_append_column(ModulesView, Column);
	gtk_tree_view_set_model(ModulesView, Modules);
	GtkTreeSelection *Selection = ModulesSelection = gtk_tree_view_get_selection(ModulesView);
	g_signal_connect(Selection, "changed", module_selection_changed, 0);
	gtk_widget_modify_font(GTK_WIDGET(ModulesView), FontDesc);

	GtkWidget *ThreadsView = glade_xml_get_widget(Xml, "ThreadsView");
	Threads = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	Renderer = gtk_cell_renderer_pixbuf_new();
	Column = gtk_tree_view_column_new_with_attributes("State", Renderer, "stock-id", 0, 0);
	gtk_tree_view_append_column(ThreadsView, Column);
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("Thread", Renderer, "text", 1, 0);
	gtk_tree_view_append_column(ThreadsView, Column);
	gtk_tree_view_set_model(ThreadsView, Threads);
	Selection = gtk_tree_view_get_selection(ThreadsView);
	g_signal_connect(Selection, "changed", thread_selection_changed, 0);
	gtk_widget_modify_font(GTK_WIDGET(ThreadsView), FontDesc);

	GlobalsView = glade_xml_get_widget(Xml, "GlobalsView");
	Globals = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_INT);
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("Name", Renderer, "text", 0, 0);
	gtk_tree_view_append_column(GlobalsView, Column);
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("Value", Renderer, "text", 1, 0);
	gtk_tree_view_append_column(GlobalsView, Column);
	gtk_tree_view_set_model(GlobalsView, Globals);
	gtk_widget_modify_font(GTK_WIDGET(GlobalsView), FontDesc);
	g_signal_connect(GlobalsView, "row-expanded", variable_expanded, 0);

	LocalsView = glade_xml_get_widget(Xml, "LocalsView");
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("Name", Renderer, "text", 0, 0);
	gtk_tree_view_append_column(LocalsView, Column);
	Renderer = gtk_cell_renderer_text_new();
	Column = gtk_tree_view_column_new_with_attributes("Value", Renderer, "text", 1, 0);
	gtk_tree_view_append_column(LocalsView, Column);
	//gtk_tree_view_set_fixed_height_mode(LocalsView, TRUE);
	gtk_widget_modify_font(GTK_WIDGET(LocalsView), FontDesc);
	g_signal_connect(LocalsView, "row-expanded", variable_expanded, 0);

	StatusBar = glade_xml_get_widget(Xml, "Statusbar");

	g_signal_connect(glade_xml_get_widget(Xml, "StepIntoButton"), "clicked", step_into_clicked, 0);
	g_signal_connect(glade_xml_get_widget(Xml, "StepOverButton"), "clicked", step_over_clicked, 0);
	g_signal_connect(glade_xml_get_widget(Xml, "RunButton"), "clicked", run_clicked, 0);
	g_signal_connect(glade_xml_get_widget(Xml, "PauseButton"), "clicked", pause_clicked, 0);
	g_signal_connect(glade_xml_get_widget(Xml, "BreakpointButton"), "clicked", breakpoint_clicked, 0);
	g_signal_connect(glade_xml_get_widget(Xml, "RefreshButton"), "clicked", refresh_clicked, 0);

	gtk_widget_show_all(glade_xml_get_widget(Xml, "MainWindow"));
};

static void *debugger_thread(void *Arg) {
	DEBUG
	pthread_setspecific(ThreadKey, (void *)0xFFFFFFFF);
	DEBUG
	gdk_threads_enter();
	DEBUG
	gtk_main();
	DEBUG
	gdk_threads_leave();
	DEBUG
	return 0;
};

INITIAL() {
	static Wrapl$Loader$debugger Debugger[1] = {{
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

	pthread_key_create(&ThreadKey, thread_exit);
	initialize();
	pthread_t *Handle;
	pthread_create(&Handle, 0, debugger_thread, 0);
	Wrapl$Loader$enable_debug(Debugger);
};
