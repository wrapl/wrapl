#ifndef STD_FUNCTION_H
#define STD_FUNCTION_H

#include <Std/Object.h>

#define RIVA_MODULE Std$Function
#include <Riva-Header.h>

RIVA_STRUCT(argument) {
	Std$Object_t *Val;
	Std$Object_t **Ref;
};

RIVA_STRUCT(result) {
	union {
		struct {
			Std$Object_t *Val;
			Std$Object_t **Ref;
		};
		Std$Function_argument Arg;
	};
	void *State;
};

#define SUSPEND -1
#define SUCCESS 0
#define FAILURE 1
#define MESSAGE 2

typedef int Std$Function$status;

typedef Std$Function$status Std$Function_status;

typedef Std$Object_t Std$Function_t;
typedef Std$Object$t Std$Function$t;

#define FUNCTION_PARAMS const Std$Function$ct *Fun, unsigned long Count, const Std$Function$argument *Args, Std$Function$result *Result
#define FUNCTION_ATTRS __attribute__ ((force_align_arg_pointer))

RIVA_STRUCT(asmt) {
	const Std$Type_t *Type;
	void *Invoke;
};

RIVA_STRUCT(checkedasmt) {
	const Std$Type_t *Type;
	Std$Function$t Target;
};

#define Std$Function$CFields \
	const Std$Type$t *Type; \
	Std$Function$status (*Invoke)(FUNCTION_PARAMS);

RIVA_STRUCT(ct) {
	Std$Function$CFields
};

RIVA_STRUCT(checkedct) {
	Std$Function$CFields
	int Count;
	const char *File;
	int Line;
};

RIVA_STRUCT(cstate);
RIVA_STRUCT(cresumedata);

typedef Std$Function$status (* Std$Function$cresumefn)(Std$Function$result *Result);
typedef Std$Function$cresumefn Std$Function_cresumefn;

RIVA_STRUCT(state_t) {
	void *Run;
	Std$Function$state_t *Chain;
	void *Resume;
};

struct Std$Function$cstate {
	void *Run, *Chain, *Resume;
	Std$Function$cresumefn Invoke;
};

RIVA_TYPE(T);
RIVA_TYPE(CT);
RIVA_TYPE(CheckedCT);
RIVA_TYPE(ConstantT);
RIVA_TYPE(VariableT);
RIVA_TYPE(StatusT);

RIVA_OBJECT(Nil);
RIVA_OBJECT(Suspend);
RIVA_OBJECT(Success);
RIVA_OBJECT(Failure);
RIVA_OBJECT(Message);
RIVA_OBJECT(ConstantNew);
RIVA_OBJECT(VariableNew);
RIVA_OBJECT(IteratorNew);
RIVA_OBJECT(IteratorNext);
RIVA_OBJECT(Fold);

RIVA_CFUN(Std$Object_t *, constant_new, Std$Object_t *);

RIVA_CFUN(long, invoke_c);
RIVA_CFUN(Std$Function_status, resume_c, Std$Function$result *) __attribute__ ((warn_unused_result));
RIVA_CFUN(Std$Function_status, invoke, const Std$Object_t *, long, Std$Function_result *, const Std$Function_argument *) __attribute__ ((warn_unused_result));
RIVA_CFUN(Std$Function_status, call, const Std$Object_t *, long, Std$Function_result *, ...) __attribute__ ((warn_unused_result));
RIVA_CFUN(Std$Function_status, resume, Std$Function_result *) __attribute__ ((warn_unused_result));

RIVA_CFUN(Std$Object_t *, new_arg_type_message, const Std$Function_ct *, int, const Std$Type_t *, const Std$Type_t *);

#ifdef DOCUMENTING

#define LOCAL_FUNCTION(ARGS...) LOCAL_FUNCTION(__LINE__, ARGS)
#define GLOBAL_FUNCTION(ARGS...) GLOBAL_FUNCTION(__LINE__, ARGS)

#else

#define LOCAL_FUNCTION(NAME)\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS);\
	static Std$Function_ct NAME[] = {{Std$Function$CT, invoke_ ## NAME}};\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS)

#define GLOBAL_FUNCTION(NAME, COUNT)\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS);\
	Std$Function_checkedct NAME[] = {{Std$Function$CheckedCT, invoke_ ## NAME, COUNT, __FILE__, __LINE__}};\
	static FUNCTION_ATTRS Std$Function_status invoke_ ## NAME(FUNCTION_PARAMS)

#endif

#define CHECK_ARG_TYPE(INDEX, TYPE) \
	if (!Std$Object$in(Args[INDEX].Val, TYPE)) {\
		Result->Val = Std$Function$new_arg_type_message(Fun, INDEX + 1, TYPE, Args[INDEX].Val->Type);\
		return MESSAGE;\
	}

#define CHECK_EXACT_ARG_TYPE(INDEX, TYPE) \
	if (Args[INDEX].Val->Type != TYPE) {\
		Result->Val = Std$Function$new_arg_type_message(Fun, INDEX + 1, TYPE, Args[INDEX].Val->Type);\
		return MESSAGE;\
	}

#define FUNCTIONAL_TYPE(NAME, PARENTS...)\
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
		(void *)Std$Function$invoke_c, \
		NAME ## _fields, \
		NAME ## _levels\
	}}

#define RETURN(X) {\
	Result->Val = (Std$Object$t *)(X);\
	return SUCCESS;\
}

#define RETURN0 {\
	Result->Arg = Args[0];\
	return SUCCESS;\
}

#define RETURN1 {\
	Result->Arg = Args[1];\
	return SUCCESS;\
}

#define FAIL {\
	return FAILURE;\
}

#define SEND(X) {\
	Result->Val = (Std$Object$t *)(X);\
	return MESSAGE;\
}

#undef RIVA_MODULE

#endif
