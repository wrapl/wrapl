#ifndef IO_POLL_H
#define IO_POLL_H

#define RIVA_MODULE IO$Poll
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);

RIVA_CFUN(int, add, IO$Poll$t *, int, int);
RIVA_CFUN(int, mod, IO$Poll$t *, int, int);
RIVA_CFUN(int, del, IO$Poll$t *, int);

#undef RIVA_MODULE

#endif

