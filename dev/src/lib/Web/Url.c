#include <Std.h>
#include <Agg.h>
#include <IO/Stream.h>
#include <Riva/Memory.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>

static int DecodeHex[] = {
	['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
	['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
	['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
	['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15
};

typedef struct decoder_t {
	const char *Next;
	size_t Rem;
	void (*advance)(struct decoder_t *Decoder);
} decoder_t;

static inline void advance(decoder_t *Decoder) {
	if (Decoder->Rem > 1) {
		--Decoder->Rem;
		++Decoder->Next;
	} else {
		Decoder->advance(Decoder);
	};
};

static Std$String$t *next(decoder_t *Decoder, int Index) {
	char *Chars = Riva$Memory$alloc_atomic(64);
	size_t Length = 0, Max = 64;
	while (Decoder->Next) {
		char Char = Decoder->Next[0];
		switch (Char) {
		case '&': case '=': goto finished;
		case '%': {
			advance(Decoder);
			if (Decoder->Next == 0) goto finished;
			Char = DecodeHex[Decoder->Next[0]];
			advance(Decoder);
			if (Decoder->Next == 0) goto finished;
			Char = (Char << 4) + DecodeHex[Decoder->Next[0]];
			break;
		};
		case '+': Char = ' '; break;
		};
		advance(Decoder);
		Chars[Length] = Char;
		if (++Length == Max) {
			if (Max == 16384) {
				Std$String$t *Key = next(Decoder, Index + 1);
				Key->Length.Value += Length;
				Key->Blocks[Index].Length.Value = Length;
				Key->Blocks[Index].Chars.Value = Chars;
				return Key;
			};
			size_t NewMax = Max * 2;
			char *NewChars = Riva$Memory$alloc_atomic(NewMax);
			memcpy(NewChars, Chars, Max);
			Max = NewMax;
			Chars = NewChars;
		};
	};
finished:
	if (Length) {
		Std$String$t *Key = Std$String$alloc(Index + 1);
		Key->Length.Value = Length;
		Key->Blocks[Index].Length.Value = Length;
		Key->Blocks[Index].Chars.Value = Chars;
		return Key;
	} else {
		return Std$String$alloc(Index);
	};
};

ASYMBOL(Decode);

static Std$Function$status decode(decoder_t *Decoder, Std$Function$result *Result) {
	Std$Object$t *Table = Agg$Table$new(0, 0);
	while (Decoder->Next) {
		Std$Object$t *Key = Std$String$freeze(next(Decoder, 0));
		Std$Object$t *Value = Std$Object$Nil;
		if (Decoder->Next && Decoder->Next[0] == '=') {
			advance(Decoder);
			Value = Std$String$freeze(next(Decoder, 0));
		};
		Agg$Table$insert(Table, Key, Value);
		if (Decoder->Next == 0) break;
		if (Decoder->Next[0] != '&') {
			Result->Val = Std$String$new("Decode Error");
			return MESSAGE;
		};
		advance(Decoder);
	};
	Result->Val = Table;
	return SUCCESS;
};

typedef struct string_decoder_t {
	decoder_t Base;
	Std$Address$t *Block;
} string_decoder_t;

static void string_advance(string_decoder_t *Decoder) {
	if (Decoder->Base.Rem = Decoder->Block->Length.Value) {
		Decoder->Base.Next = Decoder->Block->Chars.Value;
		++Decoder->Block;
	} else {
		Decoder->Base.Next = 0;
	};
};

AMETHOD(Decode, TYP, Std$String$T) {
	string_decoder_t Decoder[1];
	Decoder->Base.Next = ((Std$String$t *)Args[0].Val)->Blocks->Chars.Value;
	Decoder->Base.Rem = ((Std$String$t *)Args[0].Val)->Blocks->Length.Value;
	Decoder->Base.advance = string_advance;
	Decoder->Block = ((Std$String$t *)Args[0].Val)->Blocks + 1;
	return decode(Decoder, Result);
};

typedef struct stream_decoder_t {
	decoder_t Base;
	Std$Object$t *Stream;
	char Buffer[256];
} stream_decoder_t;

static void stream_advance(stream_decoder_t *Decoder) {
	if ((Decoder->Base.Rem = IO$Stream$read(Decoder->Stream, Decoder->Buffer, 256, 1)) > 0) {
		Decoder->Base.Next = Decoder->Buffer;
	} else {
		Decoder->Base.Next = 0;
	};
};

AMETHOD(Decode, TYP, IO$Stream$ReaderT) {
	stream_decoder_t Decoder[1];
	Decoder->Base.advance = stream_advance;
	Decoder->Stream = Args[0].Val;
	stream_advance(Decoder);
	return decode(Decoder, Result);
};

GLOBAL_FUNCTION(DecodeStream, 1) {
	CHECK_ARG_TYPE(0, IO$Stream$ReaderT);
};

GLOBAL_FUNCTION(DecodeString, 1) {
	string_decoder_t Decoder[1];
	Decoder->Base.Next = ((Std$String$t *)Args[0].Val)->Blocks->Chars.Value;
	Decoder->Base.Rem = ((Std$String$t *)Args[0].Val)->Blocks->Length.Value;
	Decoder->Base.advance = string_advance;
	Decoder->Block = ((Std$String$t *)Args[0].Val)->Blocks + 1;
	Result->Val = Std$String$freeze(next(Decoder, 0));
	return SUCCESS;
};

Std$Object$t *_decode(const char *Chars, size_t Length) {
	Std$String$block Blocks[1] = {
		{{Std$Integer$SmallT, 0}, {Std$Address$T, 0}}
	};
	string_decoder_t Decoder[1];
	Decoder->Base.Next = Chars;
	Decoder->Base.Rem = Length;
	Decoder->Base.advance = string_advance;
	Decoder->Block = Blocks;
	Std$Object$t *Table = Agg$Table$new(0, 0);
	while (Decoder->Base.Next) {
		Std$Object$t *Key = Std$String$freeze(next(Decoder, 0));
		Std$Object$t *Value = Std$Object$Nil;
		if (Decoder->Base.Next[0] == '=') {
			advance(Decoder);
			Value = Std$String$freeze(next(Decoder, 0));
		};
		Agg$Table$insert(Table, Key, Value);
		if (Decoder->Base.Next == 0) break;
		if (Decoder->Base.Next[0] != '&') return Table;
		advance(Decoder);
	};
	return Table;
};

static const char EncodeHex[16] = "0123456789abcdef";

#define BLOCK_SIZE 128

typedef struct block_t block_t;

struct block_t {
	block_t *Next;
	size_t Space;
	char *Chars, *NextChar;
};

typedef struct encoder_t {
	block_t *Head, *Tail;
	int Count, Sep;
} encoder_t;

static void encoder_write(const char *Chars, size_t Length, encoder_t *Encoder) {
	block_t *Block = Encoder->Tail;
	if (Length < Block->Space) {
		strncpy(Block->NextChar, Chars, Length);
		Block->Space -= Length;
		Block->NextChar += Length;
	} else {
		strncpy(Block->NextChar, Chars, Block->Space);
		block_t *New = new(block_t);
		New->Space = BLOCK_SIZE;
		New->Chars = New->NextChar = Riva$Memory$alloc_atomic(BLOCK_SIZE);
		Block->Next = New;
		Encoder->Tail = New;
		++Encoder->Count;
		if (Length > Block->Space) encoder_write(Chars + Block->Space, Length - Block->Space, Encoder);
	};
};

static void encode_string(Std$String$t *String, encoder_t *Encoder) {
	Std$Address$t *Block = String->Blocks;
	while (Block->Length.Value) {
		size_t Length = Block->Length.Value;
		const char *Chars = Block->Chars.Value;
		size_t I = 0;
		for (size_t J = 0; J < Length; ++J) {
			char Char = Chars[J];
			if (isalnum(Char) || Char == '-' || Char == '_' || Char == '.' || Char == '~') {
			} else if (Char == ' ') {
				encoder_write(Chars + I, J - I, Encoder);
				encoder_write("+", 1, Encoder);
				I = J + 1;
			} else {
				char HexChars[3] = {'%', EncodeHex[Char >> 4], EncodeHex[Char & 15]};
				encoder_write(Chars + I, J - I, Encoder);
				encoder_write(HexChars, 3, Encoder);
				I = J + 1;
			};
		};
		if (I < Length) encoder_write(Chars + I, Length - I, Encoder);
		++Block;
	};
};

GLOBAL_FUNCTION(EncodeString, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	block_t *Block = new(block_t);
	Block->Space = BLOCK_SIZE;
	Block->Chars = Block->NextChar = Riva$Memory$alloc_atomic(BLOCK_SIZE);
	encoder_t Encoder = {Block, Block, 1, 0};
	encode_string((Std$String$t *)Args[0].Val, &Encoder);
	Std$String$t *String;
	size_t Length = 0;
	if (Encoder.Tail->Space < BLOCK_SIZE) {
		String = Std$String$alloc(Encoder.Count);
		for (size_t I = 0; I < Encoder.Count - 1; ++I) {
			String->Blocks[I].Chars.Value = Block->Chars;
			String->Blocks[I].Length.Value = BLOCK_SIZE;
			Length += BLOCK_SIZE;
			Block = Block->Next;
		};
		String->Blocks[Encoder.Count - 1].Chars.Value = Block->Chars;
		String->Blocks[Encoder.Count - 1].Length.Value = BLOCK_SIZE - Block->Space;
		String->Length.Value = Length + BLOCK_SIZE - Block->Space;
	} else {
		String = Std$String$alloc(Encoder.Count - 1);
		for (size_t I = 0; I < Encoder.Count - 1; ++I) {
			String->Blocks[I].Chars.Value = Block->Chars;
			String->Blocks[I].Length.Value = BLOCK_SIZE;
			Block = Block->Next;
		};
		String->Length.Value = Length;
	};
	Std$String$freeze(String);
	Result->Val = (Std$Object$t *)String;
	return SUCCESS;
};


static void encode_key_value(Std$Object$t *Key, Std$Object$t *Value, encoder_t *Encoder) {
	if (Encoder->Sep) encoder_write("&", 1, Encoder);
	Std$Function$result Result[1];
	if (Std$Function$call(Std$String$Of, 1, Result, Key, 0) <= SUCCESS) {
		encode_string((Std$String$t *)Result->Val, Encoder);
	};
	encoder_write("=", 1, Encoder);
	if (Std$Function$call(Std$String$Of, 1, Result, Value, 0) <= SUCCESS) {
		encode_string((Std$String$t *)Result->Val, Encoder);
	};
	Encoder->Sep = 1;
};

GLOBAL_FUNCTION(Encode, 1) {
	CHECK_ARG_TYPE(0, Agg$Table$T);
	block_t *Block = new(block_t);
	Block->Space = BLOCK_SIZE;
	Block->Chars = Block->NextChar = Riva$Memory$alloc_atomic(BLOCK_SIZE);
	encoder_t Encoder = {Block, Block, 1, 0};
	Agg$Table$foreach(Args[0].Val, encode_key_value, &Encoder);
	Std$String$t *String;
	size_t Length = 0;
	if (Encoder.Tail->Space < BLOCK_SIZE) {
		String = Std$String$alloc(Encoder.Count);
		for (size_t I = 0; I < Encoder.Count - 1; ++I) {
			String->Blocks[I].Chars.Value = Block->Chars;
			String->Blocks[I].Length.Value = BLOCK_SIZE;
			Length += BLOCK_SIZE;
			Block = Block->Next;
		};
		String->Blocks[Encoder.Count - 1].Chars.Value = Block->Chars;
		String->Blocks[Encoder.Count - 1].Length.Value = BLOCK_SIZE - Block->Space;
		String->Length.Value = Length + BLOCK_SIZE - Block->Space;
	} else {
		String = Std$String$alloc(Encoder.Count - 1);
		for (size_t I = 0; I < Encoder.Count - 1; ++I) {
			String->Blocks[I].Chars.Value = Block->Chars;
			String->Blocks[I].Length.Value = BLOCK_SIZE;
			Block = Block->Next;
		};
		String->Length.Value = Length;
	};
	Std$String$freeze(String);
	Result->Val = (Std$Object$t *)String;
	return SUCCESS;
};
