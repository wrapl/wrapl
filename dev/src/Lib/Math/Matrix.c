#include <Std.h>
#include <Math/Vector.h>
#include <Riva/Memory.h>
#include <stdint.h>
#include <Util/TypedFunction.h>

typedef struct matrix_t {
	const Std$Type_t *Type;
	Std$Integer_smallt NoOfRows, NoOfCols;
	Std$Object_t *Entries[];
} matrix_t;

TYPE(T);
TYPE(MessageT);

STRING(LeftBracket, "[");
STRING(RightBracket, "]");
STRING(CommaSpace, ", ");
STRING(LeftRightBracket, "[]");
STRING(ValueString, "<value>");

SYMBOL($AT, "@");
SYMBOL($ADD, "+");
SYMBOL($SUB, "-");
SYMBOL($MUL, "*");
SYMBOL($DIV, "/");
SYMBOL($EQL, "=");
SYMBOL($NEQ, "~=");
SYMBOL($HASH, "#");
SYMBOL($COMP, "?");

GLOBAL_FUNCTION(New, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
    uint32_t NoOfRows = ((Std$Integer_smallt *)Args[0].Val)->Value;
    uint32_t NoOfCols = ((Std$Integer_smallt *)Args[1].Val)->Value;
    matrix_t *Matrix = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
    Matrix->Type = T;
    Matrix->NoOfRows.Type = Matrix->NoOfCols.Type = Std$Integer$SmallT;
    Matrix->NoOfRows.Value = NoOfRows;
    Matrix->NoOfCols.Value = NoOfCols;
    Std$Function_argument *Arg = Args + 1;
    Std$Object_t **Ptr = Matrix->Entries;
    if (Count == 2) {
        for (int I = NoOfRows * NoOfCols; --I >= 0;) *(Ptr++) = Std$Object$Nil;
    } else if (NoOfRows * NoOfCols != Count - 2) {
        Std$Object_t **Limit = Ptr + NoOfRows * NoOfCols;
        for (int I = Count - 1; --I;) *(Ptr++) = (++Arg)->Val;
        while (Ptr < Limit) *(Ptr++) = Arg->Val;
    } else {
        for (int I = Count - 1; --I;) *(Ptr++) = (++Arg)->Val;
    };
    Result->Val = Matrix;
    return SUCCESS;
};

METHOD("rows", TYP, T) {
	Result->Val = &((matrix_t *)Args[0].Val)->NoOfRows;
	return SUCCESS;
};

METHOD("cols", TYP, T) {
	Result->Val = &((matrix_t *)Args[0].Val)->NoOfCols;
	return SUCCESS;
};

