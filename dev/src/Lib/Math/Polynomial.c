#include <Std.h>
#include <Riva/Memory.h>
#include <Util/TypedFunction.h>
#include <stdint.h>

typedef struct polynomial_t {
	Std$Type_t *Type;
	Std$Integer_smallt Degree;
	Std$Object_t *Coefficients[];
} polynomial_t;

TYPE(T);

SYMBOL($AT, "@");
SYMBOL($ADD, "+");
SYMBOL($SUB, "-");
SYMBOL($MUL, "*");
SYMBOL($DIV, "div");

STRING(SpacePlusSpace, " + ");
STRING(ZeroStr, "0");
STRING(XStr, "x");
STRING(XPowStr, "x^");

GLOBAL_FUNCTION(New, 0) {
	if (Count > 0) {
		int Degree = Count - 1;
		while (Degree >= 0) {
			if (!Std$Number$is0(Args[Degree].Val)) break;
			--Degree;
		};
		if (Degree <= 0) {
			Result->Val = Args[0].Val;
		} else {
			polynomial_t *P = Riva$Memory$alloc(sizeof(polynomial_t) + Count * sizeof(Std$Object_t *));
			P->Type = T;
			P->Degree.Type = Std$Integer$SmallT;
			P->Degree.Value = Degree;
			for (int I = 0; I <= Degree; ++I) P->Coefficients[I] = Args[I].Val;
			Result->Val = P;
		};
	} else {
		Result->Val = Std$Real$Zero;
	};
	return SUCCESS;
};

METHOD("degree", TYP, T) {
	polynomial_t *P = Args[0].Val;
	Result->Val = &(P->Degree);
	return SUCCESS;
};

METHOD("degree", TYP, Std$Number$T) {
	static Std$Integer_smallt MinusOne[1] = {{Std$Integer$SmallT, -1}};
	Result->Val = MinusOne;
	return SUCCESS;
};

METHOD("[]", TYP, T, TYP, Std$Integer$SmallT) {
	polynomial_t *P = Args[0].Val;
	int Degree = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (Degree < 0) {
		return FAILURE;
	} else if (Degree > P->Degree.Value) {
		return FAILURE;
	} else {
		Result->Val = P->Coefficients[Degree];
		return SUCCESS;
	};
};

METHOD("@", TYP, T, VAL, Std$String$T) {
	polynomial_t *P = Args[0].Val;
	int Degree = P->Degree.Value;
	switch (Degree) {
	case -1: {
		Result->Val = ZeroStr;
		return SUCCESS;
	};
	case 0: {
		Std$Function_result Buffer;
		Std$Function$call($AT, 2, &Buffer, P->Coefficients[0], 0, Std$String$T, 0);
		Result->Val = Buffer.Val;
		return SUCCESS;
	};
	case 1: {
		Std$Function_result Buffer;
		Std$Function$call($AT, 2, &Buffer, P->Coefficients[0], 0, Std$String$T, 0);
		Std$String_t *S = Buffer.Val;
		Std$Function$call($AT, 2, &Buffer, P->Coefficients[1], 0, Std$String$T, 0);
		Result->Val = Std$String$add(Std$String$add(Buffer.Val, XStr), Std$String$add(SpacePlusSpace, S));
		return SUCCESS;
	};
	default: {
		Std$Function_result Buffer;
		Std$Function$call($AT, 2, &Buffer, P->Coefficients[0], 0, Std$String$T, 0);
		Std$String_t *S = Buffer.Val;
		Std$Function$call($AT, 2, &Buffer, P->Coefficients[1], 0, Std$String$T, 0);
		S = Std$String$add(Std$String$add(Buffer.Val, XStr), Std$String$add(SpacePlusSpace, S));
		for (int I = 2; I <= Degree; ++I) {
			Std$Function$call($AT, 2, &Buffer, P->Coefficients[I], 0, Std$String$T, 0);
			char *Power;
			asprintf(&Power, "%d", I);
			S = Std$String$add(SpacePlusSpace, S);
			S = Std$String$add(Std$String$new(Power), S);
			S = Std$String$add(XPowStr, S);
			S = Std$String$add(Buffer.Val, S);
		};
		Result->Val = S;
		return SUCCESS;
	};
	};
};

