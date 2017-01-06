#include "libriva.h"
#include <stdio.h>
#include <sys/stat.h>
#include "log.h"
#include "path.h"

int directory_import(const char *Path, const char *Name, int *IsRef, void **Data) {
	static void *(*load_module)(const char *Path, const char *Name) = 0;
	if (load_module == 0) {
		module_t *SysModule = module_load(0, "Sys/Module");
		int IsRef0;
		module_import(SysModule, "_load", &IsRef0, (void **)&load_module);
	};
	void *Module = load_module(Path, Name);
	if (Module) {
		*IsRef = 0;
		*Data = Module;
		return 1;
	} else {
		return 0;
	};
};

void *directory_find(const char *Base) {
	struct stat Stat[1];
	if (stat(Base, Stat)) return 0;
	if (S_ISDIR(Stat->st_mode)) return Base;
	return 0;
};

static int directory_load(module_provider_t *Provider, const char *FileName) {
	struct stat Stat[1];
	stat(FileName, Stat);
	if (S_ISDIR(Stat->st_mode)) {
		module_set_import_func(Provider, path_fixup(FileName), (module_import_func)directory_import);
		return 1;
	} else {
		return 0;
	};
};

void directory_init(void) {
	module_add_loader("Directory", 1000, directory_find, directory_load);
};
