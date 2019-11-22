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
	int Errno;
};

RIVA_TYPE(ErrorT);
RIVA_CFUN(const char *, error_name, Sys$Program$error_t *);
RIVA_CFUN(Std$Object$t *, error_new, const Std$Type$t *Type, const char *Description);
RIVA_CFUN(Std$Object$t *, error_new_format, const Std$Type$t *Type, const char *Format, ...);
RIVA_CFUN(Std$Object$t *, error_from_errno, const Std$Type$t *Type);

RIVA_CFUN(Sys$Program$stack_trace_t *, stack_trace, int);

#undef RIVA_MODULE

#endif
