#include "libriva.h"

#include <stdio.h>
//#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

typedef struct export_t {
	int IsRef;
	void *Data;
} export_t;

typedef struct path_node {
    struct path_node *Next;
    const char *Dir;
} path_node;

static path_node *ModuleLibrary = 0;

void module_add_directory(const char *Dir) {
	Dir = path_fixup(Dir);
	if (Dir) {
		path_node *Node = new(path_node);
		Node->Dir = path_fixup(Dir);
		Node->Next = ModuleLibrary;
		ModuleLibrary = Node;
		log_writef("Adding %s to library search path.\n", Node->Dir);
	}
};

typedef struct module_loader_t {
	struct module_loader_t *Next;
	const char *Name;
	int Priority, TimeStamp;
	module_find_func _Find;
	module_load_func Load;
} module_loader_t;

static int ModuleTimeStamp = 0;
static module_loader_t *ModuleLoaders = 0;

void module_add_loader(const char *Name, int Priority, module_find_func FindFunc, module_load_func LoadFunc) {
	log_writef("Adding loader %s with priority %d.\n", Name, Priority);
	pthread_mutex_lock(LibRivaMutex);
	module_loader_t *Node = new(module_loader_t);
	Node->Name = Name;
	Node->_Find = FindFunc;
    Node->Load = LoadFunc;
	Node->Priority = Priority;
	Node->TimeStamp = ++ModuleTimeStamp;
	module_loader_t **Slot = &ModuleLoaders;
	while (Slot[0] && (Slot[0]->Priority >= Priority)) Slot = &(Slot[0]->Next);
    Node->Next = Slot[0];
	Slot[0] = Node;
	pthread_mutex_unlock(LibRivaMutex);
};

int module_set_loader_find_func(const char *Name, module_find_func FindFunc) {
	for (module_loader_t *Node = ModuleLoaders; Node; Node = Node->Next) {
		if (!strcmp(Node->Name, Name)) {
			Node->_Find = FindFunc;
			return 0;
		};
	};
	return 1;
};

int module_set_loader_load_func(const char *Name, module_load_func LoadFunc) {
	for (module_loader_t *Node = ModuleLoaders; Node; Node = Node->Next) {
		if (!strcmp(Node->Name, Name)) {
			Node->Load = LoadFunc;
			return 0;
		};
	};
	return 1;
};

static inline module_loader_t *module_get_loader(const char *Name) {
	for (module_loader_t *Loader = ModuleLoaders; Loader; Loader = Loader->Next) {
		if (strcmp(Loader->Name, Name) == 0) return Loader;
	};
	return 0;
};

static stringtable_t Modules = {{0, 0, 0}};

int module_lookup(void *Address, const char **ModuleName, const char **SymbolName) {
	if (Address == 0) return 0;
	for (int I = 0; I < Modules->Size; ++I) {
		if (Modules->Entries[I].Key) {
			module_t *Module = Modules->Entries[I].Value;
			struct stringtable_t *Symbols = Module->Symbols;
			for (int J = 0; J < Symbols->Size; ++J) {
				if (Symbols->Entries[J].Key) {
					export_t *Export = Symbols->Entries[J].Value;
					if (Export->Data == Address) {
						*ModuleName = Module->Name;
						*SymbolName = Symbols->Entries[J].Key;
						return 1;
					};
				};
			};
		};
	};
	return 0;
};

static int ModuleLevel = 0;

const struct Std$Type_t *ModuleT = 0;

struct module_lock_t {
	pthread_t Thread;
	pthread_cond_t Cond[1];
};

static module_loader_t *ModuleLoaders;

module_provider_t *module_find_loaders(const char *Name, module_loader_t *Loader, int TimeStamp) {
	if (!Loader) return 0;
	if (Loader->TimeStamp > TimeStamp) {
		void *LoadInfo = Loader->_Find(Name);
		if (LoadInfo) {
			log_writef("Found loader %s for %s.\n", Loader->Name, Name);
			module_provider_t *Provider = new(module_provider_t);
			Provider->Loader = Loader;
			Provider->LoadInfo = LoadInfo;
			Provider->HasImports = 0;
			Provider->Next = module_find_loaders(Name, Loader->Next, TimeStamp);
			return Provider;
		};
	};
	return module_find_loaders(Name, Loader->Next, TimeStamp);
};

