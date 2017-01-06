#include <Std.h>
#include <Riva/Memory.h>
#include <Util/TypedFunction.h>

SYMBOL($is0, "is0");
SYMBOL($is1, "is1");

TYPED_FUNCTION(int, _is0, Std$Object_t *A) {
	Std$Function_result Result;
	return Std$Function$call($is0, 1, &Result, A, 0) <= SUCCESS;
};

TYPED_FUNCTION(int, _is1, Std$Object_t *A) {
	Std$Function_result Result;
	return Std$Function$call($is1, 1, &Result, A, 0) <= SUCCESS;
};
