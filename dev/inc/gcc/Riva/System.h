#ifndef RIVA_SYSTEM_H
#define RIVA_SYSTEM_H

#define RIVA_MODULE Riva$System
#include <Riva-Header.h>

extern const char **Riva$System$_Args;
extern const unsigned int Riva$System$_NoOfArgs;
RIVA_CFUN(int, get_errno);

#undef RIVA_MODULE

#endif