static void module_call_loaders(const char *Name, module_t *Module, module_provider_t *Providers) {
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_call_loaders:%d(%s, %s)\n", Thread, __LINE__, Name, Module->Name);
	do {
		module_provider_t **Slot = &Module->Providers;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Providers;
		for (module_provider_t *Provider = Providers; Provider; Provider = Provider->Next) {
			module_loader_t *Loader = Provider->Loader;
			//printf("<thread @ %x> Calling module provider:%s(%s)\n", Thread, Loader->Name, Module->Name);
			Provider->Module = Module;
			Loader->Load(Provider, Provider->LoadInfo);
			//printf("<thread @ %x> Called module provider:%s(%s)\n", Thread, Loader->Name, Module->Name);
		};
		pthread_mutex_lock(LibRivaMutex);
		Providers = module_find_loaders(Name, ModuleLoaders, Module->TimeStamp);
		Module->TimeStamp = ModuleTimeStamp;
		pthread_mutex_unlock(LibRivaMutex);
	} while (Providers);
	//printf("<thread @ %x> Leaving module_call_loaders:%d(%s, %s)\n", Thread, __LINE__, Name, Module->Name);
};

static pthread_mutex_t RecursiveMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static void module_reload(module_t *Module) {
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_reload:%d(%s)\n", Thread, __LINE__, Module->Name);
	pthread_mutex_lock(Module->Lock);
	if (Module->Type == 0) Module->Type = ModuleT;
	if (ModuleTimeStamp > Module->TimeStamp) {
		module_provider_t *Providers = module_find_loaders(Module->Name, ModuleLoaders, Module->TimeStamp);
		Module->TimeStamp = ModuleTimeStamp;
		if (Providers) module_call_loaders(Module->Name, Module, Providers);
	};
	pthread_mutex_unlock(Module->Lock);
	//printf("<thread @ %x> Leaving module_reload:%d(%s)\n", Thread, __LINE__, Module->Name);
};

static module_t *module_load_internal(const char *Path, const char *File, const char *Alias) {
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_load_internal:%d(%s, %s)\n", Thread, __LINE__, Path, File);
	const char *Name = path_join(Path, File);
	pthread_mutex_lock(LibRivaMutex);
	module_t *Module = stringtable_get(Modules, Name);
	if (Module) {
		pthread_mutex_unlock(LibRivaMutex);
		module_reload(Module);
		//printf("<thread @ %x> Leaving module_load_internal:%d(%s, %s)\n", Thread, __LINE__, Path, File);
		return Module;
	};
	module_provider_t *Providers = module_find_loaders(Name, ModuleLoaders, -1);
	if (!Providers) {
		pthread_mutex_unlock(LibRivaMutex);
		//printf("<thread @ %x> Leaving module_load_internal:%d(%s, %s)\n", Thread, __LINE__, Path, File);
		return 0;
	};
	Module = stringtable_get(Modules, Name);
	if (Module) {
		pthread_mutex_unlock(LibRivaMutex);
		module_reload(Module);
		//printf("<thread @ %x> Leaving module_load_internal:%d(%s, %s)\n", pthread_self(), __LINE__, Path, File);
		return Module;
	};
	Module = new(module_t);
	Module->Type = ModuleT;
	Module->Name = Name;
	Module->Path = path_dir(Name);
	Module->TimeStamp = ModuleTimeStamp;
	Module->Lock[0] = RecursiveMutex;
	stringtable_put(Modules, Name, Module);
	if (Alias) stringtable_put(Modules, Alias, Module);
	pthread_mutex_lock(Module->Lock);
	pthread_mutex_unlock(LibRivaMutex);
	module_call_loaders(Name, Module, Providers);
	pthread_mutex_unlock(Module->Lock);
	//printf("<thread @ %x> Leaving module_load_internal:%d(%s, %s)\n", Thread, __LINE__, Path, File);
	return Module;
};

