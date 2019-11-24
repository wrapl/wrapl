#include <Std.h>
#include <Riva.h>
#include <nettle/hmac.h>

#define HMAC(Name, name, DefaultLength)\
typedef struct name ## _t {\
	const Std$Type$t *Type;\
	struct hmac_ ## name ## _ctx Context[1];\
} name ## _t;\
\
TYPE(Name ## T);\
\
GLOBAL_FUNCTION(Name ## New, 1) {\
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);\
	name ## _t *HMAC = new(name ## _t);\
	HMAC->Type = Name ## T;\
	hmac_ ## name ## _set_key(HMAC->Context,\
		Std$String$get_length(Args[0].Val),\
		Std$String$flatten(Args[0].Val)\
	);\
	RETURN(HMAC);\
}\
\
METHOD("update", TYP, Name ## T, TYP, Std$String$T) {\
	name ## _t *HMAC = Args[0].Val;\
	Std$String$t *String = Args[1].Val;\
	for (Std$Address$t *Block = String->Blocks; Block->Length.Value; ++Block) {\
		hmac_ ## name ## _update(HMAC->Context, Block->Length.Value, Block->Value);\
	};\
	RETURN0;\
}\
\
METHOD("update", TYP, Name ## T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {\
	name ## _t *HMAC = Args[0].Val;\
	hmac_ ## name ## _update(HMAC->Context,\
		Std$Integer$get_small(Args[2].Val),\
		Std$Address$get_value(Args[1].Val)\
	);\
	RETURN0;\
}\
\
METHOD("digest", TYP, Name ## T) {\
	name ## _t *HMAC = Args[0].Val;\
	char *Buffer = Riva$Memory$alloc_atomic(DefaultLength);\
	hmac_ ## name ## _digest(HMAC->Context, DefaultLength, Buffer);\
	RETURN(Std$String$new_length(Buffer, DefaultLength));\
}\
\
METHOD("digest", TYP, Name ## T, TYP, Std$Integer$SmallT) {\
	name ## _t *HMAC = Args[0].Val;\
	int Length = Std$Integer$get_small(Args[1].Val);\
	char *Buffer = Riva$Memory$alloc_atomic(Length);\
	hmac_ ## name ## _digest(HMAC->Context, Length, Buffer);\
	RETURN(Std$String$new_length(Buffer, Length));\
}

HMAC(MD5, md5, 16);
HMAC(Ripemd160, ripemd160, 20);
HMAC(SHA1, sha1, 20);
HMAC(SHA224, sha224, 28);
HMAC(SHA256, sha256, 32);
HMAC(SHA384, sha384, 48);
HMAC(SHA512, sha512, 64);
