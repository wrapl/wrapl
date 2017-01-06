#ifndef IO_STRING_H
#define IO_STRING_H

#include <IO/Stream.h>
#include <Std/String.h>

#define RIVA_MODULE IO$String
#include <Riva-Header.h>

RIVA_CFUN(IO$Stream$t *, open, const char *, int);

#define IO$String$OPENREAD		1
#define IO$String$OPENWRITE		2
#define IO$String$OPENTEXT		4
#define IO$String$OPENAPPEND	8

#undef RIVA_MODULE

#endif
