#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <stdint.h>
#include <string.h>

typedef struct reader_t {
	const Std$Type$t *Type;
	char *Address;
	int Length;
} reader_t;

TYPE(T, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
// A simple reader pointing to a section of memory.

TYPED_INSTANCE(int, IO$Stream$eoi, T, reader_t *Stream) {
	return Stream->Length == 0;
};

TYPED_INSTANCE(int, IO$Stream$read, T, reader_t *Stream, char *Buffer, int Count, int Blocking) {
	if (Stream->Length > Count) {
		memcpy(Buffer, Stream->Address, Count);
		Stream->Address += Count;
		Stream->Length -= Count;
		return Count;
	} else {
		memcpy(Buffer, Stream->Address, Stream->Length);
		int Length = Stream->Length;
		Stream->Address = 0;
		Stream->Length = 0;
		return Length;
	}
};

ASYMBOL(New);

AMETHOD(New, TYP, Std$Address$T) {
//@buffer
//:T
// Returns a new reader starting at <code>buffer</code> with length <code>buffer:length</code>.
	reader_t *Reader = new(reader_t);
	Reader->Type = T;
	Reader->Address = Std$Address$get_value(Args[0].Val);
	Reader->Length = Std$Address$get_size(Args[0].Val);
	Result->Val = (Std$Object$t *)Reader;
	return SUCCESS;
}

AMETHOD(New, TYP, Std$Address$T) {
//@address
//@length
//:T
// Returns a new reader starting at <code>address</code> with length <code>length</code>.
	reader_t *Reader = new(reader_t);
	Reader->Type = T;
	Reader->Address = Std$Address$get_value(Args[0].Val);
	Reader->Length = Std$Address$get_length(Args[0].Val);
	Result->Val = (Std$Object$t *)Reader;
	return SUCCESS;
}
