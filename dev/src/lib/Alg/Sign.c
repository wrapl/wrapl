#include <Std.h>
#include <Riva.h>
#include <Alg/Digest.h>
#include <nettle/rsa.h>

typedef struct rsa_public_t {
	const Std$Type$t *Type;
	struct rsa_public_key Key[1];
} rsa_public_t;

typedef struct rsa_private_t {
	const Std$Type$t *Type;
	struct rsa_private_key Key[1];
} rsa_private_t;

TYPE(RsaPublicT);
TYPE(RsaPrivateT);

GLOBAL_FUNCTION(RsaPublicNew, 1) {
	rsa_public_t *RsaPublic = new(rsa_public_t);
	RsaPublic->Type = RsaPublicT;
	rsa_public_key_init(RsaPublic->Key);
	Result->Val = (Std$Object$t *)RsaPublic;
	return SUCCESS;
};

GLOBAL_FUNCTION(RsaPrivateNew, 1) {
	rsa_private_t *RsaPrivate = new(rsa_private_t);
	RsaPrivate->Type = RsaPublicT;
	rsa_private_key_init(RsaPrivate->Key);
	Result->Val = (Std$Object$t *)RsaPrivate;
	return SUCCESS;
};


