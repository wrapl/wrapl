#include <gc/gc.h>

#include "libriva.h"

#include <confuse.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef LINUX
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>
#include <malloc.h>
#include <dlfcn.h>
#endif

#ifdef MACOSX
#include <pthread.h>
#include <signal.h>
#endif

pthread_mutex_t LibRivaMutex[1] = {PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP};

#if defined(LINUX) || defined(CYGWIN)

//static void *(*old_memalign)(size_t, size_t, const void *);

#ifdef SUPER_GC_DEBUG

static void *debug_malloc(size_t Size, void *Caller) {
	return GC_debug_malloc_uncollectable(Size, "malloc", (long)Caller);
};

static void *debug_realloc(void *Ptr, size_t Size, void *Caller) {
	return GC_debug_realloc(Ptr, Size, "realloc", (long)Caller);
};

#endif

static void *memory_memalign(size_t Alignment, size_t Size, const void *Caller) {
#ifdef SUPER_GC_DEBUG
	uint8_t *Result = GC_debug_malloc_uncollectable(Size + Alignment, "memory_memalign", (long)Caller);
#else
	uint8_t *Result = GC_malloc_uncollectable(Size + Alignment);
#endif
	uint32_t Offset = (uint32_t)Result % Alignment;
	if (Offset) Result += (Alignment - Offset);
	return Result;
};

#ifdef LINUX
/*
static void memory_init_hook (void) {
#ifdef SUPER_GC_DEBUG
	__malloc_hook = (void *)debug_malloc;
	__realloc_hook = (void *)debug_realloc;
	__free_hook = (void *)GC_debug_free;
#else
	__malloc_hook = (void *)GC_malloc_uncollectable;
	__realloc_hook = (void *)GC_realloc;
	__free_hook = (void *)GC_free;
#endif
//	old_memalign = __memalign_hook;
	__memalign_hook = memory_memalign;
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = memory_init_hook;
*/
#endif

#ifdef MACOSX
void *malloc(size_t Size) {
	printf("Calling GC_malloc!\n");
	return GC_malloc(Size);
};

void *calloc(size_t Num, size_t Size) {
	return GC_malloc_uncollectable(Num * Size);
};

void free(void *Ptr) {
	return GC_free(Ptr);
};

void *realloc(void *Ptr, size_t Size) {
	return GC_realloc(Ptr, Size);
};
#endif

size_t malloc_usable_size(void *Ptr) {
	return GC_size(Ptr);
};

/*char *strdup(const char *Str) {
	return GC_strdup(Str);
};*/

/*
void *memalign(size_t Alignment, size_t Size) {
	if (Size < 256) Size = 256; // TODO: This is a temporary fix for what seems to be a bug in the garbage collector
	return GC_memalign(Alignment, Size);
};

int posix_memalign(void **Ptr, size_t Alignment, size_t Size) {
	*Ptr = memalign(Alignment, Size);
	return 0;
};
*/

void *fcfix_malloc(size_t Size) {
	//printf("%s:%d\n", __FILE__, __LINE__);
	return GC_malloc_uncollectable(Size);
};

void fcfix_free(void *Pointer) {
	//printf("%s:%d\n", __FILE__, __LINE__);
	GC_free(Pointer);
};

void *fcfix_calloc(size_t Size, size_t Num) {
	//printf("%s:%d\n", __FILE__, __LINE__);
	return GC_malloc_uncollectable(Size * Num);
};

void *fcfix_realloc(void *Pointer, size_t Size) {
	//printf("%s:%d\n", __FILE__, __LINE__);
	return GC_realloc(Pointer, Size);
};

extern size_t strlen(const char *);
extern char *strcpy(char *, const char *);

void *fcfix_strdup(const char *String) {
	//printf("%s:%d\n", __FILE__, __LINE__);
	char *Copy = GC_malloc_atomic_uncollectable(strlen(String) + 1);
	return strcpy(Copy, String);
};

