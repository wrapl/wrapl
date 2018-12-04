#ifndef UTIL_ENUM_H
#define UTIL_ENUM_H

#include <Std/Object.h>
#include <Riva/Module.h>

#define RIVA_MODULE Util$Enum
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	int Value, Count;
	const char *Names[];
};

RIVA_TYPE(T);

#define ENUM_TYPE(Type) TYPE(Type, Util$Enum$T);

#undef RIVA_MODULE

#endif
