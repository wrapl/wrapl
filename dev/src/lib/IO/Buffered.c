#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

TYPE(T, IO$Stream$T);
// A buffered stream. Uses buffers to read/write to an underlying.

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

typedef struct read_buffer_t {
	int Length;
	unsigned char *Chars;
	unsigned char *Head, *Tail;
} read_buffer_t;

typedef struct write_buffer_t {
	int Length, Space;
	unsigned char *Chars;
	unsigned char *Tail;
} write_buffer_t;

typedef struct buffered_t {
	const Std$Type$t *Type;
	IO$Stream$t *Base;
	IO$Stream$readfn read;
	IO$Stream$writefn write;
	read_buffer_t Read[1];
	write_buffer_t Write[1];
} buffered_t;

IO$Stream$t *_new(IO$Stream$t *Base) {
	static const Std$Type$t *Types[16] = {
		T, ReaderT, WriterT, ReaderWriterT, SeekerT, ReaderSeekerT, WriterSeekerT, ReaderWriterSeekerT,
		0, TextReaderT, TextWriterT, TextReaderWriterT, SeekerT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	buffered_t *Stream = new(buffered_t);
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
		Stream->Read->Head = Stream->Read->Tail = Stream->Read->Chars = Riva$Memory$alloc_atomic(Stream->Read->Length = 256);
	};
	if (TypeNo & 2) {
		Stream->write = Util$TypedFunction$get(IO$Stream$write, Base->Type);
		Stream->Write->Tail = Stream->Write->Chars = Riva$Memory$alloc_atomic(Stream->Write->Space = Stream->Write->Length = 256);
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
		0, TextReaderT, TextWriterT, TextReaderWriterT, SeekerT, TextReaderSeekerT, TextWriterSeekerT, TextReaderWriterSeekerT
	};
	buffered_t *Stream = new(buffered_t);
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
	int Length = 256;
	if (Count >= 2) {
		CHECK_ARG_TYPE(1, Std$Integer$SmallT);
		Length = Std$Integer$get_small(Args[1].Val);
	};
	if (TypeNo & 1) {
		Stream->read = Util$TypedFunction$get(IO$Stream$read, Base->Type);
		Stream->Read->Head = Stream->Read->Tail = Stream->Read->Chars = Riva$Memory$alloc_atomic(Stream->Read->Length = Length);
	};
	if (TypeNo & 2) {
		Stream->write = Util$TypedFunction$get(IO$Stream$write, Base->Type);
		Stream->Write->Tail = Stream->Write->Chars = Riva$Memory$alloc_atomic(Stream->Write->Space = Stream->Write->Length = Length);
	};
	Result->Val = (Std$Object$t *)Stream;
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$eoi, ReaderT, buffered_t *Stream) {
	return IO$Stream$eoi(Stream->Base);
};

TYPED_INSTANCE(int, IO$Stream$flush, WriterT, buffered_t *Stream) {
	write_buffer_t *Write = Stream->Write;
	int Count = Write->Tail - Write->Chars;
	char *Chars = Write->Chars;
	while (Count) {
		int Bytes = Stream->write(Stream->Base, Chars, Count, 1);
		if (Bytes == -1) return -1;
		Chars += Bytes;
		Count -= Bytes;
	};
	Write->Tail = Write->Chars;
	Write->Space = Write->Length;
	return 0;
};

static int buffered_flush(buffered_t *Stream) {
	write_buffer_t *Write = Stream->Write;
	int Count = Write->Tail - Write->Chars;
	char *Chars = Write->Chars;
	while (Count) {
		int Bytes = Stream->write(Stream->Base, Chars, Count, 1);
		if (Bytes == -1) return -1;
		Chars += Bytes;
		Count -= Bytes;
	};
	Write->Tail = Write->Chars;
	Write->Space = Write->Length;
	return 0;
};

