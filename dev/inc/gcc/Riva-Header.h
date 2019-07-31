#ifndef RIVA_HEADER_H
#define RIVA_HEADER_H

#ifdef	__cplusplus
#	define __EXTERN	extern "C"
#else
#	define __EXTERN	extern
#endif

#define ___concat(A, B) A ## B
#define __concat(A, B) ___concat(A, B)

#define ___concat3(A, B, C) A ## B ## C
#define __concat3(A, B, C) ___concat3(A, B, C)

#define ___stringify(A) #A
#define __stringify(A) ___stringify(A)

#ifdef WINDOWS
#define __asmify(A) asm(__stringify(__concat(_, A)))
#else
#define __asmify(A) asm(__stringify(A))
#endif

#define RIVA_CFUN(TYPE, FUNC, ARGS ...)\
	__EXTERN TYPE __concat(RIVA_MODULE, $ ## FUNC) ( ARGS ) __asmify(__concat(RIVA_MODULE, __concat($_, FUNC)))

#define RIVA_TYPE(NAME)\
	__EXTERN const Std$Type$t __concat(RIVA_MODULE, $ ## NAME)[];

#define RIVA_OBJECT(NAME)\
	__EXTERN Std$Object$t __concat(RIVA_MODULE, $ ## NAME)[];

#define RIVA_STRUCT(NAME)\
	typedef struct __concat(RIVA_MODULE, $ ## NAME) __concat(RIVA_MODULE, $ ## NAME);\
	struct __concat(RIVA_MODULE, $ ## NAME)

#endif
