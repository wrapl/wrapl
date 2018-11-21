#include <Std.h>
#include <Riva/Memory.h>
#include <Math/Vector.h>
#include <Math/Random.h>
#include <Util/TypedFunction.h>

typedef Math$Vector_t vector_t;

TYPE(T);

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
SYMBOL($HASH, "#");
SYMBOL($COMP, "?");

GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	uint32_t Length = ((Std$Integer_smallt *)Args[0].Val)->Value;
	vector_t *Vector = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	Vector->Type = T;
	Vector->Length.Type = Std$Integer$SmallT;
	Vector->Length.Value = Length;
	Std$Function_argument *Arg = Args;
	Std$Object_t **Ptr = Vector->Entries;
	if (Count == 1) {
		for (int I = Length; --I >= 0;) *(Ptr++) = Std$Object$Nil;
	} else if (Length != Count - 1) {
		Std$Object_t **Limit = Ptr + Length;
		for (int I = Count; --I;) *(Ptr++) = (++Arg)->Val;
		while (Ptr < Limit) *(Ptr++) = Arg->Val;
	} else {            
		for (int I = Count; --I;) *(Ptr++) = (++Arg)->Val;    
	};
	Result->Val = Vector;
	return SUCCESS;
};

GLOBAL_FUNCTION(Make, 0) {
	vector_t *Vector = Riva$Memory$alloc(sizeof(vector_t) + Count * sizeof(Std$Object_t **));
	Vector->Type = T;
	Vector->Length.Type = Std$Integer$SmallT;
	Vector->Length.Value = Count;
	Std$Function_argument *Arg = Args;
	Std$Object_t **Ptr = Vector->Entries;
	for (int I = Count; I; --I) *(Ptr++) = (Arg++)->Val;    
	Result->Val = Vector;
	return SUCCESS;
};

METHOD("length", TYP, T) {
	const vector_t *Vector = Args[0].Val;
	Result->Val = &Vector->Length;
	return SUCCESS;
};

METHOD("copy", TYP, T) {
	const vector_t *Original = Args[0].Val;
	vector_t *Copy = Riva$Memory$alloc(sizeof(vector_t) + Original->Length.Value * sizeof(Std$Object_t **));
	memcpy(Copy, Original, sizeof(vector_t) + Original->Length.Value * sizeof(Std$Object_t **));
	Result->Val = (Std$Object$t *)Copy;
	return SUCCESS;
};

