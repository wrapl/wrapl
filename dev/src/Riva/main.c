#include <string.h>
#include <stdio.h>
#include <gc/gc.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef LINUX
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#ifdef MACOSX
#include <pthread.h>
#include <signal.h>
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif

#include "libriva.h"

static const char *get_conf_name(void) {
	char Conf[1024];
#ifdef WINDOWS
    int Length = GetModuleFileName(0, Conf, 1018);
    strcpy(strrchr(Conf, '.'), ".conf");
#endif
#ifdef LINUX
    char Link[1024];
	sprintf(Link, "/proc/%i/exe", getpid());
	int Length = readlink(Link, Conf, 1018);
	strcpy(Conf + Length, ".conf");
#endif
#ifdef MACOSX
	char Path[PATH_MAX];
	int Length = PATH_MAX;
	_NSGetExecutablePath(Path, &Length);
	realpath(Path, Conf);
	strcat(Conf, ".conf");
#endif
	return GC_STRDUP(Conf);
};

static const char **Args;
static unsigned int NoOfArgs = 0;

extern int RivaDelayedLink;
extern int DebugLevels;

static int get_errno() {
	return errno;
};

int main(int Argc, const char **Argv) {
	preload_t *Preloads = 0;
	libriva_config(get_conf_name(), &Preloads);

	const char *Type = 0;
	const char *MainModule = libriva_mainmodule();

	RivaDelayedLink = 0;

	if (libriva_parseargs()) {
		for (int I = 1; I < Argc; ++I) {
			if (strcmp(Argv[I], "-nogc") == 0) {
				log_writef("Note: running without garbage collector\n");
				GC_disable();
			} else if (strcmp(Argv[I], "-nodelay") == 0) {
				RivaDelayedLink = 0;
			} else if (strcmp(Argv[I], "-heapframe") == 0) {
				config_set("AlwaysHeapFrame", "true");
			} else if (strcmp(Argv[I], "-debug") == 0) {
				++I;
				DebugLevels = atoi(Argv[I]);
			} else if (Argv[I][0] == '-') switch (Argv[I][1]) {
			case 'h': {
				puts("usage: riva [-L dir] [-v] [-nogc] [-P module] [-t loader] [-D key=value] module/file");
				exit(0);
			};
			case 'L': {
				if (Argv[I][2]) {
					module_add_directory(Argv[I] + 2);
				} else if (Argc > ++I) {
					module_add_directory(Argv[I]);
				} else {
					puts("Error: -L must be followed by a path");
					return 1;
				};
				break;
			};
			case 'v': {
				log_enable();
				break;
			};
			case 'P': {
				preload_t *Preload = new(preload_t);
				preload_t **Slot = &Preloads;
				while (Slot[0]) Slot = &Slot[0]->Next;
				*Slot = Preload;
				if (Argv[I][2]) {
					Preload->Path = Argv[I] + 2;
				} else if (Argc > ++I) {
					Preload->Path = Argv[I];
				} else {
					puts("Error: -P must be followed by a module");
					return 1;
				};
				break;
			};
			case 't': {
				if (Argv[I][2]) {
					Type = Argv[I] + 2;
				} else if (Argc > ++I) {
					Type = Argv[I];
				} else {
					puts("Error: -t must be followed by a type");
					return 1;
				};
				break;
			};
			case 'D': {
				char *Key, *Value;
				if (Argv[I][2]) {
					char *Tmp = strchr(Argv[I], '=');
					if (Tmp) {
						int KeyLen = Tmp - Argv[I] - 2;
						Key = GC_MALLOC_ATOMIC(KeyLen + 1);
						memcpy(Key, Argv[I] + 2, KeyLen);
						Key[KeyLen] = 0;
						Value = GC_STRDUP(Tmp + 1);
					} else {
						Key = GC_STRDUP(Argv[I] + 2);
						Value = "";
					}
				} else if (Argc > ++I) {
					char *Tmp = strchr(Argv[I], '=');
					if (Tmp) {
						int KeyLen = Tmp - Argv[I];
						Key = GC_MALLOC_ATOMIC(KeyLen + 1);
						memcpy(Key, Argv[I], KeyLen);
						Key[KeyLen] = 0;
						Value = GC_STRDUP(Tmp + 1);
					} else {
						Key = GC_STRDUP(Argv[I]);
						Value = "";
					};
				} else {
					puts("Error: -D must be followed by a key/value pair");
					return 1;
				};
				config_set(Key, Value);
				break;
			};
			case 'S': {
				char *Path;
				if (Argv[I][2]) {
					Path = Argv[I] + 2;
				} else {
					++I;
					Path = Argv[I];
				};
				log_writef("Preloading %s\n", Path);
				void *Handle = dlopen(Path, RTLD_LAZY | RTLD_GLOBAL);
				log_writef("\tError = %s, Handle = %x\n", dlerror(), Handle);
				break;
			};
			case '-': {
				++I;
				Args = Argv + I;
				NoOfArgs = Argc - I;
				goto finished;
			};
			default: {
				printf("Error: unknown option %s\n", Argv[I]);
				break;
			};
			} else {
				MainModule = Argv[I];
				++I;
				Args = Argv + I;
				NoOfArgs = Argc - I;
				goto finished;
			};
		};
	} else {
		Args = Argv + 1;
		NoOfArgs = Argc - 1;
	};
finished: 0;

	log_writef("GC = %s.\n", GC_is_disabled() ? "disabled" : "enabled");

	module_t *System = module_new("Riva/System");
	module_add_alias(System, "library:/Riva/System");
	module_export(System, "_Args", 0, &Args);
	module_export(System, "_NoOfArgs", 0, &NoOfArgs);
	module_export(System, "_get_errno", 0, (void *)get_errno);
	
	while (Preloads) {
		log_writef("Preloading module: %s\n", Preloads->Path);
		if (module_load(0, Preloads->Path) == 0) {
		    log_errorf("Error: module %s not found\n", Preloads->Path);
        };
		Preloads = Preloads->Next;
	}
	
	module_t *Module = module_load_file(MainModule, Type);
	if (Module == 0) Module = module_load(0, MainModule);
	if (Module == 0) printf("Error: module %s not found\n", MainModule);
#ifdef MINGW
    ExitThread(0);
#else
	//GC_enable();
	GC_pthread_exit(0);
#endif
};
