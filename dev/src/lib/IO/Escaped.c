#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Agg/Table.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef struct map_t {
	const Std$Type$t *Type;
	size_t Lengths[256];
	const char *Chars[256];
} map_t;

TYPE(MapT);

GLOBAL_FUNCTION(MapNew, 0) {
	map_t *Map = new(map_t);
	Map->Type = MapT;
	if (Count > 0) {
		CHECK_ARG_TYPE(0, Agg$Table$T);
		Agg$Table$trav *Trav = Agg$Table$trav_new();
		for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Args[0].Val); Node; Node = Agg$Table$trav_next(Trav)) {
			Std$Object$t *Key = Agg$Table$node_key(Node);
			Std$Object$t *Value = Agg$Table$node_value(Node);
			if (Key->Type != Std$String$T || ((Std$String$t *)Key)->Length.Value != 1) {
				Result->Val = Std$String$new("Can only map single characters");
				return MESSAGE;
			};
			if (Value->Type != Std$String$T) {
				Result->Val = Std$String$new("Can only map to strings");
				return MESSAGE;
			};
			char Char = ((char *)((Std$String$t *)Key)->Blocks[0].Chars.Value)[0];
			Map->Lengths[Char] = Std$String$get_length(Value);
			Map->Chars[Char] = Std$String$flatten(Value);
		};
	};
	Result->Val = (Std$Object$t *)Map;
	return SUCCESS;
};

