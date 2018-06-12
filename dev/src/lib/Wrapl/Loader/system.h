#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#include <Riva/Memory.h>

extern "C" {

extern Std$Type_t WraplT[];
extern Std$Type_t WraplPreT[];
extern Std$Type_t CodeT[];

extern void run_state();
extern void alloc_local();
extern void create_variadic();
extern void invoke_function();
extern int return_table[];
extern int return_table_debug[];
extern void invoke_limit();
extern void backtrack();
extern void send_message();
extern void resend_message();
extern void select_string();
extern void incorrect_type();
extern void incorrect_assign();

extern void invoke_backtrack();
extern void invoke_suspend();
extern void invoke_message();

extern void detect_cpu_features();

extern void debug_enter();
extern void debug_break();
extern void debug_message();
extern void debug_exit();

extern bool CmovSupport;

extern Std$Type_t IncorrectTypeMessageT[];
extern Std$Object_t IncorrectTypeMessage[];

};

#endif
