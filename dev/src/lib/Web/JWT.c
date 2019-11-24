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
	if (Count == 0) {
		jwt_new(&Jwt->Handle);
	} else {
		CHECK_ARG_TYPE(0, Std$String$T);
		const char *Token = Std$String$flatten(Args[0].Val);
		const unsigned char *Key = NULL;
		int KeyLen = 0;
		if (Count == 1) {
		} else if (Args[1].Val->Type == Std$String$T) {
			Key = Std$String$flatten(Args[1].Val);
			KeyLen = Std$String$get_length(Args[1].Val);
		} else if (Args[1].Val->Type == Std$Address$T) {
			Key = Std$Address$get_value(Args[1].Val);
			KeyLen = Std$Address$get_size(Args[1].Val);
		}
		if (jwt_decode(&Jwt->Handle, Token, Key, KeyLen)) {
			SEND(Std$String$new("Error decoding JWT"));
		}
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

METHOD("add_grant", TYP, T, TYP, Std$String$T, VAL, $true) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_grant_bool(Jwt->Handle, Grant, 1);
	RETURN0;
}

METHOD("add_grant", TYP, T, TYP, Std$String$T, VAL, $false) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_grant_bool(Jwt->Handle, Grant, 0);
	RETURN0;
}

METHOD("add_grant", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_grant_int(Jwt->Handle, Grant, Val);
	RETURN0;
}

METHOD("add_grants", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Json = Std$String$flatten(Args[1].Val);
	jwt_add_grants_json(Jwt->Handle, Json);
	RETURN0;
}

METHOD("del_grant", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	jwt_del_grant(Jwt->Handle, Grant);
	RETURN0;
}

METHOD("del_grants", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	jwt_del_grants(Jwt->Handle, Grant);
	RETURN0;
}

METHOD("get_grant", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	RETURN(Std$String$new(jwt_get_grant(Jwt->Handle, Grant)));
}

METHOD("get_grant_bool", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	RETURN(jwt_get_grant_bool(Jwt->Handle, Grant) ? $true : $false);
}

METHOD("get_grant_int", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	RETURN(Std$Integer$get_small(jwt_get_grant_int(Jwt->Handle, Grant)));
}

METHOD("get_grants", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Grant = Std$String$flatten(Args[1].Val);
	RETURN(Std$String$new(jwt_get_grants_json(Jwt->Handle, Grant)));
}

METHOD("add_header", TYP, T, TYP, Std$String$T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	const char *Val = Std$String$flatten(Args[2].Val);
	jwt_add_header(Jwt->Handle, Header, Val);
	RETURN0;
}

METHOD("add_header", TYP, T, TYP, Std$String$T, VAL, $true) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_header_bool(Jwt->Handle, Header, 1);
	RETURN0;
}

METHOD("add_header", TYP, T, TYP, Std$String$T, VAL, $false) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_header_bool(Jwt->Handle, Header, 0);
	RETURN0;
}

METHOD("add_header", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	int Val = Std$Integer$get_small(Args[2].Val);
	jwt_add_header_int(Jwt->Handle, Header, Val);
	RETURN0;
}

METHOD("add_headers", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Json = Std$String$flatten(Args[1].Val);
	jwt_add_headers_json(Jwt->Handle, Json);
	RETURN0;
}

METHOD("del_headers", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	jwt_del_grants(Jwt->Handle, Header);
	RETURN0;
}

METHOD("get_header", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	RETURN(Std$String$new(jwt_get_grant(Jwt->Handle, Header)));
}

METHOD("get_header_bool", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	RETURN(jwt_get_grant_bool(Jwt->Handle, Header) ? $true : $false);
}

METHOD("get_header_int", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	RETURN(Std$Integer$get_small(jwt_get_grant_int(Jwt->Handle, Header)));
}

METHOD("get_headers", TYP, T, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	RETURN(Std$String$new(jwt_get_grants_json(Jwt->Handle, Header)));
}

METHOD("encode", TYP, T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	RETURN(Std$String$new(jwt_encode_str(Jwt->Handle)));
}

AMETHOD(Std$String$Of, TYP, T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	RETURN(Std$String$new(jwt_dump_str(Jwt->Handle, 0)));
}

AMETHOD(Std$String$Of, TYP, T, ANY) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	RETURN(Std$String$new(jwt_dump_str(Jwt->Handle, 1)));
}

TYPE(AlgT);

Std$Integer$smallt AlgNone[] = {{AlgT, JWT_ALG_NONE}};
Std$Integer$smallt AlgHS256[] = {{AlgT, JWT_ALG_HS256}};
Std$Integer$smallt AlgHS384[] = {{AlgT, JWT_ALG_HS384}};
Std$Integer$smallt AlgHS512[] = {{AlgT, JWT_ALG_HS512}};
Std$Integer$smallt AlgRS256[] = {{AlgT, JWT_ALG_RS256}};
Std$Integer$smallt AlgRS384[] = {{AlgT, JWT_ALG_RS384}};
Std$Integer$smallt AlgRS512[] = {{AlgT, JWT_ALG_RS512}};
Std$Integer$smallt AlgES256[] = {{AlgT, JWT_ALG_ES256}};
Std$Integer$smallt AlgES384[] = {{AlgT, JWT_ALG_ES384}};
Std$Integer$smallt AlgES512[] = {{AlgT, JWT_ALG_ES512}};
Std$Integer$smallt AlgTerm[] = {{AlgT, JWT_ALG_TERM}};

METHOD("set_alg", TYP, T, TYP, AlgT, TYP, Std$String$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	jwt_alg_t Alg = Std$Integer$get_small(Args[1].Val);
	const char *Key = Std$String$flatten(Args[2].Val);
	int Len = Std$String$get_length(Args[2].Val);
	jwt_set_alg(Jwt->Handle, Alg, Key, Len);
	RETURN0;
}

METHOD("set_alg", TYP, T, TYP, AlgT, TYP, Std$Address$T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	jwt_alg_t Alg = Std$Integer$get_small(Args[1].Val);
	const char *Key = Std$Address$get_value(Args[2].Val);
	int Len = Std$Address$get_size(Args[2].Val);
	jwt_set_alg(Jwt->Handle, Alg, Key, Len);
	RETURN0;
}

METHOD("get_alg", TYP, T) {
	riva_jwt_t *Jwt = (riva_jwt_t *)Args[0].Val;
	switch (jwt_get_alg(Jwt->Handle)) {
	case JWT_ALG_NONE: RETURN(AlgNone);
	case JWT_ALG_HS256: RETURN(AlgHS256);
	case JWT_ALG_HS384: RETURN(AlgHS384);
	case JWT_ALG_HS512: RETURN(AlgHS512);
	case JWT_ALG_RS256: RETURN(AlgRS256);
	case JWT_ALG_RS384: RETURN(AlgRS384);
	case JWT_ALG_RS512: RETURN(AlgRS512);
	case JWT_ALG_ES256: RETURN(AlgES256);
	case JWT_ALG_ES384: RETURN(AlgES384);
	case JWT_ALG_ES512: RETURN(AlgES512);
	case JWT_ALG_TERM: RETURN(AlgTerm);
	}
	SEND(Std$String$new("Invalid algorithm"));
}
