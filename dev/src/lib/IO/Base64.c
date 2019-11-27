#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Agg/Table.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

TYPE(T, IO$Stream$T);
// A base64 encoded stream. Reads/writes to an underlying <code>IO.Stream.T</code>.

 TYPE(ReaderT, T, IO$Stream$ReaderT, IO$Stream$T);
 TYPE(WriterT, T, IO$Stream$WriterT, IO$Stream$T);
 TYPE(SeekerT, T, IO$Stream$SeekerT, IO$Stream$T);

 TYPE(ReaderWriterT, ReaderT, WriterT, T, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);
 TYPE(ReaderSeekerT, ReaderT, SeekerT, T, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
 TYPE(WriterSeekerT, WriterT, SeekerT, T, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

 TYPE(ReaderWriterSeekerT, ReaderWriterT, ReaderSeekerT, WriterSeekerT, ReaderT, WriterT, SeekerT, T, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

 TYPE(TextReaderT, ReaderT, T, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
 TYPE(TextWriterT, WriterT, T, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

 TYPE(TextReaderWriterT, ReaderT, WriterT, T, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);
 TYPE(TextReaderSeekerT, TextReaderT, ReaderSeekerT, ReaderT, SeekerT, T, IO$Stream$TextReaderT, IO$Stream$SeekerT, IO$Stream$T);
 TYPE(TextWriterSeekerT, TextWriterT, WriterSeekerT, WriterT, SeekerT, T, IO$Stream$TextWriterT, IO$Stream$SeekerT, IO$Stream$T);

 TYPE(TextReaderWriterSeekerT, ReaderWriterSeekerT, ReaderWriterT, ReaderSeekerT, WriterSeekerT, TextReaderT, TextWriterT, ReaderT, WriterT, SeekerT, T,
	IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

typedef struct encoder_t {
	const Std$Type$t *Type;
	IO$Stream$t *Base;
	IO$Stream$readfn read;
	IO$Stream$writefn write;
	uint8_t SpareNormal, SpareEncoded;
	char BufferNormal[2];
	char BufferEncoded[3];
} encoder_t;

IO$Stream$t *_encoder_new(IO$Stream$t *Base) {
	static const Std$Type$t *Types[16] = {
		T, ReaderT, WriterT, ReaderWriterT, SeekerT, ReaderSeekerT, WriterSeekerT, ReaderWriterSeekerT,
		0, TextReaderT, TextWriterT, TextReaderWriterT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	encoder_t *Stream = new(encoder_t);
	int TypeNo = 0;
	for (const Std$Type$t **Type = Base->Type->Types; *Type; ++Type) {
		if (*Type == IO$Stream$ReaderT) TypeNo |= 1;
		if (*Type == IO$Stream$WriterT) TypeNo |= 2;
		if (*Type == IO$Stream$SeekerT) TypeNo |= 4;
		if (*Type == IO$Stream$TextReaderT) TypeNo |= 8;
		if (*Type == IO$Stream$TextWriterT) TypeNo |= 8;
	};
	Stream->Type = Types[TypeNo];
	Stream->Base = Base;
	if (TypeNo & 1) {
		Stream->read = Util$TypedFunction$get(IO$Stream$read, Base->Type);
	};
	if (TypeNo & 2) {
		Stream->write = Util$TypedFunction$get(IO$Stream$write, Base->Type);
	};
	return (Std$Object$t *)Stream;
};

GLOBAL_FUNCTION(New, 1) {
//@base : IO$Stream$T
//:T
// Creates and returns a new buffered stream with <var>base</var> as the underlying stream.
	CHECK_ARG_TYPE(0, IO$Stream$T);
	static const Std$Type$t *Types[16] = {
		T, ReaderT, WriterT, ReaderWriterT, SeekerT, ReaderSeekerT, WriterSeekerT, ReaderWriterSeekerT,
		0, TextReaderT, TextWriterT, TextReaderWriterT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	encoder_t *Stream = new(encoder_t);
	IO$Stream$t *Base = Args[0].Val;
	int TypeNo = 0;
	for (const Std$Type$t **Type = Base->Type->Types; *Type; ++Type) {
		if (*Type == IO$Stream$ReaderT) TypeNo |= 1;
		if (*Type == IO$Stream$WriterT) TypeNo |= 2;
		if (*Type == IO$Stream$SeekerT) TypeNo |= 4;
		if (*Type == IO$Stream$TextReaderT) TypeNo |= 8;
		if (*Type == IO$Stream$TextWriterT) TypeNo |= 8;
	};
	Stream->Type = Types[TypeNo];
	Stream->Base = Base;
	if (TypeNo & 1) {
		Stream->read = Util$TypedFunction$get(IO$Stream$read, Base->Type);
	};
	if (TypeNo & 2) {
		Stream->write = Util$TypedFunction$get(IO$Stream$write, Base->Type);
	};
	Result->Val = (Std$Object$t *)Stream;
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$eoi, ReaderT, encoder_t *Stream) {
	return IO$Stream$eoi(Stream->Base);
};

TYPED_INSTANCE(int, IO$Stream$flush, WriterT, encoder_t *Stream) {
	return IO$Stream$flush(Stream->Base);
};

SYMBOL($flush, "flush");

METHOD("flush", TYP, WriterT) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($flush, 1, Result, Stream->Base, 0);
};

TYPED_INSTANCE(int, IO$Stream$close, T, encoder_t *Stream, int Mode) {
	return IO$Stream$close(Stream->Base, Mode);
};

SYMBOL($close, "close");

METHOD("close", TYP, T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($close, 1, Result, Stream->Base, 0);
};

METHOD("close", TYP, T, TYP, IO$Stream$CloseModeT) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($close, 2, Result, Stream->Base, 0, Args[1].Val, 0);
};

static int encoder_read(encoder_t *Stream, char *Buffer, int Count, int Block) {
};

TYPED_INSTANCE(int, IO$Stream$read, ReaderT, encoder_t *Stream, char *Buffer, int Count, int Block) {
	return encoder_read(Stream, Buffer, Count, Block);
};

SYMBOL($block, "block");

METHOD("read", TYP, ReaderT, TYP, Std$Address$T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesRead = encoder_read(Stream, Buffer, Count, (Count >= 3 && Args[3].Val == $block));
	if (BytesRead < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

TYPED_INSTANCE(char, IO$Stream$readc, ReaderT, encoder_t *Stream) {
	return IO$Stream$readc(Stream->Base);
};

TYPED_INSTANCE(char *, IO$Stream$readx, ReaderT, encoder_t *Stream, int Max, char *Term, int TermSize) {
	return IO$Stream$readx(Stream->Base, Max, Term, TermSize);
};

SYMBOL($readx, "readx");

METHOD("readx", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($readx, 3, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(char *, IO$Stream$readi, ReaderT, encoder_t *Stream, int Max, char *Term, int TermSize) {
	return IO$Stream$readi(Stream->Base, Max, Term, TermSize);
};

SYMBOL($readi, "readi");

METHOD("readi", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($readi, 3, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(char *, IO$Stream$readl, TextReaderT, encoder_t *Stream) {
	return IO$Stream$readl(Stream->Base);
};

static inline int encoder_write(encoder_t *Stream, const char *Buffer, int Count, int Blocks) {
	if (Stream->SpareEncoded) {
		int Bytes = Stream->write(Stream->Base, Stream->BufferEncoded, Stream->SpareEncoded, Blocks);
		if (Bytes == -1) return -1;
		if (Stream->SpareEncoded -= Bytes) {
			if (Bytes == 1) {
				Stream->BufferEncoded[0] = Stream->BufferEncoded[1];
				Stream->BufferEncoded[1] = Stream->BufferEncoded[2];
			} else {
				Stream->BufferEncoded[0] = Stream->BufferEncoded[2];
			};
			return 0;
		};
	};
	const char *Start = Buffer;
	const char *End = Start;
	int Total = 0;
	for (int I = Count; --I >= 0; ++End) {
	};
	int Temp = End - Start;
	if (Temp) {
		int Bytes = Stream->write(Stream->Base, Start, Temp, Blocks);
		if (Bytes == -1) return -1;
		Total += Bytes;
		if (Bytes < Temp) return Total;
	};
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$write, WriterT, encoder_t *Stream, const char *Buffer, int Count, int Blocks) {
	return encoder_write(Stream, Buffer, Count, Blocks);
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesWritten = encoder_write(Stream, Buffer, Size, (Count >= 3 && Args[3].Val == $block));
	if (BytesWritten < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$writec, WriterT, encoder_t *Stream, char Char) {
	return encoder_write(Stream, &Char, 1, 1);
};

TYPED_INSTANCE(int, IO$Stream$writes, WriterT, encoder_t *Stream, const char *Text) {
	return encoder_write(Stream, Text, strlen(Text), 1);
};

TYPED_INSTANCE(int, IO$Stream$writef, WriterT, encoder_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	return encoder_write(Stream, Buffer, Length, 1);
};

SYMBOL($seek, "seek");

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($seek, 2, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(int, IO$Stream$seek, SeekerT, encoder_t *Stream, int Position, int Mode) {
	return IO$Stream$seek(Stream->Base, Position, Mode);
};

SYMBOL($tell, "tell");

METHOD("tell", TYP, T) {
	encoder_t *Stream = (encoder_t *)Args[0].Val;
	return Std$Function$call($tell, 1, Result, Stream->Base, 0);
};

TYPED_INSTANCE(int, IO$Stream$tell, SeekerT, encoder_t *Stream) {
	return IO$Stream$tell(Stream->Base);
};

