#ifndef SYS_MODULE_H
#define SYS_MODULE_H

#include <Std/Object.h>
#include <Riva/Module.h>

#define RIVA_MODULE Sys$Module
#include <Riva-Header.h>

typedef Riva$Module_t Sys$Module_t;
typedef Riva$Module$t Sys$Module$t;

RIVA_TYPE(T);

RIVA_CFUN(Sys$Module_t *, new, const char *);
RIVA_CFUN(Sys$Module_t *, load, const char *, const char *);
RIVA_CFUN(int, import, Sys$Module_t *, const char *, int *, void **);
RIVA_CFUN(void, export, Sys$Module_t *, const char *, int, const void *);

RIVA_CFUN(void, set_path, Sys$Module_t *, const char *);
RIVA_CFUN(const char *, get_path, Sys$Module_t *);
RIVA_CFUN(const char *, get_name, Sys$Module_t *);

#undef RIVA_MODULE

#endif
