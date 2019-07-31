#include <Std.h>
#include <Riva.h>
#include <Gtk/Glib/GSource.h>

typedef struct GRivaSource {
	GSource Base;
	GPollFD Read[1];
} GRivaSource;

static gboolean griva_prepare(GRivaSource *Source, gint *Timeout) {
	*Timeout = -1;
	return FALSE;
};

static gboolean griva_check(GRivaSource *Source) {
	return !!(Source->Read->revents & G_IO_IN);
};

static gboolean griva_dispatch(GRivaSource *Source, GSourceFunc Callback, gpointer Data) {
	Std$Function$t *Function;
	if (read(Source->Read->fd, &Function, sizeof(Function)) == sizeof(Function)) {
		Std$Function$result Result;
		Std$Function$call(Function, 0, &Result);
	};
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
	GRivaSource *Value;
	int Write;
} grivasource_t;

TYPE(T, Gtk$Glib$GSource$T);

GLOBAL_FUNCTION(New, 0) {
	grivasource_t *Source = new(grivasource_t);
	Source->Type = T;
	Source->Value = g_source_new(&Funcs, sizeof(GRivaSource));
	int Pipe[2];
	pipe(Pipe);
	Source->Write = Pipe[1];
	GPollFD *Read = Source->Value->Read;
	Read->fd = Pipe[0];
	Read->events = G_IO_IN;
	g_source_add_poll(Source->Value, Read);
	Result->Val = Source;
	return SUCCESS;
};

METHOD("Send", TYP, T, ANY) {
	grivasource_t *Source = Args[0].Val;
	Std$Function$t *Function = Args[1].Val;
	write(Source->Write, &Function, sizeof(Function));
	Result->Arg = Args[0];
	return SUCCESS;
};
