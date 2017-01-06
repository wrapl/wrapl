#ifndef IO_ENCODER_H
#define IO_ENCODER_H

#include <Std/Object.h>
#include <IO/Stream.h>

#define RIVA_MODULE IO$Encoder
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);

RIVA_CFUN(IO$Encoder$t *, new, IO$Stream$t *);
RIVA_CFUN(void, write, IO$Encoder$t *, Std$Object$t *);

#undef RIVA_MODULE

#endif
