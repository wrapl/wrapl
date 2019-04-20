#include <Std.h>
#include <Riva/Memory.h>

typedef struct complex_t {
	Std$Type_t *Type;
	Std$Object_t *Re, *Im;
} complex_t;

TYPE(T);

SYMBOL($ADD, "+");
SYMBOL($SUB, "-");
SYMBOL($MUL, "*");
SYMBOL($DIV, "/");
SYMBOL($sqrt, "sqrt");
SYMBOL($sin, "sin");
SYMBOL($cos, "cos");
SYMBOL($exp, "exp");

STRING(SpacePlusSpace, " + ");
STRING(iStr, "i");

CONSTANT(I, T) {
	complex_t *A = new(complex_t);
	A->Type = T;
	A->Re = Std$Real$Zero;
	A->Im = Std$Real$One;
	return A;
};

METHOD("re", TYP, Std$Number$T) {
	Result->Val = Args[0].Val;
	return SUCCESS;
};

METHOD("im", TYP, Std$Number$T) {
	Result->Val = Std$Real$Zero;
	return SUCCESS;
};

METHOD("re", TYP, T) {
	const complex_t *Complex = Args[0].Val;
	Result->Val = Complex->Re;
	return SUCCESS;
};

METHOD("im", TYP, T) {
	const complex_t *Complex = Args[0].Val;
	Result->Val = Complex->Im;
	return SUCCESS;
};

GLOBAL_FUNCTION(New, 2) {
	complex_t *Complex = new(complex_t);
	Complex->Type = T;
	Complex->Re = Args[0].Val;
	Complex->Im = Args[1].Val;
	Result->Val = Complex;
	return SUCCESS;
};

