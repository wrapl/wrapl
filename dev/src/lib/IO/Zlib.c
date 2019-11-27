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
	Std$Type$t *Type;
	z_stream ZStream[1];
	IO$Stream$t *Target;
	char Buffer[BUFFERSIZE];
} zlib_t;

TYPE(T, IO$Stream$T);
TYPE(ReaderT, T, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

static void *riva_alloc(void *Opaque, unsigned int Items, unsigned int Size) {
	return Riva$Memory$alloc_atomic(Items * Size);
}

static void riva_free(void *opaque, void *Address) {
}

GLOBAL_FUNCTION(Inflate, 1) {
	zlib_t *Stream = new(zlib_t);
	Stream->Type = ReaderT;
	Stream->Target = Args[0].Val;
	Stream->ZStream->zalloc = riva_alloc;
	Stream->ZStream->zfree = riva_free;
	if (inflateInit(Stream->ZStream) != Z_OK) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$OpenMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Stream;
	return SUCCESS;
};

GLOBAL_FUNCTION(Deflate, 1) {
	zlib_t *Stream = new(zlib_t);
	Stream->Type = WriterT;
	Stream->Target = Args[0].Val;
	Stream->ZStream->zalloc = riva_alloc;
	Stream->ZStream->zfree = riva_free;
	int Level = (Count > 1) ? ((Std$Integer$smallt *)Args[1].Val)->Value : Z_DEFAULT_COMPRESSION;
	if (deflateInit(Stream->ZStream, Level) != Z_OK) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$OpenMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Stream;
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$eoi, T, zlib_t *Stream) {
	z_streamp ZStream = Stream->ZStream;
	if (ZStream->avail_out) return 0;
	return IO$Stream$eoi(Stream->Target);
};

SYMBOL($eoi, "eoi");

METHOD("eoi", TYP, ReaderT) {
	zlib_t *Stream = (zlib_t *)Args[0].Val;
	IO$Stream$t *Stream0 = Stream->Target;
	return Std$Function$call($eoi, 1, Result, Stream0, 0);
};

TYPED_INSTANCE(int, IO$Stream$close, WriterT, zlib_t *Stream, int Mode) {
	IO$Stream$t *Stream0 = Stream->Target;
	z_streamp ZStream = Stream->ZStream;
	ZStream->next_out = Stream->Buffer;
	ZStream->avail_out = BUFFERSIZE;
	deflate(ZStream, Z_FINISH);
	while (ZStream->avail_out == 0) {
		int Count0 = BUFFERSIZE;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_FINISH);
	};
	int Count0 = BUFFERSIZE - ZStream->avail_out;
	char *Buffer0 = Stream->Buffer;
	while (Count0) {
		int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
		Count0 -= Count1;
		Buffer0 += Count1;
	};
	return IO$Stream$close(Stream->Target, Mode);
};

SYMBOL($close, "close");

METHOD("close", TYP, WriterT) {
	zlib_t *Stream = (zlib_t *)Args[0].Val;
	IO$Stream$t *Stream0 = Stream->Target;
	z_streamp ZStream = Stream->ZStream;
	ZStream->next_out = Stream->Buffer;
	ZStream->avail_out = BUFFERSIZE;
	deflate(ZStream, Z_FINISH);
	while (ZStream->avail_out == 0) {
		int Count0 = BUFFERSIZE;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_FINISH);
	};
	int Count0 = BUFFERSIZE - ZStream->avail_out;
	char *Buffer0 = Stream->Buffer;
	while (Count0) {
		int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
		Count0 -= Count1;
		Buffer0 += Count1;
	};
	return Std$Function$call($close, 1, Result, Stream0, 0);
};

