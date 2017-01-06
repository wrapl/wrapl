#include "libriva.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef WINDOWS
#include <windows.h>
//#include <sys/cygwin.h>
//#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>

#ifdef CYGWIN
static int native_import(const void *Handle, const char *Symbol, int *IsRef, void **Data) {
	void *Address = dlsym(Handle, Symbol);
	if (Address) {
		*Data = Address;
		*IsRef = 0;
		return 1;
	};
	Address = dlsym(Handle, Symbol + 1);
	if (Address) {
		*Data = Address;
		*IsRef = 0;
		return 1;
	};
	log_errorf("Error (native_import): %s\n", dlerror());
	return 0;
};
#else
static int native_import(const void *Handle, const char *Symbol, int *IsRef, void **Data) {
	void *Address = GetProcAddress(Handle, Symbol);
	if (Address) {
		*Data = Address;
		*IsRef = 0;
		return 1;
	};
	log_errorf("Error (native_import): error finding import %s\n", Symbol);
	return 0;
};
#endif

static const char *native_find(const char *Base) {
	//printf("At least we got here (native_find): %s\n", Base);
	char Buffer[strlen(Base) + 5];
	strcpy(stpcpy(Buffer, Base), ".dll");
	//printf("At least we got here (native_find): %s\n", Buffer);
	struct stat Stat[1];
	if (stat(Buffer, Stat)) return 0;
	return GC_STRDUP(Buffer);
};

static int native_load(module_provider_t *Module, const char *FileName) {
	//printf("At least we got here (native_load): %s\n", FileName);
	struct stat Stat[1];
	if (stat(FileName, Stat)) return 0;
	/*void *Handle = GC_dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		module_provider_set_import_func(Module, Handle, native_import);
		return 1;
	};*/
	/*int Length = cygwin_conv_path(CCP_POSIX_TO_WIN_A, FileName, NULL, 0);
	char *Buffer = GC_MALLOC(Length);
	cygwin_conv_path(CCP_POSIX_TO_WIN_A, FileName, Buffer, Length);
	printf("At least we got here (native_load): %s\n", Buffer);*/
	HANDLE Handle = LoadLibraryA(FileName);
	if (Handle) {
		module_provider_set_import_func(Module, Handle, native_import);
		return 1;
	};
	//log_errorf("Error (native_load): %s\n", dlerror());
	log_errorf("Error (native_load): error loading dll %s\n", FileName);
	return 0;
};

//extern char *stpcpy(char *, const char *);
//extern void *mempcpy(void *, const void *, int);

#ifdef MINGW
int asprintf(char **, char *, ...); 
int vasprintf(char **, char *, va_list); 

int vasprintf(char **sptr, char *fmt, va_list argv) { 
	int wanted = vsnprintf( *sptr = NULL, 0, fmt, argv); 
	if((wanted > 0) && ((*sptr = malloc( 1 + wanted )) != NULL)) return vsprintf(*sptr, fmt, argv); 
	return wanted; 
};

int asprintf(char **sptr, char *fmt, ...) { 
	int retval; 
	va_list argv; 
	va_start(argv, fmt); 
	retval = vasprintf(sptr, fmt, argv); 
	va_end(argv); 
	return retval; 
};
#endif

void native_init(void) {
	module_add_loader("Native", 90, native_find, native_load);
	//void *Handle = GC_dlopen("cygwin1.dll", RTLD_LAZY);
	DWORD Handle = LoadLibraryA("cygwin1.dll");
	module_t *Module;

	Module = module_new("", "libc");
	module_add_alias(Module, "library:/libc");
	module_add_alias(Module, "library:/cygwin1");
	module_add_alias(Module, "library:/libpthread");
	module_set_import_func(Module->Providers, Handle, native_import);
	//module_export(Module, "stpcpy", 0, &stpcpy);
	//module_export(Module, "_mempcpy", 0, &mempcpy);

#ifdef MINGW
	module_export(Module, "asprintf", asprintf);
	module_export(Module, "vasprintf", vasprintf);
#endif
	
	Module = module_new("", "libm");
	module_add_alias(Module, "libm");
	module_set_import_func(Module->Providers, Handle, native_import);
};

#endif

#ifdef LINUX

#define LINUX_THREADS

#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <gc/gc.h>
#include <sys/stat.h>
#include <glob.h>

static int native_import(void *Handle, const char *Symbol, int *IsRef, void **Data) {
	void *Address = dlsym(Handle, Symbol);
	if (Address) {
		*Data = Address;
		*IsRef = 0;
		return 1;
	} else {
		log_errorf("Error: %s\n", dlerror());
		return 0;
	};
};

static const char *native_find(const char *Base) {
	struct stat Stat[1];
	char Pattern[strlen(Base) + 10];
	strcpy(stpcpy(Pattern, Base), ".so{,*}");
	glob_t Results[1];
	const char *Path = 0;
	if (!glob(Pattern, GLOB_PERIOD | GLOB_BRACE, 0, Results)) {
		Path = GC_STRDUP(Results->gl_pathv[Results->gl_pathc - 1]);
	};
	globfree(Results);
	return Path;
};

static int native_load(module_provider_t *Provider, const char *FileName) {
	struct stat Stat[1];
	if (stat(FileName, Stat)) return 0;
	void *Handle = GC_dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		module_set_import_func(Provider, Handle, (module_import_func)native_import);
		return 1;
	};
	log_errorf("Error: %s\n", dlerror());
	return 0;
};

extern void *__dso_handle;

