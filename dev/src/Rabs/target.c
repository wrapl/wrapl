#include "target.h"
#include "rabs.h"
#include "util.h"
#include "context.h"
#include "cache.h"
#include "map.h"
#include <lauxlib.h>
#include <libHX/map.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <gc.h>
#include <errno.h>

typedef struct target_class_t target_class_t;
typedef struct target_update_t target_update_t;
typedef struct target_pair_t target_pair_t;
typedef struct target_file_t target_file_t;
typedef struct target_meta_t target_meta_t;
typedef struct target_scan_t target_scan_t;
typedef struct target_symb_t target_symb_t;

extern const char *RootPath;
static int TargetMetatable;
static int BuildScanTarget;
static struct map_t *TargetCache;
struct target_t *CurrentTarget = 0;
static struct map_t *CurrentDepends = 0;

struct target_class_t {
	size_t Size;
	void (*tostring)(target_t *Target, luaL_Buffer *Buffer);
	int (*hash)(target_t *Target, uint8_t Digest[SHA256_DIGEST_SIZE]);
	int (*missing)(target_t *Target);
};

#define TARGET_FIELDS \
	const target_class_t *Class; \
	int Ref, Build, Version; \
	context_t *BuildContext; \
	const char *Id; \
	struct map_t *Depends; \
	struct map_t *Scans;

struct target_t {
	TARGET_FIELDS
};

void target_depends_add(target_t *Target, target_t *Depend) {
	if (Target != Depend) {
		map_set(Target->Depends, Depend->Id, Depend);
	}
}

static int target_function_hash(lua_State *L, const void *P, size_t Size, struct sha256_ctx *Ctx) {
	sha256_update(Ctx, Size, P);
	return 0;
}

static void target_value_hash(int8_t Hash[SHA256_DIGEST_SIZE]) {
	switch (lua_type(L, -1)) {
	case LUA_TNIL: {
		memset(Hash, 0, SHA256_DIGEST_SIZE);
		return;
	}
	case LUA_TNUMBER: {
		memset(Hash, 0, SHA256_DIGEST_SIZE);
		*(lua_Number *)Hash = lua_tonumber(L, -1);
		Hash[SHA256_DIGEST_SIZE - 1] = LUA_TNUMBER;
		return;
	}
	case LUA_TBOOLEAN: {
		*(int *)Hash = lua_toboolean(L, -1);
		memset(Hash, 0, SHA256_DIGEST_SIZE);
		Hash[SHA256_DIGEST_SIZE - 1] = LUA_TBOOLEAN;
		return;
	}
	case LUA_TSTRING: {
		size_t Len;
		const char *String = lua_tolstring(L, -1, &Len);
		struct sha256_ctx Ctx[1];
		sha256_init(Ctx);
		sha256_update(Ctx, Len, String);
		sha256_digest(Ctx, SHA256_DIGEST_SIZE, Hash);
		return;
	}
	case LUA_TTABLE: {
		struct sha256_ctx Ctx[1];
		sha256_init(Ctx);
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			int8_t ChildHash[SHA256_DIGEST_SIZE];
			target_value_hash(ChildHash);
			sha256_update(Ctx, SHA256_DIGEST_SIZE, ChildHash);
			lua_pop(L, 1);
			target_value_hash(ChildHash);
			sha256_update(Ctx, SHA256_DIGEST_SIZE, ChildHash);
		}
		sha256_digest(Ctx, SHA256_DIGEST_SIZE, Hash);
		return;
	}
	case LUA_TFUNCTION: {
		struct sha256_ctx Ctx[1];
		sha256_init(Ctx);
		lua_dump(L, (void *)target_function_hash, Ctx, 1);
		sha256_digest(Ctx, SHA256_DIGEST_SIZE, Hash);
		return;
	}
	}
	target_t *Target = luaL_checkudata(L, -1, "target");
	struct sha256_ctx Ctx[1];
	sha256_init(Ctx);
	sha256_update(Ctx, strlen(Target->Id), Target->Id);
	sha256_digest(Ctx, SHA256_DIGEST_SIZE, Hash);
	return;
}

