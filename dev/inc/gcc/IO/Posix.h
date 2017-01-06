#ifndef IO_POSIX_H
#define IO_POSIX_H

#include <IO/Stream.h>
#include <unistd.h>
#include <fcntl.h>

#define RIVA_MODULE IO$Posix
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	int Handle;
};

RIVA_TYPE(T);
RIVA_TYPE(ReaderT);
RIVA_TYPE(WriterT);
RIVA_TYPE(SeekerT);
RIVA_TYPE(TextReaderT);
RIVA_TYPE(TextWriterT);

RIVA_OBJECT(ReadMessage);
RIVA_OBJECT(WriteMessage);
RIVA_OBJECT(FlushMessage);
RIVA_OBJECT(SeekMessage);
RIVA_OBJECT(CloseMessage);
RIVA_OBJECT(PollMessage);

RIVA_TYPE(ReadMessageT);
RIVA_TYPE(WriteMessageT);
RIVA_TYPE(FlushMessageT);
RIVA_TYPE(SeekMessageT);
RIVA_TYPE(CloseMessageT);
RIVA_TYPE(PollMessageT);

RIVA_CFUN(Std$Object_t *, new, const Std$Type_t *, int);
RIVA_CFUN(void, register_finalizer, IO$Posix_t *);
RIVA_CFUN(void, unregister_finalizer, IO$Posix_t *);

#undef RIVA_MODULE

#endif

