#include <Std.h>
#include <Riva.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/glib.h>
#include <Gir/GLib/Source.h>

typedef struct redis_async_t {
	const Std$Type$t *Type;
	Std$Function$status (*Invoke)(FUNCTION_PARAMS);
	redisAsyncContext *Handle;
} redis_async_t;

extern Std$Type$t DB$Redis$AsyncT[];

METHOD("gsource", TYP, DB$Redis$AsyncT) {
	redis_async_t *Redis = (redis_async_t *)Args[0].Val;
	Gir$GLib$Source$t *Source = new(Gir$GLib$Source$t);
	Source->Type = Gir$GLib$Source$T;
	Source->Value = redis_source_new(Redis->Handle);
	RETURN(Source);
}