METHOD("flush", TYP, WriterT) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	write_buffer_t *Write = Stream->Write;
	int Length = Write->Tail - Write->Chars;
	char *Chars = Write->Chars;
	while (Length) {
		int Bytes = Stream->write(Stream->Base, Chars, Length, 1);
		if (Bytes == -1) {
			Result->Val = IO$Stream$Message$new_format(IO$Stream$FlushMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		Chars += Bytes;
		Length -= Bytes;
	};
	Write->Tail = Write->Chars;
	Write->Space = Write->Length;
	Result->Arg = Args[0];
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$close, T, buffered_t *Stream, int Mode) {
	buffered_flush(Stream);
	return IO$Stream$close(Stream->Base, Mode);
};

SYMBOL($close, "close");

METHOD("close", TYP, T) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	buffered_flush(Stream);
	return Std$Function$call($close, 1, Result, Stream->Base, 0);
};

METHOD("close", TYP, T, TYP, IO$Stream$CloseModeT) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	buffered_flush(Stream);
	return Std$Function$call($close, 2, Result, Stream->Base, 0, Args[1].Val, 0);
};

static int buffered_read(buffered_t *Stream, char *Buffer, int Count, int Block) {
	read_buffer_t *Read = Stream->Read;
	int Total = Read->Tail - Read->Head;
	if (Total >= Count) {
		memcpy(Buffer, Read->Head, Count);
		Read->Head += Count;
		return Count;
	};
	memcpy(Buffer, Read->Head, Total);
	Read->Head = Read->Tail = Read->Chars;
	Buffer += Total;
	Count -= Total;
	while (Count) {
		int Bytes = Stream->read(Stream->Base, Buffer, Count, Block);
		if (Bytes == 0) return Total;
		if (Bytes == -1) return -1;
		Total += Bytes;
		Buffer += Bytes;
		Count -= Bytes;
	};
	return Total;
};

TYPED_INSTANCE(int, IO$Stream$read, ReaderT, buffered_t *Stream, char *Buffer, int Count, int Block) {
	return buffered_read(Stream, Buffer, Count, Block);
};

SYMBOL($block, "block");

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesRead = buffered_read(Stream, Buffer, Count, (Count >= 3 && Args[3].Val == $block));
	if (BytesRead < 0) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

TYPED_INSTANCE(char, IO$Stream$readc, ReaderT, buffered_t *Stream) {
	read_buffer_t *Read = Stream->Read;
	if (Read->Tail > Read->Head) return *(Read->Head++);
	Read->Head = Read->Chars;
	int Bytes = Stream->read(Stream->Base, Read->Chars, Read->Length, 1);
	if (Bytes == 0) return EOF;
	if (Bytes == -1) return -1;
	Read->Tail = Read->Head + Bytes;
	return *(Read->Head++);
};

typedef struct temp_buffer_t temp_buffer_t;

struct temp_buffer_t {
	temp_buffer_t *Next;
	unsigned char *Chars, *Tail;
	int Length, Space;
};