module_t *module_load(const char *Path, const char *File) {
	if (Path) return module_load_internal(Path, File, 0);
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_load:%d(%s, %s)\n", Thread, __LINE__, Path, File);
	const char *Alias = path_join("library:", File);
	pthread_mutex_lock(LibRivaMutex);
	module_t *Module = stringtable_get(Modules, Alias);
	pthread_mutex_unlock(LibRivaMutex);
	if (Module) {
		module_reload(Module);
		//printf("Found module %s = 0x%x\n", Alias, Module);
		//printf("<thread @ %x> Leaving module_load:%d(%s, %s)\n", Thread, __LINE__, Path, File);
		return Module;
	};
	for (path_node *Path = ModuleLibrary; Path; Path = Path->Next) {
		Module = module_load_internal(Path->Dir, File, Alias);
		if (Module) {
			//printf("<thread @ %x> Leaving module_load:%d(%s, %s)\n", Thread, __LINE__, Path, File);
			return Module;
		};
	};
	//printf("<thread @ %x> Leaving module_load:%d(%s, %s)\n", Thread, __LINE__, Path, File);
	return 0;
};

int module_import(module_t *Module, const char *Symbol, int *IsRef, void **Data) {
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_import:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
	pthread_mutex_lock(Module->Lock);
	export_t *Export = stringtable_get(Module->Symbols, Symbol);
	if (Export) {
		*IsRef = Export->IsRef;
		*Data = Export->Data;
		pthread_mutex_unlock(Module->Lock);
		//printf("<thread @ %x> Leaving module_import:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
		return 1;
	};
	Symbol = GC_STRDUP(Symbol);
	for (module_provider_t *Provider = Module->Providers; Provider; Provider = Provider->Next) {
		if (!Provider->HasImports) continue;
		if (Provider->ImportFunc(Provider->ImportInfo, Symbol, IsRef, Data)) {
			export_t *Export = new(export_t);
			Export->IsRef = *IsRef;
			Export->Data = *Data;
			stringtable_put(Module->Symbols, Symbol, Export);
			pthread_mutex_unlock(Module->Lock);
			//printf("<thread @ %x> Leaving module_import:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
			return 1;
		};
	};
	pthread_mutex_unlock(Module->Lock);
	//printf("<thread @ %x> Leaving module_import:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
	return 0;
};

extern int directory_import(const void *Path, const char *Name, int *IsRef, void **Data);

int module_import0(module_t *Module, const char *Symbol, int *IsRef, void **Data) {
	//pthread_t Thread = pthread_self();
	//printf("<thread @ %x> Entering module_import0:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
	pthread_mutex_lock(Module->Lock);
	export_t *Export = stringtable_get(Module->Symbols, Symbol);
	if (Export) {
		*IsRef = Export->IsRef;
		*Data = Export->Data;
		pthread_mutex_unlock(Module->Lock);
		//printf("<thread @ %x> Leaving module_import0:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
		return 1;
	};
	Symbol = GC_STRDUP(Symbol);
	for (module_provider_t *Provider = Module->Providers; Provider; Provider = Provider->Next) {
		if (!Provider->HasImports) continue;
		if (Provider->ImportFunc == directory_import) continue;
		if (Provider->ImportFunc(Provider->ImportInfo, Symbol, IsRef, Data)) {
			export_t *Export = new(export_t);
			Export->IsRef = *IsRef;
			Export->Data = *Data;
			stringtable_put(Module->Symbols, Symbol, Export);
			pthread_mutex_unlock(Module->Lock);
			//printf("<thread @ %x> Leaving module_import0:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
			return 1;
		};
	};
	pthread_mutex_unlock(Module->Lock);
	//printf("<thread @ %x> Leaving module_import0:%d(%s, %s)\n", Thread, __LINE__, Module->Name, Symbol);
	return 0;
};

void module_export(module_t *Module, const char *Name, int IsRef, void *Data) {
	export_t *Export = new(export_t);
	Export->IsRef = IsRef;
	Export->Data = Data;
	pthread_mutex_lock(LibRivaMutex);
	stringtable_put(Module->Symbols, Name, Export);
	pthread_mutex_unlock(LibRivaMutex);
};

void module_add_alias(module_t *Module, const char *Name) {
	pthread_mutex_lock(LibRivaMutex);
	stringtable_put(Modules, Name, Module);
	pthread_mutex_unlock(LibRivaMutex);
};

