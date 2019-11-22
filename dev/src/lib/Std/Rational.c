#include <Std.h>
#include <Riva/Memory.h>
#include <Util/TypedFunction.h>
#include <gmp.h>

TYPE(T, Std$Number$T);

ASYMBOL(Of);

TYPED_INSTANCE(int, Std$Number$is0, T, Std$Rational$t *A) {
	return 0;
};

TYPED_INSTANCE(int, Std$Number$is1, T, Std$Rational$t *A) {
	return 0;
};

TYPED_INSTANCE(double, Std$Real$double, T, Std$Rational$t *A) {
	return mpq_get_d(A->Value);
};

Std$Rational$t *_alloc(void) {
	Std$Rational$t *R = new(Std$Rational$t);
	R->Type = T;
	mpq_init(R->Value);
	return R;
};

Std$Object$t *_new(mpq_t V) {
	Std$Rational$t *R = new(Std$Rational$t);
	R->Type = T;
	mpq_init(R->Value);
	mpq_set(R->Value, V);
	return (Std$Object$t *)R;
};

static inline Std$Object$t *finish_rational(mpq_t R) {
	if (mpz_cmp_si(mpq_denref(R), 1)) {
		return _new(R);
	} else if (mpz_fits_slong_p(mpq_numref(R))) {
		return Std$Integer$new_small(mpz_get_si(mpq_numref(R)));
	} else {
		return Std$Integer$new_big(mpq_numref(R));
	};
};

Std$Object$t *_new_string(const char *String) {
	mpq_t R;
	mpq_init(R);
	mpq_set_str(R, String, 10);
	mpq_canonicalize(R);
	return finish_rational(R);
};

Std$Object$t *_new_small_small(int Num, int Den) {
	Std$Rational$t *R = new(Std$Rational$t);
	R->Type = T;
	mpz_init_set_si(mpq_numref(R->Value), Num);
	mpz_init_set_si(mpq_denref(R->Value), Den);
	mpq_canonicalize(R->Value);
	return (Std$Object$t *)R;
};

/*
ASYMBOL(New);
// Returns a new rational.

AMETHOD(New, TYP, Std$String$T) {
//@str
//:T
// <var>str</var> should be of the form <code>"num/den"</code>.
	mpq_t R;
	mpq_init(R);
	mpq_set_str(R, Std$String$flatten(Args[0].Val), 10);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};

AMETHOD(New, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@num
//@den
//:T
	mpq_t R;
	mpq_init(R);
	mpq_set_si(R, ((Std$Integer$smallt *)Args[0].Val)->Value, ((Std$Integer$smallt *)Args[1].Val)->Value);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};

AMETHOD(New, TYP, Std$Integer$SmallT, TYP, Std$Integer$BigT) {
//@num
//@den
//:T
	mpq_t R;
	mpq_init(R);
	mpz_set_si(mpq_numref(R), ((Std$Integer$smallt *)Args[0].Val)->Value);
	mpz_set(mpq_denref(R), ((Std$Integer$bigt *)Args[1].Val)->Value);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};

AMETHOD(New, TYP, Std$Integer$BigT, TYP, Std$Integer$SmallT) {
//@num
//@den
//:T
	mpq_t R;
	mpq_init(R);
	mpz_set(mpq_numref(R), ((Std$Integer$bigt *)Args[0].Val)->Value);
	mpz_set_si(mpq_denref(R), ((Std$Integer$smallt *)Args[1].Val)->Value);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};

AMETHOD(New, TYP, Std$Integer$BigT, TYP, Std$Integer$BigT) {
//@num
//@den
//:T
	mpq_t R;
	mpq_init(R);
	mpz_set(mpq_numref(R), ((Std$Integer$bigt *)Args[0].Val)->Value);
	mpz_set(mpq_denref(R), ((Std$Integer$bigt *)Args[1].Val)->Value);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};
*/

GLOBAL_FUNCTION(Hash, 1) {
	Std$Rational$t *A = (Std$Rational$t *)Args[0].Val;
	mpz_t R;
	mpz_init(R);
	mpz_add(R, mpq_numref(A->Value), mpq_denref(A->Value));
	Result->Val = Std$Integer$new_small(mpz_get_si(R));
	return SUCCESS;
};

GLOBAL_FUNCTION(Compare, 2) {
	Std$Rational$t *A = (Std$Rational$t *)Args[0].Val;
	Std$Rational$t *B = (Std$Rational$t *)Args[1].Val;
	int R = mpq_cmp(A->Value, B->Value);
	if (R < 0) {
		Result->Val = Std$Object$Less;
	} else if (R > 0) {
		Result->Val = Std$Object$Greater;
	} else {
		Result->Val = Std$Object$Equal;
	};
	return SUCCESS;
};

#ifdef DOCUMENTING

#define Std$Rational$T T
#define RATIONAL_METHOD METHOD

PUSHFILE("Methods2.c");
#include "Methods2.c"
POPFILE();

#endif
