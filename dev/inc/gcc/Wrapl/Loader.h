#ifndef WRAPL_LOADER_H
#define WRAPL_LOADER_H

#include <Std/Object.h>

#define RIVA_MODULE Wrapl$Loader
#include <Riva-Header.h>

RIVA_STRUCT(debug_function);
RIVA_STRUCT(debug_instance);

RIVA_STRUCT(debug_state) {
	void *Run;
	void *Chain;
	void *Resume;
	Std$Object$t *Val;
	Std$Object$t **Ref;
	void *Code;
	void *Handler;
	Wrapl$Loader$debug_instance *Instance;
	char Locals[];
};

RIVA_STRUCT(debug_module);

typedef enum {INSTTYPE_DEFAULT, INSTTYPE_FAIL, INSTTYPE_RETURN, INSTTYPE_SUSPEND, INSTTYPE_SEND, NUM_INSTTYPES} insttype_t;

RIVA_STRUCT(debugger) {
	Wrapl$Loader$debug_module *(*add_module)(const char *Name);
	void (*add_line)(Wrapl$Loader$debug_module *Module, const char *Line);
	void (*add_global)(Wrapl$Loader$debug_module *Module, const char *Name, Std$Object$t **Address);
	Wrapl$Loader$debug_function *(*add_function)(Wrapl$Loader$debug_module *Module, int LineNo);
	void (*set_locals_offset)(Wrapl$Loader$debug_function *Function, int Offset);
	void (*add_local)(Wrapl$Loader$debug_function *Function, const char *Name, int Index);
	Wrapl$Loader$debug_instance *(*enter_function)(Wrapl$Loader$debug_function *Function, Wrapl$Loader$debug_state *State);
	void (*exit_function)(Wrapl$Loader$debug_instance *Instance);
	void (*alloc_local)(Wrapl$Loader$debug_instance *Instance, int Index, Std$Object$t **Address);
	void (*break_line)(Wrapl$Loader$debug_instance *Instance, int LineNo, insttype_t Type);
};

RIVA_CFUN(void, enable_debug, const Wrapl$Loader$debugger *);
RIVA_CFUN(void, disable_debug, void);

#undef RIVA_MODULE

#endif
