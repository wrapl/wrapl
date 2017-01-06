#ifndef SYS_COEXPR_H
#define SYS_COEXPR_H

#include <Std/Object.h>

#define RIVA_MODULE Sys$Coexpr
#include <Riva-Header.h>

RIVA_TYPE(T);

RIVA_OBJECT(New);
RIVA_OBJECT(Yield);
RIVA_OBJECT(Self);

#undef RIVA_MODULE

#endif
