#ifndef RIVA_LOG_H
#define RIVA_LOG_H

#define RIVA_MODULE Riva$Log
#include <Riva-Header.h>

RIVA_CFUN(void, writes, const char *);
RIVA_CFUN(void, writen, const char *, unsigned long);
RIVA_CFUN(void, writef, const char *, ...);
RIVA_CFUN(void, errors, const char *);
RIVA_CFUN(void, errorn, const char *, unsigned long);
RIVA_CFUN(void, errorf, const char *, ...);
RIVA_CFUN(void, enable, void);

#undef RIVA_MODULE

#endif
