#include <Std.h>
#include <Riva/Memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern Std$Type$t T[];
extern Std$Object$t *_new_length(const char *Chars, int Length);
extern Std$Object$t *_add(Std$Object$t *A, Std$String$t *B);

ASYMBOL(From);

Std$Object_t *_new_format(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	va_end(Args);
	return _new_length(Chars, Length);
};

Std$Object$t *_add_chars(Std$Object$t *String, const char *Chars, int Length) {
	return _add(String, _new_length(Chars, Length));
}

Std$Object$t *_add_format(Std$Object$t *String, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	va_end(Args);
	return _add_chars(String, Chars, Length);
}

#ifdef DOCUMENTING

#define Std$String$T T
#define STRING_METHOD METHOD

PUSHFILE("Methods2.c");
#include "Methods2.c"
POPFILE();

#endif
