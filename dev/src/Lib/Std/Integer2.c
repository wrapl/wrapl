#include <Std/Number.h>
#include <Std/Real.h>
#include <Std/String.h>
#include <Std/Symbol.h>
#include <Std/Integer.h>
#include <Std/Function.h>
#include <Util/TypedFunction.h>

extern Std$Type_t T[];
extern Std$Type_t SmallT[];
extern Std$Type_t BigT[];

extern Std$Object_t *_new_small(int32_t);
extern Std$Object_t *_new_big(mpz_t);

SYMBOL($AS, "@");

TYPED_FUNCTION(int, _int, Std$Object_t *A) {
	return 0;
};

TYPED_INSTANCE(int, _int, SmallT, Std$Integer$smallt *A) {
	return A->Value;
};

TYPED_INSTANCE(int, Std$Number$is0, SmallT, Std$Integer_smallt *A) {
	return A->Value == 0;
};

TYPED_INSTANCE(int, Std$Number$is1, SmallT, Std$Integer_smallt *A) {
	return A->Value == 1;
};

TYPED_INSTANCE(int, Std$Number$is0, BigT, Std$Integer_bigt *A) {
	return 0;
};

TYPED_INSTANCE(int, Std$Number$is1, BigT, Std$Integer_bigt *A) {
	return 0;
};

TYPED_INSTANCE(double, Std$Real$double, SmallT, Std$Integer$smallt *A) {
	return A->Value;
};

uint64_t _get_u64(Std$Object_t *Integer) {
	if (Integer->Type == SmallT) {
		return ((Std$Integer_smallt *)Integer)->Value;
	} else if (Integer->Type == BigT) {
		mpz_t Temp;
		mpz_init_set(Temp, ((Std$Integer_bigt *)Integer)->Value);
		mpz_tdiv_r_2exp(Temp, Temp, 64);
		uint64_t Value = 0;
		mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
		return Value;
	} else {
		return 0;
	};
};

int64_t _get_s64(Std$Object_t *Integer) {
	if (Integer->Type == SmallT) {
		return ((Std$Integer_smallt *)Integer)->Value;
	} else if (Integer->Type == BigT) {
		mpz_t Temp;
		mpz_init_set(Temp, ((Std$Integer_bigt *)Integer)->Value);
		mpz_tdiv_r_2exp(Temp, Temp, 63);
		int64_t Value = 0;
		mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
		return (mpz_sgn(Temp) < 0) ? -Value : Value;
	} else {
		return 0;
	};
};

Std$Object_t *_new_u64(uint64_t Value) {
	int32_t Value0 = (int32_t)Value;
	if (Value0 == Value) return _new_small(Value0);
	mpz_t Temp;
	mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
	mpz_mul_2exp(Temp, Temp, 32);
	mpz_add_ui(Temp, Temp, (uint32_t)Value);
	return _new_big(Temp);
};

Std$Object_t *_new_s64(int64_t Value) {
	int32_t Value0 = (int32_t)Value;
	if (Value0 == Value) return _new_small(Value0);
	mpz_t Temp;
	mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
	mpz_mul_2exp(Temp, Temp, 32);
	mpz_add_ui(Temp, Temp, (uint32_t)Value);
	if (Value < 0) mpz_neg(Temp, Temp);
	return _new_big(Temp);
};

GLOBAL_FUNCTION(NextPrime, 1) {
	if (Args[0].Val->Type == SmallT) {
		mpz_t Z;
		mpz_init_set_ui(Z, ((Std$Integer_smallt *)Args[0].Val)->Value);
		mpz_nextprime(Z, Z);
		if (mpz_fits_slong_p(Z)) {
			Result->Val = Std$Integer$new_small(mpz_get_si(Z));
			return SUCCESS;
		} else {
			Result->Val = Std$Integer$new_big(Z);
			return SUCCESS;
		};
	} else if (Args[0].Val->Type == BigT) {
		mpz_t Z;
		mpz_init(Z);
		mpz_nextprime(Z, ((Std$Integer_bigt *)Args[0].Val)->Value);
		if (mpz_fits_slong_p(Z)) {
			Result->Val = Std$Integer$new_small(mpz_get_si(Z));
			return SUCCESS;
		} else {
			Result->Val = Std$Integer$new_big(Z);
			return SUCCESS;
		};
	} else {
		Result->Val = Std$String$new("Invalid type to NextPrime");
		return MESSAGE;
	};
};

#ifdef DOCUMENTING

#define Std$Integer$T T
#define Std$Integer$SmallT SmallT
#define Std$Integer$BigT BigT
#define INTEGER_METHOD METHOD

PUSHFILE("Methods2.c");
#include "Methods2.c"
POPFILE();

#endif
