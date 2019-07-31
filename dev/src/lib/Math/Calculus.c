#include <Std.h>

GLOBAL_FUNCTION(Minimize, 2) {
	size_t NumParams = Std$Integer$get_small(Args[0].Val);
	Std$Object$t *Function = Args[1].Val;
	
	return SUCCESS;
};