METHOD("add", TYP, MapT, TYP, Std$String$T, TYP, Std$String$T) {
	map_t *Map = (map_t *)Args[0].Val;
	Std$Object$t *Key = Args[1].Val;
	Std$Object$t *Value = Args[2].Val;
	if (((Std$String$t *)Key)->Length.Value != 1) {
		Result->Val = Std$String$new("Can only map single characters");
		return MESSAGE;
	};
	char Char = ((char *)((Std$String$t *)Key)->Blocks[0].Chars.Value)[0];
	Map->Lengths[Char] = Std$String$get_length(Value);
	Map->Chars[Char] = Std$String$flatten(Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

TYPE(T, IO$Stream$T);
// A escaped stream. Reads/writes to an underlying <code>IO.Stream.T</code>.

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

typedef struct escaped_t {
	const Std$Type$t *Type;
	IO$Stream$t *Base;
	IO$Stream$readfn read;
	IO$Stream$writefn write;
	map_t *Map;
} escaped_t;

IO$Stream$t *_new(IO$Stream$t *Base, map_t *Map) {
	static const Std$Type$t *Types[16] = {
		T, ReaderT, WriterT, ReaderWriterT, SeekerT, ReaderSeekerT, WriterSeekerT, ReaderWriterSeekerT,
		0, TextReaderT, TextWriterT, TextReaderWriterT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	escaped_t *Stream = new(escaped_t);
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
	if (!Map) {
		Map = new(map_t);
		Map->Type = MapT;
	};
	Stream->Map = Map;
	return (Std$Object$t *)Stream;
};

GLOBAL_FUNCTION(New, 1) {
//@base : IO$Stream$T
//:T
// Creates and returns a new escaped stream with <var>base</var> as the underlying stream.
	CHECK_ARG_TYPE(0, IO$Stream$T);
	static const Std$Type$t *Types[16] = {
		T, ReaderT, WriterT, ReaderWriterT, SeekerT, ReaderSeekerT, WriterSeekerT, ReaderWriterSeekerT,
		0, TextReaderT, TextWriterT, TextReaderWriterT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	escaped_t *Stream = new(escaped_t);
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
	map_t *Map;
	if (TypeNo & 1) {
		Stream->read = Util$TypedFunction$get(IO$Stream$read, Base->Type);
	};
	if (TypeNo & 2) {
		Stream->write = Util$TypedFunction$get(IO$Stream$write, Base->Type);
	};
	if (Count > 1) {
		CHECK_EXACT_ARG_TYPE(1, MapT);
		Map = (map_t *)Args[1].Val;
	} else {
		Map = new(map_t);
		Map->Type = MapT;
	};
	Stream->Map = Map;
	Result->Val = (Std$Object$t *)Stream;
	return SUCCESS;
};

METHOD("base", TYP, T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Stream->Base;
	return SUCCESS;
};

METHOD("map", TYP, T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Stream->Map;
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$eoi, ReaderT, escaped_t *Stream) {
	return IO$Stream$eoi(Stream->Base);
};

TYPED_INSTANCE(int, IO$Stream$flush, WriterT, escaped_t *Stream) {
	return IO$Stream$flush(Stream->Base);
};

SYMBOL($flush, "flush");

METHOD("flush", TYP, WriterT) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($flush, 1, Result, Stream->Base, 0);
};

TYPED_INSTANCE(int, IO$Stream$close, T, escaped_t *Stream, int Mode) {
	return IO$Stream$close(Stream->Base, Mode);
};

SYMBOL($close, "close");

METHOD("close", TYP, T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($close, 1, Result, Stream->Base, 0);
};

METHOD("close", TYP, T, TYP, IO$Stream$CloseModeT) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($close, 2, Result, Stream->Base, 0, Args[1].Val, 0);
};

static int escaped_read(escaped_t *Stream, char *Buffer, int Count, int Block) {
	// TODO: Finish this!!!
	map_t *Map = Stream->Map;
	int Remaining = Count;
	char *Start = Buffer;
	int Bytes = Stream->read(Stream->Base, Start, Remaining, Block);
	for (int I = 0; I < Bytes; ++I) {
		
	};
};

TYPED_INSTANCE(int, IO$Stream$read, ReaderT, escaped_t *Stream, char *Buffer, int Count, int Block) {
	return escaped_read(Stream, Buffer, Count, Block);
};

SYMBOL($block, "block");

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesRead = escaped_read(Stream, Buffer, Count, (Count >= 3 && Args[3].Val == $block));
	if (BytesRead < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

TYPED_INSTANCE(char, IO$Stream$readc, ReaderT, escaped_t *Stream) {
	return IO$Stream$readc(Stream->Base);
};

TYPED_INSTANCE(char *, IO$Stream$readx, ReaderT, escaped_t *Stream, int Max, char *Term, int TermSize) {
	return IO$Stream$readx(Stream->Base, Max, Term, TermSize);
};

SYMBOL($readx, "readx");

METHOD("readx", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($readx, 3, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(char *, IO$Stream$readi, ReaderT, escaped_t *Stream, int Max, char *Term, int TermSize) {
	return IO$Stream$readi(Stream->Base, Max, Term, TermSize);
};

SYMBOL($readi, "readi");

METHOD("readi", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($readi, 3, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(char *, IO$Stream$readl, TextReaderT, escaped_t *Stream) {
	return IO$Stream$readl(Stream->Base);
};

static inline int escaped_write(escaped_t *Stream, const char *Buffer, int Count, int Blocks) {
	map_t *Map = Stream->Map;
	const char *Start = Buffer;
	const char *End = Start;
	int Total = 0;
	for (int I = Count; --I >= 0; ++End) {
		if (Map->Chars[*End]) {
			int Temp = End - Start;
			if (Temp) {
				int Bytes = Stream->write(Stream->Base, Start, Temp, Blocks);
				if (Bytes == -1) return -1;
				Total += Bytes;
				if (Bytes < Temp) return Total;
			};
			Temp = Map->Lengths[*End];
			int Bytes = Stream->write(Stream->Base, Map->Chars[*End], Temp, Blocks);
			if (Bytes == -1) return -1;
			if (Bytes < Temp) return Total;
			Total += 1;
			Start = End + 1;
		};
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

TYPED_INSTANCE(int, IO$Stream$write, WriterT, escaped_t *Stream, const char *Buffer, int Count, int Blocks) {
	return escaped_write(Stream, Buffer, Count, Blocks);
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesWritten = escaped_write(Stream, Buffer, Size, (Count >= 3 && Args[3].Val == $block));
	if (BytesWritten < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$writec, WriterT, escaped_t *Stream, char Char) {
	return escaped_write(Stream, &Char, 1, 1);
};

TYPED_INSTANCE(int, IO$Stream$writes, WriterT, escaped_t *Stream, const char *Text) {
	return escaped_write(Stream, Text, strlen(Text), 1);
};

TYPED_INSTANCE(int, IO$Stream$writef, WriterT, escaped_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	return escaped_write(Stream, Buffer, Length, 1);
};

SYMBOL($seek, "seek");

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($seek, 2, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(int, IO$Stream$seek, SeekerT, escaped_t *Stream, int Position, int Mode) {
	return IO$Stream$seek(Stream->Base, Position, Mode);
};

SYMBOL($tell, "tell");

METHOD("tell", TYP, T) {
	escaped_t *Stream = (escaped_t *)Args[0].Val;
	return Std$Function$call($tell, 1, Result, Stream->Base, 0);
};

TYPED_INSTANCE(int, IO$Stream$tell, SeekerT, escaped_t *Stream) {
	return IO$Stream$tell(Stream->Base);
};

