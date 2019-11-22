#include <Std.h>
#include <Riva.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <Util/Event/Base.h>

typedef struct redis_async_t {
	const Std$Type$t *Type;
	Std$Function$status (*Invoke)(FUNCTION_PARAMS);
	redisAsyncContext *Handle;
} redis_async_t;

extern Std$Type$t DB$Redis$AsyncT[];

METHOD("attach", TYP, DB$Redis$AsyncT, TYP, Util$Event$Base$T) {
	redis_async_t *Redis = (redis_async_t *)Args[0].Val;
	Util$Event$Base$t *EventBase = (Util$Event$Base$t *)Args[1].Val;
	redisLibeventAttach(Redis->Handle, EventBase->Handle);
	RETURN0;
}