METHOD("+", TYP, T, ANY) {
	polynomial_t *A = Args[0].Val;
	Std$Object_t *B = Args[1].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I > 0; --I) C->Coefficients[I] = A->Coefficients[I];
	Std$Function_result Buffer;
	Std$Function$call($ADD, 2, &Buffer, A->Coefficients[0], 0, B, 0);
	C->Coefficients[0] = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("+", ANY, TYP, T) {
	polynomial_t *A = Args[1].Val;
	Std$Object_t *B = Args[0].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I > 0; --I) C->Coefficients[I] = A->Coefficients[I];
	Std$Function_result Buffer;
	Std$Function$call($ADD, 2, &Buffer, A->Coefficients[0], 0, B, 0);
	C->Coefficients[0] = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
	polynomial_t *A = Args[0].Val;
	polynomial_t *B = Args[1].Val;
	if (A->Degree.Value < B->Degree.Value) {
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (B->Degree.Value + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		C->Degree.Value = B->Degree.Value;
		for (int I = A->Degree.Value; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($ADD, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		for (int I = A->Degree.Value; ++I <= B->Degree.Value;) {
			C->Coefficients[I] = B->Coefficients[I];
		};
		C->Degree.Value = B->Degree.Value;
		Result->Val = C;
		return SUCCESS;
	} else if (A->Degree.Value > B->Degree.Value) {
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		C->Degree.Value = A->Degree.Value;
		for (int I = B->Degree.Value; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($ADD, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		for (int I = B->Degree.Value; ++I <= A->Degree.Value;) {
			C->Coefficients[I] = A->Coefficients[I];
		};
		C->Degree.Value = A->Degree.Value;
		Result->Val = C;
		return SUCCESS;
	} else {
		int Degree = A->Degree.Value;
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (Degree + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		for (int I = Degree; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($ADD, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		while (Degree >= 0) {
			if (!Std$Number$is0(C->Coefficients[Degree])) break;
			--Degree;
		};
		if (Degree <= 0) {
			Result->Val = C->Coefficients[0];
		} else {
			C->Degree.Value = Degree;
			Result->Val = C;
		};
		return SUCCESS;
	};
};

METHOD("-", TYP, T) {
	polynomial_t *A = Args[0].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	for (int I = A->Degree.Value; I >= 0; --I) {
		Std$Function_result Buffer;
		Std$Function$call($SUB, 1, &Buffer, A->Coefficients[I], 0);
		C->Coefficients[I] = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T, ANY) {
	polynomial_t *A = Args[0].Val;
	Std$Object_t *B = Args[1].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I > 0; --I) C->Coefficients[I] = A->Coefficients[I];
	Std$Function_result Buffer;
	Std$Function$call($SUB, 2, &Buffer, A->Coefficients[0], 0, B, 0);
	C->Coefficients[0] = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", ANY, TYP, T) {
	polynomial_t *A = Args[1].Val;
	Std$Object_t *B = Args[0].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I > 0; --I) {
		Std$Function_result Buffer;
		Std$Function$call($SUB, 1, &Buffer, A->Coefficients[I], 0);
		C->Coefficients[I] = Buffer.Val;
	};
	Std$Function_result Buffer;
	Std$Function$call($SUB, 2, &Buffer, B, 0, A->Coefficients[0], 0);
	C->Coefficients[0] = Buffer.Val;
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, T) {
	polynomial_t *A = Args[0].Val;
	polynomial_t *B = Args[1].Val;
	if (A->Degree.Value < B->Degree.Value) {
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (B->Degree.Value + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		C->Degree.Value = B->Degree.Value;
		for (int I = A->Degree.Value; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($SUB, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		for (int I = A->Degree.Value; ++I <= B->Degree.Value;) {
			Std$Function_result Buffer;
			Std$Function$call($SUB, 1, &Buffer, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		C->Degree.Value = B->Degree.Value;
		Result->Val = C;
		return SUCCESS;
	} else if (A->Degree.Value > B->Degree.Value) {
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		C->Degree.Value = A->Degree.Value;
		for (int I = B->Degree.Value; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($SUB, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		for (int I = B->Degree.Value; ++I <= A->Degree.Value;) {
			C->Coefficients[I] = A->Coefficients[I];
		};
		C->Degree.Value = A->Degree.Value;
		Result->Val = C;
		return SUCCESS;
	} else {
		int Degree = A->Degree.Value;
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (Degree + 1) * sizeof(Std$Object_t *));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		for (int I = Degree; I >= 0; --I) {
			Std$Function_result Buffer;
			Std$Function$call($SUB, 2, &Buffer, A->Coefficients[I], 0, B->Coefficients[I], 0);
			C->Coefficients[I] = Buffer.Val;
		};
		while (Degree >= 0) {
			if (!Std$Number$is0(C->Coefficients[Degree])) break;
			--Degree;
		};
		if (Degree <= 0) {
			Result->Val = C->Coefficients[0];
		} else {
			C->Degree.Value = Degree;
			Result->Val = C;
		};
		return SUCCESS;
	};
};

METHOD("*", TYP, T, ANY) {
	polynomial_t *A = Args[0].Val;
	Std$Object_t *B = Args[1].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I >= 0; --I) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, A->Coefficients[I], 0, B, 0);
		C->Coefficients[I] = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", ANY, TYP, T) {
	polynomial_t *A = Args[1].Val;
	Std$Object_t *B = Args[0].Val;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (A->Degree.Value + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = A->Degree.Value;
	for (int I = A->Degree.Value; I >= 0; --I) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, A->Coefficients[I], 0, B, 0);
		C->Coefficients[I] = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, T, TYP, T) {
	polynomial_t *A = Args[0].Val;
	polynomial_t *B = Args[1].Val;
	if ((A->Degree.Value == -1) || (B->Degree.Value == -1)) {
		polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t));
		C->Type = T;
		C->Degree.Type = Std$Integer$SmallT;
		C->Degree.Value = -1;
		Result->Val = C;
		return SUCCESS;
	};
	int Degree = A->Degree.Value + B->Degree.Value;
	polynomial_t *C = Riva$Memory$alloc(sizeof(polynomial_t) + (Degree + 1) * sizeof(Std$Object_t *));
	C->Type = T;
	C->Degree.Type = Std$Integer$SmallT;
	C->Degree.Value = Degree;
	Std$Object_t **AP, *BC, **CP;
	for (int I = 0; I <= B->Degree.Value; ++I) {
		AP = A->Coefficients;
		BC = B->Coefficients[I];
		CP = C->Coefficients + I;
		for (int J = A->Degree.Value; J > 0; --J) {
			Std$Function_result Buffer;
			Std$Function$call($MUL, 2, &Buffer, *(AP++), 0, BC, 0);
			if (*CP) Std$Function$call($ADD, 2, &Buffer, *CP, 0, Buffer.Val, 0);
			*(CP++) = Buffer.Val;
		};
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, *AP, 0, BC, 0);
		*CP = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("div", TYP, T, TYP, T) {
};

METHOD("()", TYP, T, ANY) {
	polynomial_t *P = Args[0].Val;
	Std$Object_t *X = Args[1].Val;
	Std$Object_t *Y = P->Coefficients[0];
	for (int I = 1; I <= P->Degree.Value; ++I) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, X, 0, P->Coefficients[I], 0);
		Std$Function$call($ADD, 2, &Buffer, Y, 0, Buffer.Val, 0);
		Y = Buffer.Val;
		Std$Function$call($MUL, 2, &Buffer, X, 0, Args[1].Val, 0);
		X = Buffer.Val;
	};
	Result->Val = Y;
	return SUCCESS;
};

TYPED_INSTANCE(int, Std$Number$is0, T, polynomial_t *A) {
	return A->Degree.Value == -1;
};

METHOD("is0", TYP, T) {
	asm("int3");
	polynomial_t *P = Args[0].Val;
	if (P->Degree.Value == -1) {
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};
