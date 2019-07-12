#include <Std.h>
#include <Riva/Memory.h>
#include <Agg/Buffer.h>
#include <czmq.h>

typedef struct ctx_t {
	const Std$Type$t *Type;
	void *Handle;
} ctx_t;

TYPE(CtxT);

GLOBAL_FUNCTION(CtxNew, 0) {
	ctx_t *Ctx = new(ctx_t);
	Ctx->Type = CtxT;
	Ctx->Handle = zmq_ctx_new();
	RETURN(Ctx);
}

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

GLOBAL_FUNCTION(SockNew, 1) {
	int Type = 0;
	if (Args[0].Val == PubSockT) {
		Type = ZMQ_PUB;
	} else if (Args[0].Val == SubSockT) {
		Type = ZMQ_SUB;
	} else if (Args[0].Val == ReqSockT) {
		Type = ZMQ_REQ;
	} else if (Args[0].Val == RepSockT) {
		Type = ZMQ_REP;
	} else if (Args[0].Val == DealerSockT) {
		Type = ZMQ_DEALER;
	} else if (Args[0].Val == RouterSockT) {
		Type = ZMQ_ROUTER;
	} else if (Args[0].Val == PushSockT) {
		Type = ZMQ_PUSH;
	} else if (Args[0].Val == PullSockT) {
		Type = ZMQ_PULL;
	} else if (Args[0].Val == XPubSockT) {
		Type = ZMQ_XPUB;
	} else if (Args[0].Val == XSubSockT) {
		Type = ZMQ_XSUB;
	} else if (Args[0].Val == PairSockT) {
		Type = ZMQ_PAIR;
	} else if (Args[0].Val == StreamSockT) {
		Type = ZMQ_STREAM;
	} else {
		SEND(Std$String$new("Invalid socket type"));
	}
	sock_t *Sock = new(sock_t);
	Sock->Type = Args[0].Val;
	Sock->Handle = zsock_new(Type);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	RETURN(Sock);
}