void target_update(target_t *Target) {
	if (Target->Version == -1) {
		printf("\e[31mError: build cycle with %s\e[0m\n", Target->Id);
		exit(1);
	}
	int Top = lua_gettop(L);
	if (Target->Version == 0) {
		Target->Version = -1;
		int DependsVersion = 1;
		for (struct map_node_t *Node = Target->Depends->head; Node; Node = Node->next) {
			//printf("\e[35m%s depends on %s\e[0m\n", Target->Id, ((target_t *)Node->value)->Id);
			target_t *Depends = (target_t *)Node->value;
			target_update(Depends);
			if (Depends->Version > DependsVersion) DependsVersion = Depends->Version;
		}
		if (Target->Build) {
			struct map_t *DetectedDepends = cache_depends_get(Target->Id);
			if (DetectedDepends) {
				for (struct map_node_t *Node = DetectedDepends->head; Node; Node = Node->next) {
					target_t *Depends = (target_t *)Node->value;
					//printf("\e[35m%s depends on %s\e[0m\n", Target->Id, Depends->Id);

					target_update(Depends);
					if (Depends->Version > DependsVersion) DependsVersion = Depends->Version;
				}
			}
			int8_t Previous[SHA256_DIGEST_SIZE];
			int8_t Current[SHA256_DIGEST_SIZE];
			lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Build);
			target_value_hash(Current);
			lua_pop(L, 1);
			const char *BuildId = concat(Target->Id, "::build", 0);
			if (!cache_hash_get(BuildId, Previous) || memcmp(Previous, Current, SHA256_DIGEST_SIZE)) {
				cache_hash_set(BuildId, Current, CurrentVersion);
				DependsVersion = CurrentVersion;
			}
			int PreviousVersion = cache_hash_get(Target->Id, Previous);
			printf("\e[33mtarget_build(%s) Depends = %d, Previous = %d\e[0m\n", Target->Id, DependsVersion, PreviousVersion);
			if ((DependsVersion > PreviousVersion) || Target->Class->missing(Target)) {
				target_t *PreviousTarget = CurrentTarget;
				struct map_t *PreviousDepends = CurrentDepends;
				context_t *PreviousContext = CurrentContext;
				CurrentTarget = Target;
				CurrentDepends = new_map();
				CurrentContext = Target->BuildContext;
				chdir(concat(RootPath, CurrentContext->Path, 0));
				lua_pushcfunction(L, msghandler);
				lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Build);
				lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
				if (lua_pcall(L, 1, 0, -3) != LUA_OK) {
					fprintf(stderr, "\e[31mError: %s: %s\e[0m", Target->Id, lua_tostring(L, -1));
					exit(1);
				}
				lua_pop(L, 1);
				for (struct map_node_t *Node = CurrentDepends->head; Node; Node = Node->next) {
					target_t *Depends = (target_t *)Node->value;
					//printf("\e[36m%s depends on %s\e[0m\n", Target->Id, Depends->Id);
					target_update(Depends);
					if (Depends->Version > DependsVersion) DependsVersion = Depends->Version;
					//Depend->Class->hash(Depend, Current);
				}
				cache_depends_set(Target->Id, CurrentDepends);
				Target->Class->hash(Target, Current);
				if (PreviousVersion == 0) {
					Target->Version = CurrentVersion;
				} else if (memcmp(Previous, Current, SHA256_DIGEST_SIZE)) {
					Target->Version = CurrentVersion;
				} else {
					Target->Version = DependsVersion;
				}
				cache_hash_set(Target->Id, Current, Target->Version);
				CurrentTarget = PreviousTarget;
				CurrentDepends = PreviousDepends;
				CurrentContext = PreviousContext;
				chdir(concat(RootPath, CurrentContext->Path, 0));
			} else {
				Target->Version = PreviousVersion;
			}
		} else {
			int8_t Previous[SHA256_DIGEST_SIZE];
			int8_t Current[SHA256_DIGEST_SIZE];
			Target->Class->hash(Target, Current);
			int PreviousVersion = cache_hash_get(Target->Id, Previous);
			if (PreviousVersion == 0) {
				Target->Version = CurrentVersion;
			} else if (memcmp(Previous, Current, SHA256_DIGEST_SIZE)) {
				Target->Version = CurrentVersion;
			} else {
				Target->Version = DependsVersion;
			}
			cache_hash_set(Target->Id, Current, Target->Version);
		}
	}
	if (Top != lua_gettop(L)) {
		printf("Warning: building %s changed the lua stack from %d to %d\n", Target->Id, Top, lua_gettop(L));
	}
}

