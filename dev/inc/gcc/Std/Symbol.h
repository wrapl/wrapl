#ifndef STD_SYMBOL_H
#define STD_SYMBOL_H

#include <Std/Type.h>
#include <Std/String.h>
#include <Std/Function.h>

#define RIVA_MODULE Std$Symbol
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	long Reserved[8];
	Std$String_t *Name;
};

RIVA_STRUCT(nomethodmessage) {
	const Std$Type_t *Type;
	const Std$Symbol_t *Symbol;
	int Count;
	char *Stack[12];
};

RIVA_TYPE(T);
RIVA_TYPE(NoMethodMessageT);

RIVA_OBJECT(Set);
RIVA_OBJECT(Get);
RIVA_OBJECT(Default);

RIVA_CFUN(Std$Object_t *, new, void) __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new_string, const char *) __attribute__ ((malloc));

#define Std$Symbol$get_name(A) ((Std$Symbol$t *)A)->Name

RIVA_STRUCT(array) {
	const Std$Type_t *Type;
	unsigned long Count;
	const Std$Symbol_t *Values[];
};

RIVA_TYPE(ArrayT);

#ifdef DOCUMENTING

#define SYMBOL(ARGS...) SYMBOL(__LINE__, ARGS)

#define METHOD(ARGS...) METHOD(__LINE__, ARGS)
#define GLOBAL_METHOD(ARGS...) GLOBAL_METHOD(__LINE__, ARGS)

#define ASYMBOL(NAME) GLOBAL(Std$Symbol$T, void *, NAME)
#define AMETHOD(ARGS...) AMETHOD(__LINE__, ARGS)

#define SET_METHOD(ARGS...) SET_METHOD(__LINE__, ARGS)

#else

#define SYMBOL(NAME, VALUE)\
	extern Std$Object_t NAME[];\
	static const char __symbol ## NAME[] __asmify(NAME) __attribute__ ((section (".symbols"), used)) = VALUE;

#define ANY (void *)1
#define SKP (void *)1
#define VAL (void *)2
#define TYP (void *)3

#define METHOD(SYMBOL, SIGNATURE...)\
	static const char __concat(__symbol, __LINE__)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	static FUNCTION_ATTRS Std$Function_status __concat(__invoke, __LINE__)(FUNCTION_PARAMS);\
	static const Std$Function_ct __concat(__method, __LINE__) = {Std$Function$CT, &__concat(__invoke, __LINE__)};\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, __LINE__), SIGNATURE, 0, &__concat(__method, __LINE__)\
	};\
	static FUNCTION_ATTRS Std$Function_status __concat(__invoke, __LINE__)(FUNCTION_PARAMS)

#define GLOBAL_METHOD(NAME, COUNT, SYMBOL, SIGNATURE...)\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS);\
	Std$Function_checkedct NAME[] = {{Std$Function$CheckedCT, invoke_ ## NAME, COUNT, __FILE__, __LINE__}};\
	static const char __concat(__symbol, __LINE__)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	static FUNCTION_ATTRS Std$Function_status __concat(__invoke, __LINE__)(FUNCTION_PARAMS);\
	static Std$Function_ct __concat(__method, __LINE__) = {Std$Function$CT, invoke_ ## NAME};\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, __LINE__), SIGNATURE, 0, &__concat(__method, __LINE__)\
	};\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS)

#define ASYMBOL(NAME)\
	extern Std$Object_t NAME[];\
	char __symbol ## NAME[] __asmify(NAME) __attribute__ ((section (".asymbol"))) = "";

#define AMETHOD(SYMBOL, SIGNATURE...)\
	static FUNCTION_ATTRS Std$Function_status __concat(__invoke, __LINE__)(FUNCTION_PARAMS);\
	static Std$Function_ct __concat(__method, __LINE__) = {Std$Function$CT, __concat(__invoke, __LINE__)};\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".methods"))) = {\
		SYMBOL, SIGNATURE, 0, &__concat(__method, __LINE__)\
	};\
	static FUNCTION_ATTRS Std$Function_status __concat(__invoke, __LINE__)(FUNCTION_PARAMS)

#define SET_METHOD(SYMBOL, FUNCTION, SIGNATURE...)\
	static char __concat(__symbol, __LINE__)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, __LINE__), SIGNATURE, 0, &FUNCTION\
	};

#define SET_AMETHOD(SYMBOL, FUNCTION, SIGNATURE...)\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".methods"))) = {\
		SYMBOL, SIGNATURE, 0, &FUNCTION\
	};

#endif

#undef RIVA_MODULE

#endif
