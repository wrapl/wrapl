#include <Std.h>
#include <Riva.h>
#include <Num/Bitset.h>

#include "roaring.c"

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	Num$Bitset$t *Bitset = new(Num$Bitset$t);
	Bitset->Type = T;
	Bitset->Value = roaring_bitmap_create();
	RETURN(Bitset);
}

METHOD("copy", TYP, T) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *Copy = new(Num$Bitset$t);
	Copy->Type = T;
	Copy->Value = roaring_bitmap_copy(Bitset->Value);
	RETURN(Copy);
}

METHOD("insert", TYP, T, TYP, Std$Integer$SmallT) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	int Bit = Std$Integer$get_small(Args[1].Val);
	if (roaring_bitmap_add_checked(Bitset->Value, Bit)) {
		RETURN0;
	} else {
		FAIL;
	}
}

METHOD("delete", TYP, T, TYP, Std$Integer$SmallT) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	int Bit = Std$Integer$get_small(Args[1].Val);
	if (roaring_bitmap_remove_checked(Bitset->Value, Bit)) {
		RETURN0;
	} else {
		FAIL;
	}
}

METHOD("in", TYP, T, TYP, Std$Integer$SmallT) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	int Bit = Std$Integer$get_small(Args[1].Val);
	if (roaring_bitmap_contains(Bitset->Value, Bit)) {
		RETURN0;
	} else {
		FAIL;
	}
}

METHOD("size", TYP, T) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	RETURN(Std$Integer$new_small(roaring_bitmap_get_cardinality(Bitset->Value)));
}

METHOD("=", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	if (roaring_bitmap_equals(A->Value, B->Value)) {
		RETURN1;
	} else {
		FAIL;
	}
}

METHOD("~=", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	if (!roaring_bitmap_equals(A->Value, B->Value)) {
		RETURN1;
	} else {
		FAIL;
	}
}

METHOD("+", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	Num$Bitset$t *C = new(Num$Bitset$t);
	C->Type = T;
	C->Value = roaring_bitmap_or(A->Value, B->Value);
	RETURN(C);
}

METHOD("*", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	Num$Bitset$t *C = new(Num$Bitset$t);
	C->Type = T;
	C->Value = roaring_bitmap_and(A->Value, B->Value);
	RETURN(C);
}

METHOD("-", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	Num$Bitset$t *C = new(Num$Bitset$t);
	C->Type = T;
	C->Value = roaring_bitmap_andnot(A->Value, B->Value);
	RETURN(C);
}

METHOD("^", TYP, T, TYP, T) {
	Num$Bitset$t *A = (Num$Bitset$t *)Args[0].Val;
	Num$Bitset$t *B = (Num$Bitset$t *)Args[1].Val;
	Num$Bitset$t *C = new(Num$Bitset$t);
	C->Type = T;
	C->Value = roaring_bitmap_xor(A->Value, B->Value);
	RETURN(C);
}

typedef struct generator_t {
	Std$Function_cstate State;
	roaring_uint32_iterator_t Iterator[1];
} generator_t;

static Std$Function$status resume_generator(Std$Function$result *Result) {
	generator_t *Generator = Result->State;
	if (roaring_advance_uint32_iterator(Generator->Iterator)) {
		Result->Val = Std$Integer$new_small(Generator->Iterator->current_value);
		return SUSPEND;
	} else {
		return FAILURE;
	}
}

METHOD("values", TYP, T) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	if (roaring_bitmap_is_empty(Bitset->Value)) FAIL;
	generator_t *Generator = new(generator_t);
	Generator->State.Invoke = resume_generator;
	roaring_init_iterator(Bitset->Value, Generator->Iterator);
	Result->Val = Std$Integer$new_small(Generator->Iterator->current_value);
	Result->State = Generator;
	return SUSPEND;
}

STRING(LeftBrace, "{");
STRING(RightBrace, "}");

static bool to_string_iterator(uint32_t Value, Std$Object$t **String) {
	if (*String != LeftBrace) {
		*String = Std$String$add_format(*String, ", %d", Value);
	} else {
		*String = Std$String$add_format(*String, "%d", Value);
	}
	return true;
}

METHOD("@", TYP, T, VAL, Std$String$T) {
	Num$Bitset$t *Bitset = (Num$Bitset$t *)Args[0].Val;
	Std$Object$t *String = LeftBrace;
	roaring_iterate(Bitset->Value, to_string_iterator, &String);
	String = Std$String$add(String, RightBrace);
	RETURN(String);
}

