#ifndef UTIL_TYPED_FUNCTION_H
#define UTIL_TYPED_FUNCTION_H

#include <stdint.h>

#define RIVA_MODULE Util$TypedFunction
#include <Riva-Header.h>

RIVA_CFUN(void *, get, void *, const Std$Type$t *);
RIVA_CFUN(void, set, void *, const Std$Type$t *, void *);

RIVA_STRUCT(t) {
	uint8_t _Part1[34];
	void *_Entries;
	uint8_t _Part2[8];
	void *_Entries2;
	uint8_t _Part3[6];
	uint32_t _Part4;
	uint32_t _Part5;
	void *_Part6[6];
}  __attribute__ ((packed));

#ifdef DOCUMENTING

#define TYPED_FUNCTION(ARGS...) TYPED_FUNCTION(__LINE__, ARGS)
#define TYPED_INSTANCE(ARGS...) TYPED_INSTANCE(__LINE__, ARGS)

#else

#define TYPED_FUNCTION(RETURN, NAME, PARAMS...)\
	extern RETURN NAME(PARAMS) __asmify(NAME);\
	static RETURN __concat(__invoke, __LINE__)(PARAMS);\
	void *__concat(__initial, __LINE__)[8] = {0, 0, 0, (void *)&__concat(__invoke, __LINE__), 0, 0, 0, 0};\
	Util$TypedFunction$t __concat(__typedfn, __LINE__) __asmify(NAME) = {\
		{0x53, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x18, 0x8B, 0x5B, 0x04,\
		 0x8B, 0x0B, 0x83, 0xC3, 0x04, 0x89, 0xCA, 0x89, 0xC8, 0xC1,\
		 0xCA, 0x04, 0x8D, 0x44, 0x50, 0x01, 0x25, 0x01, 0x00, 0x00,\
		 0x00, 0x39, 0x0C, 0xC5},\
		&__concat(__initial, __LINE__)[0],\
		{0x72, 0xE2, 0x77, 0xEC, 0x5B, 0xFF, 0x24, 0xC5},\
		&__concat(__initial, __LINE__)[1],\
		{0x90, 0x90, 0x01, 0x00, 0x00, 0x00},\
		0\
	};\
	static RETURN __concat(__invoke, __LINE__)(PARAMS)

#define TYPED_INSTANCE(RETURN, FUNCTION, TYPE, PARAMS...)\
	static RETURN __concat(__invoke, __LINE__)(PARAMS);\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".typedfn"))) = {\
		(void *)FUNCTION, TYPE, (void *)&__concat(__invoke, __LINE__)\
	};\
	static RETURN __concat(__invoke, __LINE__)(PARAMS)

#endif

#undef RIVA_MODULE

#endif
