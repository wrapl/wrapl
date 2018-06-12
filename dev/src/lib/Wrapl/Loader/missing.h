#ifndef MISSING_H
#define MISSING_H

#define PATHSTR "/"
#define PATHCHR '/'

#ifdef WINDOWS

extern "C" char *stpcpy(char *Dst, const char *Src);

#endif

#ifdef MINGW

#include <stdarg.h>

extern "C" int asprintf(char **, char *, ...); 
extern "C" int vasprintf(char **, char *, va_list);

#endif

#endif
