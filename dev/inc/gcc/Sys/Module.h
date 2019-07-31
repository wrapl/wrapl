#ifndef SYS_MODULE_H
#define SYS_MODULE_H

#include <Std/Object.h>
#include <Riva/Module.h>

#define RIVA_MODULE Sys$Module
#include <Riva-Header.h>

typedef Riva$Module$t Sys$Module$t;
typedef Riva$Module$t Sys$Module$t;

RIVA_TYPE(T);

RIVA_CFUN(Sys$Module$t *, new, const char *);
RIVA_CFUN(Sys$Module$t *, load, const char *, const char *);
RIVA_CFUN(int, import, Sys$Module$t *, const char *, int *, void **);
RIVA_CFUN(void, export, Sys$Module$t *, const char *, int, const void *);

RIVA_CFUN(void, set_path, Sys$Module$t *, const char *);
RIVA_CFUN(const char *, get_path, Sys$Module$t *);
RIVA_CFUN(const char *, get_name, Sys$Module$t *);

#undef RIVA_MODULE

#endif