void target_depends_auto(target_t *Depend) {
	if (CurrentTarget && CurrentTarget != Depend && !map_get(CurrentTarget->Depends, Depend->Id)) {
		map_set(CurrentDepends, Depend->Id, Depend);
	}
}

void target_push(target_t *Target) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
}

static target_t *target_new(target_class_t *Class, const char *Id) {
	target_t *Target = (target_t *)lua_newuserdata(L, Class->Size);
	lua_rawgeti(L, LUA_REGISTRYINDEX, TargetMetatable);
	lua_setmetatable(L, -2);
	Target->Class = Class;
	Target->Id = Id;
	Target->Build = 0;
	Target->Version = 0;
	Target->Depends = new_map();
	Target->Scans = new_map();
	Target->Ref = luaL_ref(L, LUA_REGISTRYINDEX);
	map_set(TargetCache, Id, Target);
	return Target;
}

static int target_default_missing(target_t *Target) {
	return 0;
}

int target_tostring(lua_State *L) {
	target_t *Target = luaL_checkudata(L, 1, "target");
	luaL_Buffer Buffer[1];
	luaL_buffinit(L, Buffer);
	Target->Class->tostring(Target, Buffer);
	luaL_pushresult(Buffer);
	return 1;
}

static int target_concat(lua_State *L) {
	const char *String;
	if ((String = lua_tostring(L, 1))) {
		lua_pushstring(L, String);
	} else if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
	} else {
		return luaL_error(L, "Cannot convert arg 1 to string");
	}
	if ((String = lua_tostring(L, 2))) {
		lua_pushstring(L, String);
	} else if (luaL_callmeta(L, 2, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
	} else {
		return luaL_error(L, "Cannot convert arg 2 to string");
	}
	lua_concat(L, 2);
	return 1;
}

struct target_file_t {
	TARGET_FIELDS
	int Absolute;
	const char *Path;
};

static void target_file_tostring(target_file_t *Target, luaL_Buffer *Buffer) {
	//target_depends_auto((target_t *)Target);
	if (Target->Absolute) {
		luaL_addstring(Buffer, Target->Path);
	} else {
		const char *Path = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", Target->Path, 0));
		luaL_addstring(Buffer, Path);
	}
}

static int target_file_hash(target_file_t *Target, uint8_t Digest[SHA256_DIGEST_SIZE]) {
	const char *FileName;
	if (Target->Absolute) {
		FileName = Target->Path;
	} else {
		FileName = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", Target->Path, 0));
	}
	struct stat Stat[1];
	if (stat(FileName, Stat)) {
		printf("\e[31mError: rule failed to build: %s\e[0m\n", FileName);
		//exit(1);
	}
	if (!S_ISREG(Stat->st_mode)) {
		memset(Digest, -1, SHA256_DIGEST_SIZE);
	} else {
		int File = open(FileName, 0, O_RDONLY);
		struct sha256_ctx Ctx[1];
		uint8_t Buffer[256];
		sha256_init(Ctx);
		for (;;) {
			int Count = read(File, Buffer, 256);
			if (Count == 0) break;
			if (Count == -1) {
				printf("\e[31mError: read error: %s\e[0m\n", FileName);
				exit(1);
			}
			sha256_update(Ctx, Count, Buffer);
		}
		close(File);
		sha256_digest(Ctx, SHA256_DIGEST_SIZE, Digest);
	}
}

static int target_file_missing(target_file_t *Target) {
	const char *FileName;
	if (Target->Absolute) {
		FileName = Target->Path;
	} else {
		FileName = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", Target->Path, 0));
	}
	struct stat Stat[1];
	return !!stat(FileName, Stat);
}

