#include "libriva.h"
#include <stdio.h>

static void *make_symbol0(const char *Name);
static void add_methods0(void *Data);

void *(*make_symbol)(const char *) = make_symbol0;
void (*add_methods)(void *) = add_methods0;
module_t *Symbol = 0;

static int symbol_import(const void *Ignore, const char *Name, int *IsRef, void **Data) {
	*IsRef = 0;
	*Data = make_symbol(Name);
	return 1;
};

static int symbol_import0(void *Ignore, const char *Name, int *IsRef, void **Data) {
	module_t *LangSymbol = module_load(0, "Std/Symbol");
	int IsRef0;
	module_import(LangSymbol, "_new_string", &IsRef0, (void **)&make_symbol);
	module_import(LangSymbol, "_add_methods", &IsRef0, (void **)&add_methods);
	module_importer_set(Symbol->Providers, 0, (module_import_func)symbol_import);
	return module_import(Symbol, Name, IsRef, Data);
};

static void *make_symbol0(const char *Name) {
	module_t *LangSymbol = module_load(0, "Std/Symbol");
	int IsRef0;
	module_import(LangSymbol, "_new_string", &IsRef0, (void **)&make_symbol);
	module_import(LangSymbol, "_add_methods", &IsRef0, (void **)&add_methods);
	return make_symbol(Name);
}

static void add_methods0(void *Data) {
	module_t *LangSymbol = module_load(0, "Std/Symbol");
	int IsRef0;
	module_import(LangSymbol, "_new_string", &IsRef0, (void **)&make_symbol);
	module_import(LangSymbol, "_add_methods", &IsRef0, (void **)&add_methods);
	return add_methods(Data);
}

void symbol_init(void) {
	Symbol = module_new("Riva/Symbol");
	module_add_alias(Symbol, "library:/Riva/Symbol");
	module_importer_set(Symbol->Providers, 0, (module_import_func)symbol_import0);
};