TYPED_INSTANCE(int, IO$Stream$read, ReaderT, zlib_t *Stream, char *Buffer, int Count) {
	z_streamp ZStream = Stream->ZStream;
	IO$Stream$t *Stream0 = Stream->Target;
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

METHOD("read", TYP, T, TYP, Std$Address$T) {
	zlib_t *Stream = Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	z_streamp ZStream = Stream->ZStream;
	IO$Stream$t *Stream0 = Stream->Target;
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
	Result->Val = Args[2].Val;
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$write, WriterT, zlib_t *Stream, const char *Buffer, int Count) {
	z_streamp ZStream = Stream->ZStream;
	IO$Stream$t *Stream0 = Stream->Target;
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
				int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
				Count0 -= Count1;
				Buffer0 += Count1;
			};
			ZStream->next_out = Stream->Buffer;
			ZStream->avail_out = BUFFERSIZE;
			deflate(ZStream, Z_NO_FLUSH);
		};
		int Count0 = BUFFERSIZE - ZStream->avail_out;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
	};	
	return Count;
};

METHOD("write", TYP, T, TYP, Std$Address$T) {
	zlib_t *Stream = Args[0].Val;
	z_streamp ZStream = Stream->ZStream;
	IO$Stream$t *Stream0 = Stream->Target;
	ZStream->next_in = Std$Address$get_value(Args[1].Val);
	ZStream->avail_in = Std$Address$get_length(Args[1].Val);
	while (ZStream->avail_in) {
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_NO_FLUSH);
		while (ZStream->avail_out == 0) {
			int Count0 = BUFFERSIZE;
			char *Buffer0 = Stream->Buffer;
			while (Count0) {
				int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
				Count0 -= Count1;
				Buffer0 += Count1;
			};
			ZStream->next_out = Stream->Buffer;
			ZStream->avail_out = BUFFERSIZE;
			deflate(ZStream, Z_NO_FLUSH);
		};
		int Count0 = BUFFERSIZE - ZStream->avail_out;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
	};
	Result->Val = Args[2].Val;
	return SUCCESS;
};

TYPED_INSTANCE(void, IO$Stream$flush, WriterT, zlib_t *Stream) {
	IO$Stream$t *Stream0 = Stream->Target;
	z_streamp ZStream = Stream->ZStream;
	ZStream->next_out = Stream->Buffer;
	ZStream->avail_out = BUFFERSIZE;
	deflate(ZStream, Z_PARTIAL_FLUSH);
	while (ZStream->avail_out == 0) {
		int Count0 = BUFFERSIZE;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_PARTIAL_FLUSH);
	};
	int Count0 = BUFFERSIZE - ZStream->avail_out;
	char *Buffer0 = Stream->Buffer;
	while (Count0) {
		int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
		Count0 -= Count1;
		Buffer0 += Count1;
	};
	IO$Stream$flush(Stream0);
};

SYMBOL($flush, "flush");

METHOD("flush", TYP, WriterT) {
	zlib_t *Stream = (zlib_t *)Args[0].Val;
	IO$Stream$t *Stream0 = Stream->Target;
	z_streamp ZStream = Stream->ZStream;
	ZStream->next_out = Stream->Buffer;
	ZStream->avail_out = BUFFERSIZE;
	deflate(ZStream, Z_PARTIAL_FLUSH);
	while (ZStream->avail_out == 0) {
		int Count0 = BUFFERSIZE;
		char *Buffer0 = Stream->Buffer;
		while (Count0) {
			int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
			Count0 -= Count1;
			Buffer0 += Count1;
		};
		ZStream->next_out = Stream->Buffer;
		ZStream->avail_out = BUFFERSIZE;
		deflate(ZStream, Z_PARTIAL_FLUSH);
	};
	int Count0 = BUFFERSIZE - ZStream->avail_out;
	char *Buffer0 = Stream->Buffer;
	while (Count0) {
		int Count1 = IO$Stream$write(Stream0, Buffer0, Count0, 0);
		Count0 -= Count1;
		Buffer0 += Count1;
	};
	return Std$Function$call($flush, 1, Result, Stream0, 0);
}
