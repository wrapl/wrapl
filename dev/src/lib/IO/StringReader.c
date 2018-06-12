#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <stdint.h>
#include <string.h>

typedef struct reader_t {
	const Std$Type$t *Type;
	Std$String$t *String;
	Std$String$block *Block;
	int Offset;
} reader_t;

TYPE(T, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);

TYPED_INSTANCE(int, IO$Stream$eoi, T, reader_t *Stream) {
	return Stream->Block->Length.Value == 0;
};

TYPED_INSTANCE(int, IO$Stream$read, T, reader_t *Stream, char *Buffer, int Count, int Blocking) {
	Std$String$block *Block = Stream->Block;
	int Offset = Stream->Offset;
	int Total = 0;
	while (Count) {
		if (Block->Length.Value == 0) {
			Stream->Block = Block;
			return Total;
		}
		int Remaining = Block->Length.Value - Offset;
		if (Remaining > Count) {
			memcpy(Buffer, Block->Chars.Value + Offset, Count);
			Stream->Block = Block;
			Stream->Offset = Offset + Count;
			return Total + Count;
			Count = 0;
		} else {
			memcpy(Buffer, Block->Chars.Value + Offset, Remaining);
			Total += Remaining;
			Count -= Remaining;
			++Block;
			Offset = 0;
		};
	};
	Stream->Block = Block;
	return Total;
};

GLOBAL_FUNCTION(New, 1) {
	CHECK_ARG_TYPE(0, Std$String$T);
	reader_t *Reader = new(reader_t);
	Reader->Type = T;
	Reader->String = (Std$String$t *)Args[0].Val;
	Reader->Block = Reader->String->Blocks;
	Reader->Offset = 0;
	Result->Val = (Std$Object$t *)Reader;
	return SUCCESS;
};
