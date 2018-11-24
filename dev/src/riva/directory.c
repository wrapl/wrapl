#include "libriva.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
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

int directory_suggest(const char *Path, const char *Prefix, module_suggest_callback Callback, void *Data) {
	int Length = strlen(Prefix);
	DIR *Dir = opendir(Path);
	if (Dir) for (;;) {
		struct dirent *Entry = readdir(Dir);
		if (!Entry) break;
		if (!memcmp(Entry->d_name, Prefix, Length)) {
			char *End = Entry->d_name;
			while (*End && *End != '.') ++End;
			int Length2 = End - Entry->d_name;
			char *Name = GC_malloc_atomic(Length2 + 1);
			memcpy(Name, Entry->d_name, Length2);
			Name[Length2] = 0;
			Callback(Name, Data);
		}
	}
	closedir(Dir);
	return 0;
}

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
		module_importer_set(Provider, path_fixup(FileName), (module_import_func)directory_import);
		module_suggest_set(Provider, (module_suggest_func)directory_suggest);
		return 1;
	} else {
		return 0;
	};
};

void directory_init(void) {
	module_add_loader("Directory", 1000, directory_find, directory_load);
};