static char *chars_rest(buffered_t *Stream) {
	read_buffer_t *Read = Stream->Read;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		InitialChars = Read->Head;
		TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static Std$Function$status string_rest(buffered_t *Stream, Std$Function$result *Result) {
	read_buffer_t *Read = Stream->Read;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
		    Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static char *buffered_reads(buffered_t *Stream, int Max) {
	if (Max == 0) return chars_rest(Stream);
	read_buffer_t *Read = Stream->Read;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
		    char *Result = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Result, Read->Head, Max);
			Result[Max] = 0;
			Read->Head += Max;
			return Result;
		} else {
			InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	} else {
	    Remaining = Max;
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 1);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        TotalLength += Remaining;
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
                Temp = mempcpy(Temp, Tail->Chars, (Tail->Tail + Remaining) - Tail->Chars);
				Temp[0] = 0;
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				return Result;
			} else {
			    Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static Std$Function$status stringn(buffered_t *Stream, int Max, Std$Function$result *Result) {
	if (Max == 0) return string_rest(Stream, Result);
	read_buffer_t *Read = Stream->Read;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
			char *Chars = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Chars, Read->Head, Max);
			Chars[Max] = 0;
			Read->Head += Max;
			Result->Val = Std$String$new_length(Chars, Max);
			return SUCCESS;
		} else {
		    InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	} else {
	    Remaining = Max;
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 1);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        TotalLength += Remaining;
                int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			} else {
				Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static char *charsx_nolimit(buffered_t *Stream, char *Term, int TermSize) {
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
	        if (IsTerm[*Ptr]) {
	            int Length = Ptr - Read->Head;
	            char *Result = Riva$Memory$alloc_atomic(Length + 1);
	            memcpy(Result, Read->Head, Length);
	            Result[Length] = 0;
	            Read->Head = Ptr + 1;
	            return Result;
			};
		};
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
	            if (IsTerm[*Ptr]) {
	                TotalLength += Ptr - Tail->Tail;
	                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
	                char *Temp = mempcpy(Result, InitialChars, InitialLength);
	                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
	                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
	                };
	                Temp = mempcpy(Temp, Tail->Chars, Ptr - Tail->Chars);
					Temp[0] = 0;
					Read->Chars = Tail->Chars;
					Read->Head = Ptr + 1;
					Read->Tail = Tail->Tail + Bytes;
					Read->Length = Tail->Length;
					return Result;
				};
			};
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

TYPED_INSTANCE(char *, IO$Stream$readx, ReaderT, buffered_t *Stream, int Max, char *Term, int TermSize) {
	if (Max == 0) return charsx_nolimit(Stream, Term, TermSize);
	if (TermSize == 0) return buffered_reads(Stream, Max);
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
		    for (unsigned char *Ptr = Read->Head; Ptr < Read->Head + Max; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = Ptr - Read->Head;
		            char *Result = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Result, Read->Head, Length);
		            Result[Length] = 0;
		            Read->Head = Ptr + 1;
		            return Result;
				};
			};
			char *Result = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Result, Read->Head, Max);
			Result[Max] = 0;
			Read->Head += Max;
			return Result;
		} else {
			for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = Ptr - Read->Head;
		            char *Result = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Result, Read->Head, Length);
		            Result[Length] = 0;
		            Read->Head = Ptr + 1;
		            return Result;
				};
			};
		    InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	} else {
	    Remaining = Max;
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Remaining; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += Ptr - Tail->Tail;
		                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
		                char *Temp = mempcpy(Result, InitialChars, InitialLength);
		                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
		                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
		                };
		                Temp = mempcpy(Temp, Tail->Chars, Ptr - Tail->Chars);
						Temp[0] = 0;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						return Result;
					};
				};
		        TotalLength += Remaining;
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
                Temp = mempcpy(Temp, Tail->Chars, (Tail->Tail + Remaining) - Tail->Chars);
				Temp[0] = 0;
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				return Result;
			} else {
			    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += Ptr - Tail->Tail;
		                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
		                char *Temp = mempcpy(Result, InitialChars, InitialLength);
		                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
		                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
		                };
		                Temp = mempcpy(Temp, Tail->Chars, Ptr - Tail->Chars);
						Temp[0] = 0;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						return Result;
					};
				};
				Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static Std$Function$status stringx_nolimit(buffered_t *Stream, const Std$String$t *Term, Std$Function$result *Result) {
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
	        if (IsTerm[*Ptr]) {
	            int Length = Ptr - Read->Head;
	            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
	            memcpy(Chars, Read->Head, Length);
	            Chars[Length] = 0;
	            Read->Head = Ptr + 1;
	            Result->Val = Std$String$new_length(Chars, Length);
	            return SUCCESS;
			};
		};
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
		    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
	            if (IsTerm[*Ptr]) {
	                TotalLength += Ptr - Tail->Tail;
	                int NoOfBlocks = InitialLength ? 1 : 0;
					for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
					if (Ptr != Tail->Chars) ++NoOfBlocks;
					Std$String$t *String = Std$String$alloc(NoOfBlocks);
					String->Length.Value = TotalLength;
					Std$String$block *Block = String->Blocks;
					if (InitialLength) {
						Block->Length.Value = InitialLength;
						Block->Chars.Value = InitialChars;
						++Block;
					};
					for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
						Block->Length.Value = Buffer->Length;
						Block->Chars.Value = Buffer->Chars;
						++Block;
					};
					if (Ptr != Tail->Chars) {
						char *Chars = Riva$Memory$alloc_atomic(Ptr - Tail->Chars);
						memcpy(Chars, Tail->Chars, Ptr - Tail->Chars);
						Block->Length.Value = Ptr - Tail->Chars;
						Block->Chars.Value = Chars;
					};
					Read->Chars = Tail->Chars;
					Read->Head = Ptr + 1;
					Read->Tail = Tail->Tail + Bytes;
					Read->Length = Tail->Length;
					Result->Val = Std$String$freeze(String);
					return SUCCESS;
				};
			};
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

