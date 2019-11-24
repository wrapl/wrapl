#include <Std.h>
#include <Riva.h>
#include <Alg/Digest.h>
#include <nettle/md5.h>
#include "ow-crypt.h"

typedef struct md5_t {
	Std$Type$t *Type;
	struct md5_ctx Context[1];
} md5_t;

TYPE(Md5T);

GLOBAL_FUNCTION(Md5New, 0) {
	md5_t *Digest = new(md5_t);
	Digest->Type = Md5T;
	md5_init(Digest->Context);
	Result->Val = Digest;
	return SUCCESS;
};

METHOD("update", TYP, Md5T, TYP, Std$String$T) {
	md5_t *Digest = Args[0].Val;
	Std$String$t *String = Args[1].Val;
	for (Std$Address$t *Block = String->Blocks; Block->Length.Value; Block++) {
		md5_update(Digest->Context, Block->Length.Value, Block->Value);
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("update", TYP, Md5T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	md5_t *Digest = Args[0].Val;
	Std$Address$t *Chars = Args[1].Val;
	Std$Integer$smallt *Length = Args[2].Val;
	md5_update(Digest->Context, Length->Value, Chars->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("digest", TYP, Md5T) {
	md5_t *Digest = Args[0].Val;
	char *Buffer = Riva$Memory$alloc_atomic(16);
	md5_digest(Digest->Context, 16, Buffer);
	Result->Val = Std$String$new_length(Buffer, 16);
	return SUCCESS;
};

METHOD("digest", TYP, Md5T, TYP, Std$Integer$SmallT) {
	md5_t *Digest = Args[0].Val;
	int Length = ((Std$Integer$smallt *)Args[1].Val)->Value;
	char *Buffer = Riva$Memory$alloc_atomic(Length);
	md5_digest(Digest->Context, Length, Buffer);
	Result->Val = Std$String$new_length(Buffer, Length);
	return SUCCESS;
};

GLOBAL_FUNCTION(GenSalt, 4) {
	const char *Prefix = Std$String$flatten(Args[0].Val);
	unsigned long IterationCount = Std$Integer$get_small(Args[1].Val);
	const char *Input = Std$String$flatten(Args[2].Val);
	int Size = Std$Integer$get_small(Args[3].Val);
	Result->Val = Std$String$new(crypt_gensalt_ra(Prefix, IterationCount, Input, Size));
	return SUCCESS;
};

GLOBAL_FUNCTION(Crypt, 2) {
	const char *Key = Std$String$flatten(Args[0].Val);
	const char *Settings = Std$String$flatten(Args[1].Val);
	void *Data = 0;
	int Size = 0;
	Result->Val = Std$String$new(crypt_ra(Key, Settings, &Data, &Size));
	return SUCCESS;
};