GLOBAL_FUNCTION(PubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PubSockT;
	Sock->Handle = zsock_new_pub(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
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
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(ReqNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = ReqSockT;
	Sock->Handle = zsock_new_req(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(RepNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = RepSockT;
	Sock->Handle = zsock_new_rep(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(DealerNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = DealerSockT;
	Sock->Handle = zsock_new_dealer(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(RouterNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = RouterSockT;
	Sock->Handle = zsock_new_router(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(PushNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PushSockT;
	Sock->Handle = zsock_new_push(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(PullNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PullSockT;
	Sock->Handle = zsock_new_pull(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(XPubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = XPubSockT;
	Sock->Handle = zsock_new_xpub(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(XSubNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = XSubSockT;
	Sock->Handle = zsock_new_xsub(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(PairNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = PairSockT;
	Sock->Handle = zsock_new_pair(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

GLOBAL_FUNCTION(StreamNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Endpoint = Std$String$flatten(Args[0].Val);
	sock_t *Sock = new(sock_t);
	Sock->Type = StreamSockT;
	Sock->Handle = zsock_new_stream(Endpoint);
	if (!Sock->Handle) SEND(Std$String$new("Socket creation error"));
	Riva$Memory$register_finalizer(Sock, sock_finalize, 0, 0, 0);
	RETURN(Sock);
}

METHOD("destroy", TYP, SockT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	zsock_destroy(&Sock->Handle);
	RETURN(Std$Object$Nil);
}

METHOD("bind", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoint = Std$String$flatten(Args[1].Val);
	int Port = zsock_bind(Sock->Handle, "%s", Endpoint);
	if (Port == -1) SEND(Std$String$new("Bind error"));
	RETURN(Std$Integer$new_small(Port));
}

METHOD("unbind", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoint = Std$String$flatten(Args[1].Val);
	if (zsock_unbind(Sock->Handle, "%s", Endpoint)) {
		SEND(Std$String$new("Unbind error"));
	}
	RETURN0;
}

METHOD("connect", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoint = Std$String$flatten(Args[1].Val);
	if (zsock_connect(Sock->Handle, "%s", Endpoint)) {
		SEND(Std$String$new("Connect error"));
	}
	RETURN0;
}

METHOD("disconnect", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoint = Std$String$flatten(Args[1].Val);
	if (zsock_disconnect(Sock->Handle, "%s", Endpoint)) {
		SEND(Std$String$new("Disconnect error"));
	}
	RETURN0;
}

METHOD("attach", TYP, SockT, TYP, Std$String$T, TYP, Std$Symbol$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	const char *Endpoints = Std$String$flatten(Args[1].Val);
	bool Serverish = Args[2].Val == $true;
	if (zsock_attach(Sock->Handle, Endpoints, Serverish)) {
		SEND(Std$String$new("Syntax error"));
	}
	RETURN0;
}

METHOD("type", TYP, SockT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	RETURN(Std$String$new(zsock_type_str(Sock->Handle)));
}

METHOD("recv", TYP, SockT, VAL, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	zframe_t *Frame = zframe_recv(Sock->Handle);
	if (Frame) {
		Std$Object$t *String = Std$String$copy_length(zframe_data(Frame), zframe_size(Frame));
		zframe_destroy(&Frame);
		RETURN(String);
	} else {
		FAIL;
	}
}

METHOD("send", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	int Length = Std$String$get_length(Args[1].Val);
	zframe_t *Frame = zframe_new(0, Length);
	char *Data = zframe_data(Frame);
	for (Std$String$block *Block = ((Std$String$t *)Args[1].Val)->Blocks; Block->Length.Value; ++Block) {
		memcpy(Data, Block->Chars.Value, Block->Length.Value);
		Data += Block->Length.Value;
	}
	if (zframe_send(&Frame, Sock->Handle, 0) == -1) {
		SEND(Std$String$new("Send error"));
	}
	RETURN0;
}

METHOD("sendm", TYP, SockT, TYP, Std$String$T) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	int Length = Std$String$get_length(Args[1].Val);
	zframe_t *Frame = zframe_new(0, Length);
	char *Data = zframe_data(Frame);
	for (Std$String$block *Block = ((Std$String$t *)Args[1].Val)->Blocks; Block->Length.Value; ++Block) {
		memcpy(Data, Block->Chars.Value, Block->Length.Value);
		Data += Block->Length.Value;
	}
	if (zframe_send(&Frame, Sock->Handle, ZFRAME_MORE) == -1) {
		SEND(Std$String$new("Send error"));
	}
	RETURN0;
}

METHOD("proxy", TYP, SockT, TYP, SockT) {
	sock_t *Frontend = (sock_t *)Args[0].Val;
	sock_t *Backend = (sock_t *)Args[1].Val;
	zmq_proxy(Frontend->Handle, Backend->Handle, NULL);
	return SUCCESS;
}

METHOD("proxy", TYP, SockT, TYP, SockT, TYP, SockT) {
	sock_t *Frontend = (sock_t *)Args[0].Val;
	sock_t *Backend = (sock_t *)Args[1].Val;
	sock_t *Capture = (sock_t *)Args[2].Val;
	zmq_proxy(Frontend->Handle, Backend->Handle, Capture->Handle);
	return SUCCESS;
}

typedef struct frame_t {
	const Std$Type$t *Type;
	zframe_t *Handle;
} frame_t;

TYPE(FrameT);

GLOBAL_FUNCTION(FrameNew, 0) {
	frame_t *Frame = new(frame_t);
	Frame->Type = FrameT;
	if (Count > 0) {
		CHECK_EXACT_ARG_TYPE(0, Std$String$T);
		Frame->Handle = zframe_new(Std$String$flatten(Args[0].Val), Std$String$get_length(Args[0].Val));
	} else {
		Frame->Handle = zframe_new_empty();
	}
	RETURN(Frame);
}

METHOD("recv", TYP, SockT, VAL, FrameT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	zframe_t *Handle = zframe_recv(Sock->Handle);
	if (Handle) {
		frame_t *Frame = new(frame_t);
		Frame->Type = FrameT;
		Frame->Handle = Handle;
		RETURN(Frame);
	} else {
		FAIL;
	}
}

METHOD("send", TYP, SockT, TYP, FrameT, TYP, Std$Integer$SmallT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	frame_t *Frame = (frame_t *)Args[1].Val;
	int Flags = Std$Integer$get_small(Args[2].Val);
	if (zframe_send(&Frame->Handle, Sock->Handle, Flags) == -1) {
		SEND(Std$String$new("Send error"));
	}
	RETURN0;
}

METHOD("length", TYP, FrameT) {
	frame_t *Frame = (frame_t *)Args[0].Val;
	RETURN(zframe_size(Frame->Handle));
}

METHOD("data", TYP, FrameT) {
	frame_t *Frame = (frame_t *)Args[0].Val;
	RETURN(Std$String$new_length(zframe_data(Frame->Handle), zframe_size(Frame->Handle)));
}

typedef struct msg_t {
	const Std$Type$t *Type;
	zmsg_t *Handle;
} msg_t;

TYPE(MsgT);

GLOBAL_FUNCTION(MsgNew, 0) {
	msg_t *Msg = new(msg_t);
	Msg->Type = MsgT;
	Msg->Handle = zmsg_new();
	RETURN(Msg);
}

METHOD("recv", TYP, SockT, VAL, MsgT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	zmsg_t *Handle = zmsg_recv(Sock->Handle);
	if (Handle) {
		msg_t *Msg = new(msg_t);
		Msg->Type = MsgT;
		Msg->Handle = Handle;
		RETURN(Msg);
	} else {
		FAIL;
	}
}

METHOD("send", TYP, SockT, TYP, MsgT) {
	sock_t *Sock = (sock_t *)Args[0].Val;
	msg_t *Msg = (msg_t *)Args[1].Val;
	if (zmsg_send(&Msg->Handle, Sock) == -1) {
		SEND(Std$String$new("Send error"));
	}
	RETURN0;
}

METHOD("size", TYP, MsgT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	RETURN(Std$Integer$new_small(zmsg_size(Msg->Handle)));
}

METHOD("length", TYP, MsgT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	RETURN(Std$Integer$new_small(zmsg_content_size(Msg->Handle)));
}

METHOD("prepend", TYP, MsgT, TYP, FrameT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	frame_t *Frame = (frame_t *)Args[1].Val;
	zmsg_prepend(Msg->Handle, &Frame->Handle);
	RETURN0;
}

METHOD("append", TYP, MsgT, TYP, FrameT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	frame_t *Frame = (frame_t *)Args[1].Val;
	zmsg_append(Msg->Handle, &Frame->Handle);
	RETURN0;
}

METHOD("prepend", TYP, MsgT, TYP, Std$String$T) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Std$String$flatten(Args[1].Val);
	size_t *Size = Std$String$get_length(Args[1].Val);
	zmsg_pushmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("append", TYP, MsgT, TYP, Std$String$T) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Std$String$flatten(Args[1].Val);
	size_t *Size = Std$String$get_length(Args[1].Val);
	zmsg_addmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("prepend", TYP, MsgT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Std$Address$get_value(Args[1].Val);
	size_t *Size = Std$Integer$get_small(Args[2].Val);
	zmsg_pushmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("append", TYP, MsgT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Std$Address$get_value(Args[1].Val);
	size_t *Size = Std$Integer$get_small(Args[2].Val);
	zmsg_addmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("prepend", TYP, MsgT, TYP, Agg$Buffer$T) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Agg$Buffer$get_value(Args[1].Val);
	size_t *Size = Agg$Buffer$get_length(Args[1].Val);
	zmsg_pushmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("append", TYP, MsgT, TYP, Agg$Buffer$T) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	const char *Mem = Agg$Buffer$get_value(Args[1].Val);
	size_t *Size = Agg$Buffer$get_length(Args[1].Val);
	zmsg_addmem(Msg->Handle, Mem, Size);
	RETURN0;
}

METHOD("pop", TYP, MsgT, VAL, Std$String$T) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	zframe_t *Frame = zmsg_pop(Msg->Handle);
	if (Frame) {
		Std$Object$t *String = Std$String$copy_length(zframe_data(Frame), zframe_size(Frame));
		zframe_destroy(&Frame);
		RETURN(String);
	} else {
		FAIL;
	}
}

METHOD("pop", TYP, MsgT, VAL, MsgT) {
	msg_t *Msg = (msg_t *)Args[0].Val;
	zmsg_t *Handle = zmsg_popmsg(Msg->Handle);
	if (Handle) {
		msg_t *Msg2 = new(msg_t);
		Msg2->Type = MsgT;
		Msg2->Handle = Handle;
		RETURN(Msg2);
	} else {
		FAIL;
	}
}

typedef struct loop_t {
	const Std$Type$t *Type;
	zloop_t *Handle;
	Std$Object$t *Message;
} loop_t;

typedef struct callback_t {
	Std$Object$t *Function;
	Std$Object$t **Message;
} callback_t;

TYPE(LoopT);

GLOBAL_FUNCTION(LoopNew, 0) {
	loop_t *Loop = new(loop_t);
	Loop->Type = LoopT;
	Loop->Handle = zloop_new();
	RETURN(Loop);
}

static int riva_reader_fn(zloop_t *Loop, zsock_t *Reader, callback_t *Callback) {
	Std$Function$result Result[1];
	if (Std$Function$call(Callback->Function, 0, Result) == MESSAGE) {
		Callback->Message[0] = Result->Val;
		return -1;
	} else {
		return 0;
	}
}

METHOD("reader", TYP, LoopT, TYP, SockT, ANY) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	sock_t *Sock = (sock_t *)Args[1].Val;
	callback_t *Callback = new(callback_t);
	Callback->Message = &Loop->Message;
	Callback->Function = Args[2].Val;
	if (zloop_reader(Loop->Handle, Sock->Handle, riva_reader_fn, Callback)) {
		SEND(Std$String$new("error registering reader"));
	} else {
		RETURN0;
	}
}

METHOD("cancel", TYP, LoopT, TYP, SockT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	sock_t *Sock = (sock_t *)Args[1].Val;
	zloop_reader_end(Loop->Handle, Sock->Handle);
	RETURN0;
}

static int riva_timer_fn(zloop_t *Loop, int TimerId, callback_t *Callback) {
	Std$Function$result Result[1];
	if (Std$Function$call(Callback->Function, 1, Result, Std$Integer$new_small(TimerId), 0) == MESSAGE) {
		Callback->Message[0] = Result->Val;
		return -1;
	} else {
		return 0;
	}
}

METHOD("timer", TYP, LoopT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, ANY) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	size_t Delay = Std$Integer$get_small(Args[1].Val);
	size_t Times = Std$Integer$get_small(Args[2].Val);
	callback_t *Callback = new(callback_t);
	Callback->Message = &Loop->Message;
	Callback->Function = Args[3].Val;
	int TimerId = zloop_timer(Loop->Handle, Delay, Times, riva_timer_fn, Callback);
	if (TimerId == -1) {
		SEND(Std$String$new("error registering timer"));
	} else {
		RETURN(Std$Integer$new_small(TimerId));
	}
}

METHOD("cancel", TYP, LoopT, TYP, Std$Integer$SmallT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	int TimerId = Std$Integer$get_small(Args[1].Val);
	zloop_timer_end(Loop->Handle, TimerId);
	RETURN0;
}

typedef struct loop_ticket_t {
	const Std$Type$t *Type;
	void *Handle;
} loop_ticket_t;

TYPE(LoopTicketT);

METHOD("ticket", TYP, LoopT, ANY) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	callback_t *Callback = new(callback_t);
	Callback->Message = &Loop->Message;
	Callback->Function = Args[1].Val;
	loop_ticket_t *Ticket = new(loop_ticket_t);
	Ticket->Type = LoopTicketT;
	Ticket->Handle = zloop_ticket(Loop->Handle, riva_timer_fn, Callback);
	RETURN(Ticket);
}

METHOD("reset", TYP, LoopT, TYP, LoopTicketT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	loop_ticket_t *Ticket = (loop_ticket_t *)Args[1].Val;
	zloop_ticket_reset(Loop->Handle, Ticket->Handle);
	RETURN0;
}

METHOD("cancel", TYP, LoopT, TYP, LoopTicketT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	loop_ticket_t *Ticket = (loop_ticket_t *)Args[1].Val;
	zloop_ticket_delete(Loop->Handle, Ticket->Handle);
	RETURN0;
}

METHOD("ticket_delay", TYP, LoopT, TYP, Std$Integer$SmallT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	size_t Delay = Std$Integer$get_small(Args[1].Val);
	zloop_set_ticket_delay(Loop->Handle, Delay);
	RETURN0;
}

METHOD("verbose", TYP, LoopT, TYP, Std$Symbol$T) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	zloop_set_verbose(Loop->Handle, Args[1].Val == $true);
	RETURN0;
}

METHOD("nonstop", TYP, LoopT, TYP, Std$Symbol$T) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	zloop_set_nonstop(Loop->Handle, Args[1].Val == $true);
	RETURN0;
}

METHOD("start", TYP, LoopT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	int Status = zloop_start(Loop->Handle);
	if (Status == -1) {
		SEND(Loop->Message);
	} else if (Status == 0) {
		SEND(Std$String$new("Loop interrupted"));
	} else if (Status > 0) {
		SEND(Std$String$new("Loop error"));
	} else {
		RETURN0;
	}
}

METHOD("destroy", TYP, LoopT) {
	loop_t *Loop = (loop_t *)Args[0].Val;
	zloop_destroy(&Loop->Handle);
	RETURN(Std$Object$Nil);
}