METHOD("@", TYP, T, VAL, Std$String$T) {
	const matrix_t *Matrix = Args[0].Val;
	Std$Object_t **Ptr = Matrix->Entries;

	Std$Function_result Buffer;
	Std$String_t *Final = Std$String$add(LeftBracket, LeftBracket);
	if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
		Final = Std$String$add(Final, Buffer.Val);
	} else {
		Final = Std$String$add(Final, ValueString);
	};
	for (int J = 1; J < Matrix->NoOfCols.Value; ++J) {
		Final = Std$String$add(Final, CommaSpace);
		if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
			Final = Std$String$add(Final, Buffer.Val);
		} else {
			Final = Std$String$add(Final, ValueString);
		};
	};
	Final = Std$String$add(Final, RightBracket);
	for (int I = 1; I < Matrix->NoOfRows.Value; ++I) {
		Final = Std$String$add(Final, CommaSpace);
		Final = Std$String$add(Final, LeftBracket);
		if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
			Final = Std$String$add(Final, Buffer.Val);
		} else {
			Final = Std$String$add(Final, ValueString);
		};
		for (int J = 1; J < Matrix->NoOfCols.Value; ++J) {
			Final = Std$String$add(Final, CommaSpace);
			if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
				Final = Std$String$add(Final, Buffer.Val);
			} else {
				Final = Std$String$add(Final, ValueString);
			};
		};
		Final = Std$String$add(Final, RightBracket);
	};
	Result->Val = Std$String$add(Final, RightBracket);
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	if (NoOfRows != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of equal dimension");
		return MESSAGE;
	};
	uint32_t NoOfCols = A->NoOfCols.Value;
	if (NoOfCols != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of equal dimension");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($ADD, 2, &Buffer, *(AP++), 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			*(CP++) = Buffer.Val;
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"+\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	if (NoOfRows != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of equal dimension");
		return MESSAGE;
	};
	uint32_t NoOfCols = A->NoOfCols.Value;
	if (NoOfCols != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of equal dimension");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($SUB, 2, &Buffer, *(AP++), 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			*(CP++) = Buffer.Val;
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"-\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = B->NoOfCols.Value;
	uint32_t Temp = A->NoOfCols.Value;
	if (Temp != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of compatible dimension");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	for (int I = 0; I < NoOfRows; ++I) {
		for (int J = 0; J < NoOfCols; ++J) {
			Std$Function_result Buffer;
			Std$Object_t **AP = A->Entries + I * Temp;
			Std$Object_t **BP = B->Entries + J;
			Std$Function$call($MUL, 2, &Buffer, *(AP), 0, *(BP), 0);
			Std$Object_t *Value = Buffer.Val;
			for (int K = Temp; --K;) {
				Std$Function$call($MUL, 2, &Buffer, *(++AP), 0, *(BP += NoOfCols), 0);
				Std$Function$call($ADD, 2, &Buffer, Value, 0, Buffer.Val, 0);
				Value = Buffer.Val;
			};
			C->Entries[I * NoOfCols + J] = Value;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, T, TYP, Math$Vector$T) {
	const matrix_t *A = Args[0].Val;
	const Math$Vector$t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t Temp = A->NoOfCols.Value;
	if (Temp != B->Length.Value) {
		Result->Val = Std$String$new("Matrices not of compatible dimension");
		return MESSAGE;
	};
	Math$Vector$t *C = Riva$Memory$alloc(sizeof(Math$Vector$t) + NoOfRows * sizeof(Std$Object_t **));
	C->Type = Math$Vector$T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = NoOfRows;
	for (int I = 0; I < NoOfRows; ++I) {
		Std$Function_result Buffer;
		Std$Object_t **AP = A->Entries + I * Temp;
		Std$Object_t **BP = B->Entries;
		Std$Function$call($MUL, 2, &Buffer, *(AP), 0, *(BP), 0);
		Std$Object_t *Value = Buffer.Val;
		for (int K = Temp; --K;) {
			Std$Function$call($MUL, 2, &Buffer, *(++AP), 0, *(++BP), 0);
			Std$Function$call($ADD, 2, &Buffer, Value, 0, Buffer.Val, 0);
			Value = Buffer.Val;
		};
		C->Entries[I] = Value;
	};
	Result->Val = C;
	return SUCCESS;
};

static inline void sq_mat_mul(int N, matrix_t *R, const matrix_t *A, const matrix_t *B) {
	for (int I = 0; I < N; ++I) {
		for (int J = 0; J < N; ++J) {
			Std$Function_result Buffer;
			Std$Object_t **AP = A->Entries + I * N;
			Std$Object_t **BP = B->Entries + J;
			Std$Function$call($MUL, 2, &Buffer, *(AP), 0, *(BP), 0);
			Std$Object_t *Value = Buffer.Val;
			for (int K = N; --K;) {
				Std$Function$call($MUL, 2, &Buffer, *(++AP), 0, *(BP += N), 0);
				Std$Function$call($ADD, 2, &Buffer, Value, 0, Buffer.Val, 0);
				Value = Buffer.Val;
			};
			R->Entries[I * N + J] = Value;
		};
	};
};

static Std$Object_t *determinant2(matrix_t *M, int N, int *Rows, int *Cols) {
	Std$Object_t **Row = M->Entries + Rows[0] * M->NoOfCols.Value;
	++Rows;
	if (--N == 0) return Row[Cols[0]];
	int Cols2[N];
	for (int I = 0; I < N; ++I) Cols2[I] = Cols[I + 1];
	Std$Function_result Buffer;
	Std$Function$call($MUL, 2, &Buffer, Row[Cols[0]], 0, determinant2(M, N, Rows, Cols2), 0);
	Std$Object_t *Det = Buffer.Val;
	for (int I = 0; I < N; ++I) {
		Cols2[I] = Cols[I];
		Std$Function$call($MUL, 2, &Buffer, Row[Cols[I + 1]], 0, determinant2(M, N, Rows, Cols2), 0);
		Std$Function$call(I % 2 ? $ADD : $SUB, 2, &Buffer, Det, 0, Buffer.Val);
		Det = Buffer.Val;
	};
	return Det;
};

static inline matrix_t *inverse(matrix_t *M) {
	int N = M->NoOfRows.Value;
	if (N != M->NoOfCols.Value) return 0;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = N;
	C->NoOfCols.Value = N;

	int Rows[N], Cols[N];
	for (int I = 0; I < N; ++I) Rows[I] = Cols[I] = I;
	Std$Object_t *Det = determinant2(M, N, Rows, Cols);
	for (int I = 1; I < N; ++I) Rows[I - 1] = I;

	for (int I = 0; I < N; ++I) {
		for (int J = 1; J < N; ++J) Cols[J - 1] = J;
		for (int J = 0; J < N; ++J) {
			Std$Function_result Buffer;
			if ((I + J) % 2) {
				Std$Function$call($SUB, 1, &Buffer, determinant2(M, N - 1, Rows, Cols), 0);
				Std$Function$call($DIV, 2, &Buffer, Buffer.Val, 0, Det, 0);
			} else {
				Std$Function$call($DIV, 2, &Buffer, determinant2(M, N - 1, Rows, Cols), 0, Det, 0);
			};
			C->Entries[I + J * N] = Buffer.Val;
			Cols[J] = J;
		};
		Rows[I] = I;
	};
	return C;
};

static inline Std$Function_status sq_mat_pow(Std$Function_result *Result, int N, const matrix_t *A, int Power) {
	if (Power == 1) {
		Result->Val = A;
		return SUCCESS;
	} else {
		matrix_t *R = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
		Std$Object_t *One = Std$Integer$new_small(1);
		Std$Object_t *Zero = Std$Integer$new_small(0);
		for (int I = 0; I < N; ++I) {
			for (int J = 0; J < N; ++J) {
				R->Entries[I * N + J] = (I == J) ? One : Zero;
			}
		};
		matrix_t *P = A;
		for (;;) {
			if (Power & 1) {
				matrix_t *S = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
				sq_mat_mul(N, S, R, P);
				R = S;
			};
			Power >>= 1;
			if (Power == 0) break;
			matrix_t *Q = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
			sq_mat_mul(N, Q, P, P);
			P = Q;
		};
		R->Type = T;
		R->NoOfRows.Type = R->NoOfCols.Type = Std$Integer$SmallT;
		R->NoOfRows.Value = N;
		R->NoOfCols.Value = N;
		Result->Val = R;
		return SUCCESS;
	};
};

METHOD("^", TYP, T, TYP, Std$Integer$SmallT) {
	const matrix_t *A = Args[0].Val;
	int N = A->NoOfRows.Value;
	if (N != A->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix not square");
		return MESSAGE;
	};
	int Power = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (Power > 0) {
		return sq_mat_pow(Result, N, A, Power);
	} else if (Power < 0) {
		return sq_mat_pow(Result, N, inverse(A), -Power);
	} else {
		matrix_t *R = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
		R->Type = T;
		R->NoOfRows.Type = R->NoOfCols.Type = Std$Integer$SmallT;
		R->NoOfRows.Value = N;
		R->NoOfCols.Value = N;
		Std$Object_t *One = Std$Integer$new_small(1);
		Std$Object_t *Zero = Std$Integer$new_small(0);
		for (int I = 0; I < N; ++I) {
			for (int J = 0; J < N; ++J) {
				R->Entries[I * N + J] = (I == J) ? One : Zero;
			}
		};
		Result->Val = R;
		return SUCCESS;
	};
};

METHOD("/", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = inverse(Args[1].Val);
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = B->NoOfCols.Value;
	uint32_t Temp = A->NoOfCols.Value;
	if (Temp != B->NoOfRows.Value) {
		Result->Val = Std$String$new("Matrices not of compatible dimension");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	for (int I = 0; I < NoOfRows; ++I) {
		for (int J = 0; J < NoOfCols; ++J) {
			Std$Function_result Buffer;
			Std$Object_t **AP = A->Entries + I * Temp;
			Std$Object_t **BP = B->Entries + J;
			Std$Function$call($MUL, 2, &Buffer, *(AP), 0, *(BP), 0);
			Std$Object_t *Value = Buffer.Val;
			for (int K = Temp; --K;) {
				Std$Function$call($MUL, 2, &Buffer, *(++AP), 0, *(BP += NoOfCols), 0);
				Std$Function$call($ADD, 2, &Buffer, Value, 0, Buffer.Val, 0);
				Value = Buffer.Val;
			};
			C->Entries[I * NoOfCols + J] = Value;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("t", TYP, T) {
	const matrix_t *A = Args[0].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *B = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	B->Type = T;
	B->NoOfRows.Type = B->NoOfCols.Type = Std$Integer$SmallT;
	B->NoOfRows.Value = NoOfCols;
	B->NoOfCols.Value = NoOfRows;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	for (int I = 0; I < NoOfRows; ++I) {
		Std$Object_t **BR = BP + I;
		for (int J = 0; J < NoOfCols; ++J) {
			BR[0] = AP[0];
			++AP;
			BR += NoOfRows;
		};
	};
	Result->Val = B;
	return SUCCESS;
};

METHOD("tr", TYP, T) {
	const matrix_t *M = Args[0].Val;
	int N = M->NoOfRows.Value;
	if (N != M->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix must be square for :tr");
		return MESSAGE;
	};
	Std$Object_t **AP = M->Entries;
	Std$Function_result Buffer;
	Buffer.Val = *(AP);
	for (int I = 1; I < N; ++I) {
		AP += N + 1;
		Std$Function$call($ADD, 2, &Buffer, Buffer.Val, 0, *(AP), 0);
	};
	Result->Val = Buffer.Val;
	return SUCCESS;
};

METHOD("det", TYP, T) {
	const matrix_t *M = Args[0].Val;
	int N = M->NoOfRows.Value;
	if (N != M->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix must be square for :det");
		return MESSAGE;
	};
	int Rows[N], Cols[N];
	for (int I = 0; I < N; ++I) Rows[I] = Cols[I] = I;
	Result->Val = determinant2(M, N, Rows, Cols);
	return SUCCESS;
};

METHOD("adj", TYP, T) {
	const matrix_t *M = Args[0].Val;
	int N = M->NoOfRows.Value;
	if (N != M->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix must be square for :adj");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = N;
	C->NoOfCols.Value = N;

	int Rows[N];
	for (int I = 1; I < N; ++I) Rows[I - 1] = I;

	for (int I = 0; I < N; ++I) {
		int Cols[N];
		for (int J = 1; J < N; ++J) Cols[J - 1] = J;
		for (int J = 0; J < N; ++J) {
			if ((I + J) % 2) {
				Std$Function_result Buffer;
				Std$Function$call($SUB, 1, &Buffer, determinant2(M, N - 1, Rows, Cols), 0);
				C->Entries[I + J * N] = Buffer.Val;
			} else {
				C->Entries[I + J * N] = determinant2(M, N - 1, Rows, Cols);
			};
			Cols[J] = J;
		};
		Rows[I] = I;
	};

	Result->Val = C;
	return SUCCESS;
};

METHOD("inv", TYP, T) {
	const matrix_t *M = Args[0].Val;
	int N = M->NoOfRows.Value;
	if (N != M->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix must be square for :inv");
		return MESSAGE;
	};
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = N;
	C->NoOfCols.Value = N;

	int Rows[N], Cols[N];
	for (int I = 0; I < N; ++I) Rows[I] = Cols[I] = I;
	Std$Object_t *Det = determinant2(M, N, Rows, Cols);
	for (int I = 1; I < N; ++I) Rows[I - 1] = I;

	for (int I = 0; I < N; ++I) {
		for (int J = 1; J < N; ++J) Cols[J - 1] = J;
		for (int J = 0; J < N; ++J) {
			Std$Function_result Buffer;
			if ((I + J) % 2) {
				Std$Function$call($SUB, 1, &Buffer, determinant2(M, N - 1, Rows, Cols), 0);
				Std$Function$call($DIV, 2, &Buffer, Buffer.Val, 0, Det, 0);
			} else {
				Std$Function$call($DIV, 2, &Buffer, determinant2(M, N - 1, Rows, Cols), 0, Det, 0);
			};
			C->Entries[I + J * N] = Buffer.Val;
			Cols[J] = J;
		};
		Rows[I] = I;
	};

	Result->Val = C;
	return SUCCESS;
};

METHOD("+", TYP, T, ANY) {
	const matrix_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	memcpy(C->Entries, A->Entries, NoOfRows * NoOfCols * sizeof(Std$Object_t **));
	Std$Object_t **Ptr = C->Entries;
	for (int I = (NoOfRows < NoOfCols ? NoOfRows : NoOfCols); --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($ADD, 2, &Buffer, Ptr[0], 0, B, 0);
		Ptr[0] = Buffer.Val;
		Ptr += NoOfCols + 1;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", TYP, T, ANY) {
	const matrix_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	memcpy(C->Entries, A->Entries, NoOfRows * NoOfCols * sizeof(Std$Object_t **));
	Std$Object_t **Ptr = C->Entries;
	for (int I = (NoOfRows < NoOfCols ? NoOfRows : NoOfCols); --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($SUB, 2, &Buffer, Ptr[0], 0, B, 0);
		Ptr[0] = Buffer.Val;
		Ptr += NoOfCols + 1;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("+", ANY, TYP, T) {
	const matrix_t *A = Args[1].Val;
	const Std$Object_t *B = Args[0].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	memcpy(C->Entries, A->Entries, NoOfRows * NoOfCols * sizeof(Std$Object_t **));
	Std$Object_t **Ptr = C->Entries;
	for (int I = (NoOfRows < NoOfCols ? NoOfRows : NoOfCols); --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($ADD, 2, &Buffer, B, 0, Ptr[0], 0);
		Ptr[0] = Buffer.Val;
		Ptr += NoOfCols + 1;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("-", ANY, TYP, T) {
	const matrix_t *A = Args[1].Val;
	const Std$Object_t *B = Args[0].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	memcpy(C->Entries, A->Entries, NoOfRows * NoOfCols * sizeof(Std$Object_t **));
	Std$Object_t **Ptr = C->Entries;
	for (int I = (NoOfRows < NoOfCols ? NoOfRows : NoOfCols); --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($SUB, 2, &Buffer, B, 0, Ptr[0], 0);
		Ptr[0] = Buffer.Val;
		Ptr += NoOfCols + 1;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", TYP, T, ANY) {
	const matrix_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, *(AP++), 0, B, 0);
		*(CP++) = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", ANY, TYP, T) {
	const matrix_t *A = Args[1].Val;
	const Std$Object_t *B = Args[0].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, B, 0, *(AP++), 0);
		*(CP++) = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("/", TYP, T, ANY) {
	const matrix_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($DIV, 2, &Buffer, *(AP++), 0, B, 0);
		*(CP++) = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("/", ANY, TYP, T) {
	const matrix_t *A = inverse(Args[1].Val);
	const Std$Object_t *B = Args[0].Val;
	uint32_t NoOfRows = A->NoOfRows.Value;
	uint32_t NoOfCols = A->NoOfCols.Value;
	matrix_t *C = Riva$Memory$alloc(sizeof(matrix_t) + NoOfCols * NoOfRows * sizeof(Std$Object_t **));
	C->Type = T;
	C->NoOfRows.Type = C->NoOfCols.Type = Std$Integer$SmallT;
	C->NoOfRows.Value = NoOfRows;
	C->NoOfCols.Value = NoOfCols;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($MUL, 2, &Buffer, *(AP++), 0, B, 0);
		*(CP++) = Buffer.Val;
	};
	Result->Val = C;
	return SUCCESS;
};

TYPED_INSTANCE(int, Std$Number$is0, T, matrix_t *A) {
	Std$Object_t **AP = A->Entries;
	for (int I = A->NoOfRows.Value * A->NoOfCols.Value; --I >= 0;) {
		if (!Std$Number$is0(*(AP++))) return 0;
	};
	return 1;
};

METHOD("is0", TYP, T) {
	const matrix_t *A = Args[0].Val;
	Std$Object_t **AP = A->Entries;
	for (int I = A->NoOfRows.Value * A->NoOfCols.Value; --I >= 0;) {
		if (!Std$Number$is0(*(AP++))) return FAILURE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(Hash, 1, "#", TYP, T) {
//@matrix
//:Std$Integer$SmallT
// Returns a hash value for <var>matrix</var>.
	int Hash = 0;
	const matrix_t *A = Args[0].Val;
	Std$Object_t **AP = A->Entries;
	for (int I = A->NoOfRows.Value * A->NoOfCols.Value; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($HASH, 1, &Buffer, *(AP++), 0);
		Hash ^= ((Std$Integer_smallt *)Buffer.Val)->Value;
		asm("roll $2, %0" : "=r" (Hash) : "0" (Hash));
	};
	Result->Val = Std$Integer$new_small(Hash);
	return SUCCESS;
};

GLOBAL_METHOD(Compare, 2, "?", TYP, T, TYP, T) {
//@a
//@b
//:Std$Object$T
// Compares <var>a</var> and <var>b</var>.
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	if (A == B) {
		Result->Val = Std$Object$Equal;
		return SUCCESS;
	};
	if (A->NoOfRows.Value < B->NoOfRows.Value) {
		Result->Val = Std$Object$Less;
		return SUCCESS;
	};
	if (A->NoOfRows.Value > B->NoOfRows.Value) {
		Result->Val = Std$Object$Greater;
		return SUCCESS;
	};
	if (A->NoOfCols.Value < B->NoOfCols.Value) {
		Result->Val = Std$Object$Less;
		return SUCCESS;
	};
	if (A->NoOfCols.Value > B->NoOfCols.Value) {
		Result->Val = Std$Object$Greater;
		return SUCCESS;
	};
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	for (int I = A->NoOfRows.Value * A->NoOfCols.Value; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($COMP, 2, &Buffer, *(AP++), 0, *(BP++), 0);
		if (Buffer.Val != Std$Object$Equal) {
			Result->Val = Buffer.Val;
			return SUCCESS;
		};
	};
	Result->Val = Std$Object$Equal;
	return SUCCESS;
};

METHOD("[]", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
    const matrix_t *A = Args[0].Val;
    Std$Integer_smallt *B = Args[1].Val;
    Std$Integer_smallt *C = Args[2].Val;
    Result->Val = *(Result->Ref = A->Entries + (B->Value - 1) * A->NoOfCols.Value + C->Value - 1);
    return SUCCESS;
};

METHOD("=", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	if (A == B) {
		Result->Arg = Args[1];
		return SUCCESS;
	};
	uint32_t NoOfRows = A->NoOfRows.Value;
	if (NoOfRows != B->NoOfRows.Value) return FAILURE;
	uint32_t NoOfCols = A->NoOfCols.Value;
	if (NoOfCols != B->NoOfCols.Value) return FAILURE;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($EQL, 2, &Buffer, *(AP++), 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			break;
		case FAILURE:
			return FAILURE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Arg = Args[1];
	return SUCCESS;
};

METHOD("~=", TYP, T, TYP, T) {
	const matrix_t *A = Args[0].Val;
	const matrix_t *B = Args[1].Val;
	if (A == B) return FAILURE;
	uint32_t NoOfRows = A->NoOfRows.Value;
	if (NoOfRows != B->NoOfRows.Value) {Result->Arg = Args[1]; return SUCCESS;};
	uint32_t NoOfCols = A->NoOfCols.Value;
	if (NoOfCols != B->NoOfCols.Value) {Result->Arg = Args[1]; return SUCCESS;};
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	for (int I = NoOfRows * NoOfCols; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($NEQ, 2, &Buffer, *(AP++), 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			Result->Arg = Args[1];
			return SUCCESS;
		case FAILURE:
			break;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	return FAILURE;
};

METHOD("cholesky", TYP, T, ANY, ANY) {
//@a
//@l
//@m
// Computes the Cholesky decomposition of the symmetric matrix <var>a</var> return a matrix in <var>l</var> and a vector in <var>d</var> such that <code>a = l * diag(d) * l<sup>T</sup></code>.
	const matrix_t *A = Args[0].Val;
	int N = A->NoOfRows.Value;
	if (N != A->NoOfCols.Value) {
		Result->Val = Std$String$new("Matrix not square");
		return MESSAGE;
	};
	matrix_t *L = Riva$Memory$alloc(sizeof(matrix_t) + N * N * sizeof(Std$Object_t **));
	Math$Vector$t *D = Riva$Memory$alloc(sizeof(Math$Vector$t) + N * sizeof(Std$Object_t **));
	L->Type = T;
	D->Type = Math$Vector$T;
	D->Length.Type = L->NoOfRows.Type = L->NoOfCols.Type = Std$Integer$SmallT;
	D->Length.Value = L->NoOfRows.Value = L->NoOfCols.Value = N;
	Std$Object_t *One = Std$Integer$new_small(1);
	Std$Object_t *Zero = Std$Integer$new_small(0);
	Std$Object_t **AP = A->Entries;
	Std$Object_t **LP = L->Entries;
	Std$Object_t **DP = D->Entries;
	Std$Function_result Result0[1];
	for (int I = 0; I < N * N; ++I) LP[I] = Zero;
	for (int I = 0; I < N; ++I) LP[N * I + I] = One;
	for (int I = 0; I < N; ++I) {
		Std$Object_t **DV = DP + I;
		DV[0] = AP[N * I + I];
		for (int K = 0; K < I; ++K) {
			Std$Function$call($MUL, 2, Result0, LP[N * I + K], 0, LP[N * I + K], 0);
			Std$Function$call($MUL, 2, Result0, Result0->Val, 0, DP[K], 0);
			Std$Function$call($SUB, 2, Result0, DV[0], 0, Result0->Val, 0);
			DV[0] = Result0->Val;
		};
		for (int J = I + 1; J < N; ++J) {
			Std$Object_t **LV = LP + N * J + I;
			LV[0] = AP[N * J + I];
			for (int M = 0; M < I; ++M) {
				Std$Function$call($MUL, 2, Result0, LP[N * J + M], 0, LP[N * I + M], 0);
				Std$Function$call($MUL, 2, Result0, Result0->Val, 0, DP[M], 0);
				Std$Function$call($SUB, 2, Result0, LV[0], 0, Result0->Val, 0);
				LV[0] = Result0->Val;
			};
			Std$Function$call($DIV, 2, Result0, LV[0], 0, DV[0], 0);
			LV[0] = Result0->Val;
		};
	};
	Args[1].Ref[0] = L;
	Args[2].Ref[0] = D;
	return SUCCESS;
};

