#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <Std.h>
#include <stdint.h>
#include <stdarg.h>
#include <zlib.h>

#include <stdio.h>

#define BUFFERSIZE 256

typedef struct zlib_t {
	Std$Type_t *Type;
	z_stream ZStream[1];
	IO$Stream_t *Target;
	char Buffer[BUFFERSIZE];
} zlib_t;

TYPE(T, IO$Stream$T);
TYPE(ReaderT, T, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, IO$Stream$WriterT, IO$Stream$T);

IO$Stream_messaget CreateMessage[] = {{IO$Stream$MessageT, "Create Error"}};
IO$Stream_messaget ReadMessage[] = {{IO$Stream$MessageT, "Read Error"}};
IO$Stream_messaget WriteMessage[] = {{IO$Stream$MessageT, "Write Error"}};
IO$Stream_messaget FlushMessage[] = {{IO$Stream$MessageT, "Flush Error"}};
IO$Stream_messaget SeekMessage[] = {{IO$Stream$MessageT, "Seek Error"}};
IO$Stream_messaget CloseMessage[] = {{IO$Stream$MessageT, "Close Error"}};
IO$Stream_messaget PollMessage[] = {{IO$Stream$MessageT, "Poll Error"}};

static void *_alloc(void *Ignore, unsigned int Items, unsigned int Size) {
	return Riva$Memory$alloc(Items * Size);
};

static void _free(void *Ignore, void *Address) {
};

GLOBAL_FUNCTION(InflateNew, 1) {
	zlib_t *Stream = new(zlib_t);
	Stream->Type = ReaderT;
	Stream->Target = Args[0].Val;
	
};

GLOBAL_FUNCTION(DeflateNew, 1) {
	zlib_t *Stream = new(zlib_t);
	Stream->Type = WriterT;
	Stream->Target = Args[0].Val;
	int Level = (Count > 1) ? ((Std$Integer_smallt *)Args[1].Val)->Value : Z_DEFAULT_COMPRESSION;
	if (deflateInit(Stream->ZStream, Level) != Z_OK) {
		Result->Val = CreateMessage;
		return MESSAGE;
	};
	Result->Val = Stream;
	return SUCCESS;
};

static int zlib_eoi(zlib_t *Stream) {
};

static int zlib_close(zlib_t *Stream, int Mode) {
};

static Std$Integer_smallt Zero[] = {{Std$Integer$SmallT, 0}};

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	zlib_t *Stream = Args[0].Val;
};

static int zlib_read(zlib_t *Stream, char *Buffer, int Count) {
	z_streamp ZStream = Stream->ZStream;
	IO$Stream_t *Stream0 = Stream->Target;
	char *Buffer0 = Stream->Buffer;
	while (ZStream->avail_out == 0) {
		memcpy(Buffer, Buffer0, BUFFERSIZE);
		Buffer += BUFFERSIZE;
		ZStream->avail_out = BUFFERSIZE;
		ZStream->next_out = Buffer0;
		inflate(ZStream, Z_NO_FLUSH);
	};
	if (ZStream->avail_out < BUFFERSIZE) {
		int Count0 = BUFFERSIZE - ZStream->avail_out;
		memcpy(Buffer, Buffer0, ZStream->avail_out);
	};
	return Count;
};

static char zlib_readc(zlib_t *Stream) {
};

METHOD("rest", TYP, T) {
};

METHOD("read", TYP, T, TYP, Std$Integer$SmallT) {
};

static char *zlib_readn(zlib_t *Stream, int Count) {
};

static char *zlib_readl(zlib_t *Stream) {
};

METHOD("read", TYP, T) {
};

static int zlib_write(zlib_t *Stream, const char *Buffer, int Count) {
	z_streamp ZStream = Stream->ZStream;
	IO$Stream_t *Stream0 = Stream->Target;
	ZStream->next_in = Buffer;
	ZStream->avail_in = Count;
	while (ZStream->avail_in) {
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_NO_FLUSH);
		while (ZStream->avail_out == 0) {
			int Count0 = BUFFERSIZE;
			char *Buffer0 = Stream->Buffer;
			while (Count0) {
				int Count1 = IO$Stream$write(Stream0, Buffer0, Count0);
				Count0 -= Count1;
				Buffer0 += Count1;
			};
			deflate(ZStream, Z_NO_FLUSH);
		};
		int Count0 = BUFFERSIZE - ZStream->avail_out;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
	};	
	return Count;};

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	zlib_t *Stream = Args[0].Val;
	z_streamp ZStream = Stream->ZStream;
	IO$Stream_t *Stream0 = Stream->Target;
	ZStream->next_in = ((Std$Address_t *)Args[1].Val)->Value;
	ZStream->avail_in = ((Std$Integer_smallt *)Args[2].Val)->Value;
	while (ZStream->avail_in) {
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_NO_FLUSH);
		while (ZStream->avail_out == 0) {
			int Count0 = BUFFERSIZE;
			char *Buffer0 = Stream->Buffer;
			while (Count0) {
				int Count1 = IO$Stream$write(Stream0, Buffer0, Count0);
				Count0 -= Count1;
				Buffer0 += Count1;
			};
			deflate(ZStream, Z_NO_FLUSH);
		};
		int Count0 = BUFFERSIZE - ZStream->avail_out;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
	};
	Result->Val = Args[2].Val;
	return SUCCESS;
};

static void zlib_writec(zlib_t *Stream, char Char) {
	return 1;
};

METHOD("write", TYP, T, TYP, Std$String$T) {
	zlib_t *Stream = Args[0].Val;
	int NoOfBlocks = ((Std$String_t *)Args[1].Val)->Count;
	Std$String_block *Block = ((Std$String_t *)Args[1].Val)->Blocks;
	if (Block->Length.Value) {
		while ((++Block)->Length.Value) {
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

static void zlib_writes(zlib_t *Stream, const char *Text) {
	int Length = strlen(Text);
	return Length;
};

static void zlib_writef(zlib_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	return Length;
};

INITIAL() {
	Util$TypedFunction$set(IO$Stream$eoi, T, zlib_eoi);
	Util$TypedFunction$set(IO$Stream$close, T, zlib_close);
	Util$TypedFunction$set(IO$Stream$read, T, zlib_read);
	Util$TypedFunction$set(IO$Stream$readc, T, zlib_readc);
	Util$TypedFunction$set(IO$Stream$readn, T, zlib_readn);
	Util$TypedFunction$set(IO$Stream$readl, T, zlib_readl);
	Util$TypedFunction$set(IO$Stream$write, T, zlib_write);
	Util$TypedFunction$set(IO$Stream$writec, T, zlib_writec);
	Util$TypedFunction$set(IO$Stream$writes, T, zlib_writes);
	Util$TypedFunction$set(IO$Stream$writef, T, zlib_writef);
};