void module_set_path(module_t *Module, const char *Path) {
	Module->Path = path_fixup(Path);
};

const char *module_get_path(module_t *Module) {
	return Module->Path;
};

const char *module_get_name(module_t *Module) {
	return Module->Name;
};

void module_set_version(module_t *Module, int Version) {
	Module->Version = Version;
};

int module_get_version(module_t *Module) {
	return Module->Version;
};

module_t *module_new(const char *Name) {
	module_t *Module = GC_MALLOC(sizeof(module_t) + sizeof(module_provider_t));
	Module->Type = ModuleT;
	Module->Name = Name;
	Module->Path = path_dir(Name);
	Module->Lock[0] = RecursiveMutex;
	Module->TimeStamp = 9999;
	Module->Providers = new(module_provider_t);
	Module->Providers->Module = Module;
	Module->Providers->HasImports = 0;
	return Module;
};

module_provider_t *module_get_default_provider(module_t *Module) {
	return Module->Providers;
};

int module_provider_import(module_provider_t *Provider, const char *Symbol, int *IsRef, void **Data) {
	return module_import(Provider->Module, Symbol, IsRef, Data);
};

void module_provider_export(module_provider_t *Provider, const char *Symbol, int IsRef, void *Data) {
	export_t *Export = new(export_t);
	Export->IsRef = IsRef;
	Export->Data = Data;
	pthread_mutex_lock(LibRivaMutex);
	stringtable_put(Provider->Module->Symbols, Symbol, Export);
	pthread_mutex_unlock(LibRivaMutex);
};

void module_importer_set(module_provider_t *Provider, void *ImportInfo, module_import_func ImportFunc) {
	if (ImportFunc) {
		Provider->ImportInfo = ImportInfo;
		Provider->ImportFunc = ImportFunc;
		Provider->HasImports = 1;
	} else {
		Provider->HasImports = 0;
	};
};

#include <sys/stat.h>

module_t *module_load_file(const char *File, const char *Type) {
	struct stat Stat[1];
	if (stat(File, Stat)) return 0;
	module_t *Module = module_new(File);
	if (Type) {
		module_loader_t *Loader = module_get_loader(Type);
		if (Loader == 0) {
			log_errorf("Error: loader %s not found.\n", Type);
		} else {
			if (Loader->Load(Module->Providers, File)) return Module;
		};
	} else {
		for (module_loader_t *Loader = ModuleLoaders; Loader; Loader = Loader->Next) {
			if (Loader->Load(Module->Providers, File)) return Module;
		};
	};
	return 0;
};

void *module_load_symbol(const char *Symbol) {
	return dlsym(0, Symbol);
};

void module_init(void) {
	module_t *Module = module_new("Riva/Module");
	module_add_alias(Module, "library:/Riva/Module");
	module_export(Module, "_load", 0, module_load);
	module_export(Module, "_load_file", 0, module_load_file);
	module_export(Module, "_get_path", 0, module_get_path);
	module_export(Module, "_get_name", 0, module_get_name);
	module_export(Module, "_import", 0, module_import);
	module_export(Module, "_lookup", 0, module_lookup);
	module_export(Module, "_new", 0, module_new);
	module_export(Module, "_add_alias", 0, module_add_alias);
	module_export(Module, "_set_path", 0, module_set_path);
	module_export(Module, "_export", 0, module_export);
	module_export(Module, "_add_directory", 0, module_add_directory);
	module_export(Module, "_add_loader", 0, module_add_loader);
	module_export(Module, "_set_import_func", 0, module_importer_set);
	module_export(Module, "_get_default_provider", 0, module_get_default_provider);
	module_export(Module, "_set_version", 0, module_set_version);
	module_export(Module, "_get_version", 0, module_get_version);
	module_export(Module, "_load_symbol", 0, module_load_symbol);
};

void module_init2(void) {
	module_t *SysModule = module_load(0, "Sys/Module");
	int IsRef;
	module_import(SysModule, "T", &IsRef, (void **)&ModuleT);
	for (int I = 0; I < Modules->Size; ++I) {
		if (Modules->Entries[I].Key) {
			module_t *Module = Modules->Entries[I].Value;
			Module->Type = ModuleT;
		};
	};
};
