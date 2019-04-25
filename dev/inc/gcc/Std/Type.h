#ifndef STD_TYPE_H
#define STD_TYPE_H

#define RIVA_MODULE Std$Type
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	const Std$Type_t **Types;
	void (*Invoke)(void);
	const struct Std$Array$t *Fields;
	const unsigned long *Levels;
};

RIVA_TYPE(T);

RIVA_CFUN(const Std$Type_t *, is, const Std$Type_t *, const Std$Type_t *);

#ifdef WINDOWS
extern void Std$Type$default_invoke(void) asm("_Std$Type$New.invoke");
#else
extern void Std$Type$default_invoke(void) asm("Std$Type$New.invoke");
#endif

#ifdef DOCUMENTING

#define TYPE(ARGS...) TYPE(__LINE__, ARGS)

#else

#define PP_NARG(...) \
	PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
	PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
	_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
	_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
	_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
	_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
	_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
	_61,_62,_63,N,...) N
#define PP_RSEQ_N() \
	63,62,61,60,                   \
	59,58,57,56,55,54,53,52,51,50, \
	49,48,47,46,45,44,43,42,41,40, \
	39,38,37,36,35,34,33,32,31,30, \
	29,28,27,26,25,24,23,22,21,20, \
	19,18,17,16,15,14,13,12,11,10, \
	9,8,7,6,5,4,3,2,1,0 

#define TYPE(NAME, PARENTS...)\
	extern const Std$Type_t NAME[];\
	static const Std$Type_t *NAME ## _parents[] = {NAME, ## PARENTS, 0};\
	static const Std$Array_t NAME ## _fields[] = {{\
		Std$Array$T, 0,\
		{Std$Integer$SmallT, 0}\
	}};\
	static unsigned long NAME ## _levels[] = __concat(LEVELS_, PP_NARG(NAME, ## PARENTS));\
	const Std$Type_t NAME[] = {{\
		Std$Type$T, \
		NAME ## _parents, \
		Std$Type$default_invoke, \
		NAME ## _fields, \
		NAME ## _levels \
	}}

#define __strip(FIELDS...) FIELDS

#define TYPEF(NAME, FIELDS, PARENTS...)\
	extern const Std$Type_t NAME[];\
	static const Std$Type_t *NAME ## _parents[] = {NAME, ## PARENTS, 0};\
	static Std$Object_t * const NAME ## _fields[] = {__strip FIELDS};\
	static const Std$Array_t NAME ## _fields_array[] = {{\
		Std$Array$T, (Std$Object_t **)NAME ## _fields,\
		{Std$Integer$SmallT, PP_NARG FIELDS}\
	}};\
	static unsigned long NAME ## _levels[] = __concat(LEVELS_, PP_NARG(NAME, ## PARENTS));\
	const Std$Type_t NAME[] = {{\
		Std$Type$T, \
		NAME ## _parents, \
		Std$Type$default_invoke, \
		NAME ## _fields_array, \
		NAME ## _levels \
	}}

#define LEVELS_1 {1, 0}
#define LEVELS_2 {2, 0, 1}
#define LEVELS_3 {3, 0, 1, 2}
#define LEVELS_4 {4, 0, 1, 2, 3}
#define LEVELS_5 {5, 0, 1, 2, 3, 4}
#define LEVELS_6 {6, 0, 1, 2, 3, 4, 5}
#define LEVELS_7 {7, 0, 1, 2, 3, 4, 5, 6}
#define LEVELS_8 {8, 0, 1, 2, 3, 4, 5, 6, 7}
#define LEVELS_9 {9, 0, 1, 2, 3, 4, 5, 6, 7, 8}
#define LEVELS_10 {10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
#define LEVELS_11 {11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
#define LEVELS_12 {12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}
#define LEVELS_13 {13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
#define LEVELS_14 {14, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}
#define LEVELS_15 {15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}
#define LEVELS_16 {16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
#define LEVELS_17 {17, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
#define LEVELS_18 {18, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17}
#define LEVELS_19 {19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}
#define LEVELS_20 {20, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}


#endif

#undef RIVA_MODULE

#endif
