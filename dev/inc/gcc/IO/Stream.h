#ifndef IO_STREAM_H
#define IO_STREAM_H

#include <Std/Object.h>
#include <Std/Integer.h>
#include <Sys/Program.h>

#define RIVA_MODULE IO$Stream
#include <Riva-Header.h>

typedef Std$Object$t IO$Stream$t;
typedef Std$Object$t IO$Stream$t;

RIVA_STRUCT(messaget) {
	const Std$Type$t *Type;
	Sys$Program$stack_trace_t *StackTrace;
	const char *Message;
};

RIVA_TYPE(T);
RIVA_TYPE(ReaderT);
RIVA_TYPE(WriterT);
RIVA_TYPE(TextReaderT);
RIVA_TYPE(TextWriterT);
RIVA_TYPE(SeekerT);

RIVA_TYPE(MessageT);
RIVA_OBJECT(ConvertMessage);
RIVA_TYPE(ConvertMessageT);
RIVA_OBJECT(GenericMessage);
RIVA_TYPE(GenericMessageT);
RIVA_OBJECT(OpenMessage);
RIVA_TYPE(OpenMessageT);
RIVA_OBJECT(ReadMessage);
RIVA_TYPE(ReadMessageT);
RIVA_OBJECT(WriteMessage);
RIVA_TYPE(WriteMessageT);
RIVA_OBJECT(FlushMessage);
RIVA_TYPE(FlushMessageT);
RIVA_OBJECT(SeekMessage);
RIVA_TYPE(SeekMessageT);
RIVA_OBJECT(CloseMessage);
RIVA_TYPE(CloseMessageT);
RIVA_OBJECT(PollMessage);
RIVA_TYPE(PollMessageT);

RIVA_CFUN(Std$Object$t *, Message$new, const Std$Type$t *Type, const char *Description);
RIVA_CFUN(Std$Object$t *, Message$new_format, const Std$Type$t *Type, const char *Format, ...);
RIVA_CFUN(Std$Object$t *, Message$from_errno, const Std$Type$t *Type);

typedef enum {
	IO$Stream$CLOSE_BOTH,
	IO$Stream$CLOSE_READ,
	IO$Stream$CLOSE_WRITE
} IO$Stream$closemode;

RIVA_TYPE(CloseModeT);

RIVA_CFUN(int, close, IO$Stream$t *Stream, IO$Stream$closemode Mode);

typedef int (*IO$Stream$readfn)(IO$Stream$t *Stream, char *Buffer, int Count, int Block);

RIVA_CFUN(int, read, IO$Stream$t *Stream, char *Buffer, int Count, int Block);
RIVA_CFUN(char, readc, IO$Stream$t *Stream);
RIVA_CFUN(char *, readx, IO$Stream$t *Stream, int Max, const char *Term, int TermSize);
RIVA_CFUN(char *, readi, IO$Stream$t *Stream, int Max, const char *Term, int TermSize);
RIVA_CFUN(char *, readl, IO$Stream$t *Stream);
RIVA_CFUN(int, eoi, IO$Stream$t *Stream);

typedef int (*IO$Stream$writefn)(IO$Stream$t *Stream, const char *Buffer, int Count, int Block);

RIVA_CFUN(int, write, IO$Stream$t *Stream, const char *Buffer, int Count, int Block);
RIVA_CFUN(int, writec, IO$Stream$t *Stream, char Char);
RIVA_CFUN(int, writes, IO$Stream$t *Stream, const char *String);
RIVA_CFUN(int, writef, IO$Stream$t *Stream, const char *Format, ...);
RIVA_CFUN(int, flush, IO$Stream$t *Stream);

#define IO$Stream$BufferSize 1000
RIVA_STRUCT(buffer) {
	struct IO$Stream$buffer *Next;
	char Chars[IO$Stream$BufferSize];
};

RIVA_CFUN(IO$Stream$buffer *, alloc_buffer);
RIVA_CFUN(void, free_buffer, IO$Stream$buffer *Buffer);
RIVA_CFUN(void, free_buffers, IO$Stream$buffer *First, IO$Stream$buffer *Last);

typedef enum {
	IO$Stream$SEEK_SET,
	IO$Stream$SEEK_CUR,
	IO$Stream$SEEK_END
} IO$Stream$seekmode;

RIVA_TYPE(SeekModeT);

RIVA_CFUN(int, seek, IO$Stream$t *Stream, int, IO$Stream$seekmode Mode);
RIVA_CFUN(int, tell, IO$Stream$t *Stream);
RIVA_CFUN(size_t, remaining, IO$Stream$t *Stream);

#ifdef __GNUC__
#include <stdio.h>

RIVA_CFUN(FILE *, cfile, IO$Stream$t *);

#endif

#undef RIVA_MODULE

#define RIVA_MODULE IO$Stream$Message

RIVA_CFUN(Std$Object$t *, new, const Std$Type$t *, const char *);
RIVA_CFUN(Std$Object$t *, new_format, const Std$Type$t *, const char *, ...);
RIVA_CFUN(Std$Object$t *, from_errno, const Std$Type$t *);

#undef RIVA_MODULE

#endif
