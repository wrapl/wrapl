#include <Std.h>
#include <Sys/Module.h>
#include <Riva/Memory.h>
#include <jack/jack.h>

typedef struct client_t {
	const Std$Type_t *Type;
	jack_client_t *Handle;
	jack_status_t Status;
} client_t;

TYPE(ClientT);

typedef struct position_t {
	const Std$Type_t *Type;
	jack_position_t Value[1];
} position_t;

TYPE(PositionT);

TYPE(StatusT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

CONSTANT(Option, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("Option");
	Sys$Module$export(Module, "NullOption", 0, Std$Integer$new_small(JackNullOption));
	Sys$Module$export(Module, "NoStartServer", 0, Std$Integer$new_small(JackNoStartServer));
	Sys$Module$export(Module, "UseExactName", 0, Std$Integer$new_small(JackUseExactName));
	Sys$Module$export(Module, "ServerName", 0, Std$Integer$new_small(JackServerName));
	Sys$Module$export(Module, "LoadName", 0, Std$Integer$new_small(JackLoadName));
	Sys$Module$export(Module, "LoadInit", 0, Std$Integer$new_small(JackLoadInit));
	Sys$Module$export(Module, "SessionID", 0, Std$Integer$new_small(JackSessionID));
	return (Std$Object_t *)Module;
};

CONSTANT(Status, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("Status");
	Sys$Module$export(Module, "Failure", 0, Std$Integer$new_small(JackFailure));
	Sys$Module$export(Module, "InvalidOption", 0, Std$Integer$new_small(JackInvalidOption));
	Sys$Module$export(Module, "NameNotUnique", 0, Std$Integer$new_small(JackNameNotUnique));
	Sys$Module$export(Module, "ServerStarted", 0, Std$Integer$new_small(JackServerStarted));
	Sys$Module$export(Module, "ServerFailed", 0, Std$Integer$new_small(JackServerFailed));
	Sys$Module$export(Module, "ServerError", 0, Std$Integer$new_small(JackServerError));
	Sys$Module$export(Module, "NoSuchClient", 0, Std$Integer$new_small(JackNoSuchClient));
	Sys$Module$export(Module, "LoadFailure", 0, Std$Integer$new_small(JackLoadFailure));
	Sys$Module$export(Module, "InitFailure", 0, Std$Integer$new_small(JackInitFailure));
	Sys$Module$export(Module, "ShmFailure", 0, Std$Integer$new_small(JackShmFailure));
	Sys$Module$export(Module, "VersionError", 0, Std$Integer$new_small(JackVersionError));
	Sys$Module$export(Module, "BackendError", 0, Std$Integer$new_small(JackBackendError));
	Sys$Module$export(Module, "ClientZombie", 0, Std$Integer$new_small(JackClientZombie));
	return (Std$Object_t *)Module;
};

CONSTANT(TransportState, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("TransportState");
	Sys$Module$export(Module, "Stopped", 0, Std$Integer$new_small(JackTransportStopped));
	Sys$Module$export(Module, "Rolling", 0, Std$Integer$new_small(JackTransportRolling));
	Sys$Module$export(Module, "Looping", 0, Std$Integer$new_small(JackTransportLooping));
	Sys$Module$export(Module, "Starting", 0, Std$Integer$new_small(JackTransportStarting));
	return (Std$Object_t *)Module;
};

GLOBAL_FUNCTION(Open, 3) {
	const char *ClientName = Std$String$flatten(Args[0].Val);
	jack_options_t Options = Std$Integer$get_small(Args[1].Val);
	jack_status_t Status = 0;
	jack_client_t *Handle;
	if (Count > 3) {
		if (Args[3].Val->Type == Std$Integer$SmallT) {
			Handle = jack_client_open(ClientName, Options, &Status, Std$Integer$get_small(Args[3].Val));
		} else if (Args[3].Val->Type == Std$String$T) {
			Handle = jack_client_open(ClientName, Options, &Status, Std$String$flatten(Args[3].Val));
		} else {
			Handle = jack_client_open(ClientName, Options, &Status);
		};
	} else {
		Handle = jack_client_open(ClientName, Options, &Status);
	};
	if (Status) {
		if (Args[2].Ref) Args[2].Ref[0] = Std$Integer$new_small(Status);
		return FAILURE;
	} else {
		client_t *Client = new(client_t);
		Client->Type = ClientT;
		Client->Handle = Handle;
		Client->Status = 0;
		Result->Val = (Std$Object_t *)Client;
		return SUCCESS;
	};
};

METHOD("status", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Client->Status);
	return SUCCESS;
};

METHOD("activate", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	Client->Status = jack_activate(Client->Handle);
	return Client->Status ? FAILURE : SUCCESS;
};

METHOD("deactivate", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	Client->Status = jack_deactivate(Client->Handle);
	return Client->Status ? FAILURE : SUCCESS;
};

METHOD("close", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	Client->Status = jack_client_close(Client->Handle);
	return Client->Status ? FAILURE : SUCCESS;
};

METHOD("get_client_name", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	Result->Val = Std$String$new(jack_get_client_name(Client->Handle));
	return SUCCESS;
};

METHOD("connect", TYP, ClientT, TYP, Std$String$T, TYP, Std$String$T) {
	client_t *Client = (client_t *)Args[0].Val;
	Client->Status = jack_connect(Client->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val));
	return Client->Status ? FAILURE : SUCCESS;
};

METHOD("disconnect", TYP, ClientT, TYP, Std$String$T, TYP, Std$String$T) {
	client_t *Client = (client_t *)Args[0].Val;
	Client->Status = jack_disconnect(Client->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val));
	return Client->Status ? FAILURE : SUCCESS;
};

METHOD("transport_locate", TYP, ClientT, TYP, Std$Integer$SmallT) {
	client_t *Client = (client_t *)Args[0].Val;
	Std$Integer_smallt *Frame = (Std$Integer_smallt *)Args[1].Val;
	return jack_transport_locate(Client->Handle, Frame->Value) ? FAILURE : SUCCESS;
};

METHOD("transport_query", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		position_t *Position = new(position_t);
		Position->Type = PositionT;
		Args[1].Ref[0] = (Std$Object_t *)Position;
		Result->Val = Std$Integer$new_small(jack_transport_query(Client->Handle, Position->Value));
		return SUCCESS;
	} else {
		Result->Val = Std$Integer$new_small(jack_transport_query(Client->Handle, 0));
		return SUCCESS;
	};
};

METHOD("transport_start", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	jack_transport_start(Client->Handle);
	return SUCCESS;
};

METHOD("transport_stop", TYP, ClientT) {
	client_t *Client = (client_t *)Args[0].Val;
	jack_transport_stop(Client->Handle);
	return SUCCESS;
};


