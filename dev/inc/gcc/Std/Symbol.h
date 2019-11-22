#ifndef STD_SYMBOL_H
#define STD_SYMBOL_H

#include <Std/Type.h>
#include <Std/String.h>
#include <Std/Function.h>

#define RIVA_MODULE Std$Symbol
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type$t *Type;
	long Reserved[8];
	Std$String$t *Name;
};

RIVA_STRUCT(nomethodmessage) {
	const Std$Type$t *Type;
	const Std$Symbol$t *Symbol;
	int Count;
	char *Stack[12];
};

RIVA_TYPE(T);
RIVA_TYPE(NoMethodMessageT);

RIVA_OBJECT(Set);
RIVA_OBJECT(Get);
RIVA_OBJECT(Default);

RIVA_CFUN(Std$Object$t *, new, void) __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new_string, const char *) __attribute__ ((malloc));

#define Std$Symbol$get_name(A) ((Std$Symbol$t *)A)->Name

RIVA_STRUCT(array) {
	const Std$Type$t *Type;
	unsigned long Count;
	const Std$Symbol$t *Values[];
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
	extern Std$Object$t NAME[];\
	static const char __symbol ## NAME[] __asmify(NAME) __attribute__ ((section (".symbols"), used)) = VALUE;

#define ANY (void *)1
#define SKP (void *)1
#define VAL (void *)2
#define TYP (void *)3

#define _METHOD(TOKEN, SYMBOL, SIGNATURE...)\
	static const char __concat(__symbol, TOKEN)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	static FUNCTION_ATTRS Std$Function$status __concat(__invoke, TOKEN)(FUNCTION_PARAMS);\
	static const Std$Function$ct __concat(__method, TOKEN) = {Std$Function$CT, &__concat(__invoke, TOKEN)};\
	const void *__concat(__instance, TOKEN)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, TOKEN), SIGNATURE, 0, &__concat(__method, TOKEN)\
	};\
	static FUNCTION_ATTRS Std$Function$status __concat(__invoke, TOKEN)(FUNCTION_PARAMS)

#define _GLOBAL_METHOD(TOKEN, NAME, COUNT, SYMBOL, SIGNATURE...)\
	static FUNCTION_ATTRS Std$Function$status invoke_ ## NAME(FUNCTION_PARAMS);\
	Std$Function$checkedct NAME[] = {{Std$Function$CheckedCT, invoke_ ## NAME, COUNT, __FILE__, __LINE__}};\
	static const char __concat(__symbol, TOKEN)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	static FUNCTION_ATTRS Std$Function$status __concat(__invoke, TOKEN)(FUNCTION_PARAMS);\
	static Std$Function$ct __concat(__method, TOKEN) = {Std$Function$CT, invoke_ ## NAME};\
	const void *__concat(__instance, TOKEN)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, TOKEN), SIGNATURE, 0, &__concat(__method, TOKEN)\
	};\
	static FUNCTION_ATTRS Std$Function$status invoke_ ## NAME(FUNCTION_PARAMS)

#define _ASYMBOL(TOKEN, NAME)\
	extern Std$Object$t NAME[];\
	char __symbol ## NAME[] __asmify(NAME) __attribute__ ((section (".asymbol"))) = "";

#define _AMETHOD(TOKEN, SYMBOL, SIGNATURE...)\
	static FUNCTION_ATTRS Std$Function$status __concat(__invoke, TOKEN)(FUNCTION_PARAMS);\
	static Std$Function$ct __concat(__method, TOKEN) = {Std$Function$CT, __concat(__invoke, TOKEN)};\
	const void *__concat(__instance, TOKEN)[] __attribute__ ((section (".methods"))) = {\
		SYMBOL, SIGNATURE, 0, &__concat(__method, TOKEN)\
	};\
	static FUNCTION_ATTRS Std$Function$status __concat(__invoke, TOKEN)(FUNCTION_PARAMS)

#define _SET_METHOD(TOKEN, SYMBOL, FUNCTION, SIGNATURE...)\
	static char __concat(__symbol, TOKEN)[] __attribute__ ((section (".symbols"))) = SYMBOL;\
	const void *__concat(__instance, TOKEN)[] __attribute__ ((section (".methods"))) = {\
		__concat(__symbol, TOKEN), SIGNATURE, 0, &FUNCTION\
	};

#define _SET_AMETHOD(TOKEN, SYMBOL, FUNCTION, SIGNATURE...)\
	const void *__concat(__instance, TOKEN)[] __attribute__ ((section (".methods"))) = {\
		SYMBOL, SIGNATURE, 0, &FUNCTION\
	};


#define METHOD(SYMBOL, SIGNATURE...)\
	_METHOD(__concat3(__LINE__, _, __COUNTER__), SYMBOL, SIGNATURE)

#define GLOBAL_METHOD(NAME, COUNT, SYMBOL, SIGNATURE...)\
	_GLOBAL_METHOD(__concat3(__LINE__, _, __COUNTER__), NAME, COUNT, SYMBOL, SIGNATURE)

#define ASYMBOL(NAME)\
	_ASYMBOL(__concat3(__LINE__, _, __COUNTER__), NAME)

#define AMETHOD(SYMBOL, SIGNATURE...)\
	_AMETHOD(__concat3(__LINE__, _, __COUNTER__), SYMBOL, SIGNATURE)

#define SET_METHOD(SYMBOL, FUNCTION, SIGNATURE...)\
	_SET_METHOD(__concat3(__LINE__, _, __COUNTER__), SYMBOL, FUNCTION, SIGNATURE)

#define SET_AMETHOD(SYMBOL, FUNCTION, SIGNATURE...)\
	_SET_AMETHOD(__concat3(__LINE__, _, __COUNTER__), SYMBOL, FUNCTION, SIGNATURE)

#endif

#undef RIVA_MODULE

#endif
