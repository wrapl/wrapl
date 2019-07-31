#ifndef STD_NUMBER_H
#define STD_NUMBER_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Number
#include <Riva-Header.h>

RIVA_TYPE(T);

RIVA_OBJECT(Of);

RIVA_CFUN(int, is0, Std$Object$t *);
RIVA_CFUN(int, is1, Std$Object$t *);

#undef RIVA_MODULE

#endif
