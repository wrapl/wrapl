#ifndef RIVA_HEADER_H
#define RIVA_HEADER_H

#ifdef	__cplusplus
#	define __EXTERN	extern "C"
#else
#	define __EXTERN	extern
#endif

#define __concat2(A, B) A ## B
#define __concat(A, B) __concat2(A, B)
#define __stringify2(A) #A
#define __stringify(A) __stringify2(A)

#ifdef WINDOWS
#define __asmify(A) asm(__stringify(__concat(_, A)))
#else
#define __asmify(A) asm(__stringify(A))
#endif

#define RIVA_CFUN(TYPE, FUNC, ARGS ...)\
	__EXTERN TYPE __concat(RIVA_MODULE, $ ## FUNC) ( ARGS ) __asmify(__concat(RIVA_MODULE, __concat($_, FUNC)))

#define RIVA_TYPE(NAME)\
	__EXTERN const Std$Type_t __concat(RIVA_MODULE, $ ## NAME)[];

#define RIVA_OBJECT(NAME)\
	__EXTERN Std$Object_t __concat(RIVA_MODULE, $ ## NAME)[];

#define RIVA_STRUCT(NAME)\
	typedef struct __concat(RIVA_MODULE, $ ## NAME) __concat(RIVA_MODULE, $ ## NAME);\
	typedef __concat(RIVA_MODULE, $ ## NAME) __concat(RIVA_MODULE, _ ## NAME);\
	struct __concat(RIVA_MODULE, $ ## NAME)

#endif
