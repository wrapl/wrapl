#ifndef UTIL_TYPED_FUNCTION_H
#define UTIL_TYPED_FUNCTION_H

#include <stdint.h>

#define RIVA_MODULE Util$TypedFunction
#include <Riva-Header.h>

RIVA_CFUN(void *, get, void *, const Std$Type_t *);
RIVA_CFUN(void, set, void *, const Std$Type_t *, void *);

#ifdef X86

RIVA_STRUCT(t) {
	uint8_t _Part1[34];
	void *_Entries;
	uint8_t _Part2[8];
	void *_Entries2;
	uint8_t _Part3[6];
	void *_Entries3;
}  __attribute__ ((packed));

#endif

#ifdef X64

RIVA_STRUCT(t) {
	uint8_t _Part1[10];
	void *_Entries;
	uint8_t _Part2[46];
	void *_Entries2;
	uint32_t Space;
}  __attribute__ ((packed));

#endif

#ifdef DOCUMENTING

#define TYPED_FUNCTION(ARGS...) TYPED_FUNCTION(__LINE__, ARGS)
#define TYPED_INSTANCE(ARGS...) TYPED_INSTANCE(__LINE__, ARGS)

#else

#ifdef X86

#define TYPED_FUNCTION(RETURN, NAME, PARAMS...)\
	extern RETURN NAME(PARAMS) __asmify(NAME);\
	static RETURN __concat(__invoke, __LINE__)(PARAMS);\
	void *__concat(__initial, __LINE__)[8] = {0, 0, 0, (void *)&__concat(__invoke, __LINE__), 0, 0, 0, 0};\
	Util$TypedFunction_t __concat(__typedfn, __LINE__) __asmify(NAME) = {\
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

#endif

#ifdef X64

#define TYPED_FUNCTION(RETURN, NAME, PARAMS...)\
	extern RETURN NAME(PARAMS) __asmify(NAME);\
	static RETURN __concat(__invoke, __LINE__)(PARAMS);\
	void *__concat(__initial, __LINE__)[8] = {0, 0, 0, (void *)&__concat(__invoke, __LINE__), 0, 0, 0, 0};\
	Util$TypedFunction_t __concat(__typedfn, __LINE__) __asmify(NAME) = {\
		{0x4C, 0x8B, 0x44, 0x24, 0x08, 0x4D, 0x8B, 0x08, 0x48, 0xB9},\
		&__concat(__initial, __LINE__)[0],\
		{0x4D, 0x8B, 0x49, 0x08, 0x4D, 0x8B, 0x11, 0x49, 0x83, 0xC1,\
		 0x08, 0x4D, 0x89, 0xD3, 0x4C, 0x89, 0xD0, 0x49, 0xC1, 0xCB,\
		 0x08, 0x4A, 0x8D, 0x44, 0xD8, 0x02, 0x48, 0x25, 0x02, 0x00,\
		 0x00, 0x00, 0x4C, 0x39, 0x14, 0xC1, 0x72, 0xDE, 0x77, 0xED,\
		 0xFF, 0x64, 0xC1, 0x04, 0x90, 0x90},\
		0, 1\
	};\
	static RETURN __concat(__invoke, __LINE__)(PARAMS)

#endif

#define TYPED_INSTANCE(RETURN, FUNCTION, TYPE, PARAMS...)\
	static RETURN __concat(__invoke, __LINE__)(PARAMS);\
	const void *__concat(__instance, __LINE__)[] __attribute__ ((section (".typedfn"))) = {\
		(void *)FUNCTION, TYPE, (void *)&__concat(__invoke, __LINE__)\
	};\
	static RETURN __concat(__invoke, __LINE__)(PARAMS)

#endif

#undef RIVA_MODULE

#endif
