#ifndef STD_OBJECT_H
#define STD_OBJECT_H

#include <Std/Type.h>

#define RIVA_MODULE Std$Object
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
};

RIVA_TYPE(T);

RIVA_OBJECT(Create);

RIVA_OBJECT(Nil);
RIVA_OBJECT(Less);
RIVA_OBJECT(Equal);
RIVA_OBJECT(Greater);
RIVA_OBJECT(Hash);
RIVA_OBJECT(Compare);

RIVA_CFUN(int, in, const Std$Object_t *, const Std$Type_t *);

#define INITIAL(ARG)\
	static void __concat(__init_fn_, __LINE__)(ARG);\
	void (*__concat(__init_, __LINE__))(ARG) __attribute__ ((section (".initial"))) = __concat(__init_fn_, __LINE__);\
	static void __concat(__init_fn_, __LINE__)(ARG)

#ifdef DOCUMENTING

#define CONSTANT(ARGS...) CONSTANT(__LINE__, ARGS)
#define GLOBAL(TYPE, CTYPE, NAME) GLOBAL(__LINE__, NAME, TYPE)
#define MODULE(ARGS...) MODULE(__LINE__, ARGS)
#define SUBMODULE(ARGS...) SUBMODULE(__LINE__, ARGS)

#else

#define CONSTANT(NAME, TYPE)\
	static Std$Object_t *__init_ ## NAME(void);\
	Std$Object_t NAME[] __attribute__ ((section (".constants"))) = {{(Std$Type_t *)&__init_ ## NAME}};\
	static Std$Object_t *__init_ ## NAME(void)

#define GLOBAL(TYPE, CTYPE, NAME) CTYPE NAME

#define MODULE(PATH...)

#define SUBMODULE(NAME...)

#endif

#undef RIVA_MODULE

#endif
