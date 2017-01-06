#include <Std.h>
#include <Util/TypedFunction.h>

extern Std$Type_t T[];

TYPED_INSTANCE(int, Std$Number$is0, T, Std$Real_t *A) {
	return A->Value == 0.0;
};

TYPED_INSTANCE(int, Std$Number$is1, T, Std$Real_t *A) {
	return A->Value == 1.0;
};

SYMBOL($AS, "@");

TYPED_FUNCTION(double, _double, Std$Object_t *A) {
	return 0.0 / 0.0;
};

TYPED_INSTANCE(double, _double, T, Std$Real$t *A) {
	return A->Value;
};

TYPED_INSTANCE(int, Std$Integer$int, T, Std$Real$t *A) {
	return A->Value;
};

#ifdef DOCUMENTING

#define Std$Real$T T
#define REAL_METHOD METHOD

PUSHFILE("Methods2.c");
#include "Methods2.c"
POPFILE();

#endif