METHOD("readx", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	if (Term->Length.Value == 0) return stringn(Stream, Max, Result);
	if (Max == 0) return stringx_nolimit(Stream, Term, Result);
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
		    for (unsigned char *Ptr = Read->Head; Ptr < Read->Head + Max; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = Ptr - Read->Head;
		            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Chars, Read->Head, Length);
		            Chars[Length] = 0;
		            Read->Head = Ptr + 1;
		            Result->Val = Std$String$new_length(Chars, Length);
		            return SUCCESS;
				};
			};
			char *Chars = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Chars, Read->Head, Max);
			Chars[Max] = 0;
			Read->Head += Max;
			Result->Val = Std$String$new_length(Chars, Max);
			return SUCCESS;
		} else {
			for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = Ptr - Read->Head;
		            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Chars, Read->Head, Length);
		            Chars[Length] = 0;
		            Read->Head = Ptr + 1;
		            Result->Val = Std$String$new_length(Chars, Length);
		            return SUCCESS;
				};
			};
		    InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	} else {
	    Remaining = Max;
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Remaining; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += Ptr - Tail->Tail;
		                int NoOfBlocks = InitialLength ? 1 : 0;
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
						if (Ptr != Tail->Chars) ++NoOfBlocks;
						Std$String$t *String = Std$String$alloc(NoOfBlocks);
						String->Length.Value = TotalLength;
						Std$String$block *Block = String->Blocks;
						if (InitialLength) {
							Block->Length.Value = InitialLength;
							Block->Chars.Value = InitialChars;
							++Block;
						};
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
							Block->Length.Value = Buffer->Length;
							Block->Chars.Value = Buffer->Chars;
							++Block;
						};
						if (Ptr != Tail->Chars) {
							char *Chars = Riva$Memory$alloc_atomic(Ptr - Tail->Chars);
							memcpy(Chars, Tail->Chars, Ptr - Tail->Chars);
							Block->Length.Value = Ptr - Tail->Chars;
							Block->Chars.Value = Chars;
						};
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						Result->Val = Std$String$freeze(String);
						return SUCCESS;
					};
				};
		        TotalLength += Remaining;
                int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			} else {
			    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += Ptr - Tail->Tail;
		                int NoOfBlocks = InitialLength ? 1 : 0;
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
						if (Ptr != Tail->Chars) ++NoOfBlocks;
						Std$String$t *String = Std$String$alloc(NoOfBlocks);
						String->Length.Value = TotalLength;
						Std$String$block *Block = String->Blocks;
						if (InitialLength) {
							Block->Length.Value = InitialLength;
							Block->Chars.Value = InitialChars;
							++Block;
						};
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
							Block->Length.Value = Buffer->Length;
							Block->Chars.Value = Buffer->Chars;
							++Block;
						};
						if (Ptr != Tail->Chars) {
							char *Chars = Riva$Memory$alloc_atomic(Ptr - Tail->Chars);
							memcpy(Chars, Tail->Chars, Ptr - Tail->Chars);
							Block->Length.Value = Ptr - Tail->Chars;
							Block->Chars.Value = Chars;
						};
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						Result->Val = Std$String$freeze(String);
						return SUCCESS;
					};
				};
				Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static char *charsi_nolimit(buffered_t *Stream, char *Term, int TermSize) {
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
	        if (IsTerm[*Ptr]) {
	            int Length = (Ptr + 1) - Read->Head;
	            char *Result = Riva$Memory$alloc_atomic(Length + 1);
	            memcpy(Result, Read->Head, Length);
	            Result[Length] = 0;
	            Read->Head = Ptr + 1;
	            return Result;
			};
		};
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
	            if (IsTerm[*Ptr]) {
	                TotalLength += (Ptr + 1) - Tail->Tail;
	                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
	                char *Temp = mempcpy(Result, InitialChars, InitialLength);
	                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
	                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
	                };
	                Temp = mempcpy(Temp, Tail->Chars, Ptr - Tail->Chars);
					Temp[0] = 0;
					Read->Chars = Tail->Chars;
					Read->Head = Ptr + 1;
					Read->Tail = Tail->Tail + Bytes;
					Read->Length = Tail->Length;
					return Result;
				};
			};
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

