#include <Std.h>
#include <Riva/Config.h>

GLOBAL_FUNCTION(Get, 1) {
//@key : Std$String$T
//: Std$String$T
// Returns the string value associated with <var>key</var> if it is defined, fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Value = Riva$Config$get(Std$String$flatten(Args[0].Val));
	if (Value) {
		Result->Val = Std$String$new(Value);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

GLOBAL_FUNCTION(Set, 1) {
//@key:Std$String$T
//@value:Std$String$T=&quot;&quot;
// Defines <var>key</var> with associated value <var>value</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	if (Count > 1) {
		CHECK_EXACT_ARG_TYPE(1, Std$String$T);
		Riva$Config$set(Std$String$flatten(Args[0].Val), Std$String$flatten(Args[1].Val));
	} else {
		Riva$Config$set(Std$String$flatten(Args[0].Val), "");
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(UnSet, 1) {
//@key : Std$String$T
// Undefines <var>key</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Riva$Config$set(Std$String$flatten(Args[0].Val), 0);
	return SUCCESS;
};
