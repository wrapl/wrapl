#include "libriva.h"

__thread handler_t *CurrentHandler = 0;

__attribute__ ((returns_twice)) void *handler_push(handler_t *Handler) {
	Handler->Prev = CurrentHandler;
	asm("movl\t%%esp, %0" : "=m" (Handler->EIP));
	register int Temp;
	asm("movl\t(%%esp), %0\n\tmovl\t%0, %1" : "=r" (Temp), "=m" (Handler->ESP));
	CurrentHandler = Handler;
	return 0;
};

void handler_pop(handler_t *Handler) {
	CurrentHandler = Handler->Prev;
};

__attribute__ ((noreturn)) void exception_raise(void *Exception) {
	asm("movl\t%0, %%esp" : : "m" (CurrentHandler->ESP));
	asm("movl\t%0, %%eax" : : "m" (Exception) : "%eax");
	asm("jmpl\t*%0" : : "m" (CurrentHandler->EIP));
};

void exception_init(void) {
	module_t *Module = module_new("Riva/Exception");
	module_add_alias(Module, "library:/Riva/Exception");
	module_export(Module, "_push", 0, handler_push);
	module_export(Module, "_pop", 0, handler_pop);
	module_export(Module, "_raise", 0, exception_raise);
};