#include <Std.h>
#include <Riva/Memory.h>
#include <Sys/Signal.h>
#include <IO/Posix.h>
#include <sys/epoll.h>

typedef struct poll_t {
	const Std$Type$t *Type;
#ifdef LINUX
	int Handle;
#endif
} poll_t;

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	poll_t *Poll = new(poll_t);
	Poll->Type = T;
#ifdef LINUX
	Poll->Handle = epoll_create(1);
#endif
	Result->Val = (Std$Object$t *)Poll;
	return SUCCESS;
};

#ifdef LINUX

typedef struct event_t {
	const Std$Type$t *Type;
	struct epoll_event Value[1];
} event_t;

TYPE(EventT);

TYPE(PollMessageT, IO$Stream$MessageT);

Std$Integer$smallt Event_EPOLLIN[1] = {{Std$Integer$SmallT, EPOLLIN}};
Std$Integer$smallt Event_EPOLLOUT[1] = {{Std$Integer$SmallT, EPOLLOUT}};
Std$Integer$smallt Event_EPOLLRDHUP[1] = {{Std$Integer$SmallT, EPOLLRDHUP}};
Std$Integer$smallt Event_EPOLLPRI[1] = {{Std$Integer$SmallT, EPOLLPRI}};
Std$Integer$smallt Event_EPOLLERR[1] = {{Std$Integer$SmallT, EPOLLERR}};
Std$Integer$smallt Event_EPOLLHUP[1] = {{Std$Integer$SmallT, EPOLLHUP}};
Std$Integer$smallt Event_EPOLLET[1] = {{Std$Integer$SmallT, EPOLLET}};
Std$Integer$smallt Event_EPOLLONESHOT[1] = {{Std$Integer$SmallT, EPOLLONESHOT}};

GLOBAL_FUNCTION(EventNew, 0) {
	event_t *Event = new(event_t);
	Event->Type = EventT;
	if (Count > 0) {
		CHECK_ARG_TYPE(0, Std$Integer$SmallT);
		Event->Value->events = Std$Integer$get_small(Args[0].Val);
	};
	Event->Value->data.ptr = (Count > 1) ? Args[1].Val : Std$Object$Nil;
	Result->Val = (Std$Object$t *)Event;
	return SUCCESS;
};

METHOD("data", TYP, EventT) {
	event_t *Event = (event_t *)Args[0].Val;
	Result->Ref = (Std$Object$t **)&Event->Value->data.ptr;
	Result->Val = *Result->Ref;
	return SUCCESS;
};

METHOD("events", TYP, EventT) {
	event_t *Event = (event_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Event->Value->events);
	return SUCCESS;
};

METHOD("events", TYP, EventT, TYP, Std$Integer$SmallT) {
	event_t *Event = (event_t *)Args[0].Val;
	Event->Value->events = Std$Integer$get_small(Args[1].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(Add, 3, "add", TYP, T, TYP, IO$Posix$T, TYP, EventT) {
	poll_t *Poll = (poll_t *)Args[0].Val;
	IO$Posix$t *Posix = (IO$Posix$t *)Args[1].Val;
	event_t *Event = (event_t *)Args[2].Val;
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_ADD, Posix->Handle, Event->Value)) {
		Result->Val = Sys$Program$error_from_errno(PollMessageT);
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

int _add(poll_t *Poll, int Handle, int Event) {
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_ADD, Handle, Event)) {
		return 1;
	} else {
		return 0;
	};
};

GLOBAL_METHOD(Modify, 3, "mod", TYP, T, TYP, IO$Posix$T, TYP, EventT) {
	poll_t *Poll = (poll_t *)Args[0].Val;
	IO$Posix$t *Posix = (IO$Posix$t *)Args[1].Val;
	event_t *Event = (event_t *)Args[2].Val;
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_MOD, Posix->Handle, Event->Value)) {
		Result->Val = Sys$Program$error_from_errno(PollMessageT);
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

int _mod(poll_t *Poll, int Handle, int Event) {
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_MOD, Handle, Event)) {
		return 1;
	} else {
		return 0;
	};
};

GLOBAL_METHOD(Delete, 2, "del", TYP, T, TYP, IO$Posix$T) {
	poll_t *Poll = (poll_t *)Args[0].Val;
	IO$Posix$t *Posix = (IO$Posix$t *)Args[1].Val;
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_DEL, Posix->Handle, 0)) {
		Result->Val = Sys$Program$error_from_errno(PollMessageT);
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

int _del(poll_t *Poll, int Handle) {
	if (epoll_ctl(Poll->Handle, EPOLL_CTL_DEL, Handle, 0)) {
		return 1;
	} else {
		return 0;
	};
};

GLOBAL_METHOD(Wait, 3, "wait", TYP, T, TYP, EventT, TYP, Std$Integer$SmallT) {
	poll_t *Poll = (poll_t *)Args[0].Val;
	event_t *Event = (event_t *)Args[1].Val;
	switch (epoll_wait(Poll->Handle, Event->Value, 1, Std$Integer$get_small(Args[2].Val))) {
	case -1: Result->Val = Sys$Program$error_from_errno(PollMessageT); return MESSAGE;
	case 0: return FAILURE;
	default: Result->Arg = Args[1]; return SUCCESS;
	};
};

GLOBAL_METHOD(PWait, 3, "wait", TYP, T, TYP, EventT, TYP, Std$Integer$SmallT, TYP, Sys$Signal$SetT) {
	poll_t *Poll = (poll_t *)Args[0].Val;
	event_t *Event = (event_t *)Args[1].Val;
	Sys$Signal$set_t *Signals = (Sys$Signal$set_t *)Args[3].Val;
	switch (epoll_pwait(Poll->Handle, Event->Value, 1, Std$Integer$get_small(Args[2].Val), Signals->Value)) {
	case -1: Result->Val = Sys$Program$error_from_errno(PollMessageT); return MESSAGE;
	case 0: return FAILURE;
	default: Result->Arg = Args[1]; return SUCCESS;
	};
};

#endif
