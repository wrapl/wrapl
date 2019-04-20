#include <Std.h>
#include <IO/Stream.h>
#include <Riva/Memory.h>
#include <uuid/uuid.h>

typedef struct _uuid_t {
	const Std$Type$t *Type;
	uuid_t Value;
} _uuid_t;

TYPE(T);

AMETHOD(Std$String$Of, TYP, T) {
	_uuid_t *UUID = (_uuid_t *)Args[0].Val;
	char *Buffer = Riva$Memory$alloc_atomic(37);
	uuid_unparse_lower(UUID->Value, Buffer);
	Result->Val = Std$String$new_length(Buffer, 36);
	return SUCCESS;
};

GLOBAL_METHOD(Hash, 1, "#", TYP, T) {
	_uuid_t *UUID = (_uuid_t *)Args[0].Val;
	int *Value = (int *)UUID->Value;
	int Hash = Value[0] ^ Value[1] ^ Value[2] ^ Value[3];
	Result->Val = Std$Integer$new_small(Hash);
	return SUCCESS;
};

GLOBAL_METHOD(Compare, 2, "?", TYP, T, TYP, T) {
	_uuid_t *A = (_uuid_t *)Args[0].Val;
	_uuid_t *B = (_uuid_t *)Args[1].Val;
	for (int I = 0; I < 16; ++I) {
		if (A->Value[I] < B->Value[I]) {
			Result->Val = Std$Object$Less;
			return SUCCESS;
		} else if (A->Value[I] > B->Value[I]) {
			Result->Val = Std$Object$Greater;
			return SUCCESS;
		};
	};
	Result->Val = Std$Object$Equal;
	return SUCCESS;
};

GLOBAL_FUNCTION(New, 0) {
	_uuid_t *UUID = new(_uuid_t);
	UUID->Type = T;
	if (Count > 0) {
		if (uuid_parse(Std$String$flatten(Args[0].Val), UUID->Value)) {
			Result->Val = Std$String$new("UUID parse error");
			return MESSAGE;
		};
	} else {
		uuid_generate(UUID->Value);
	};
	Result->Val = (Std$Object$t *)UUID;
	return SUCCESS;
};

GLOBAL_FUNCTION(Encode, 2) {
	CHECK_EXACT_ARG_TYPE(1, T);
	Std$Object$t *Stream = Args[0].Val;
	_uuid_t *UUID = (_uuid_t *)Args[1].Val;
	int Remaining = 16;
	char *Buffer = UUID->Value;
	while (Remaining) {
		int Written = IO$Stream$write(Stream, Buffer, Remaining, 0);
		if (Written < 0) {
			Result->Val = Std$String$new("Write Error");
			return MESSAGE;
		};
		Buffer += Written;
		Remaining -= Written;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Decode, 1) {
	Std$Object$t *Stream = Args[0].Val;
	_uuid_t *UUID = new(_uuid_t);
	UUID->Type = T;
	int Remaining = 16;
	char *Buffer = UUID->Value;
	while (Remaining) {
		int Read = IO$Stream$read(Stream, Buffer, Remaining, 0);
		if (Read < 0) {
			Result->Val = Std$String$new("Read Error");
			return MESSAGE;
		};
		Buffer += Read;
		Remaining -= Read;
	};
	Result->Val = (Std$Object$t *)UUID;
	return SUCCESS;
};

