#ifndef IO_DECODER_H
#define IO_DECODER_H

#include <Std/Object.h>
#include <IO/Stream.h>

#define RIVA_MODULE IO$Decoder
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);

RIVA_CFUN(IO$Decoder$t *, new, IO$Stream$t *);

#undef RIVA_MODULE

#endif