/*void *__wrap_dlsym(void *Handle, const char *Symbol) {
	//printf("dlsym(%x, %s)\n", Handle, Symbol);
	void *Result = __real_dlsym(Handle, Symbol);
	//printf("Result = %x\n", Result);
	return Result;
};*/

#endif

static int add_config(cfg_t *Cfg, cfg_opt_t *Opt, int Argc, const char **Argv) {
	if (Argc == 1) {
		config_set(GC_STRDUP(Argv[0]), "");
	} else if (Argc == 2) {
		config_set(GC_STRDUP(Argv[0]), GC_STRDUP(Argv[1]));
	} else {
		cfg_error(Cfg, "Error: invalid number of arguments to config() in configuration file\n");
		return -1;
	};
	return 0;
};	

static int BatchMode = 0;
static int ParseArgs = 0;
static char *MainModule;

int libriva_batchmode(void) {return BatchMode;};
int libriva_parseargs(void) {return ParseArgs;};
const char *libriva_mainmodule(void) {return MainModule;};

extern void module_init2(void);

void libriva_config(const char *Conf, preload_t **Preloads) {
	static cfg_opt_t OptsMain[] = {
		CFG_STR_LIST("library", 0, CFGF_NONE),
		CFG_STR_LIST("modules", 0, CFGF_NONE),
		CFG_BOOL("batch", 0, CFGF_NONE),
		CFG_BOOL("parseargs", 0, CFGF_NONE),
		CFG_STR("module", "Main", CFGF_NONE),
		CFG_FUNC("config", add_config),
		CFG_END()
	};
	cfg_t *Cfg = cfg_init(OptsMain, CFGF_NONE);
	switch (cfg_parse(Cfg, Conf)) {
	case CFG_FILE_ERROR:
		log_errorf("Error: configuration file %s not present\n", Conf);
		exit(1);
	case CFG_PARSE_ERROR:
		log_errorf("Error: configuration file %s corrupt\n", Conf);
		exit(1);
	};
	for (int I = 0; I < cfg_size(Cfg, "library"); ++I) {
		const char *Path = cfg_getnstr(Cfg, "library", I);
		module_add_directory(Path);
	};
	setenv("G_SLICE", "always-malloc", 1);
	module_init2();
	for (int I = 0; I < cfg_size(Cfg, "modules"); ++I) {
		const char *Path = cfg_getnstr(Cfg, "modules", I);
		preload_t *Preload = new(preload_t);
		Preload->Path = Path;
		preload_t **Slot = Preloads;
		while (Slot[0]) Slot = &Slot[0]->Next;
		*Slot = Preload;
	};
	BatchMode = cfg_getbool(Cfg, "batch");
	ParseArgs = cfg_getbool(Cfg, "parseargs");
	MainModule = cfg_getstr(Cfg, "module");
	//GC_dlopen(get_wrapper_name(), RTLD_GLOBAL | RTLD_NOW | RTLD_DEEPBIND);
};

extern void module_init(void);
extern void memory_init(void);
extern void log_init(void);
extern void thread_init(void);
extern void config_init(void);
extern void directory_init(void);
extern void native_init(void);
extern void fileset_init(void);
extern void riva_init(void);
extern void symbol_init(void);
extern void dynamic_init(void);
extern void exception_init(void);

#ifdef LINUX
extern void debug_init();
#endif

extern void memory_log_enable(void);

void __attribute__ ((constructor)) init(void) {
	//GC_enable_incremental();
	//GC_disable();
	GC_set_warn_proc(GC_ignore_warn_proc);
	GC_INIT();
	//GC_enable();
	//GC_disable();
	
	//GC_set_dont_expand(1);

	module_init();
	memory_init();
	log_init();
	
	//log_enable();
	//memory_log_enable();
	
	//thread_init();
	config_init();
	directory_init();
	native_init();
	fileset_init();
	riva_init();
	symbol_init();
	dynamic_init();
	exception_init();
#ifdef LINUX
	debug_init();
#endif
};
