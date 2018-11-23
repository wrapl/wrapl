#ifndef RIVA_MODULE_H
#define RIVA_MODULE_H

#define RIVA_MODULE Riva$Module
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const struct Std$Type$t *Type;
};

RIVA_STRUCT(provider_t) {
	const struct Std$Type$t *Type;
	Riva$Module$t *Module;
};

typedef const char *(*Riva$Module$find_func)(const char *);
typedef int (*Riva$Module$load_func)(Riva$Module$provider_t *, const char *);
typedef int (*Riva$Module$import_func)(void *, const char *, int *, void **);

typedef int (*Riva$Module$suggest_callback)(const char *Name, void *Data);
typedef int (*Riva$Module$suggest_func)(const void *Handle, const char *Prefix, Riva$Module$suggest_callback Callback);

RIVA_CFUN(Riva$Module_t *, load, const char *, const char *);
RIVA_CFUN(Riva$Module_t *, load_file, const char *, const char *);

RIVA_CFUN(const char *, get_path, Riva$Module_t *);
RIVA_CFUN(const char *, get_name, Riva$Module_t *);
RIVA_CFUN(int, import, Riva$Module_t *, const char *, int *, void **);
RIVA_CFUN(int, lookup, void *, const char **, const char **);

RIVA_CFUN(int, suggest, Riva$Module_t *, const char *, Riva$Module$suggest_callback *, void *);

RIVA_CFUN(Riva$Module_t *, new, const char *);
RIVA_CFUN(void, add_alias, Riva$Module_t *, const char *);
RIVA_CFUN(void, set_path, Riva$Module_t *, const char *);
RIVA_CFUN(void, export, Riva$Module_t *, const char *, int , void *);

RIVA_CFUN(void, set_import_func, Riva$Module$provider_t *, void *, Riva$Module$import_func);
RIVA_CFUN(Riva$Module$provider_t *, get_default_provider, Riva$Module$t *);

RIVA_CFUN(void, set_suggest_func, Riva$Module$provider_t *, Riva$Module$suggest_func);

RIVA_CFUN(int, get_version, Riva$Module_t *);
RIVA_CFUN(void, set_version, Riva$Module_t *, int);

RIVA_CFUN(void, add_directory, const char *);

RIVA_CFUN(void, add_loader, const char *, int, Riva$Module$find_func, Riva$Module$load_func);

RIVA_CFUN(void *, load_symbol, const char *);

#undef RIVA_MODULE

#endif
