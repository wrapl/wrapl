#include <IO/Windows.h>
#include <Util/TypedFunction.h>
#include <Std.h>
#include <Riva.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

SYMBOL($AS, "@");
SYMBOL($block, "block");

TYPE(T, IO$Stream$T);

TYPE(ReaderT, T, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, IO$Stream$WriterT, IO$Stream$T);
TYPE(SeekerT, T, IO$Stream$SeekerT, IO$Stream$T);

TYPE(TextReaderT, T, ReaderT, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
TYPE(TextWriterT, T, WriterT, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

static IO$Stream_messaget ConvertMessage[] = {{IO$Stream$MessageT, "Conversion Error"}};
static IO$Stream_messaget ReadMessage[] = {{IO$Stream$MessageT, "Read Error"}};
static IO$Stream_messaget WriteMessage[] = {{IO$Stream$MessageT, "Write Error"}};
static IO$Stream_messaget FlushMessage[] = {{IO$Stream$MessageT, "Flush Error"}};
static IO$Stream_messaget SeekMessage[] = {{IO$Stream$MessageT, "Seek Error"}};
static IO$Stream_messaget CloseMessage[] = {{IO$Stream$MessageT, "Close Error"}};

typedef struct fast_buffer {
	struct fast_buffer *Next;
	unsigned char *Chars;
} fast_buffer;

#define FastBufferSize 100
static fast_buffer *alloc_fast_buffer(void) {
	fast_buffer *Buffer = new(fast_buffer);
	Buffer->Chars = Riva$Memory$alloc_atomic(FastBufferSize);
	return Buffer;
};

static void windows_flush(IO$Windows_t *Stream) {
	FlushFileBuffers(Stream->Handle);
};

static void windows_close(IO$Windows_t *Stream) {
	CloseHandle(Stream->Handle);
	Riva$Memory$register_finalizer(Stream, 0, 0, 0, 0);
};

static void windows_finalize(IO$Windows_t *Stream, void *Data) {
	CloseHandle(Stream->Handle);
};

void _windows_register_finalizer(IO$Windows_t *Stream) {
	Riva$Memory$register_finalizer(Stream, windows_finalize, 0, 0, 0);
};

IO$Windows_t *_windows_new(Std$Type_t *Type, int Handle) {
	IO$Windows_t *Stream = new(IO$Windows_t);
	Stream->Type = Type;
	Stream->Handle = Handle;
	Riva$Memory$register_finalizer(Stream, windows_finalize, 0, 0, 0);
	return Stream;
};

static int windows_eoi(IO$Windows_t *Stream) {
    char Buffer;
    unsigned long BytesRead;
	return (ReadFile(Stream->Handle, &Buffer, 0, &BytesRead, 0) == 0);
};

static inline read(DWORD Handle, char *Buffer, int Count) {
	DWORD BytesRead;
	if (ReadFile(Handle, Buffer, Count, &BytesRead, 0) == 0) {
	    return -1;
	};
	return BytesRead;
};

static inline write(DWORD Handle, char *Buffer, int Count) {
	DWORD BytesWritten;
	if (WriteFile(Handle, Buffer, Count, &BytesWritten, 0) == 0) {
	    return -1;
	};
	return BytesWritten;
};

static inline int write_all(int Handle, char *Chars, int Count) {
	while (Count) {
		int Bytes = write(Handle, Chars, Count);
		if (Bytes == -1) return -1;
		Count -= Bytes;
		Chars += Bytes;
	};
	return 0;
};

static int windows_read(IO$Windows_t *Stream, char *Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = read(Stream->Handle, Buffer, Count);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return read(Stream->Handle, Buffer, Count);
	};
};

static char *windows_readx(IO$Windows_t *Stream, int Max, const char *Term, int TermSize) {
	int Handle = Stream->Handle;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: return -1;
	case 0: return 0;
	};
	unsigned char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	if (IsTerm[Char]) return "";
	if (Max == 1) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		return Chars;
	};
	IO$Stream_buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return -1;
		default: {
			if (IsTerm[Char]) {
			} else {
				if (Space == 0) {
					IO$Stream_buffer *Buffer = IO$Stream$alloc_buffer();
					Tail = (Tail->Next = Buffer);
					Space = IO$Stream$BufferSize;
					Ptr = Tail->Chars;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				if (Length == Max) {
				} else {
					break;
				};
			};
		};
		case 0: {
			unsigned char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream_buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, (Length - 1) % IO$Stream$BufferSize + 1);
			Chars[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

static char *windows_readi(IO$Windows_t *Stream, int Max, const char *Term, int TermSize) {
	int Handle = Stream->Handle;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: return -1;
	case 0: return 0;
	};
	unsigned char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	if (IsTerm[Char] || (Max == 1)) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		return Chars;
	};
	IO$Stream_buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return -1;
		default: {
			if (Space == 0) {
				IO$Stream_buffer *Buffer = IO$Stream$alloc_buffer();
				Tail = (Tail->Next = Buffer);
				Space = IO$Stream$BufferSize;
				Ptr = Tail->Chars;
			};
			*(Ptr++) = Char;
			--Space;
			++Length;
			if (IsTerm[Char] || (Length == Max)) {
			} else {
				break;
			};
		};
		case 0: {
			unsigned char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream_buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, (Length - 1) % IO$Stream$BufferSize + 1);
			Chars[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

static char windows_readc(IO$Windows_t *Stream) {
	char Char;
	int Status = read(Stream->Handle, &Char, 1);
	if (Status < 0) return 0;
	if (Status == 0) return EOF;
	return Char;
};

static char *windows_readl(IO$Windows_t *Stream) {
	int Handle = Stream->Handle;
	char Char;
	do {
		switch (read(Handle, &Char, 1)) {
		case -1: return -1;
		case 0: return 0;
		};
	} while (Char == '\r');
	if (Char == '\n') return "";
	IO$Stream_buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return -1;
		default: {
			if (Char == '\n') {
			} else if (Char == '\r') {
				break;
			} else {
				if (Space == 0) {
					IO$Stream_buffer *Buffer = IO$Stream$alloc_buffer();
					Tail = (Tail->Next = Buffer);
					Space = IO$Stream$BufferSize;
					Ptr = Tail->Chars;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				break;
			};
		};
		case 0: {
			char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream_buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, (Length - 1) % IO$Stream$BufferSize + 1);
			Chars[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

static int windows_write(IO$Windows_t *Stream, const char *Buffer, int Count, int Blocks) {
	if (Blocks) {
		int Total = 0;
		while (Count) {
			int Bytes = write(Stream->Handle, Buffer, Count);
			if (Bytes == -1) return -1;
			if (Bytes == 0) return Total;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return write(Stream->Handle, Buffer, Count);
	};
};
static void windows_writec(IO$Windows_t *Stream, char Char) {
	write(Stream->Handle, &Char, 1);
};

static void windows_writes(IO$Windows_t *Stream, const char *Text) {
	write_all(Stream->Handle, Text, strlen(Text));
};

static void windows_writef(IO$Windows_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	write_all(Stream->Handle, Buffer, Length);
};

static int windows_seek(IO$Windows_t *Stream, int Position) {
	return SetFilePointer(Stream->Handle, Position, 0, FILE_BEGIN);
};

static int windows_tell(IO$Windows_t *Stream) {
	return SetFilePointer(Stream->Handle, 0, 0, FILE_CURRENT);
};

INITIAL() {
	Util$TypedFunction$set(IO$Stream$close, T, windows_close);
	Util$TypedFunction$set(IO$Stream$eoi, ReaderT, windows_eoi);
	Util$TypedFunction$set(IO$Stream$read, ReaderT, windows_read);
	Util$TypedFunction$set(IO$Stream$readx, ReaderT, windows_readx);
	Util$TypedFunction$set(IO$Stream$readi, ReaderT, windows_readi);
	Util$TypedFunction$set(IO$Stream$readc, ReaderT, windows_readc);
	Util$TypedFunction$set(IO$Stream$readl, ReaderT, windows_readl);
	Util$TypedFunction$set(IO$Stream$flush, WriterT, windows_flush);
	Util$TypedFunction$set(IO$Stream$write, WriterT, windows_write);
	Util$TypedFunction$set(IO$Stream$writec, WriterT, windows_writec);
	Util$TypedFunction$set(IO$Stream$writes, WriterT, windows_writes);
	Util$TypedFunction$set(IO$Stream$writef, WriterT, windows_writef);
	Util$TypedFunction$set(IO$Stream$seek, SeekerT, windows_seek);
	Util$TypedFunction$set(IO$Stream$tell, SeekerT, windows_tell);
};

METHOD("flush", TYP, T) {
	IO$Windows_t *Stream = Args[0].Val;
	if (FlushFileBuffers(Stream->Handle) == 0) {
		Result->Val = FlushMessage;
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("close", TYP, T) {
	IO$Windows_t *Stream = Args[0].Val;
	if (CloseHandle(Stream->Handle) == 0) {
		Result->Val = CloseMessage;
		return MESSAGE;
	};
	Riva$Memory$register_finalizer(Stream, 0, 0, 0, 0);
	Result->Val = Std$Object$Nil;
	return SUCCESS;
};

METHOD("closed", TYP, T) {
	IO$Windows_t *Stream = Args[0].Val;
	// TO BE FIXED
	return FAILURE;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	IO$Windows_t *Stream = Args[0].Val;
	char *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	long Size = ((Std$Integer_smallt *)Args[2].Val)->Value;
	size_t BytesRead = read(Stream->Handle, Buffer, Size);
	if (BytesRead < 0) {
		Result->Val = IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	IO$Windows_t *Stream = Args[0].Val;
	char *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	long Size = ((Std$Integer_smallt *)Args[2].Val)->Value;
	size_t BytesRead = 0;
	while (Size) {
		long Bytes = read(Stream->Handle, Buffer, Size);
		if (Bytes < 0) {
			Result->Val = IO$Stream$ReadMessage;
			return MESSAGE;
		};
		if (Bytes == 0) break;
		BytesRead += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

METHOD("readx", TYP, TextReaderT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	int Handle = ((IO$Windows_t *)Args[0].Val)->Handle;
	Std$String_t *Term = Args[2].Val;
	int Max = ((Std$Integer_smallt *)Args[1].Val)->Value;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
	case 0: return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (Std$String_block *Block = Term->Blocks; Block->Length.Value; Block++) {
		unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	if (IsTerm[Char]) {Result->Val = Std$String$Empty; return SUCCESS;};
	if (Max == 1) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		Result->Val = Std$String$new_length(Chars, 1);
		return SUCCESS;
	};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
		default: {
			if (IsTerm[Char]) {
			} else {
				if (Space == 0) {
					Tail = (Tail->Next = alloc_fast_buffer());
					Space = FastBufferSize;
					Ptr = Tail->Chars;
					++NoOfBlocks;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				if (Length == Max) {
				} else {
					break;
				};
			};
		};
		case 0: {
			Std$String_t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$String_block *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Chars.Value = Tail->Chars;
			Std$String$freeze(String);
			Result->Val = String;
			return SUCCESS;
		};
		};
	};
};

METHOD("readi", TYP, TextReaderT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	int Handle = ((IO$Windows_t *)Args[0].Val)->Handle;
	Std$String_t *Term = Args[2].Val;
	int Max = ((Std$Integer_smallt *)Args[1].Val)->Value;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
	case 0: return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (Std$String_block *Block = Term->Blocks; Block->Length.Value; Block++) {
		unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	if (IsTerm[Char] || (Max == 1)) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		Result->Val = Std$String$new_length(Chars, 1);
		return SUCCESS;
	};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
		default: {
			if (Space == 0) {
				Tail = (Tail->Next = alloc_fast_buffer());
				Space = FastBufferSize;
				Ptr = Tail->Chars;
				++NoOfBlocks;
			};
			*(Ptr++) = Char;
			--Space;
			++Length;
			if (IsTerm[Char] || (Length == Max)) {
			} else {
				break;
			};
		};
		case 0: {
			Std$String_t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$String_block *Block = String->Blocks;
			IO$Stream_buffer *Buffer = Head;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Chars.Value = Tail->Chars;
			Std$String$freeze(String);
			Result->Val = String;
			return SUCCESS;
		};
		};
	};
};

METHOD("rest", TYP, ReaderT) {
	int Handle = ((IO$Windows_t *)Args[0].Val)->Handle;
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	for (;;) {
		int Bytes = read(Handle, Ptr, Space);
		if (Bytes == -1) {
			Result->Val = IO$Stream$ReadMessage;
			return MESSAGE;
		};
		if (Bytes == 0) break;
		Length += Bytes;
		Ptr += Bytes;
		Space -= Bytes;
		if (Space == 0) {
			Tail = (Tail->Next = alloc_fast_buffer());
			Space = FastBufferSize;
			Ptr = Tail->Chars;
			++NoOfBlocks;
		};
	};
	if (Ptr == Tail->Chars) NoOfBlocks--;
	Std$String_t *String = Std$String$alloc(NoOfBlocks);
	String->Length.Value = Length;
	Std$String_block *Block = String->Blocks;
	for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
		Block->Length.Value = FastBufferSize;
		Block->Chars.Value = Buffer->Chars;
		++Block;
	};
	if (Ptr != Tail->Chars) {
		Block->Length.Value = Ptr - Tail->Chars;
		Block->Chars.Value = Tail->Chars;
	};
	Std$String$freeze(String);
	Result->Val = String;
	return SUCCESS;
};

METHOD("read", TYP, ReaderT, TYP, Std$Integer$SmallT) {
	int Handle = ((IO$Windows_t *)Args[0].Val)->Handle;
	int Max = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (Max <= FastBufferSize) {
		char *String = Riva$Memory$alloc_atomic(Max + 1);
		int Length = 0;
		char *Buffer = String;
		while (Max) {
			int Bytes = read(Handle, Buffer, Max);
			if (Bytes == 0) break;
			if (Bytes == -1) {
				Result->Val = IO$Stream$ReadMessage;
				return MESSAGE;
			};
			Buffer += Bytes;
			Length += Bytes;
			Max -= Bytes;
		};
		Buffer[0] = 0;
		if (Length) {
			Result->Val = Std$String$new_length(String, Length);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	} else {
		fast_buffer *Head, *Tail;
		Head = Tail = alloc_fast_buffer();
		int Length = 0, NoOfBlocks = 1;
		int Space = FastBufferSize;
		unsigned char *Ptr = Tail->Chars;
		while (Max) {
			int Bytes = read(Handle, Ptr, Space);
			if (Bytes == -1) {
				Result->Val = IO$Stream$ReadMessage;
				return MESSAGE;
			};
			if (Bytes == 0) break;
			Length += Bytes;
			Ptr += Bytes;
			Space -= Bytes;
			if (Space == 0) {
				Tail = (Tail->Next = alloc_fast_buffer());
				Space = FastBufferSize;
				Ptr = Tail->Chars;
				++NoOfBlocks;
			};
			Max -= Bytes;
		};
		if (Length == 0) return FAILURE;
		if (Ptr == Tail->Chars) NoOfBlocks--;
		Std$String_t *String = Std$String$alloc(NoOfBlocks);
		String->Length.Value = Length;
		Std$String_block *Block = String->Blocks;
		for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
			Block->Length.Value = FastBufferSize;
			Block->Chars.Value = Buffer->Chars;
			++Block;
		};
		if (Ptr != Tail->Chars) {
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Chars.Value = Tail->Chars;
		};
		Std$String$freeze(String);
		Result->Val = String;
		return SUCCESS;
	};
};

METHOD("read", TYP, TextReaderT) {
	int Handle = ((IO$Windows_t *)Args[0].Val)->Handle;
	char Char;
	do {
		switch (read(Handle, &Char, 1)) {
		case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
		case 0: return FAILURE;
		};
	} while (Char == '\r');
	if (Char == '\n') {Result->Val = Std$String$Empty; return SUCCESS;};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: Result->Val = IO$Stream$ReadMessage; return MESSAGE;
		default: {
			if (Char == '\n') {
			} else if (Char == '\r') {
				break;
			} else {
				if (Space == 0) {
					Tail = (Tail->Next = alloc_fast_buffer());
					Space = FastBufferSize;
					Ptr = Tail->Chars;
					++NoOfBlocks;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				break;
			};
		};
		case 0: {
			Std$String_t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$String_block *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Chars.Value = Tail->Chars;
			Std$String$freeze(String);
			Result->Val = String;
			return SUCCESS;
		};
		};
	};
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	IO$Windows_t *Stream = Args[0].Val;
	char *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	long Size = ((Std$Integer_smallt *)Args[2].Val)->Value;
	size_t BytesWritten = write(Stream->Handle, Buffer, Size);
	if (BytesWritten < 0) {
		Result->Val = IO$Stream$WriteMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

METHOD("write", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	IO$Windows_t *Stream = Args[0].Val;
	char *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	long Size = ((Std$Integer_smallt *)Args[2].Val)->Value;
	size_t BytesWritten = 0;
	while (Size) {
		long Bytes = write(Stream->Handle, Buffer, Size);
		if (Bytes < 0) {
			Result->Val = IO$Stream$ReadMessage;
			return MESSAGE;
		};
		if (Bytes == 0) break;
		BytesWritten += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

METHOD("write", TYP, WriterT, TYP, Std$String$T) {
	IO$Windows_t *Stream = Args[0].Val;
	Std$String_t *String = Args[1].Val;
	for (long I = 0; I < String->Count; ++I) {
		if (write_all(Stream->Handle, String->Blocks[I].Chars.Value, String->Blocks[I].Length.Value) < 0) {
			Result->Val = IO$Stream$WriteMessage;
			return MESSAGE;
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, TextWriterT, ANY) {
	IO$Windows_t *Stream = Args[0].Val;
	Std$Function_result Result0;
	switch (Std$Function$call($AS, 2, &Result0, Args[1].Val, Args[1].Ref, Std$String$T, 0)) {
	case SUSPEND:
	case SUCCESS: {
		Std$String_t *String = Result0.Val;
		for (long I = 0; I < String->Count; ++I) {
			if (write_all(Stream->Handle, String->Blocks[I].Chars.Value, String->Blocks[I].Length.Value) < 0) {
				Result->Val = IO$Stream$WriteMessage;
				return MESSAGE;
			};
		};
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case FAILURE:
		Result->Val = IO$Stream$ConvertMessage;
		return MESSAGE;
	case MESSAGE:
		Result->Arg = Result0.Arg;
		return MESSAGE;
	};
};

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT) {
	IO$Windows_t *Stream = Args[0].Val;
	long Position = ((Std$Integer_smallt *)Args[1].Val)->Value;
	Position = SetFilePointer(Stream->Handle, Position, 0, FILE_BEGIN);
	if (Position < 0) {
		Result->Val = SeekMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Position);
	return SUCCESS;
};

METHOD("tell", TYP, T) {
	IO$Windows_t *Stream = Args[0].Val;
	Result->Val = Std$Integer$new_small(SetFilePointer(Stream->Handle, 0, 0, FILE_CURRENT));
	return SUCCESS;
};
