#include <IO/Socket.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <Riva/System.h>
#include <Sys/Module.h>
#include <Std.h>

#include <stdarg.h>
#include <stdio.h>

#if defined(LINUX) || defined(CYGWIN)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#else
#endif

TYPE(T, IO$Native$(TextReaderT), IO$Native$(TextWriterT), IO$Native$(ReaderT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
// A handle to one end of a connection
TYPE(LocalT, T, IO$Native$(TextReaderT), IO$Native$(TextWriterT), IO$Native$(ReaderT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
// Sockets for connections between processes on the same machine
TYPE(InetT, T, IO$Native$(TextReaderT), IO$Native$(TextWriterT), IO$Native$(ReaderT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
// Sockets for internet connections


#define ERROR_TYPE(NAME) \
TYPE(NAME ## MessageT, IO$Stream$MessageT, Sys$Program$ErrorT); \
\
TYPED_INSTANCE(const char *, Sys$Program$error_name, NAME ## MessageT, Sys$Program$error_t *Error) { \
	return __stringify(NAME) " error"; \
}

ERROR_TYPE(Create);
ERROR_TYPE(Bind);
ERROR_TYPE(Listen);
ERROR_TYPE(Accept);
ERROR_TYPE(Connect);
ERROR_TYPE(HostNotFound);
ERROR_TYPE(Shutdown);
ERROR_TYPE(Option);
ERROR_TYPE(PeerName);

CONSTANT(Message, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Message");
	Sys$Module$export(Module, "CreateError", 0, (void *)CreateMessageT);
	Sys$Module$export(Module, "BindError", 0, (void *)BindMessageT);
	Sys$Module$export(Module, "ListenError", 0, (void *)ListenMessageT);
	Sys$Module$export(Module, "AcceptError", 0, (void *)AcceptMessageT);
	Sys$Module$export(Module, "ConnectError", 0, (void *)ConnectMessageT);
	Sys$Module$export(Module, "HostNotFoundError", 0, (void *)HostNotFoundMessageT);
	Sys$Module$export(Module, "ShutdownError", 0, (void *)ShutdownMessageT);
	Sys$Module$export(Module, "OptionError", 0, (void *)OptionMessageT);
	Sys$Module$export(Module, "PeerNameError", 0, (void *)PeerNameMessageT);
	return (Std$Object$t *)Module;
};

#if defined(LINUX) || defined(CYGWIN)
Std$Integer$smallt FlagStream[] = {{Std$Object$T, IO$Socket$SOCK_STREAM}};
Std$Integer$smallt FlagDgram[] = {{Std$Object$T, IO$Socket$SOCK_DGRAM}};
Std$Integer$smallt FlagRaw[] = {{Std$Object$T, IO$Socket$SOCK_RAW}};

Std$Integer$smallt FlagInet[] = {{Std$Object$T, IO$Socket$PF_INET}};
Std$Integer$smallt FlagLocal[] = {{Std$Object$T, IO$Socket$PF_LOCAL}};
#else
#endif

GLOBAL_FUNCTION(New, 2) {
//@type
//@stream
//:T
#if defined(LINUX) || defined(CYGWIN)
	int Handle, Style;
	const Std$Type$t *Type;
	if (Args[1].Val == (Std$Object$t *)FlagStream) {
		Style = SOCK_STREAM;
	} else if (Args[1].Val == (Std$Object$t *)FlagDgram) {
		Style = SOCK_DGRAM;
	} else if (Args[1].Val == (Std$Object$t *)FlagRaw) {
		Style = SOCK_RAW;
	} else {
		Result->Val = Sys$Program$error_new_format(CreateMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	if (Args[0].Val == (Std$Object$t *)FlagInet) {
		Handle = socket(PF_INET, Style, 0);
		Type = InetT;
	} else if (Args[0].Val == (Std$Object$t *)FlagLocal) {
		Handle = socket(PF_LOCAL, Style, 0);
		Type = LocalT;
	} else {
		Result->Val = Sys$Program$error_new_format(CreateMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	if (Handle < 0) {
		Result->Val = Sys$Program$error_new_format(CreateMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Val = IO$Native$(new)(Type, Handle);
	return SUCCESS;
#else
#endif
};

#if defined(LINUX) || defined(CYGWIN)

static inline int send_all_oob(int Handle, const char *Chars, int Count) {
	while (Count) {
		int Bytes = send(Handle, Chars, Count, MSG_OOB);
		if (Bytes == -1) return -1;
		Count -= Bytes;
		Chars += Bytes;
	};
	return 0;
};

METHOD("urge", TYP, T, TYP, Std$String$T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	Std$String$t *String = (Std$String$t *)Args[1].Val;
	for (long I = 0; I < String->Count; ++I) {
		if (send_all_oob(Stream->Handle, String->Blocks[I].Chars.Value, String->Blocks[I].Length.Value) < 0) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

#ifdef LINUX
#include <sys/syscall.h>

METHOD("setown", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	if (fcntl(Stream->Handle, F_SETOWN, syscall(SYS_gettid)) < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$GenericMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
    Result->Arg = Args[0];
	return SUCCESS;
};
#endif

METHOD("sendfd", TYP, LocalT, TYP, IO$Posix$T) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	int Handle = ((IO$Posix$t *)Args[1].Val)->Handle;
	char Payload[4];
	union {
		struct cmsghdr a;
		char b[CMSG_SPACE(sizeof(int))];
	} ControlX;
	struct iovec IOVec[1] = {{
		.iov_base = Payload, .iov_len = 4
	}};
	struct msghdr Message = {
		.msg_name = 0, .msg_namelen = 0,
		.msg_iov = IOVec, .msg_iovlen = 1,
		.msg_control = &ControlX, .msg_controllen = sizeof(ControlX)
	};
	struct cmsghdr *CMPtr = CMSG_FIRSTHDR(&Message);
	CMPtr->cmsg_len = CMSG_LEN(sizeof(int));
	CMPtr->cmsg_level = SOL_SOCKET;
	CMPtr->cmsg_type = SCM_RIGHTS;
	int *Data = (int *)CMSG_DATA(CMPtr);
	Data[0] = Handle;
	if (sendmsg(Socket, &Message, 0) < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	};
};

METHOD("recvfd", TYP, LocalT) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	char Payload[4];
	union {
		struct cmsghdr a;
		char b[CMSG_SPACE(sizeof(int))];
	} ControlX;
	struct iovec IOVec[1] = {{
		.iov_base = Payload, .iov_len = 4
	}};
	struct msghdr Message = {
		.msg_name = 0, .msg_namelen = 0,
		.msg_iov = IOVec, .msg_iovlen = 1,
		.msg_control = &ControlX, .msg_controllen = sizeof(ControlX)
	};
	struct cmsghdr *CMPtr = CMSG_FIRSTHDR(&Message);
	CMPtr->cmsg_len = CMSG_LEN(sizeof(int));
	CMPtr->cmsg_level = SOL_SOCKET;
	CMPtr->cmsg_type = SCM_RIGHTS;
	int *Data = (int *)CMSG_DATA(CMPtr);
	Data[0] = -1;
	if (recvmsg(Socket, &Message, 0) < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	} else {
		CMPtr = CMSG_FIRSTHDR(&Message);
		if (CMPtr != 0 &&
			CMPtr->cmsg_len == CMSG_LEN(sizeof(int)) &&
			CMPtr->cmsg_level == SOL_SOCKET &&
			CMPtr->cmsg_type == SCM_RIGHTS
		) {
			int *Data = (int *)CMSG_DATA(CMPtr);
			Result->Val = IO$Posix$new(T, Data[0]);
			return SUCCESS;
		} else {
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
	};
};

METHOD("bind", TYP, LocalT, TYP, Std$String$T) {
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	Std$String$flatten_to(Args[1].Val, Name.sun_path);
	if (bind(((IO$Posix$t *)Args[0].Val)->Handle, &Name, SUN_LEN(&Name)) < 0) {
		Result->Val = Sys$Program$error_new_format(BindMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("bind", TYP, InetT, TYP, Std$Integer$SmallT) {
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(((Std$Integer$smallt *)Args[1].Val)->Value);
	Name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(((IO$Posix$t *)Args[0].Val)->Handle, &Name, sizeof(Name)) < 0) {
		Result->Val = Sys$Program$error_new_format(BindMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("listen", TYP, T, TYP, Std$Integer$SmallT) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	if (listen(Socket, ((Std$Integer$smallt *)Args[1].Val)->Value) < 0) {
		Result->Val = Sys$Program$error_new_format(ListenMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("accept", TYP, T) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	struct sockaddr Addr;
	socklen_t Length = sizeof(Addr);
	int Socket0 = accept(Socket, &Addr, &Length);
	if (Socket0 < 0) {
		Result->Val = Sys$Program$error_new_format(AcceptMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Val = IO$Native$(new)(Args[0].Val->Type, Socket0);
	return SUCCESS;
};

static int _CLOSE[] = {
	[IO$Stream$CLOSE_READ] = 0,
	[IO$Stream$CLOSE_WRITE] = 1,
	[IO$Stream$CLOSE_BOTH] = 2
};

TYPED_INSTANCE(void, IO$Stream$close, T, IO$Posix$t *Stream, int Mode) {
	shutdown(Stream->Handle, _CLOSE[Mode]);
};

METHOD("close", TYP, T, TYP, IO$Stream$CloseModeT) {
    int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
    if (shutdown(Socket, _CLOSE[((Std$Integer$smallt *)Args[1].Val)->Value])) {
        Result->Val = Sys$Program$error_new_format(ShutdownMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
        return MESSAGE;
    };
    IO$Posix$unregister_finalizer((void *)Args[0].Val);
    return SUCCESS;
};

METHOD("connect", TYP, LocalT, TYP, Std$String$T) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	Std$String$flatten_to(Args[1].Val, Name.sun_path);
	if (connect(((IO$Posix$t *)Args[0].Val)->Handle, &Name, SUN_LEN(&Name)) < 0) {
		Result->Val = Sys$Program$error_new_format(ConnectMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("connect", TYP, InetT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	char HostName[256];
	Std$String$flatten_to(Args[1].Val, HostName);
	struct hostent *HostInfo = gethostbyname(HostName);
	if (HostInfo == 0) {
		Result->Val = Sys$Program$error_new_format(HostNotFoundMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(((Std$Integer$smallt *)Args[2].Val)->Value);
	Name.sin_addr = *(struct in_addr *)HostInfo->h_addr;
	if (connect(((IO$Posix$t *)Args[0].Val)->Handle, &Name, sizeof(Name)) < 0) {
		Result->Val = Sys$Program$error_new_format(ConnectMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

typedef enum {
	OPT_UNDEFINED,
	OPT_BOOLEAN
} option_type;

typedef struct option_t {
	const Std$Type$t *Type;
	int Level, OptName;
	option_type OptType;
} option_t;

TYPE(OptionT);

option_t OptionNoDelay[] = {{OptionT, SOL_TCP, TCP_NODELAY, OPT_BOOLEAN}};
option_t OptionOOBInline[] = {{OptionT, SOL_SOCKET, SO_OOBINLINE, OPT_BOOLEAN}};
option_t OptionReuseAddr[] = {{OptionT, SOL_SOCKET, SO_REUSEADDR, OPT_BOOLEAN}};

METHOD("setopt", TYP, T, TYP, OptionT) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	option_t *Option = (option_t *)Args[1].Val;
	switch (Option->OptType) {
	case OPT_BOOLEAN: {
		if (Count < 3) {
			Result->Val = Sys$Program$error_new_format(OptionMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
		int On = Args[2].Val == $true;
		setsockopt(Socket, Option->Level, Option->OptName, &On, sizeof(On));
		break;
	};
	default: {
		Result->Val = Sys$Program$error_new_format(OptionMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("sockname", TYP, InetT, ANY, ANY) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	struct sockaddr_in Name;
	socklen_t Length = sizeof(Name);
	if (getsockname(Socket, (struct sockaddr*)&Name, &Length)) {
		Result->Val = Sys$Program$error_new_format(PeerNameMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	} else {
		if (Args[1].Ref) {
			Args[1].Ref[0] = Std$String$copy(inet_ntoa(Name.sin_addr));
		};
		if (Args[2].Ref) Args[2].Ref[0] = Std$Integer$new_small(ntohs(Name.sin_port));
		Result->Arg = Args[0];
		return SUCCESS;
	};
}

METHOD("peername", TYP, InetT, ANY, ANY) {
	int Socket = ((IO$Posix$t *)Args[0].Val)->Handle;
	struct sockaddr_in Name;
	socklen_t Length = sizeof(Name);
	if (getpeername(Socket, (struct sockaddr*)&Name, &Length)) {
		Result->Val = Sys$Program$error_new_format(PeerNameMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	} else {
		if (Args[1].Ref) {
			Args[1].Ref[0] = Std$String$copy(inet_ntoa(Name.sin_addr));
		};
		if (Args[2].Ref) Args[2].Ref[0] = Std$Integer$new_small(ntohs(Name.sin_port));
		Result->Arg = Args[0];
		return SUCCESS;
	};
};

static void pipe_handler(int Signal) {
};

INITIAL() {
	struct sigaction Action;
	Action.sa_handler = (void *)pipe_handler;
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = SA_RESTART;
	sigaction(SIGPIPE, &Action, NULL);
	
};

#else

INITIAL() {
	//Util$TypedFunction$set(IO$Stream$close, T, socket_close);
};

#endif
