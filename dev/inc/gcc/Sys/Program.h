#ifndef SYS_PROGRAM_H
#define SYS_PROGRAM_H

#include <Std/Object.h>

#define RIVA_MODULE Sys$Program
#include <Riva-Header.h>

RIVA_STRUCT(stack_trace_t) {
	int Depth;
	const char *Trace[];
};

RIVA_STRUCT(error_t) {
	const Std$Type$t *Type;
	Sys$Program$stack_trace_t *StackTrace;
	const char *Message;
};

RIVA_TYPE(ErrorT);

RIVA_CFUN(Sys$Program$stack_trace_t *, stack_trace, int);

#undef RIVA_MODULE

#endif