METHOD("@", TYP, T, VAL, Std$String$T) {
	const vector_t *Vector = Args[0].Val;
	if (Vector->Length.Value == 0) {
		Result->Val = LeftRightBracket;
		return SUCCESS;
	};
	Std$Object_t **Ptr = Vector->Entries;
	Std$Function_result Buffer;
	Std$String_t *Final = LeftBracket;
	if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
		Final = Std$String$add(Final, Buffer.Val);
	} else {
		Final = Std$String$add(Final, ValueString);
	};
	for (int J = 1; J < Vector->Length.Value; ++J) {
		Final = Std$String$add(Final, CommaSpace);
		if (Std$Function$call($AT, 2, &Buffer, *(Ptr++), 0, Std$String$T, 0) < FAILURE) {
			Final = Std$String$add(Final, Buffer.Val);
		} else {
			Final = Std$String$add(Final, ValueString);
		};
	};
	Result->Val = Std$String$add(Final, RightBracket);
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
	const vector_t *A = Args[0].Val;
	const vector_t *B = Args[1].Val;
	uint32_t Length = A->Length.Value;
	if (Length != B->Length.Value) {
		Result->Val = Std$String$new("Vectors not of equal length");
		return MESSAGE;
	};
	vector_t *C = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	C->Type = T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = Length;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = Length; --I >= 0;) {
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
	const vector_t *A = Args[0].Val;
	const vector_t *B = Args[1].Val;
	uint32_t Length = A->Length.Value;
	if (Length != B->Length.Value) {
		Result->Val = Std$String$new("Vectors not of equal length");
		return MESSAGE;
	};
	vector_t *C = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	C->Type = T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = Length;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = Length; --I >= 0;) {
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
	const vector_t *A = Args[0].Val;
	const vector_t *B = Args[1].Val;
	uint32_t Length = A->Length.Value;
	if (Length != B->Length.Value) {
		Result->Val = Std$String$new("Vectors not of equal length");
		return MESSAGE;
	};
	if (Length == 0) return SUCCESS;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	Std$Function_result Total;
	switch (Std$Function$call($MUL, 2, &Total, *(AP++), 0, *(BP++), 0)) {
	case SUSPEND: case SUCCESS:
		break;
	case FAILURE:
		Result->Val = Std$String$new(":\"*\" failed to return a result");
		return MESSAGE;
	case MESSAGE:
		Result->Val = Total.Val;
		return MESSAGE;
	};
	for (int I = Length; --I > 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($MUL, 2, &Buffer, *(AP++), 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"*\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
		switch (Std$Function$call($ADD, 2, &Total, Total.Val, 0, Buffer.Val, 0)) {
		case SUSPEND: case SUCCESS:
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"*\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Total.Val;
			return MESSAGE;
		};
	};
	Result->Val = Total.Val;
	return SUCCESS;
};

METHOD("*", TYP, T, ANY) {
	const vector_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t Length = A->Length.Value;
	vector_t *C = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	C->Type = T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = Length;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = Length; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($MUL, 2, &Buffer, *(AP++), 0, B, 0)) {
		case SUSPEND: case SUCCESS:
			*(CP++) = Buffer.Val;
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"*\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("*", ANY, TYP, T) {
	const Std$Object_t *A = Args[0].Val;
	const vector_t *B = Args[1].Val;
	uint32_t Length = B->Length.Value;
	vector_t *C = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	C->Type = T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = Length;
	Std$Object_t **BP = B->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = Length; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($MUL, 2, &Buffer, A, 0, *(BP++), 0)) {
		case SUSPEND: case SUCCESS:
			*(CP++) = Buffer.Val;
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"*\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

METHOD("/", TYP, T, ANY) {
	const vector_t *A = Args[0].Val;
	const Std$Object_t *B = Args[1].Val;
	uint32_t Length = A->Length.Value;
	vector_t *C = Riva$Memory$alloc(sizeof(vector_t) + Length * sizeof(Std$Object_t **));
	C->Type = T;
	C->Length.Type = Std$Integer$SmallT;
	C->Length.Value = Length;
	Std$Object_t **AP = A->Entries;
	Std$Object_t **CP = C->Entries;
	for (int I = Length; --I >= 0;) {
		Std$Function_result Buffer;
		switch (Std$Function$call($DIV, 2, &Buffer, *(AP++), 0, B, 0)) {
		case SUSPEND: case SUCCESS:
			*(CP++) = Buffer.Val;
			break;
		case FAILURE:
			Result->Val = Std$String$new(":\"/\" failed to return a result");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
	};
	Result->Val = C;
	return SUCCESS;
};

SYMBOL($is0, "is0");

TYPED_INSTANCE(int, Std$Number$is0, T, vector_t *A) {
	const Std$Object_t **AP = A->Entries;
	for (int I = A->Length.Value; --I >= 0;) {
		if (!Std$Number$is0(*(AP++))) return 0;
	};
	return 1;
};

METHOD("is0", TYP, T) {
	const vector_t *A = Args[0].Val;
	const Std$Object_t **AP = A->Entries;
	for (int I = A->Length.Value; --I >= 0;) {
		if (!Std$Number$is0(*(AP++))) return FAILURE;
	};
	Result->Val = A;
	return SUCCESS;
};

METHOD("[]", TYP, T, TYP, Std$Integer$SmallT) {
	const vector_t *A = Args[0].Val;
	Std$Integer_smallt *B = Args[1].Val;
    Result->Val = *(Result->Ref = A->Entries + B->Value - 1);
    return SUCCESS;
};

METHOD("x", TYP, T) {
	const vector_t *A = Args[0].Val;
    Result->Val = *(Result->Ref = A->Entries);
    return SUCCESS;
};

METHOD("y", TYP, T) {
	const vector_t *A = Args[0].Val;
    Result->Val = *(Result->Ref = A->Entries + 1);
    return SUCCESS;
};

METHOD("z", TYP, T) {
	const vector_t *A = Args[0].Val;
    Result->Val = *(Result->Ref = A->Entries + 2);
    return SUCCESS;
};

GLOBAL_METHOD(Hash, 1, "#", TYP, T) {
	const vector_t *A = Args[0].Val;
	int Hash = 0;
	Std$Object_t **AP = A->Entries;
	for (int I = A->Length.Value; --I >= 0;) {
		Std$Function_result Buffer;
		Std$Function$call($HASH, 1, &Buffer, *(AP++), 0);
		Hash ^= ((Std$Integer_smallt *)Buffer.Val)->Value;
		asm("roll $2, %0" : "=r" (Hash) : "0" (Hash));
	};
	Result->Val = Std$Integer$new_small(Hash);
	return SUCCESS;
};

GLOBAL_METHOD(Compare, 2, "?", TYP, T, TYP, T) {
	const vector_t *A = Args[0].Val;
	const vector_t *B = Args[1].Val;
	if (A->Length.Value < B->Length.Value) {
		Result->Val = Std$Object$Less;
		return SUCCESS;
	};
	if (A->Length.Value > B->Length.Value) {
		Result->Val = Std$Object$Greater;
		return SUCCESS;
	};
	Std$Object_t **AP = A->Entries;
	Std$Object_t **BP = B->Entries;
	for (int I = A->Length.Value; --I >= 0;) {
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

