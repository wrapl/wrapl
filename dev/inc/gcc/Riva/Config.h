#ifndef RIVA_CONFIG_H
#define RIVA_CONFIG_H

#define RIVA_MODULE Riva$Config
#include <Riva-Header.h>

RIVA_CFUN(void *, get, const char *);
RIVA_CFUN(void, set, const char *, void *);

#undef RIVA_MODULE

#endif
