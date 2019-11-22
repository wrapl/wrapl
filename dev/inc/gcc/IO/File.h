#ifndef IO_FILE_H
#define IO_FILE_H

#include <IO/Native.h>

#define RIVA_MODULE IO$File
#include <Riva-Header.h>

typedef enum {
	IO$File$OPEN_READ		= 1,
	IO$File$OPEN_WRITE		= 2,
	IO$File$OPEN_TEXT		= 4,
	IO$File$OPEN_APPEND		= 8,
	IO$File$OPEN_NOBLOCK	= 16,
	IO$File$OPEN_EXCLUSIVE	= 32,
	IO$File$OPEN_TRUNCATE	= 64
} IO$File_openflag;

RIVA_CFUN(IO$Native$(t) *, open, const char *, IO$File_openflag);

#undef RIVA_MODULE

#endif