TYPED_INSTANCE(char *, IO$Stream$readi, ReaderT, buffered_t *Stream, int Max, char *Term, int TermSize) {
	if (Max == 0) return charsi_nolimit(Stream, Term, TermSize);
	if (TermSize == 0) return buffered_reads(Stream, Max);
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
		    for (unsigned char *Ptr = Read->Head; Ptr < Read->Head + Max; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = (Ptr + 1) - Read->Head;
		            char *Result = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Result, Read->Head, Length);
		            Result[Length] = 0;
		            Read->Head = Ptr + 1;
		            return Result;
				};
			};
			char *Result = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Result, Read->Head, Max);
			Result[Max] = 0;
			Read->Head += Max;
			return Result;
		} else {
			for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = (Ptr + 1) - Read->Head;
		            char *Result = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Result, Read->Head, Length);
		            Result[Length] = 0;
		            Read->Head = Ptr + 1;
		            return Result;
				};
			};
		    InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Remaining; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += (Ptr + 1) - Tail->Tail;
		                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
		                char *Temp = mempcpy(Result, InitialChars, InitialLength);
		                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
		                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
		                };
		                Temp = mempcpy(Temp, Tail->Chars, (Ptr + 1) - Tail->Chars);
						Temp[0] = 0;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						return Result;
					};
				};
		        TotalLength += Remaining;
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
                Temp = mempcpy(Temp, Tail->Chars, (Tail->Tail + Remaining) - Tail->Chars);
				Temp[0] = 0;
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				return Result;
			} else {
			    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += (Ptr + 1) - Tail->Tail;
		                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
		                char *Temp = mempcpy(Result, InitialChars, InitialLength);
		                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
		                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
		                };
		                Temp = mempcpy(Temp, Tail->Chars, (Ptr + 1) - Tail->Chars);
						Temp[0] = 0;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						return Result;
					};
				};
				Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static Std$Function$status stringi_nolimit(buffered_t *Stream, const Std$String$t *Term, Std$Function$result *Result) {
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
		    if (IsTerm[*Ptr]) {
	            int Length = (Ptr + 1) - Read->Head;
	            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
	            memcpy(Chars, Read->Head, Length);
	            Chars[Length] = 0;
	            Read->Head = Ptr + 1;
	            Result->Val = Std$String$new_length(Chars, Length);
	            return SUCCESS;
			};
		};
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
	        for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
	            if (IsTerm[*Ptr]) {
	                TotalLength += Ptr - Tail->Tail;
	                int NoOfBlocks = InitialLength ? 1 : 0;
					for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
					++NoOfBlocks;
					Std$String$t *String = Std$String$alloc(NoOfBlocks);
					String->Length.Value = TotalLength;
					Std$String$block *Block = String->Blocks;
					if (InitialLength) {
						Block->Length.Value = InitialLength;
						Block->Chars.Value = InitialChars;
						++Block;
					};
					for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
						Block->Length.Value = Buffer->Length;
						Block->Chars.Value = Buffer->Chars;
						++Block;
					};
					char *Chars = Riva$Memory$alloc_atomic((Ptr + 1) - Tail->Chars);
					memcpy(Chars, Tail->Chars, (Ptr + 1) - Tail->Chars);
					Block->Length.Value = (Ptr + 1) - Tail->Chars;
					Block->Chars.Value = Chars;
					Read->Chars = Tail->Chars;
					Read->Head = Ptr + 1;
					Read->Tail = Tail->Tail + Bytes;
					Read->Length = Tail->Length;
					Result->Val = Std$String$freeze(String);
					return SUCCESS;
				};
			};
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

