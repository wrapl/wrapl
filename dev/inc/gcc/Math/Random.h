#ifndef MATH_RANDOM_H
#define MATH_RANDOM_H

#define RIVA_MODULE Math$Random
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);

RIVA_CFUN(double, uniform01, Std$Object_t *);

#undef RIVA_MODULE

#endif
