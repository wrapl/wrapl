#ifndef UTIL_BASE64_H
#define UTIL_BASE64_H

#include <stdint.h>

#define RIVA_MODULE Util$Base64
#include <Riva-Header.h>

RIVA_CFUN(void, encode, char *, const char *, size_t);
RIVA_CFUN(size_t, decode, char *, const char *, size_t);

#undef RIVA_MODULE

#endif