AMETHOD(Std$String$Of, TYP, T) {
	const complex_t *Complex = Args[0].Val;
	Std$Function_result Buffer;
	Std$Function$call(Std$String$Of, 1, &Buffer, Complex->Re, 0);
	Std$String_t *Final = Std$String$add(Buffer.Val, SpacePlusSpace);
	Std$Function$call(Std$String$Of, 1, &Buffer, Complex->Im, 0);
	Final = Std$String$add(Final, Buffer.Val);
	Result->Val = Std$String$add(Final, iStr);
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
	const complex_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($ADD, 2, &Buffer, A->Re, 0, B->Re, 0);
	C->Re = Buffer.Val;
	Std$Function$call($ADD, 2, &Buffer, A->Im, 0, B->Im, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("+", TYP, Std$Number$T, TYP, T) {
	const Std$Object_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($ADD, 2, &Buffer, A, 0, B->Re, 0);
	C->Re = Buffer.Val;
	C->Im = B->Im;
	Result->Val = C;
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, Std$Number$T) {
	const complex_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($ADD, 2, &Buffer, A->Re, 0, B, 0);
	C->Re = Buffer.Val;
	C->Im = A->Im;
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T) {
	const complex_t *A = Args[0].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($SUB, 1, &Buffer, A->Re, 0);
	C->Re = Buffer.Val;
	Std$Function$call($SUB, 1, &Buffer, A->Im, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};


METHOD("-", TYP, T, TYP, T) {
	const complex_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($SUB, 2, &Buffer, A->Re, 0, B->Re, 0);
	C->Re = Buffer.Val;
	Std$Function$call($SUB, 2, &Buffer, A->Im, 0, B->Im, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, Std$Number$T, TYP, T) {
	const Std$Object_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($SUB, 2, &Buffer, A, 0, B->Re, 0);
	C->Re = Buffer.Val;
	Std$Function$call($SUB, 1, &Buffer, B->Im, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, Std$Number$T) {
	const complex_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($SUB, 2, &Buffer, A->Re, 0, B, 0);
	C->Re = Buffer.Val;
	C->Im = A->Im;
	Result->Val = C;
	return SUCCESS;
};

static inline void conj(complex_t *R, const complex_t *A) {
	R->Re = A->Re;
	Std$Function_result Buffer;
	Std$Function$call($SUB, 1, &Buffer, A->Im, 0);
	R->Im = Buffer.Val;
};

METHOD("~", TYP, T) {
	complex_t *B = new(complex_t);
	conj(B, Args[0].Val);
	B->Type = T;
	Result->Val = B;
	return SUCCESS;
};

static inline void multiply(complex_t *R, const complex_t *A, const complex_t *B) {
	Std$Function_result Buffer0, Buffer1;
	Std$Function$call($MUL, 2, &Buffer0, A->Re, 0, B->Re, 0);
	Std$Function$call($MUL, 2, &Buffer1, A->Im, 0, B->Im, 0);
	Std$Function$call($SUB, 2, &Buffer0, Buffer0.Val, 0, Buffer1.Val, 0);
	Std$Object_t *Re = Buffer0.Val;
	Std$Function$call($MUL, 2, &Buffer0, A->Re, 0, B->Im, 0);
	Std$Function$call($MUL, 2, &Buffer1, A->Im, 0, B->Re, 0);
	Std$Function$call($ADD, 2, &Buffer0, Buffer0.Val, 0, Buffer1.Val, 0);
	R->Re = Re;
	R->Im = Buffer0.Val;
};

METHOD("*", TYP, T, TYP, T) {
	const complex_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	multiply(C, A, B);
	C->Type = T;
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, T, TYP, Std$Number$T) {
	const complex_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($MUL, 2, &Buffer, A->Re, 0, B, 0);
	C->Re = Buffer.Val;
	Std$Function$call($MUL, 2, &Buffer, A->Im, 0, B, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, Std$Number$T, TYP, T) {
	const Std$Object_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($MUL, 2, &Buffer, A, 0, B->Re, 0);
	C->Re = Buffer.Val;
	Std$Function$call($MUL, 2, &Buffer, A, 0, B->Im, 0);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

static Std$Object_t *norm2(complex_t *A) {
	Std$Function_result Buffer0, Buffer1;
	Std$Function$call($MUL, 2, &Buffer0, A->Re, 0, A->Re, 0);
	Std$Function$call($MUL, 2, &Buffer1, A->Im, 0, A->Im, 0);
	Std$Function$call($ADD, 2, &Buffer0, Buffer0.Val, 0, Buffer1.Val, 0);
	return Buffer0.Val;
};

static inline void inverse(complex_t *R, const complex_t *A) {
	Std$Function_result Buffer0, Buffer1;
	Std$Function$call($MUL, 2, &Buffer0, A->Re, 0, A->Re, 0);
	Std$Function$call($MUL, 2, &Buffer1, A->Im, 0, A->Im, 0);
	Std$Function$call($ADD, 2, &Buffer0, Buffer0.Val, 0, Buffer1.Val, 0);
	Std$Function$call($DIV, 2, &Buffer1, A->Re, 0, Buffer0.Val, 0);
	R->Re = Buffer1.Val;
	Std$Function$call($SUB, 1, &Buffer1, A->Im, 0);
	Std$Function$call($DIV, 2, &Buffer1, Buffer1.Val, 0, Buffer0.Val, 0);
	R->Im = Buffer1.Val;
};

METHOD("/", TYP, T, TYP, Std$Number$T) {
	const complex_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	complex_t *C = new(complex_t);
	C->Type = T;
	Std$Function_result Buffer;
	Std$Function$call($DIV, 2, &Buffer, A->Re, 0, B);
	C->Re = Buffer.Val;
	Std$Function$call($DIV, 2, &Buffer, A->Im, 0, B);
	C->Im = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("/", TYP, Std$Number$T, TYP, T) {
	const Std$Object_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t Inverse;
	inverse(&Inverse, B);
	return Std$Function$call($MUL, 2, Result, A, 0, Inverse, 0);
};

METHOD("/", TYP, T, TYP, T) {
	const complex_t *A = Args[0].Val;
	const complex_t *B = Args[1].Val;
	complex_t Inverse;
	inverse(&Inverse, B);
	return Std$Function$call($MUL, 2, Result, A, 0, Inverse, 0);
};

METHOD("abs", TYP, T) {
	const complex_t *A = Args[0].Val;
	return Std$Function$call($sqrt, 1, Result, norm2(A), 0);
};

static inline Std$Function_status power(Std$Function_result *Result, const complex_t *A, int Power) {
	if (Power == 1) {
		Result->Val = A;
		return SUCCESS;
	} else {
		complex_t P = *A;
		complex_t S = {T, Std$Real$One, Std$Real$Zero};
		for (;;) {
			if (Power & 1) multiply(&S, &S, &P);
			Power >>= 1;
			if (Power == 0) break;
			multiply(&P, &P, &P);
		};
		complex_t *R = new(complex_t);
		R->Type = T;
		R->Re = S.Re;
		R->Im = S.Im;
		Result->Val = R;
		return SUCCESS;
	};
};

METHOD("^", TYP, T, TYP, Std$Integer$SmallT) {
	const complex_t *A = Args[0].Val;
	int Power = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (Power > 0) {
		return power(Result, A, Power);
	} else if (Power < 0) {
		complex_t Inverse;
		inverse(&Inverse, A);
		return power(Result, &Inverse, Power);
	} else {
		Result->Val = Std$Real$One;
		return SUCCESS;
	};
};

METHOD("exp", TYP, T) {
	const complex_t *A = Args[0].Val;
	Std$Function_result Buffer;
	Std$Function$call($exp, 1, &Buffer, A->Re, 0);
	Std$Object_t *Exp = Buffer.Val;
	complex_t *R = new(complex_t);
	R->Type = T;
	Std$Function$call($cos, 1, &Buffer, A->Im, 0);
	Std$Function$call($MUL, 2, &Buffer, Exp, 0, Buffer.Val, 0);
	R->Re = Buffer.Val;
	Std$Function$call($sin, 1, &Buffer, A->Im, 0);
	Std$Function$call($MUL, 2, &Buffer, Exp, 0, Buffer.Val, 0);
	R->Im = Buffer.Val;
	Result->Val = R;
	return SUCCESS;
};
