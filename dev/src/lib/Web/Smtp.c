#include <Riva/Memory.h>
#include <Std.h>
#include <Sys/Time.h>
#include <IO/Stream.h>
#include <libesmtp.h>
#include <auth-client.h>

typedef struct session_t {
	const Std$Type$t *Type;
	smtp_session_t Handle;
} session_t;

TYPE(T);

typedef struct message_t {
	const Std$Type$t *Type;
	smtp_message_t Handle;
} message_t;

TYPE(MessageT);

typedef struct authcontext_t {
	const Std$Type$t *Type;
	auth_context_t Handle;
} authcontext_t;

SYMBOL($server, "server");
SYMBOL($hostname, "hostname");

GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Symbol$ArrayT);
	session_t *Session = new(session_t);
	Session->Type = T;
	smtp_session_t Handle = Session->Handle = smtp_create_session();
	Std$Symbol$array *Options = (Std$Symbol$array *)Args[0].Val;
	for (int I = 0; I < Options->Count; ++I) {
		if (Options->Values[I] == (Std$Symbol$t *)$server) {
			smtp_set_server(Handle, Std$String$flatten(Args[1 + I].Val));
		} else if (Options->Values[I] == (Std$Symbol$t *)$hostname) {
			smtp_set_hostname(Handle, Std$String$flatten(Args[1 + I].Val));
		};
	};
	Result->Val = (Std$Object$t *)Session;
	return SUCCESS;
};

METHOD("start", TYP, T) {
	session_t *Session = (session_t *)Args[0].Val;
	smtp_start_session(Session->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("destroy", TYP, T) {
	session_t *Session = (session_t *)Args[0].Val;
	smtp_destroy_session(Session->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
};

SYMBOL($disabled, "disabled");
SYMBOL($enabled, "enabled");
SYMBOL($required, "required");

METHOD("starttls", TYP, T, TYP, Std$Symbol$T) {
	session_t *Session = (session_t *)Args[0].Val;
	if (Args[1].Val == $disabled) {
		smtp_starttls_enable(Session->Handle, Starttls_DISABLED);
	} else if (Args[1].Val == $enabled) {
		smtp_starttls_enable(Session->Handle, Starttls_ENABLED);
	} else if (Args[1].Val == $required) {
		smtp_starttls_enable(Session->Handle, Starttls_REQUIRED);
	}
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("message", TYP, T) {
	session_t *Session = (session_t *)Args[0].Val;
	message_t *Message = new(message_t);
	Message->Type = MessageT;
	Message->Handle = smtp_add_message(Session->Handle);
	Result->Val = (Std$Object$t *)Message;
	return SUCCESS;
};

METHOD("from", TYP, MessageT, TYP, Std$String$T) {
	message_t *Message = (message_t *)Args[0].Val;
	smtp_set_reverse_path(Message->Handle, Std$String$flatten(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("to", TYP, MessageT, TYP, Std$String$T) {
	message_t *Message = (message_t *)Args[0].Val;
	smtp_add_recipient(Message->Handle, Std$String$flatten(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("header", TYP, MessageT, TYP, Std$String$T, ANY) {
	message_t *Message = (message_t *)Args[0].Val;
	const char *Header = Std$String$flatten(Args[1].Val);
	if (Args[2].Val == Std$Object$Nil) {
		smtp_set_header(Message->Handle, Header, NULL);
	} else if (Args[2].Val->Type == Std$String$T) {
		smtp_set_header(Message->Handle, Header, Std$String$flatten(Args[2].Val));
	} else if (Args[2].Val->Type == Sys$Time$T) {
		smtp_set_header(Message->Handle, Header, ((Sys$Time$t *)Args[2].Val)->Value);
	} else {
		Result->Val = Std$String$new("Unknown header type");
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("body", TYP, MessageT, TYP, Std$String$T) {
	message_t *Message = (message_t *)Args[0].Val;
	smtp_set_message_str(Message->Handle, Std$String$flatten(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

static const char *message_stream_cb(void **BufferOut, int *LengthOut, Std$Object$t *Stream) {
	if (LengthOut == 0) {
		///TODO: handle rewind case (Length == 0)
	} else {
		char *Buffer = *BufferOut = Riva$Memory$alloc_atomic(128);
		int Length = *LengthOut = IO$Stream$read(Stream, Buffer, 128, 0);
		return (Length > 0) ? Buffer : 0;
	};
};

METHOD("body", TYP, MessageT, TYP, IO$Stream$T) {
	message_t *Message = (message_t *)Args[0].Val;
	smtp_set_messagecb(Message->Handle, message_stream_cb, Args[1].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};
