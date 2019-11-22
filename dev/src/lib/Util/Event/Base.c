#include <Std.h>
#include <IO/Native.h>
#include <Riva/Memory.h>
#include <Sys/Time.h>
#include <Util/Event/Base.h>
#include <event2/event.h>

typedef Util$Event$Base$t event_base_t;

TYPE(T);

typedef struct event_t {
	const Std$Type$t *Type;
	struct event *Handle;
	IO$Native$(t) *Stream;
	Std$Object$t *Callback;
} event_t;

TYPE(EventT);

Std$Integer$smallt EventFlagTimeout[] = {{Std$Integer$SmallT, EV_TIMEOUT}};
Std$Integer$smallt EventFlagRead[] = {{Std$Integer$SmallT, EV_READ}};
Std$Integer$smallt EventFlagWrite[] = {{Std$Integer$SmallT, EV_WRITE}};
Std$Integer$smallt EventFlagSignal[] = {{Std$Integer$SmallT, EV_SIGNAL}};
Std$Integer$smallt EventFlagPersist[] = {{Std$Integer$SmallT, EV_PERSIST}};
Std$Integer$smallt EventFlagET[] = {{Std$Integer$SmallT, EV_ET}};
Std$Integer$smallt EventFlagFinalize[] = {{Std$Integer$SmallT, EV_FINALIZE}};
Std$Integer$smallt EventFlagClosed[] = {{Std$Integer$SmallT, EV_CLOSED}};

Std$Integer$smallt LoopFlagOnce[] = {{Std$Integer$SmallT, EVLOOP_ONCE}};
Std$Integer$smallt LoopFlagNonBlock[] = {{Std$Integer$SmallT, EVLOOP_NONBLOCK}};
Std$Integer$smallt LoopFlagNoExitOnEmpty[] = {{Std$Integer$SmallT, EVLOOP_NO_EXIT_ON_EMPTY}};

GLOBAL_FUNCTION(New, 0) {
	event_base_t *EventBase = new(event_base_t);
	EventBase->Type = T;
	EventBase->Handle = event_base_new();
	RETURN(EventBase);
}

METHOD("loop", TYP, T) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	switch (event_base_dispatch(EventBase->Handle)) {
	case 0: RETURN0;
	case 1: FAIL;
	default: SEND(Std$String$new("Error dispatching events"));
	}
}

METHOD("loop", TYP, T, TYP, Std$Integer$SmallT) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	int Flags = Std$Integer$get_small(Args[1].Val);
	switch (event_base_loop(EventBase->Handle, Flags)) {
	case 0: RETURN0;
	case 1: FAIL;
	default: SEND(Std$String$new("Error dispatching events"));
	}
}

METHOD("break", TYP, T) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	event_base_loopbreak(EventBase->Handle);
	RETURN0;
}

METHOD("continue", TYP, T) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	event_base_loopcontinue(EventBase->Handle);
	RETURN0;
}

METHOD("exit", TYP, T, TYP, Sys$Time$PreciseT) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[1].Val;
	event_base_loopexit(EventBase->Handle, &Time->Value);
	RETURN0;
}

METHOD("dump_events", TYP, T) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	event_base_dump_events(EventBase->Handle, stderr);
	RETURN0;
}

static void riva_event_callback_fn(evutil_socket_t Socket, short Flags, void *Data) {
	event_t *Event = (event_t *)Data;
	Std$Function$result Result[1];
	Std$Function$argument Args[2] = {
		{Event->Stream, 0},
		{Std$Integer$new_small(Flags), 0}
	};
	Std$Function$invoke(Event->Callback, 2, Result, Args);
}

METHOD("once", TYP, T, TYP, IO$Native$(T), TYP, Std$Integer$SmallT, TYP, Sys$Time$PreciseT, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	IO$Native$(t) *Stream = (IO$Native$(t) *)Args[1].Val;
	short Flags = Std$Integer$get_small(Args[2].Val);
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[3].Val;
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Stream = Stream;
	Event->Callback = Args[4].Val;
	event_base_once(EventBase->Handle, Stream->Handle, Flags, riva_event_callback_fn, Event, &Time->Value);
	RETURN(Event);
}

METHOD("once", TYP, T, TYP, Std$Integer$SmallT, TYP, Sys$Time$PreciseT, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	short Flags = Std$Integer$get_small(Args[1].Val);
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[2].Val;
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Stream = Std$Object$Nil;
	Event->Callback = Args[3].Val;
	event_base_once(EventBase->Handle, -1, Flags, riva_event_callback_fn, Event, &Time->Value);
	RETURN(Event);
}

METHOD("once", TYP, T, TYP, IO$Native$(T), TYP, Std$Integer$SmallT, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	IO$Native$(t) *Stream = (IO$Native$(t) *)Args[1].Val;
	short Flags = Std$Integer$get_small(Args[2].Val);
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Stream = Stream;
	Event->Callback = Args[3].Val;
	event_base_once(EventBase->Handle, Stream->Handle, Flags, riva_event_callback_fn, Event, NULL);
	RETURN(Event);
}

METHOD("new", TYP, T, TYP, IO$Native$(T), TYP, Std$Integer$SmallT, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	IO$Native$(t) *Stream = (IO$Native$(t) *)Args[1].Val;
	short Flags = Std$Integer$get_small(Args[2].Val);
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Stream = Stream;
	Event->Callback = Args[3].Val;
	Event->Handle = event_new(EventBase->Handle, Stream->Handle, Flags, riva_event_callback_fn, Event);
	RETURN(Event);
}

METHOD("new", TYP, T, TYP, Std$Integer$SmallT, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	short Flags = Std$Integer$get_small(Args[1].Val);
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Handle = event_new(EventBase->Handle, -1, Flags, riva_event_callback_fn, Event);
	Event->Stream = Std$Object$Nil;
	Event->Callback = Args[2].Val;
	RETURN(Event);
}

METHOD("new", TYP, T, ANY) {
	event_base_t *EventBase = (event_base_t *)Args[0].Val;
	event_t *Event = new(event_t);
	Event->Type = EventT;
	Event->Handle = event_new(EventBase->Handle, -1, 0, riva_event_callback_fn, Event);
	Event->Stream = Std$Object$Nil;
	Event->Callback = Args[1].Val;
	RETURN(Event);
}

METHOD("add", TYP, EventT) {
	event_t *Event = (event_t *)Args[0].Val;
	event_add(Event->Handle, NULL);
	RETURN0;
}

METHOD("add", TYP, EventT, TYP, Sys$Time$PreciseT) {
	event_t *Event = (event_t *)Args[0].Val;
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[1].Val;
	event_add(Event->Handle, &Time->Value);
	RETURN0;
}

METHOD("del", TYP, EventT) {
	event_t *Event = (event_t *)Args[0].Val;
	event_del(Event->Handle);
	RETURN0;
}

METHOD("active", TYP, EventT, TYP, Std$Integer$SmallT) {
	event_t *Event = (event_t *)Args[0].Val;
	int Res = Std$Integer$get_small(Args[2].Val);
	event_active(Event->Handle, Res, 0);
	RETURN0;
}

static void nop_free(void *Ptr) {}

INITIAL() {
	event_set_mem_functions(Riva$Memory$alloc, Riva$Memory$realloc, nop_free);
}
