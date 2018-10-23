#include <Std.h>
#include <Riva/Memory.h>
#include <czmq.h>

typedef struct sock_t {
	const Std$Type$t *Type;
	zsock_t *Handle;
} sock_t;

TYPE(SockT);
TYPE(PubSockT, SockT);
TYPE(SubSockT, SockT);
TYPE(ReqSockT, SockT);
TYPE(RepSockT, SockT);
TYPE(DealerSockT, SockT);
TYPE(RouterSockT, SockT);
TYPE(PushSockT, SockT);
TYPE(PullSockT, SockT);
TYPE(XPubSockT, SockT);
TYPE(XSubSockT, SockT);
TYPE(PairSockT, SockT);
TYPE(StreamSockT, SockT);

static void sock_finalize(sock_t *Sock, void *Data) {
	if (Sock->Handle) zsock_destroy(&Sock->Handle);
}

GLOBAL_FUNCTION(PubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PubSockT;
	Sock->Handle = zsock_new_pub(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(SubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	const char *Subscribe = 0;
	if (Count > 1) {
		CHECK_EXACT_ARG_TYPE(1, Std$String$T);
		Subscribe = Std$String$flatten(Args[1].Val);
	}
	sock_t *Sock = new(sock_t);
	Sock->Type = SubSockT;
	Sock->Handle = zsock_new_sub(Endpoint, Subscribe);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(ReqNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = ReqSockT;
	Sock->Handle = zsock_new_req(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(RepNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = RepSockT;
	Sock->Handle = zsock_new_rep(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(DealerNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = DealerSockT;
	Sock->Handle = zsock_new_dealer(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(RouterNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = RouterSockT;
	Sock->Handle = zsock_new_router(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(PushNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PushSockT;
	Sock->Handle = zsock_new_push(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(PullNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PullSockT;
	Sock->Handle = zsock_new_pull(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(XPubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = XPubSockT;
	Sock->Handle = zsock_new_xpub(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(XSubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = XSubSockT;
	Sock->Handle = zsock_new_xsub(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(PairNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PairSockT;
	Sock->Handle = zsock_new_pair(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

GLOBAL_FUNCTION(StreamNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = StreamSockT;
	Sock->Handle = zsock_new_stream(Endpoint);
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	Result->Val = (Std$Object$t *)Sock;
	return SUCCESS;
}

METHOD("attach", TYP, SockT, TYP, Std$String$T, TYP, Std$Symbol$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoints = Std$String$flatten(Args[1].Val);
	bool Serverish = Args[2].Val == $true;
	if (zsock_attach(Sock->Handle, Endpoints, Serverish)) {
		Result->Val = Std$String$new("Syntax error");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;

}

METHOD("type", TYP, SockT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	Result->Val = Std$String$new(zsock_type_str(Sock->Handle));
	return SUCCESS;
}


