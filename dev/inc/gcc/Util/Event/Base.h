#ifndef UTIL_EVENT_BASE_H
#define UTIL_EVENT_BASE_H

#include <Std/Object.h>

#define RIVA_MODULE Util$Event$Base
#include <Riva-Header.h>

#include <event2/event.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	struct event_base *Handle;
};

RIVA_TYPE(T);

#undef RIVA_MODULE

#endif
