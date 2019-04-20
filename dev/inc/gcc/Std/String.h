#ifndef STD_STRING_H
#define STD_STRING_H

#include <Std/Integer.h>
#include <Std/Address.h>

#define RIVA_MODULE Std$String
#include <Riva-Header.h>

#define Std$String$MaxBlockSize 32704

RIVA_STRUCT(block) {
	Std$Integer_smallt Length;
	Std$Address_constt Chars;
};

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	unsigned long Count;
	Std$Integer_smallt Length;
	Std$Integer_smallt Hash;
	Std$String_block Blocks[];
};

#define STRING(NAME, VALUE)\
	static const char __chars ## NAME[] = VALUE;\
	extern const Std$Object_t NAME[];\
	asm(".globl " __stringify(NAME));\
	const struct {\
		const Std$Type_t *Type;\
		unsigned long Count;\
		Std$Integer_smallt Length;\
		Std$Integer_smallt Hash;\
		const Std$String_block Blocks[2];\
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
RIVA_OBJECT(From);

RIVA_CFUN(const char *, flatten, Std$Object_t *) __attribute__ ((pure));
RIVA_CFUN(void, flatten_to, Std$Object_t *, char *);
RIVA_CFUN(const char *, content, Std$Object_t *) __attribute__ ((pure));
RIVA_CFUN(void, content_to, Std$Object_t *, char *);

RIVA_CFUN(Std$Object_t *, new, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, copy, const char *) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_length, const char *, long) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, copy_length, const char *, long) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_format, const char *, ...) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_char, char) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, add, Std$Object_t *, Std$Object_t *) __attribute__ ((const));
RIVA_CFUN(Std$Object_t *, add_chars, Std$Object_t *, const char *, int) __attribute__ ((const));
RIVA_CFUN(Std$Object_t *, add_format, Std$Object_t *, const char *, ...) __attribute__ ((const));
RIVA_CFUN(Std$Object_t *, slice, Std$Object_t *, int, int) __attribute__ ((const));

RIVA_CFUN(Std$String_t *, alloc, int) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, freeze, Std$String_t *);

#define Std$String$get_length(A) ((Std$String_t *)A)->Length.Value
#define Std$String$get_block_length(A, N) ((Std$String_t *)A)->Blocks[N].Length.Value
#define Std$String$get_block_chars(A, N) (char *)((Std$String_t *)A)->Blocks[N].Chars.Value
#define Std$String$get_char(A) *((char *)((Std$String_t *)A)->Blocks[0].Chars.Value)

#undef RIVA_MODULE

#endif
