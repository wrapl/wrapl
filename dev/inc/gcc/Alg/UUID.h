#ifndef ALG_UUID_H
#define ALG_UUID_H

#include <uuid/uuid.h>

#define RIVA_MODULE Alg$UUID
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	uuid_t Value;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