/*static target_status_t target_file_update(target_file_t *Target, int Updated) {
	const char *FileName;
	if (Target->Absolute) {
		FileName = Target->Path;
	} else {
		FileName = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", Target->Path, 0));
	}
	struct stat Stat[1];
	int Missing = stat(FileName, Stat);
	//printf("\e[33m%s: Updated = %d, Missing = %d\e[0m\n", FileName, Updated, Missing);
	if (!Missing && !S_ISREG(Stat->st_mode)) return STATUS_UNCHANGED;
	if (Target->Build && (Missing || Updated)) {
		context_t *PreviousContext = CurrentContext;
		CurrentContext = Target->BuildContext;
		chdir(concat(RootPath, CurrentContext->Path, 0));
		lua_pushcfunction(L, msghandler);
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Build);
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
		if (lua_pcall(L, 1, 0, -3) != LUA_OK) {
			fprintf(stderr, "\e[31mError: %s: %s\e[0m", FileName, lua_tostring(L, -1));
			exit(1);
		}
		lua_pop(L, 1);
		CurrentContext = PreviousContext;
		chdir(concat(RootPath, CurrentContext->Path, 0));
	}
	int8_t Previous[SHA256_DIGEST_SIZE];
	int8_t Current[SHA256_DIGEST_SIZE];
	if (stat(FileName, Stat)) {
		printf("\e[31mError: rule failed to build file: %s\e[0m\n", FileName);
		exit(1);
	}
	if (!S_ISREG(Stat->st_mode)) {
		memset(Current, -1, SHA256_DIGEST_SIZE);
	} else {
		int File = open(FileName, 0, O_RDONLY);
		struct sha256_ctx Ctx[1];
		uint8_t Buffer[256];
		sha256_init(Ctx);
		for (;;) {
			int Count = read(File, Buffer, 256);
			if (Count == 0) break;
			if (Count == -1) {
				printf("\e[31mError: read error: %s\e[0m\n", FileName);
				exit(1);
			}
			sha256_update(Ctx, Count, Buffer);
		}
		close(File);
		sha256_digest(Ctx, SHA256_DIGEST_SIZE, Current);
	}
	if (cache_hash_get(Target->Id, Previous)) {
		if (!memcmp(Previous, Current, SHA256_DIGEST_SIZE)) {
			return STATUS_UNCHANGED;
		}
	}
	cache_hash_set(Target->Id, Current);
	return STATUS_UPDATED;
}*/

target_class_t FileClass[] = {{
	sizeof(target_file_t),
	(void *)target_file_tostring,
	(void *)target_file_hash,
	(void *)target_file_missing
}};

static target_t *target_file_check(const char *Path, int Absolute) {
	Path = concat(Path, 0);
	const char *Id = concat("file:", Path, 0);
	target_file_t *Target = (target_file_t *)map_get(TargetCache, Id);
	if (!Target) {
		Target = (target_file_t *)target_new(FileClass, Id);
		Target->Absolute = Absolute;
		Target->Path = Path;
	}
	return (target_t *)Target;
}

