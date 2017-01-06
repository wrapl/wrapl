#include <stdio.h>#include <pthread.h>

#include "libriva.h"

static const char **Args;
static unsigned int NoOfArgs = 0;

int main(int Argc, const char **Argv) {
	libriva_config("/usr/bin/riva.conf");
	const char *MainModule = Argv[1];
	Args = Argv + 2;
	NoOfArgs = Argc - 2;
	module_t *System = module_new("Riva/System");
	module_add_alias(System, "library:/Riva/System");
	module_export(System, "_Args", 0, &Args);
	module_export(System, "_NoOfArgs", 0, &NoOfArgs);
	module_t *Module = module_load_file(MainModule, "Riva");
	if (Module == 0) printf("Error: module %s not found\n", MainModule);
	pthread_exit(0);
};
