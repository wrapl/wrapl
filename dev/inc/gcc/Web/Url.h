#ifndef WEB_URL_H
#define WEB_URL_H

#define RIVA_MODULE Web$Url
#include <Riva-Header.h>

RIVA_CFUN(Std$Object$t *, decode, const char *, size_t);
RIVA_CFUN(const char *, encode, Std$Object$t *);

#undef RIVA_MODULE

#endif
