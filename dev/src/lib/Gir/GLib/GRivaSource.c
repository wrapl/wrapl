#include <Std.h>
#include <Riva.h>
#include <Gir/GLib/Source.h>
#include <pthread.h>

typedef struct event_t event_t;

struct event_t {
	struct event_t *Next;
	Std$Object$t *Function;
};

typedef struct GRivaSource {
	GSource Base;
	event_t *Head, **Tail;
	pthread_mutex_t Mutex[1];
	GPollFD Read[1];
	int Write;
} GRivaSource;

static gboolean griva_prepare(GRivaSource *Source, gint *Timeout) {
	*Timeout = -1;
	return FALSE;
};

static gboolean griva_check(GRivaSource *Source) {
	return !!(Source->Read->revents & G_IO_IN);
};

static gboolean griva_dispatch(GRivaSource *Source, GSourceFunc Callback, gpointer Data) {
	char Ignore;
	read(Source->Read->fd, &Ignore, 1);
	pthread_mutex_lock(Source->Mutex);
	event_t *Event = Source->Head;
	if (!(Source->Head = Event->Next)) Source->Tail = &Source->Head;
	pthread_mutex_unlock(Source->Mutex);
	Std$Function$result Result;
	Std$Function$call(Event->Function, 0, &Result);
	return TRUE;
};

static GSourceFuncs Funcs = {
	.prepare = griva_prepare,
	.check = griva_check,
	.dispatch = griva_dispatch,
	.finalize = 0
};

typedef struct grivasource_t {
	Std$Type$t *Type;
	GRivaSource *Handle;
} grivasource_t;

TYPE(T, Gir$GLib$Source$T);

GLOBAL_FUNCTION(New, 0) {
	grivasource_t *Source = new(grivasource_t);
	Source->Type = T;
	Source->Handle = g_source_new(&Funcs, sizeof(GRivaSource));
	int Pipe[2];
	pipe(Pipe);
	Source->Handle->Write = Pipe[1];
	GPollFD *Read = Source->Handle->Read;
	Read->fd = Pipe[0];
	Read->events = G_IO_IN;
	pthread_mutex_init(Source->Handle->Mutex, NULL);
	Source->Handle->Head = 0;
	Source->Handle->Tail = &Source->Handle->Head;
	g_source_add_poll(Source->Handle, Read);
	Result->Val = Source;
	return SUCCESS;
};

METHOD("send", TYP, T, ANY) {
	GRivaSource *Source = ((grivasource_t *)Args[0].Val)->Handle;
	event_t *Event = new(event_t);
	Event->Function = Args[1].Val;
	pthread_mutex_lock(Source->Mutex);
	Source->Tail[0] = Event;
	Source->Tail = &Event->Next;
	if (!Source->Head) Source->Head = Event;
	pthread_mutex_unlock(Source->Mutex);
	write(Source->Write, " ", 1);
	Result->Arg = Args[0];
	return SUCCESS;
};
