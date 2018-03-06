#include "libriva.h"

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <dirent.h>
#include <sys/stat.h>


typedef struct fileset_t {
	uint32_t NoOfModules;
	module_t *Modules[];
} fileset_t;

static int fileset_import(fileset_t *FileSet, const char *Symbol, int *IsRef, void **Data) {
	for (int I = 0; I < FileSet->NoOfModules; ++I) {
		if (module_import(FileSet->Modules[I], Symbol, IsRef, Data)) return 1;
	};
	return 0;
};

static fileset_t *fileset_load_next(const char *Path, FILE *List, int Index) {
	char Name[256];
	if (fgets(Name, 256, List) && Name[0]) {
		char *End = Name + strlen(Name);
		while (*End < ' ') *(End--) = 0;
		module_t *Module = module_load(Path, Name);
		if (Module == 0) {
			log_errorf("Error: module not found %s/%s\n", Path, Name);
			return 0;
		};
		fileset_t *FileSet = fileset_load_next(Path, List, Index + 1);
		if (FileSet == 0) return 0;
		FileSet->Modules[Index] = Module;
		return FileSet;
	} else {
		fileset_t *FileSet = GC_MALLOC(sizeof(fileset_t) + Index * sizeof(module_t *));
		FileSet->NoOfModules = Index;
	};
};

static void *fileset_find(const char *Base) {
	struct stat Stat[1];
	char Buffer[strlen(Base) + 6];
	strcpy(stpcpy(Buffer, Base), ".fset");
	if (stat(Buffer, Stat)) return 0;
	if (S_ISDIR(Stat->st_mode)) return GC_strdup(Buffer);
	return 0;
};

static int fileset_load(module_provider_t *Provider, const char *FileName) {
	FILE *List = fopen(path_join(FileName, "list"), "r");
	if (List == 0) return 0;
	fileset_t *FileSet = fileset_load_next(FileName, List, 0);
	fclose(List);
	if (FileSet == 0) return 0;
	module_importer_set(Provider, FileSet, (module_import_func)fileset_import);
	return 1;
}; 

void fileset_init(void) {
	module_add_loader("FileSet", 10, fileset_find, fileset_load);
};
