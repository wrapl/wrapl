#ifndef RIVA_DEBUG_H
#define RIVA_DEBUG_H

#define RIVA_MODULE Riva$Debug
#include <Riva-Header.h>

RIVA_STRUCT(hdr) {
	const char *StrInfo;
	int IntInfo;
};

RIVA_CFUN(int, stack_trace, void **, char **, int);

#undef RIVA_MODULE

#endif

