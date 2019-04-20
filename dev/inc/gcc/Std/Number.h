#ifndef STD_NUMBER_H
#define STD_NUMBER_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Number
#include <Riva-Header.h>

RIVA_TYPE(T);

RIVA_OBJECT(From);

RIVA_CFUN(int, is0, Std$Object_t *);
RIVA_CFUN(int, is1, Std$Object_t *);

#undef RIVA_MODULE

#endif
