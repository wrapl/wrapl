#include <Std.h>
#include <Riva/Memory.h>
#include <jwt.h>

typedef struct riva_jwt_t {
	const Std$Type$t *Type;
	jwt_t *Handle;
} riva_jwt_t;

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	riva_jwt_t *Jwt = new(riva_jwt_t);
	Jwt->Type = T;
	jwt_new(&Jwt->Handle);
	RETURN(Jwt);
}

GLOBAL_FUNCTION(Decode, 2) {
	CHECK_ARG_TYPE(0, Std$String$T);
	const char *Token = Std$String$flatten(Args[0].Val);
	const unsigned char *Key = NULL;
	int KeyLen = 0;
	if (Args[1].Val->Type == Std$String$T) {
		Key = Std$String$flatten(Args[1].Val);
		KeyLen = Std$String$get_length(Args[1].Val);
	} else if (Args[1].Val->Type == Std$Address$SizedT) {
		Key = Std$Address$get_value(Args[1].Val);
		KeyLen = Std$Address$get_size(Args[1].Val);
	} else if (Args[1].Val->Type == Std$Address$T) {
		CHECK_ARG_COUNT(3);
		CHECK_ARG_TYPE(2, Std$Integer$SmallT);
		Key = Std$Address$get_value(Args[1].Val);
		KeyLen = Std$Integer$get_small(Args[2].Val);
	}
	riva_jwt_t *Jwt = new(riva_jwt_t);
	Jwt->Type = T;
	if (jwt_decode(&Jwt->Handle, Token, Key, KeyLen)) {
		SEND(Std$String$new("Error decoding JWT"));
	}
	RETURN(Jwt);
}

METHOD("dup", TYP, T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	riva_jwt_t *Jwt2 = new(riva_jwt_t);
	Jwt2->Type = T;
	Jwt2->Handle = jwt_dup(Jwt->Handle);
	RETURN(Jwt2);
}

METHOD("add_grant", TYP, T, TYP, Std$String$T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	const char *Val = Std$String$flatten(Args[2].Val);
	jwt_add_grant(Jwt->Handle, Grant, Val);
	RETURN0;
}