void native_init(void) {
	module_add_loader("Native", 90, native_find, native_load);
	void *Handle = GC_dlopen(0, RTLD_LOCAL| RTLD_LAZY);
	module_t *Module;
	
	Module = module_new("libc");
	//printf("Adding alias libc = 0x%x\n", Module);
	module_add_alias(Module, "library:/__unknown__");
	module_add_alias(Module, "library:/libc");
	module_add_alias(Module, "library:/libgcc");
	module_set_import_func(Module->Providers, Handle, (module_import_func)native_import);
//#include "libc_exports.c"
	module_export(Module, "atexit", 0, &atexit);
	module_export(Module, "stat", 0, &stat);
	module_export(Module, "__dso_handle", 0, &__dso_handle);
	
	/*module_export(Module, "malloc", 0, malloc);
	module_export(Module, "calloc", 0, calloc);
	module_export(Module, "realloc", 0, realloc);
	module_export(Module, "free", 0, free);
	module_export(Module, "strdup", 0, strdup);
	module_export(Module, "malloc_usable_size", 0, GC_size);
	module_export(Module, "memalign", 0, memalign);
	module_export(Module, "posix_memalign", 0, posix_memalign);*/

	Module = module_new("libpthread");
	module_add_alias(Module, "library:/libpthread");
	module_set_import_func(Module->Providers, Handle, (module_import_func)native_import);
	module_export(Module, "pthread_create", 0, GC_pthread_create);
	module_export(Module, "pthread_join", 0, GC_pthread_join);
	module_export(Module, "pthread_detach", 0, GC_pthread_detach);
	module_export(Module, "pthread_sigmask", 0, GC_pthread_sigmask);
	module_export(Module, "pthread_cancel", 0, GC_pthread_cancel);
	module_export(Module, "pthread_exit", 0, GC_pthread_exit);
	
	Module = module_new("libdl");
	module_add_alias(Module, "library:/libdl");
	module_set_import_func(Module->Providers, Handle, (module_import_func)native_import);
	module_export(Module, "dlopen", 0, GC_dlopen);
};

#endif
#ifdef MACOSX

#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <gc/gc.h>
#include <sys/stat.h>
#include <glob.h>
#include <errno.h>
#include <setjmp.h>

static int native_import(void *Handle, const char *Symbol, int *IsRef, void **Data) {
	void *Address = dlsym(Handle, Symbol);
	if (Address) {
		*Data = Address;
		*IsRef = 0;
		return 1;
	} else {
		log_errorf("Error: %s\n", dlerror());
		return 0;
	};
};

static void *native_find(const char *Base) {
	struct stat Stat[1];
	char Pattern[strlen(Base) + 13];
	strcpy(stpcpy(Pattern, Base), ".dylib");
	glob_t Results[1];
	if (!glob(Pattern, GLOB_BRACE, 0, Results)) {
		return GC_STRDUP(Results->gl_pathv[Results->gl_pathc - 1]);
	};
	return 0;
};

static int native_load(module_provider_t *Module, const char *FileName) {
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_NOW);
	struct stat Stat[1];
	if (stat(FileName, Stat)) return 0;
	if (Handle) {
		module_provider_set_import_func(Module, Handle, native_import);
		return 1;
	};
	log_errorf("Error: %s\n", dlerror());
	return 0;
};

extern void *mempcpy(void *, const void *, int);

static void __stack_chk_fail() {
	asm("int3");
};

static int *__errno_location() {
	return &errno;
};

void native_init(void) {
	module_add_loader("Native", 90, native_find, native_load);
	void *Handle = dlopen(0, RTLD_LOCAL | RTLD_NOW);
	module_t *Module;
	
	Module = module_new("libc");
	printf("Adding alias libc = 0x%x\n", Module);
	module_add_alias(Module, "library:/libc");
	module_add_alias(Module, "library:/libgcc");
	module_set_import_func(Module->Provider, Handle, native_import);
	module_export(Module, "atexit", 0, &atexit);
	module_export(Module, "stat", 0, &stat);
	module_export(Module, "mempcpy", 0, &mempcpy);
	module_export(Module, "__stack_chk_fail", 0, &__stack_chk_fail);
	module_export(Module, "__errno_location", 0, &__errno_location);
	module_export(Module, "_IO_putc", 0, &fputc);
	module_export(Module, "__xstat", 0, &stat);
	module_export(Module, "__sigsetjmp", 0, &sigsetjmp);
	
	module_export(Module, "malloc", 0, GC_malloc);
	module_export(Module, "realloc", 0, GC_realloc);
	module_export(Module, "free", 0, GC_free);
	
	module_export(Module, "stderr", 0, stderr);
	module_export(Module, "stdout", 0, stdout);
	module_export(Module, "stdin", 0, stdin);
	
	Module = module_new("libpthread");
	module_add_alias(Module, "library:/libpthread");
	module_set_import_func(Module->Provider, Handle, native_import);
	module_export(Module, "pthread_create", 0, GC_pthread_create);
	module_export(Module, "pthread_join", 0, GC_pthread_join);
	module_export(Module, "pthread_detach", 0, GC_pthread_detach);
	module_export(Module, "pthread_sigmask", 0, GC_pthread_sigmask);
	module_export(Module, "pthread_cancel", 0, GC_pthread_cancel);
	module_export(Module, "pthread_exit", 0, GC_pthread_exit);
	
	Module = module_new("libdl");
	module_add_alias(Module, "library:/libdl");
	module_set_import_func(Module->Provider, Handle, native_import);
	module_export(Module, "dlopen", 0, GC_dlopen);
};

#endif