METHOD("readi", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	const const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	if (Term->Length.Value == 0) return stringn(Stream, Max, Result);
	if (Max == 0) return stringi_nolimit(Stream, Term, Result);
	read_buffer_t *Read = Stream->Read;
	char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int Remaining, TotalLength;
	if (InitialLength) {
		if (InitialLength > Max) {
		    for (unsigned char *Ptr = Read->Head; Ptr < Read->Head + Max; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = (Ptr + 1) - Read->Head;
		            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Chars, Read->Head, Length);
		            Chars[Length] = 0;
		            Read->Head = Ptr + 1;
		            Result->Val = Std$String$new_length(Chars, Length);
		            return SUCCESS;
				};
			};
			char *Chars = Riva$Memory$alloc_atomic(Max + 1);
			memcpy(Chars, Read->Head, Max);
			Chars[Max] = 0;
			Read->Head += Max;
			Result->Val = Std$String$new_length(Chars, Max);
			return SUCCESS;
		} else {
			for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
		        if (IsTerm[*Ptr]) {
		            int Length = (Ptr + 1) - Read->Head;
		            char *Chars = Riva$Memory$alloc_atomic(Length + 1);
		            memcpy(Chars, Read->Head, Length);
		            Chars[Length] = 0;
		            Read->Head = Ptr + 1;
		            Result->Val = Std$String$new_length(Chars, Length);
		            return SUCCESS;
				};
			};
		    InitialChars = Read->Head;
		    Remaining = Max - InitialLength;
		    TotalLength = InitialLength;
		};
	} else {
	    Remaining = Max;
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
				int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			};
		    Tail->Space -= Bytes;
		    if (Bytes > Remaining) {
		        for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Remaining; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += (Ptr + 1) - Tail->Tail;
		                int NoOfBlocks = InitialLength ? 1 : 0;
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
						++NoOfBlocks;
						Std$String$t *String = Std$String$alloc(NoOfBlocks);
						String->Length.Value = TotalLength;
						Std$String$block *Block = String->Blocks;
						if (InitialLength) {
							Block->Length.Value = InitialLength;
							Block->Chars.Value = InitialChars;
							++Block;
						};
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
							Block->Length.Value = Buffer->Length;
							Block->Chars.Value = Buffer->Chars;
							++Block;
						};
						char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
						memcpy(Chars, Tail->Chars, (Ptr + 1) - Tail->Chars);
						Block->Length.Value = (Ptr + 1) - Tail->Chars;
						Block->Chars.Value = Chars;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						Result->Val = Std$String$freeze(String);
						return SUCCESS;
					};
				};
		        TotalLength += Remaining;
                int NoOfBlocks = InitialLength ? 1 : 0;
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
				if (Tail->Tail != Tail->Chars) ++NoOfBlocks;
				Std$String$t *String = Std$String$alloc(NoOfBlocks);
				String->Length.Value = TotalLength;
				Std$String$block *Block = String->Blocks;
				if (InitialLength) {
					Block->Length.Value = InitialLength;
					Block->Chars.Value = InitialChars;
					++Block;
				};
				for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
					Block->Length.Value = Buffer->Length;
					Block->Chars.Value = Buffer->Chars;
					++Block;
				};
				if (Tail->Tail != Tail->Chars) {
					char *Chars = Riva$Memory$alloc_atomic(Tail->Tail - Tail->Chars);
					memcpy(Chars, Tail->Chars, Tail->Tail - Tail->Chars);
					Block->Length.Value = Tail->Tail - Tail->Chars;
					Block->Chars.Value = Chars;
				};
				Read->Chars = Tail->Chars;
				Read->Head = Tail->Tail + Remaining;
				Read->Tail = Tail->Tail + Bytes;
				Read->Length = Tail->Length;
				Result->Val = Std$String$freeze(String);
				return SUCCESS;
			} else {
			    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
		            if (IsTerm[*Ptr]) {
		                TotalLength += Ptr - Tail->Tail;
		                int NoOfBlocks = InitialLength ? 1 : 0;
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) ++NoOfBlocks;
						++NoOfBlocks;
						Std$String$t *String = Std$String$alloc(NoOfBlocks);
						String->Length.Value = TotalLength;
						Std$String$block *Block = String->Blocks;
						if (InitialLength) {
							Block->Length.Value = InitialLength;
							Block->Chars.Value = InitialChars;
							++Block;
						};
						for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
							Block->Length.Value = Buffer->Length;
							Block->Chars.Value = Buffer->Chars;
							++Block;
						};
						char *Chars = Riva$Memory$alloc_atomic((Ptr + 1) - Tail->Chars);
						memcpy(Chars, Tail->Chars, (Ptr + 1) - Tail->Chars);
						Block->Length.Value = (Ptr + 1) - Tail->Chars;
						Block->Chars.Value = Chars;
						Read->Chars = Tail->Chars;
						Read->Head = Ptr + 1;
						Read->Tail = Tail->Tail + Bytes;
						Read->Length = Tail->Length;
						Result->Val = Std$String$freeze(String);
						return SUCCESS;
					};
				};
				Remaining -= Bytes;
				Tail->Tail += Bytes;
				TotalLength += Bytes;
			};
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

