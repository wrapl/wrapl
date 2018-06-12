#include "libriva.h"
#include <string.h>

#ifdef CYGWIN
#	include <sys/cygwin.h>
#	include <unistd.h>
#	define PATHSTR "/"
#	define PATHCHR '/'
#endif

#ifdef MINGW
#	include <windows.h>
#	include <libgen.h>
#	define PATHSTR "\\"
#	define PATHCHR '\\'
#endif

#ifdef LINUX
#	include <unistd.h>
#	include <stdlib.h>
#	define PATHSTR "/"
#	define PATHCHR '/'
#endif

#ifdef MACOSX
#	define PATHSTR "/"
#	define PATHCHR '/'
#endif

const char *path_dir(const char *Path) {
#ifdef CYGWIN
	int Length = strlen(Path);
	char *Dir = GC_MALLOC_ATOMIC(Length + 1);
	char File[Length + 1];
	cygwin_split_path(Path, Dir, File);
	return Dir;
#endif
#ifdef MINGW
	int Length = (const char *)basename(Path) - Path - 1;
	if (Length < 0) return "";
	char *Dir = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Dir, Path, Length);
	Dir[Length] = 0;
	return Dir;
#endif
#ifdef LINUX
	int Length = (const char *)basename(Path) - Path - 1;
	if (Length < 0) return "";
	char *Dir = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Dir, Path, Length);
	Dir[Length] = 0;
	return Dir;
#endif
#ifdef MACOSX
	int Length = strlen(Path) - strlen(basename(Path)) - 1;
	if (Length < 0) return "";
	char *Dir = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Dir, Path, Length);
	Dir[Length] = 0;
	return Dir;
#endif
};

const char *path_file(const char *Path) {
#ifdef CYGWIN
	int Length = strlen(Path);
	char *File = GC_MALLOC_ATOMIC(Length + 1);
	char Dir[Length + 1];
	cygwin_split_path(Path, Dir, File);
	return File;
#else
	return GC_STRDUP(basename(Path));
#endif
};

const char *path_join(const char *Path, const char *Name) {
	if (Path[0] == 0) return GC_STRDUP(Name);
	char *Result = GC_MALLOC_ATOMIC(strlen(Path) + strlen(Name) + 2);
	char *Ptr = stpcpy(Result, Path);
	if (Ptr[-1] != PATHCHR) *Ptr++ = PATHCHR;
	strcpy(Ptr, Name);
	for (char *C = Result; *C; ++C) if (*C == '/') *C = PATHCHR;
	return Result;
};

#ifdef MACOSX
#include <sys/param.h>
#endif

const char *path_fixup(const char *Path) {
#ifdef CYGWIN
	if (Path[0]) {
		return cygwin_create_path(CCP_WIN_A_TO_POSIX, Path);
	} else {
		return getcwd(0, 0);
	};
#endif
#ifdef MINGW
	if (Path[0]) {
		char Buffer[MAX_PATH];
		char *Extension;
		DWORD Length = GetFullPathName(Path, MAX_PATH, Buffer, &Extension);
		return Length ? GC_strdup(Buffer) : GC_strdup(Path);
	};
	return getcwd(0, 0);
#endif
#ifdef LINUX
	if (Path[0]) {
		size_t Length = strlen(Path);
		if (Length == 1 && Path[0] == PATHCHR) return Path;
		if (Path[Length - 1] == PATHCHR) {
			char *Path2 = GC_STRDUP(Path);
			Path2[Length - 1] = 0;
			Path = Path2;
		};
		return realpath(Path, 0);
	};
	return getcwd(0, 0);
#endif
#ifdef MACOSX
	if (Path[0]) {
		char RealPath[PATH_MAX];
		realpath(Path, RealPath);
		return GC_STRDUP(RealPath);
	};
	return getcwd(0, 0);
#endif
};

