#ifndef LIBRIVA_H
#define LIBRIVA_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

typedef struct stringtable_node {
	char *Key;
	unsigned long Hash, Incr;
	void *Value;
} stringtable_node;

typedef struct stringtable_t {
	unsigned long Size, Space;
	stringtable_node *Entries;
} stringtable_t[1];

extern void stringtable_init(stringtable_t Table);
extern void stringtable_put(stringtable_t Table, const char *Key, void *Value);
extern void *stringtable_get(const stringtable_t Table, const char *Key);

extern pthread_mutex_t LibRivaMutex[1];

extern void *config_get(const char *);
extern void config_set(const char *, void *);

typedef void (*__log_writes_fn)(const char *String);
typedef void (*__log_writen_fn)(const char *String, unsigned long Length);
typedef void (*__log_writef_fn)(const char *Format, ...);

extern __log_writes_fn __log_Writes;
extern __log_writen_fn __log_Writen;
extern __log_writef_fn __log_Writef;
extern __log_writes_fn __log_Errors;
extern __log_writen_fn __log_Errorn;
extern __log_writef_fn __log_Errorf;

extern void log_writes(const char *);
extern void log_writen(const char *, unsigned long);
extern int log_writef(const char *, ...);

extern void log_errors(const char *);
extern void log_errorn(const char *, unsigned long);
extern int log_errorf(const char *, ...);

extern void log_enable(void);
extern void log_disable(void);


typedef struct module_t module_t;
typedef struct module_provider_t module_provider_t;
typedef struct module_loader_t module_loader_t;
typedef struct module_load_info_t module_load_info_t;
typedef struct module_lock_t module_lock_t;

typedef void *(*module_find_func)(const char *Path);
typedef int (*module_load_func)(module_provider_t *Module, void *Data);
typedef int (*module_import_func)(const void *Handle, const char *Name, int *IsRef, void **Data);

struct module_provider_t {
	const struct Std$Type_t *Type;
	module_t *Module;
	module_provider_t *Next;
	union {
		struct {
			void *ImportInfo;
			module_import_func ImportFunc;
		};
		struct {
			void *LoadInfo;
			module_loader_t *Loader;
		};
	};
	int HasImports;
};

struct module_t {
	const struct Std$Type_t *Type;
	const char *Path, *Name;
	stringtable_t Symbols;
	int Version, TimeStamp;
	//module_lock_t *Lock;
	pthread_mutex_t Lock[1];
	module_provider_t *Providers;
};

extern int module_provider_import(module_provider_t *Provider, const char *Name, int *IsRef, void **Data);
extern void module_provider_export(module_provider_t *Provider, const char *Name, int IsRef, void *Data);
extern void module_importer_set(module_provider_t *Provider, void *ImportInfo, module_import_func ImportFunc);

extern module_t *module_load(const char *Path, const char *Name);

extern module_t *module_load_file(const char *FileName, const char *Type);

extern const char *module_get_path(module_t *);
extern int module_import(module_t *, const char *Name, int *IsRef, void **Data);
extern int module_import0(module_t *, const char *Name, int *IsRef, void **Data);
extern int module_lookup(void *, const char **, const char **);

extern module_t *module_new(const char *Name);
extern void module_add_alias(module_t *Module, const char *Alias);
extern void module_set_path(module_t *, const char *Path);
extern void module_export(module_t *, const char *Name, int IsRef, void *Data);
extern void module_set_version(module_t *Module, int Version);

extern void module_add_directory(const char *Directory);

extern void module_add_loader(const char *Name, int Priority, module_find_func FindFunc, module_load_func LoadFunc);

extern const char *path_join(const char *Path, const char *Name);
extern const char *path_dir(const char *Path);
extern const char *path_file(const char *Path);
extern const char *path_fixup(const char *Path);

extern char *concat2(const char *, const char *);
extern char *concat3(const char *, const char *, const char *);

extern void *memory_alloc_code(size_t);

extern module_t *Symbol;
extern void *(*make_symbol)(const char *);
extern void (*add_methods)(void *);

typedef struct preload_t preload_t;
struct preload_t {
	preload_t *Next;
	const char *Path;
};

extern int libriva_batchmode(void);
extern int libriva_parseargs(void);
extern const char *libriva_mainmodule(void);
extern void libriva_config(const char *, preload_t **);

#include <gc/gc.h>

#define new(T) (T *)GC_MALLOC(sizeof(T))
#define unew(T) (T *)GC_MALLOC_UNCOLLECTABLE(sizeof(T))

typedef struct debug_hdr {
	const char *StrInfo;
	int IntInfo;
} debug_hdr;

extern const char DebugKey[];

extern debug_hdr *debug_get_hdr(void *);

typedef struct handler_t {
	struct handler_t *Prev;
	int32_t EIP, ESP;
} handler_t;

extern void *handler_push(handler_t *Handler) __attribute__ ((returns_twice));
extern void handler_pop(handler_t *Handler);
extern void exception_raise(void *Exception) __attribute__ ((noreturn));

#endif

