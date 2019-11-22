#ifndef IO_WINDOWS_H
#define IO_WINDOWS_H

#include <IO/Stream.h>
#include <windows.h>

#define RIVA_MODULE IO$Windows
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	Std$Type$t *Type;
	HANDLE Handle;
};

RIVA_TYPE(T);
RIVA_TYPE(ReaderT);
RIVA_TYPE(WriterT);
RIVA_TYPE(SeekerT);
RIVA_TYPE(TextReaderT);
RIVA_TYPE(TextWriterT);

RIVA_CFUN(IO$Windows_t *, new, Std$Type$t *, int);
RIVA_CFUN(void, register_finalizer, IO$Windows_t *);

#undef RIVA_MODULE

#endif