TYPED_INSTANCE(char *, IO$Stream$readl, TextReaderT, buffered_t *Stream) {
	read_buffer_t *Read = Stream->Read;
	char *InitialChars = 0;
	int InitialLength = Read->Tail - Read->Head;
	int TotalLength;
	if (InitialLength) {
		for (unsigned char *Ptr = Read->Head; Ptr < Read->Tail; ++Ptr) {
	        if (*Ptr == '\n') {
	            int Length = Ptr - Read->Head;
	            char *Result = Riva$Memory$alloc_atomic(Length + 1);
	            memcpy(Result, Read->Head, Length);
	            Result[Length] = 0;
	            Read->Head = Ptr + 1;
	            return Result;
			};
		};
	    InitialChars = Read->Head;
	    TotalLength = InitialLength;
	} else {
		TotalLength = 0;
	};
	temp_buffer_t *Head = new(temp_buffer_t);
	temp_buffer_t *Tail = Head;
	Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
	for (;;) {
		if (Tail->Space) {
		    int Bytes = Stream->read(Stream->Base, Tail->Tail, Tail->Space, 0);
			if (Bytes == 0) {
                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
                char *Temp = mempcpy(Result, InitialChars, InitialLength);
                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
                };
				Temp = mempcpy(Temp, Tail->Chars, Tail->Tail - Tail->Chars);
				Temp[0] = 0;
				Read->Head = Read->Tail = Read->Chars = Tail->Chars;
				Read->Length = Tail->Length;
				return Result;
			};
		    Tail->Space -= Bytes;
		    for (unsigned char *Ptr = Tail->Tail; Ptr < Tail->Tail + Bytes; ++Ptr) {
	            if (*Ptr == '\n') {
	                TotalLength += Ptr - Tail->Tail;
	                char *Result = Riva$Memory$alloc_atomic(TotalLength + 1);
	                char *Temp = mempcpy(Result, InitialChars, InitialLength);
	                for (temp_buffer_t *Buffer = Head; Buffer != Tail; Buffer = Buffer->Next) {
	                    Temp = mempcpy(Temp, Buffer->Chars, Buffer->Length);
	                };
	                Temp = mempcpy(Temp, Tail->Chars, Ptr - Tail->Chars);
					Temp[0] = 0;
					Read->Chars = Tail->Chars;
					Read->Head = Ptr + 1;
					Read->Tail = Tail->Tail + Bytes;
					Read->Length = Tail->Length;
					return Result;
				};
			};
			Tail->Tail += Bytes;
			TotalLength += Bytes;
		} else {
			temp_buffer_t *Buffer = new(temp_buffer_t);
			Tail->Next = Buffer;
			Tail = Buffer;
            Tail->Chars = Tail->Tail = Riva$Memory$alloc_atomic(Tail->Length = Tail->Space = Read->Length);
		};
	};
};

