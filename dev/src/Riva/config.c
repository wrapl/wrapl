#include "libriva.h"

static stringtable_t Config = {{0, 0, 0}};

void *config_get(const char *Key) {
	return stringtable_get(Config, Key);
};

void config_set(const char *Key, void *Value) {
	stringtable_put(Config, Key, Value);
};

void config_init(void) {
	module_t *Module = module_new("Riva/Config");
	module_add_alias(Module, "library:/Riva/Config");
	module_export(Module, "_get", 0, config_get);
	module_export(Module, "_set", 0, config_set);
};
