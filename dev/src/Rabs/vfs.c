#include <gc/gc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "vfs.h"
#include "util.h"

struct vmount_t {
	const vmount_t *Previous;
	const char *Path;
	const char *Target;
};

extern const char *RootPath;

const vmount_t *vfs_mount(const vmount_t *Previous, const char *Path, const char *Target) {
	//printf("vmount(%s, %s)\n", Path, Target);
	vmount_t *Mount = (vmount_t *)GC_malloc(sizeof(vmount_t));
	Mount->Previous = Previous;
	Mount->Path = concat(RootPath, Path, 0);
	Mount->Target = concat(RootPath, Target, 0);
	return Mount;
}

static char *resolve0(const vmount_t *Mount, const char *Path) {
	//printf("resolve(%s)\n", Path);
	while (Mount) {
		const char *Suffix = match_prefix(Path, Mount->Path);
		if (Suffix) {
			char *Resolved = concat(Mount->Target, Suffix, 0);
			struct stat Stat[1];
			if (stat(Resolved, Stat) == 0) return Resolved;
			Resolved = resolve0(Mount->Previous, Resolved);
			if (Resolved) return Resolved;
		}
		Mount = Mount->Previous;
	}
	return 0;
}

char *vfs_resolve(const vmount_t *Mount, const char *Path) {
	struct stat Stat[1];
	if (stat(Path, Stat) == 0) return concat(Path, 0);
	return resolve0(Mount, Path) ?: concat(Path, 0);
}
void vfs_init() {
	
}
