#include <Std.h>
#include <Riva/Memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern Std$Type_t T[];
extern Std$Object_t *_new_length(const char *Chars, int Length);

Std$Object_t *_new_format(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	va_end(Args);
	return _new_length(Chars, Length);
};

#ifdef DOCUMENTING

#define Std$String$T T
#define STRING_METHOD METHOD

PUSHFILE("Methods2.c");
#include "Methods2.c"
POPFILE();

#endif
