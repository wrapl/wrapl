#ifndef UTIL_EVENT_BUFFER_H
#define UTIL_EVENT_BUFFER_H

#include <Std/Object.h>

#define RIVA_MODULE Util$Event$Buffer
#include <Riva-Header.h>

#include <event2/buffer.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	struct evbuffer *Handle;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
