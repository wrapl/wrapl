#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <Std.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <nettle/arcfour.h>

typedef struct cipher_t {
	const Std$Type_t *Type;
	IO$Stream_t *Base;
	struct arcfour_ctx Context[1];
} cipher_t;

TYPE(T, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

GLOBAL_FUNCTION(New, 2) {
	cipher_t *Stream = new(cipher_t);
	Stream->Type = T;
	Stream->Base = Args[0].Val;
	Std$String_t *Key = Args[1].Val;
	arcfour_set_key(Stream->Context, Key->Length.Value, Std$String$flatten(Key));
	Result->Val = Stream;
	return SUCCESS;
};

METHOD("base", TYP, T) {
	cipher_t *Stream = Args[0].Val;
	Result->Val = Stream->Base;
	return SUCCESS;
};

static int cipher_eoi(cipher_t *Stream) {
	return IO$Stream$eoi(Stream->Base);
};

static int cipher_close(cipher_t *Stream, int Mode) {
	return IO$Stream$close(Stream->Base, Mode);
};

static Std$Integer_smallt Zero[] = {{Std$Integer$SmallT, 0}};

#ifdef WINDOWS

static inline void *mempcpy(void *Dst, const void *Src, int Len) {
	memcpy(Dst, Src, Len);
	return (char *)Dst + Len;
};

#endif

static int cipher_read(cipher_t *Stream, char *Buffer, int Count, int Block) {
	int Length = IO$Stream$read(Stream->Base, Buffer, Count, Block);
	arcfour_crypt(Stream->Context, Length, Buffer, Buffer);
	return Length;
};

static char *cipher_readx(cipher_t *Stream, int Max, const char *Term, int TermSize) {
	unsigned char Char;
	switch (cipher_read(Stream, &Char, 1, 0)) {
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
		switch (cipher_read(Stream, &Char, 1, 0)) {
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

static char *cipher_readi(cipher_t *Stream, int Max, const char *Term, int TermSize) {
	unsigned char Char;
	switch (cipher_read(Stream, &Char, 1, 0)) {
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
		switch (cipher_read(Stream, &Char, 1, 0)) {
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

static char cipher_readc(cipher_t *Stream) {
	char Char = IO$Stream$readc(Stream->Base);
	if (Char == EOF) return EOF;
	arcfour_crypt(Stream->Context, 1, &Char, &Char);
	return Char;
};

static inline Std$String_t *crypt_string(struct arcfour_ctx *Context, Std$String_t *Src) {
	size_t Size = sizeof(Std$String_t) + (Src->Count + 1) * sizeof(Std$String_block);
	Std$String_t *Dst = Riva$Memory$alloc_stubborn(Size);
	memcpy(Dst, Src, Size);
	for (Std$String_block *Block = Dst->Blocks; Block->Length.Value; Block++) {
		char *Chars = Riva$Memory$alloc_atomic(Block->Length.Value);
		arcfour_crypt(Context, Block->Length.Value, Chars, Block->Chars.Value);
		Block->Chars.Value = Chars;
	};
	Riva$Memory$freeze_stubborn(Dst);
	return Dst;
};

static char *cipher_readl(cipher_t *Stream) {
	char Char;
	do {
		switch (cipher_read(Stream, &Char, 1, 0)) {
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
		switch (cipher_read(Stream, &Char, 1, 0)) {
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

static int cipher_write(cipher_t *Stream, const char *Source, int Length, int Block) {
	char Buffer[256];
	int Total = 0;
	IO$Stream_writefn write = Util$TypedFunction$get(IO$Stream$write, Stream->Base->Type);
	while (Length > 256) {
		arcfour_crypt(Stream->Context, 256, Source, Buffer);
		int Rem = 256;
		int Ptr = Buffer;
		while (Rem) {
			int Bytes = write(Stream->Base, Ptr, Rem, 1);
			if (Bytes == -1) return -1;
			if (Bytes == 0) return Total;
			Total += Bytes;
			Ptr += Bytes;
			Rem -= 256;
		};
		Length -= 256;
	};
	arcfour_crypt(Stream->Context, Length, Source, Buffer);
	char *Ptr = Buffer;
	while (Length) {
		int Bytes = write(Stream->Base, Ptr, Length, 1);
		if (Bytes == -1) return -1;
		if (Bytes == 0) return Total;
		Total += Bytes;
		Ptr += Bytes;
		Length -= Bytes;
	};
	return Total;
};

static void cipher_writec(cipher_t *Stream, char Char) {
	arcfour_crypt(Stream->Context, 1, &Char, &Char);
	IO$Stream$writec(Stream->Base, Char);
};

static void cipher_writes(cipher_t *Stream, const char *Text) {
	cipher_write(Stream, Text, strlen(Text), 1);
};

static void cipher_writef(cipher_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	cipher_write(Stream, Chars, Length, 1);
};

INITIAL() {
	Util$TypedFunction$set(IO$Stream$eoi, T, cipher_eoi);
	Util$TypedFunction$set(IO$Stream$close, T, cipher_close);
	Util$TypedFunction$set(IO$Stream$read, T, cipher_read);
	Util$TypedFunction$set(IO$Stream$readc, T, cipher_readc);
	Util$TypedFunction$set(IO$Stream$readl, T, cipher_readl);
	Util$TypedFunction$set(IO$Stream$write, T, cipher_write);
	Util$TypedFunction$set(IO$Stream$writec, T, cipher_writec);
	Util$TypedFunction$set(IO$Stream$writes, T, cipher_writes);
	Util$TypedFunction$set(IO$Stream$writef, T, cipher_writef);
};
SYMBOL($read, "read");
SYMBOL($rest, "rest");
SYMBOL($write, "write");
SYMBOL($block, "block");

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	cipher_t *Stream = Args[0].Val;
	uint8_t *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	int Bytes = IO$Stream$read(Stream->Base, Buffer, ((Std$Integer_smallt *)Args[2].Val)->Value, 0);
	if (Bytes == -1) {
		Result->Val = IO$Stream$ReadMessage;
		return MESSAGE;
	};
	if (Bytes == 0) return FAILURE;
	arcfour_crypt(Stream->Context, Bytes, Buffer, Buffer);
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	cipher_t *Stream = Args[0].Val;
	uint8_t *Buffer = ((Std$Address_t *)Args[1].Val)->Value;
	int Bytes = IO$Stream$read(Stream->Base, Buffer, ((Std$Integer_smallt *)Args[2].Val)->Value, 1);
	if (Bytes == -1) {
		Result->Val = IO$Stream$ReadMessage;
		return MESSAGE;
	};
	if (Bytes == 0) return FAILURE;
	arcfour_crypt(Stream->Context, Bytes, Buffer, Buffer);
	return SUCCESS;
};

METHOD("crypt", TYP, T, TYP, Std$String$T) {
	cipher_t *Stream = Args[0].Val;
	Std$String_t *String = Args[1].Val;
	Result->Val = crypt_string(Stream->Context, String);
	return SUCCESS;
};

METHOD("rest", TYP, T) {
	cipher_t *Stream = Args[0].Val;
	switch (Std$Function$call($rest, 1, Result, Stream->Base, 0)) {
	case SUCCESS: case SUSPEND: {
		Result->Val = crypt_string(Stream->Context, Result->Val);
		return SUCCESS;
	};
	case FAILURE: return FAILURE;
	case MESSAGE: return MESSAGE;
	};
};

METHOD("read", TYP, T, TYP, Std$Integer$SmallT) {
	cipher_t *Stream = Args[0].Val;
	switch (Std$Function$call($read, 2, Result, Stream->Base, 0, Args[1].Val, 0)) {
	case SUCCESS: case SUSPEND: {
		Result->Val = crypt_string(Stream->Context, Result->Val);
		return SUCCESS;
	};
	case FAILURE: return FAILURE;
	case MESSAGE: return MESSAGE;
	};
};

METHOD("read", TYP, T) {
	cipher_t *Stream = Args[0].Val;
	switch (Std$Function$call($read, 1, Result, Stream->Base, 0)) {
	case SUCCESS: case SUSPEND: {
		Result->Val = crypt_string(Stream->Context, Result->Val);
		return SUCCESS;
	};
	case FAILURE: return FAILURE;
	case MESSAGE: return MESSAGE;
	};
};

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	cipher_t *Stream = Args[0].Val;
	char *Source = ((Std$Address_t *)Args[1].Val)->Value;
	int Length = ((Std$Integer_smallt *)Args[2].Val)->Value;
	char Buffer[256];
	int Total = 0;
	while (Length > 256) {
		arcfour_crypt(Stream->Context, 256, Source, Buffer);
		int Bytes = IO$Stream$write(Stream->Base, Buffer, 256, 1);
		if (Bytes == -1) {
			Result->Val = IO$Stream$WriteMessage;
			return MESSAGE;
		};
		if (Bytes < 256) {
			Result->Val = Std$Integer$new_small(Total + Bytes);
			return SUCCESS;
		};
		Total += 256;
		Source += 256;
		Length -= 256;
	};
	arcfour_crypt(Stream->Context, Length, Source, Buffer);
	int Bytes = IO$Stream$write(Stream->Base, Buffer, Length, 1);
	if (Bytes == -1) {
		Result->Val = IO$Stream$WriteMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Total + Bytes);
	return SUCCESS;
};

METHOD("write", TYP, T, TYP, Std$String$T) {
	cipher_t *Stream = Args[0].Val;
	return Std$Function$call($write, 2, Result, Stream->Base, 0, crypt_string(Stream->Context, Args[1].Val), 0);
};

