#ifndef IO_STREAM_H
#define IO_STREAM_H

#include <Std/Object.h>
#include <Std/Integer.h>

#define RIVA_MODULE IO$Stream
#include <Riva-Header.h>

typedef Std$Object_t IO$Stream_t;
typedef Std$Object$t IO$Stream$t;

RIVA_STRUCT(messaget) {
	const Std$Type_t *Type;
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
} IO$Stream_closemode;

RIVA_TYPE(CloseModeT);

RIVA_CFUN(int, close, IO$Stream_t *Stream, IO$Stream_closemode Mode);

typedef int (*IO$Stream_readfn)(IO$Stream_t *Stream, char *Buffer, int Count, int Block);

RIVA_CFUN(int, read, IO$Stream_t *Stream, char *Buffer, int Count, int Block);
RIVA_CFUN(char, readc, IO$Stream_t *Stream);
RIVA_CFUN(char *, readx, IO$Stream_t *Stream, int Max, const char *Term, int TermSize);
RIVA_CFUN(char *, readi, IO$Stream_t *Stream, int Max, const char *Term, int TermSize);
RIVA_CFUN(char *, readl, IO$Stream_t *Stream);
RIVA_CFUN(int, eoi, IO$Stream_t *Stream);

typedef int (*IO$Stream_writefn)(IO$Stream_t *Stream, const char *Buffer, int Count, int Block);

RIVA_CFUN(int, write, IO$Stream_t *Stream, const char *Buffer, int Count, int Block);
RIVA_CFUN(int, writec, IO$Stream_t *Stream, char Char);
RIVA_CFUN(int, writes, IO$Stream_t *Stream, const char *String);
RIVA_CFUN(int, writef, IO$Stream_t *Stream, const char *Format, ...);
RIVA_CFUN(int, flush, IO$Stream_t *Stream);

#define IO$Stream$BufferSize 1000
RIVA_STRUCT(buffer) {
	struct IO$Stream$buffer *Next;
	char Chars[IO$Stream$BufferSize];
};

RIVA_CFUN(IO$Stream_buffer *, alloc_buffer);
RIVA_CFUN(void, free_buffer, IO$Stream_buffer *Buffer);
RIVA_CFUN(void, free_buffers, IO$Stream_buffer *First, IO$Stream_buffer *Last);

typedef enum {
	IO$Stream$SEEK_SET,
	IO$Stream$SEEK_CUR,
	IO$Stream$SEEK_END
} IO$Stream_seekmode;

RIVA_TYPE(SeekModeT);

RIVA_CFUN(int, seek, IO$Stream_t *Stream, int, IO$Stream_seekmode Mode);
RIVA_CFUN(int, tell, IO$Stream_t *Stream);
RIVA_CFUN(size_t, remaining, IO$Stream_t *Stream);

#ifdef __GNUC__
#include <stdio.h>

RIVA_CFUN(FILE *, cfile, IO$Stream_t *);

#endif

#undef RIVA_MODULE

#define RIVA_MODULE IO$Stream$Message

RIVA_CFUN(Std$Object$t *, new, const Std$Type$t *, const char *);
RIVA_CFUN(Std$Object$t *, new_format, const Std$Type$t *, const char *, ...);
RIVA_CFUN(Std$Object$t *, from_errno, const Std$Type$t *);

#undef RIVA_MODULE

#endif
