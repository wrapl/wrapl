#ifndef SYS_SERVICE_H
#define SYS_SERVICE_H

#include <Std/Object.h>
#include <Riva/Module.h>

#define RIVA_MODULE Sys$Service
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);

RIVA_CFUN(Sys$Service$t *, new, const char *);
RIVA_CFUN(void, start, Sys$Service$t *, Std$Object$t *);
RIVA_CFUN(Std$Object$t *, get, const char *);

#undef RIVA_MODULE

#endif
