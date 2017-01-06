#include "libriva.h"
#include <stdio.h>
#include "log.h"
#include "path.h"

extern const struct Std$Type_t *ModuleT;

static void *dynamic_find(const char *Base) {
	//printf("Base = <%s>\n", Base);
	const char *ModuleName = path_dir(Base);
	if (!ModuleName[0]) return 0;
	//printf("ModuleName = <%s>\n", ModuleName);
	
	module_t *Module = module_load("", ModuleName);
	if (!Module) return 0;
	const char *Import = path_file(Base);
	int IsRef;
	module_t *Data;
	if (module_import0(Module, Import, &IsRef, &Data)) {
		if (Data->Type == ModuleT) {
			Data->Path = Base;
			return Data;
		}
	}
	
	return 0;
};

static int dynamic_load(module_provider_t *Provider, void *Dynamic) {
	module_set_import_func(Provider, Dynamic, (module_import_func)module_import);
	return 1;
};

void dynamic_init(void) {
	module_add_loader("Dynamic", 1222220, dynamic_find, dynamic_load);
};