static int buffered_write(buffered_t *Stream, const char *Buffer, int Count, int Block) {
	write_buffer_t *Write = Stream->Write;
	int Total = 0;
	if (Write->Space >= Count) {
		memcpy(Write->Tail, Buffer, Count);
		Write->Tail += Count;
		Write->Space -= Count;
		return Total + Count;
	};
	if (buffered_flush(Stream)) return -1;
	int Bytes = Stream->write(Stream->Base, Buffer, (Count / Write->Length) * Write->Length, Block);
	if (Bytes == -1) return -1;
	Total += Bytes;
	Buffer += Bytes;
	Count -= Bytes;
	if (Count) {
		if (Count > Write->Space) Count = Write->Space;
		memcpy(Write->Tail, Buffer, Count);
		Write->Tail += Count;
		Write->Space -= Count;
		return Total + Count;
	};
};

TYPED_INSTANCE(int, IO$Stream$write, WriterT, buffered_t *Stream, const char *Buffer, int Count, int Block) {
	return buffered_write(Stream, Buffer, Count, Block);
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesWritten = buffered_write(Stream, Buffer, Size, (Count >= 3 && Args[3].Val == $block));
	if (BytesWritten < 0) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$writec, WriterT, buffered_t *Stream, char Char) {
	write_buffer_t *Write = Stream->Write;
	if (Write->Space == 0) {
		if (buffered_flush(Stream)) return -1;
	};
	Write->Tail[0] = Char;
	++Write->Tail;
	--Write->Space;
	return 1;
};

METHOD("write", TYP, WriterT, TYP, Std$String$T) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	const Std$String$t *String = (Std$String$t *)Args[1].Val;
	for (int I = 0; I < String->Count; ++I) {
		if (buffered_write(Stream, String->Blocks[I].Chars.Value, String->Blocks[I].Length.Value, 1) < 0) {
			Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$writes, WriterT, buffered_t *Stream, const char *Text) {
	return buffered_write(Stream, Text, strlen(Text), 1);
};

TYPED_INSTANCE(int, IO$Stream$writef, WriterT, buffered_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	return buffered_write(Stream, Buffer, Length, 1);
};

SYMBOL($seek, "seek");

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	read_buffer_t *Read = Stream->Read;
	Read->Head = Read->Tail = Read->Chars;
	buffered_flush(Stream);
	return Std$Function$call($seek, 2, Result, Stream->Base, 0, Args[1].Val, 0, Args[2].Val, 0);
};

TYPED_INSTANCE(int, IO$Stream$seek, SeekerT, buffered_t *Stream, int Position, int Mode) {
	read_buffer_t *Read = Stream->Read;
	Read->Head = Read->Tail = Read->Chars;
	buffered_flush(Stream);
	return IO$Stream$seek(Stream->Base, Position, Mode);
};

SYMBOL($tell, "tell");

METHOD("tell", TYP, T) {
	buffered_t *Stream = (buffered_t *)Args[0].Val;
	int Base = IO$Stream$tell(Stream->Base);
	Result->Val = Std$Integer$new_small(Base + Stream->Read->Tail - Stream->Read->Head);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$tell, SeekerT, buffered_t *Stream) {
	return IO$Stream$tell(Stream->Base) + Stream->Read->Tail - Stream->Read->Head;
};

