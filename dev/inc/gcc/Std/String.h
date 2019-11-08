#ifndef STD_STRING_H
#define STD_STRING_H

#include <Std/Integer.h>
#include <Std/Address.h>

#define RIVA_MODULE Std$String
#include <Riva-Header.h>

#define Std$String$MaxBlockSize 32704

RIVA_STRUCT(block) {
	Std$Integer$smallt Length;
	Std$Address$t Chars;
};

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	unsigned long Count;
	Std$Integer$smallt Length;
	Std$Integer$smallt Hash;
	Std$String$block Blocks[];
};

#define STRING(NAME, VALUE)\
	static const char __chars ## NAME[] = VALUE;\
	extern const Std$Object$t NAME[];\
	asm(".globl " __stringify(NAME));\
	const struct {\
		const Std$Type$t *Type;\
		unsigned long Count;\
		Std$Integer$smallt Length;\
		Std$Integer$smallt Hash;\
		const Std$String$block Blocks[2];\
	} __string ## NAME[1] __asmify(NAME) __attribute__ ((used)) = {{\
		Std$String$T,\
		1,\
		{Std$Integer$SmallT, sizeof(__chars ## NAME) - 1},\
		{0, 0},\
		{\
			{{Std$Integer$SmallT, sizeof(__chars ## NAME) - 1}, {Std$Address$T, __chars ## NAME}},\
			{{0, 0}, {0, 0}}\
		}\
	}};

RIVA_TYPE(T);
RIVA_OBJECT(Add);
RIVA_OBJECT(Hash);
RIVA_OBJECT(Compare);
RIVA_OBJECT(Create);
RIVA_OBJECT(Nil);
RIVA_OBJECT(Empty);
RIVA_OBJECT(Of);

RIVA_CFUN(const char *, flatten, Std$Object$t *) __attribute__ ((pure));
RIVA_CFUN(void, flatten_to, Std$Object$t *, char *);
RIVA_CFUN(const char *, content, Std$Object$t *) __attribute__ ((pure));
RIVA_CFUN(void, content_to, Std$Object$t *, char *);

RIVA_CFUN(Std$Object$t *, new, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, copy, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_length, const char *, long) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, copy_length, const char *, long) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_format, const char *, ...) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_char, char) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, add, Std$Object$t *, Std$Object$t *) __attribute__ ((const));
RIVA_CFUN(Std$Object$t *, add_chars, Std$Object$t *, const char *, int) __attribute__ ((const));
RIVA_CFUN(Std$Object$t *, add_format, Std$Object$t *, const char *, ...) __attribute__ ((const));
RIVA_CFUN(Std$Object$t *, slice, Std$Object$t *, int, int) __attribute__ ((const));

RIVA_CFUN(Std$String$t *, alloc, int) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, freeze, Std$String$t *);

#define Std$String$blocks(A) ((Std$String$t *)(A))->Blocks
#define Std$String$get_length(A) ((Std$String$t *)(A))->Length.Value
#define Std$String$get_block_length(A, N) ((Std$String$t *)(A))->Blocks[N].Length.Value
#define Std$String$get_block_chars(A, N) (char *)((Std$String$t *)(A))->Blocks[N].Chars.Value
#define Std$String$get_char(A) *((char *)((Std$String$t *)(A))->Blocks[0].Chars.Value)

#undef RIVA_MODULE

#endif