int target_file_new(lua_State *L) {
	const char *Path = luaL_checkstring(L, 1);
	target_t *Target;
	if (Path[0] != '/') {
		Path = concat(CurrentContext->Path, "/", Path, 0) + 1;
		Target = target_file_check(Path, 0);
	} else {
		const char *Relative = match_prefix(Path, RootPath);
		if (Relative) {
			Target = target_file_check(Relative + 1, 0);
		} else {
			Target = target_file_check(Path, 1);
		}
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	return 1;
}

int target_file_dir(lua_State *L) {
	target_file_t *FileTarget = (target_file_t *)luaL_checkudata(L, 1, "target");
	char *Path;
	int Absolute;
	if ((lua_gettop(L) == 2) && lua_toboolean(L, 2)) {
		Path = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", FileTarget->Path, 0));
		Absolute = 1;
	} else {
		Path = concat(FileTarget->Path, 0);
		Absolute = FileTarget->Absolute;
	}
	char *Last = Path;
	for (char *P = Path; *P; ++P) if (*P == '/') Last = P;
	*Last = 0;
	target_t *Target = target_file_check(Path, Absolute);
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	return 1;
}

int target_file_basename(lua_State *L) {
	target_file_t *FileTarget = (target_file_t *)luaL_checkudata(L, 1, "target");
	const char *Path = FileTarget->Path;
	const char *Last = Path;
	for (const char *P = Path; *P; ++P) if (*P == '/') Last = P;
	lua_pushstring(L, concat(Last + 1, 0));
	return 1;
}

int target_file_exists(lua_State *L) {
	target_file_t *Target = (target_file_t *)luaL_checkudata(L, 1, "target");
	const char *FileName;
	if (Target->Absolute) {
		FileName = Target->Path;
	} else {
		FileName = vfs_resolve(CurrentContext->Mounts, concat(RootPath, "/", Target->Path, 0));
	}
	struct stat Stat[1];
	if (!stat(FileName, Stat)) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

int target_div(lua_State *L) {
	target_file_t *FileTarget = (target_file_t *)luaL_checkudata(L, 1, "target");
	if (FileTarget->Class != FileClass) return luaL_error(L, "Error: target is not a file");
	const char *Path = concat(FileTarget->Path, "/", luaL_checkstring(L, 2), 0);
	target_t *Target = target_file_check(Path, FileTarget->Absolute);
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	return 1;
}

int target_mod(lua_State *L) {
	target_file_t *FileTarget = (target_file_t *)luaL_checkudata(L, 1, "target");
	if (FileTarget->Class != FileClass) return luaL_error(L, "Error: target is not a file");
	const char *Replacement = luaL_checkstring(L, 2);
	char *Path = concat(FileTarget->Path, ".", Replacement, 0);
	for (char *End = Path + strlen(FileTarget->Path); --End >= Path;) {
		if (*End == '.') {
			strcpy(End + 1, Replacement);
			break;
		} else if (*End == '/') {
			break;
		}
	}
	target_t *Target = target_file_check(Path, FileTarget->Absolute);
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	return 1;
}

struct target_meta_t {
	TARGET_FIELDS
	const char *Name;
};

static void target_meta_tostring(target_meta_t *Target, luaL_Buffer *Buffer) {
	luaL_addstring(Buffer, CurrentContext->Path);
	luaL_addstring(Buffer, "::");
	luaL_addstring(Buffer, Target->Name);
}

static void target_meta_hash(target_meta_t *Target, uint8_t Digest[SHA256_DIGEST_SIZE]) {
	memset(Digest, -1, SHA256_DIGEST_SIZE);
}

/*static target_status_t target_meta_build(target_meta_t *Target, int Updated) {
	return Updated ? STATUS_UPDATED : STATUS_UNCHANGED;
}*/

static target_class_t MetaClass[] = {{
	sizeof(target_meta_t),
	(void *)target_meta_tostring,
	(void *)target_meta_hash,
	target_default_missing
}};

int target_meta_new(lua_State *L) {
	const char *Name = luaL_checkstring(L, 1);
	const char *Id = concat("meta:", CurrentContext->Path, "::", Name, 0);
	target_meta_t *Target = (target_meta_t *)target_new(MetaClass, Id);
	Target->Name = Name;
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	return 1;
}

int target_depends(lua_State *L) {
	target_t *Target = luaL_checkudata(L, 1, "target");
	if (lua_type(L, 2) == LUA_TTABLE) {
		lua_pushnil(L);
		while (lua_next(L, 2)) {
			lua_pushcfunction(L, target_depends);
			lua_pushvalue(L, 1);
			lua_pushvalue(L, -3);
			lua_call(L, 2, 0);
			lua_pop(L, 1);
		}
	} else {
		target_t *Depend = luaL_checkudata(L, 2, "target");
		map_set(Target->Depends, Depend->Id, Depend);
	}
	lua_pushvalue(L, 1);
	return 1;
}

static int target_build(lua_State *L) {
	target_t *Target = luaL_checkudata(L, 1, "target");
	if (Target->Build) {
		//return luaL_error(L, "Error: multiple build rules defined for target %s", Target->Id);
	}
	lua_pushvalue(L, 2);
	Target->Build = luaL_ref(L, LUA_REGISTRYINDEX);
	Target->BuildContext = CurrentContext;
	Target->Version = 0;
	lua_pushvalue(L, 1);
	return 1;
}

struct target_scan_t {
	TARGET_FIELDS
	const char *Name;
	target_t *Source;
	int Scan;
};

static void target_scan_tostring(target_scan_t *Target, luaL_Buffer *Buffer) {
}

static void target_scan_hash(target_scan_t *Target, uint8_t Digest[SHA256_DIGEST_SIZE]) {
	memset(Digest, -1, SHA256_DIGEST_SIZE);
}

/*static target_status_t target_scan_update(target_scan_t *Target, int Updated) {
	target_status_t Status = STATUS_UNCHANGED;
	struct map_t *ScanTargets = cache_scan_get(Target->Id);
	if (!ScanTargets || Updated) {
		context_t *PreviousContext = CurrentContext;
		CurrentContext = Target->BuildContext;
		chdir(concat(RootPath, CurrentContext->Path, 0));
		lua_pushcfunction(L, msghandler);
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Build);
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Source->Ref);
		if (lua_pcall(L, 1, 1, -3) != LUA_OK) {
			fprintf(stderr, "Error: %s: %s", Target->Source->Path, lua_tostring(L, -1));
			exit(1);
		}
		CurrentContext = PreviousContext;
		chdir(concat(RootPath, CurrentContext->Path, 0));
		luaL_checktype(L, -1, LUA_TTABLE);
		ScanTargets = new_map();
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			target_t *ScanTarget = (target_t *)luaL_checkudata(L, -1, "target");
			//printf("ScanTarget = %s\n", ScanTarget->Id);
			map_set(ScanTargets, ScanTarget->Id, ScanTarget);
			lua_pop(L, 1);
		}
		lua_pop(L, 2);
		cache_scan_set(Target->Id, ScanTargets);
	}
	for (struct map_node_t *Node = ScanTargets->head; Node; Node = Node->next) {
		//printf("%s scanned on %s\n", Target->Id, ((target_t *)Node->value)->Id);
		target_status_t SubStatus = target_update((target_t *)Node->value);
		if (SubStatus > Status) Status = SubStatus;
	}
	return Status;
}*/

static target_class_t ScanClass[] = {{
	sizeof(target_scan_t),
	(void *)target_scan_tostring,
	(void *)target_scan_hash,
	target_default_missing
}};

static int build_scan_target(lua_State *L) {
	target_scan_t *Target = (target_scan_t *)luaL_checkudata(L, 1, "target");
	lua_pushcfunction(L, msghandler);
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Scan);
	lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Source->Ref);
	if (lua_pcall(L, 1, 1, -3) != LUA_OK) {
		fprintf(stderr, "Error: %s: %s", Target->Id, lua_tostring(L, -1));
		exit(1);
	}
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		target_t *ScanTarget = (target_t *)luaL_checkudata(L, -1, "target");
		map_set(CurrentDepends, ScanTarget->Id, ScanTarget);
		lua_pop(L, 1);
	}
	lua_pop(L, 2);
	return 0;
}

