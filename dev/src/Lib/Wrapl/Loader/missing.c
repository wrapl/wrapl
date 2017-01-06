#include "missing.h"

#ifdef WINDOWS

#include <string.h>

#ifdef MINGW

void *mempcpy(void *Dst, const void *Src, size_t Len) {
	memcpy(Dst, Src, Len);
	return Dst + Len;
};

#endif

char *stpcpy(char *Dst, const char *Src) {
	return (char *)mempcpy(Dst, Src, strlen(Src));
};

#endif

#ifdef LINUX
#endif
