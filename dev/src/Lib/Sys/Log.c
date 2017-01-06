#include <Std.h>
#include <Riva/Log.h>

GLOBAL_FUNCTION(Enable, 0) {
    Riva$Log$enable();
    return SUCCESS;
};