int target_scan_new(lua_State *L) {
	target_t *ParentTarget = (target_t *)luaL_checkudata(L, 1, "target");
	const char *Name = luaL_checkstring(L, 2);
	target_scan_t *Target = (target_scan_t *)map_get(ParentTarget->Scans, Name);
	if (Target) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	} else {
		const char *Id = concat("scan:", ParentTarget->Id, "::", Name, 0);
		Target = (target_scan_t *)target_new(ScanClass, Id);
		map_set(Target->Depends, ParentTarget->Id, ParentTarget);
		Target->Name = Name;
		Target->Source = ParentTarget;
		Target->Build = BuildScanTarget;
		Target->BuildContext = CurrentContext;
		lua_pushvalue(L, 3);
		Target->Scan = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_rawgeti(L, LUA_REGISTRYINDEX, Target->Ref);
	}
	return 1;
}

struct target_symb_t {
	TARGET_FIELDS
	const char *Name;
	const char *Path;
};

static void target_symb_tostring(target_symb_t *Target, luaL_Buffer *Buffer) {
}

static void target_symb_hash(target_symb_t *Target, uint8_t Digest[SHA256_DIGEST_SIZE]) {
	context_t *Context = context_find(Target->Path);
	context_symb_get(Context, Target->Name);
	target_value_hash(Digest);
	/*printf("target_symb_hash(%s)\n", Target->Id, Target->Name);
	for (int I = 0; I < SHA256_DIGEST_SIZE; ++I) printf(" %02x", Current[I] & 0xFF);
	printf("\n");*/
	lua_pop(L, 1);
}

