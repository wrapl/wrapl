#include <Std.h>
#include "itex2MML.h"

GLOBAL_FUNCTION(ToMathML, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *MathML = itex2MML_parse(
		Std$String$flatten(Args[0].Val),
		Std$String$get_length(Args[0].Val)
	);
	if (MathML) {
		RETURN(Std$String$new(MathML));
	} else {
		SEND(Std$String$new("iTeX Parse Error"));
	}
}
