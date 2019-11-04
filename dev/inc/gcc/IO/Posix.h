#ifndef IO_POSIX_H
#define IO_POSIX_H

#include <IO/Stream.h>
#include <unistd.h>
#include <fcntl.h>

#define RIVA_MODULE IO$Posix
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	int Handle;
};

RIVA_TYPE(T);
RIVA_TYPE(ReaderT);
RIVA_TYPE(WriterT);
RIVA_TYPE(SeekerT);
RIVA_TYPE(TextReaderT);
RIVA_TYPE(TextWriterT);

RIVA_CFUN(Std$Object$t *, new, const Std$Type$t *, int);
RIVA_CFUN(void, register_finalizer, IO$Posix$t *);
RIVA_CFUN(void, unregister_finalizer, IO$Posix$t *);

#undef RIVA_MODULE

#endif