/*static target_status_t target_symb_update(target_symb_t *Target, int Updated) {
	context_t *Context = context_find(Target->Path);
	context_symb_get(Context, Target->Name);
	int8_t Current[SHA256_DIGEST_SIZE];
	target_value_hash(Current);
	lua_pop(L, 1);
	int8_t Previous[SHA256_DIGEST_SIZE];
	if (cache_hash_get(Target->Id, Previous)) {
		if (!memcmp(Previous, Current, SHA256_DIGEST_SIZE)) {
			return STATUS_UNCHANGED;
		}
	}
	cache_hash_set(Target->Id, Current);
	return STATUS_UPDATED;
}*/

static target_class_t SymbClass[] = {{
	sizeof(target_symb_t),
	(void *)target_symb_tostring,
	(void *)target_symb_hash,
	target_default_missing
}};

target_t *target_symb_new(const char *Name) {
	const char *Id = concat("symb:", CurrentContext->Path, CurrentContext->Name, "/", Name, 0);
	target_symb_t *Target = (target_symb_t *)map_get(TargetCache, Id);
	if (!Target) {
		Target = (target_symb_t *)target_new(SymbClass, Id);
		Target->Path = CurrentContext->Path;
		Target->Name = Name;
	}
	return (target_t *)Target;
}

static int target_index(lua_State *L) {
	target_t *Target = luaL_checkudata(L, 1, "target");
	const char *Method = luaL_checkstring(L, 2);
	if (!strcmp(Method, "depends")) {
		lua_pushcfunction(L, target_depends);
		return 1;
	}
	if (!strcmp(Method, "build")) {
		lua_pushcfunction(L, target_build);
		return 1;
	}
	if (!strcmp(Method, "scan")) {
		lua_pushcfunction(L, target_scan_new);
		return 1;
	}
	if (Target->Class == FileClass) {
		if (!strcmp(Method, "dir")) {
			lua_pushcfunction(L, target_file_dir);
			return 1;
		}
		if (!strcmp(Method, "basename")) {
			lua_pushcfunction(L, target_file_basename);
			return 1;
		}
		if (!strcmp(Method, "exists")) {
			lua_pushcfunction(L, target_file_exists);
			return 1;
		}
	}
	return 0;
}

target_t *target_find(const char *Id) {
	target_t *Target = (target_t *)map_get(TargetCache, Id);
	if (Target) return Target;
	if (!memcmp(Id, "file", 4)) return target_file_check(Id + 5, Id[5] == '/');
	if (!memcmp(Id, "symb", 4)) {
		target_symb_t *Target = (target_symb_t *)target_new(SymbClass, Id);
		const char *Name;
		for (Name = Id + strlen(Id); --Name > Id + 5;) {
			if (*Name == '/') break;
		}
		size_t PathLength = Name - Id - 5;
		char *Path = GC_malloc_atomic(PathLength);
		memcpy(Path, Id + 5, PathLength);
		Path[PathLength] = 0;
		Target->Path = Path;
		Target->Name = Name + 1;
		return (target_t *)Target;
	}
	return 0;
}

static const luaL_Reg TargetMethods[] = {
	{"__tostring", target_tostring},
	{"__concat", target_concat},
	{"__index", target_index},
	{"__call", target_depends},
	{"__div", target_div},
	{"__mod", target_mod},
	{0, 0}
};

void target_init() {
	luaL_newmetatable(L, "target");
	luaL_setfuncs(L, TargetMethods, 0);
	TargetMetatable = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushcfunction(L, build_scan_target);
	BuildScanTarget = luaL_ref(L, LUA_REGISTRYINDEX);
	TargetCache = new_map();
}
