#include <Std.h>
#include <IO/Stream.h>
#include <Riva/Memory.h>
#include <Sys/Time.h>
#include <Util/Event/Base.h>
#include <Util/Event/Buffer.h>
#include <event2/event.h>

typedef struct http_t {
	const Std$Type$t *Type;
	struct evhttp *Handle;
} http_t;

TYPE(T);

GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Util$Event$Base$T);
	Util$Event$Base$t *EventBase = (Util$Event$Base$t *)Args[0].Val;
	http_t *Http = new(http_t);
	Http->Type = T;
	Http->Handle = evhttp_new(EventBase->Handle);
	RETURN(Http);
}

METHOD("allowed_methods", TYP, T, TYP, Std$Integer$SmallT) {
	http_t *Http = (http_t *)Args[0].Val;
	int Methods = Std$Integer$get_small(Args[1].Val);
	evhttp_set_allowed_methods(Http, Methods);
	RETURN0;
}

typedef struct http_request_t {
	const Std$Type$t *Type;
	struct evhttp_request *Handle;
} http_request_t;

TYPE(RequestT);

static void riva_http_request_cb(struct evhttp_request *Handle, void *Data) {
	Std$Object$t *Callback = (Std$Object$t *)Data;
	http_request_t *Request = new(http_request_t);
	Request->Type = RequestT;
	Request->Handle = Handle;
	Std$Function$result Result[1];
	Std$Function$argument Args[1] = {{Request, 0}};
	Std$Function$invoke(Callback, 1, Result, Args);
}

METHOD("set_cb", TYP, T, TYP, Std$String$T, ANY) {
	http_t *Http = (http_t *)Args[0].Val;
	const char *Path = Std$String$flatten(Args[1].Val);
	evhttp_set_cb(Http->Handle, Path, riva_http_request_cb, Args[2].Val);
	RETURN0;
}

METHOD("bind", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	http_t *Http = (http_t *)Args[0].Val;
	const char *Address = Std$String$flatten(Args[1].Val);
	int Port = Std$Integer$get_small(Args[2].Val);
	evhttp_bind_socket(Http->Handle, Address, Port);
	RETURN0;
}

METHOD("send_reply", TYP, RequestT, TYP, Std$Integer$SmallT, TYP, Std$String$T, TYP, Util$Event$Buffer$T) {
	http_request_t *Request = (http_request_t *)Args[0].Val;
	int Code = Std$Integer$get_small(Args[1].Val);
	const char *Reason = Std$String$flatten(Args[2].Val);
	Util$Event$Buffer$t *Body = (Util$Event$Buffer$t *)Args[3].Val;
	evhttp_send_reply(Request->Handle, Code, Reason, Body->Handle);
	RETURN0;
}

typedef struct headers_t {
	const Std$Type$t *Type;
	struct evkeyvalq *Handle;
} headers_t;

TYPE(HeadersT);

METHOD("empty", TYP, HeadersT) {
	headers_t *Headers = (headers_t *)Args[0].Val;
	evhttp_clear_headers(Headers->Handle);
	RETURN0;
}

METHOD("[]", TYP, HeadersT, TYP, Std$String$T) {
	headers_t *Headers = (headers_t *)Args[0].Val;
	const char *Key = Std$String$flatten(Args[1].Val);
	const char *Value = evhttp_find_header(Headers->Handle, Key);
	if (Value) RETURN(Std$String$new(Value));
	FAIL;
}

METHOD("insert", TYP, HeadersT, TYP, Std$String$T, TYP, Std$String$T) {
	headers_t *Headers = (headers_t *)Args[0].Val;
	const char *Key = Std$String$flatten(Args[1].Val);
	const char *Value = Std$String$flatten(Args[2].Val);
	if (evhttp_add_header(Headers->Handle, Key, Value)) {
		SEND(Std$String$new("Error adding header"));
	}
	RETURN0;
}

METHOD("delete", TYP, HeadersT, TYP, Std$String$T) {
	headers_t *Headers = (headers_t *)Args[0].Val;
	const char *Key = Std$String$flatten(Args[1].Val);
	if (evhttp_remove_header(Headers->Handle, Key)) FAIL;
	RETURN0;
}

METHOD("input_headers", TYP, RequestT) {
	http_request_t *Request = (http_request_t *)Args[0].Val;
	headers_t *Headers = new(headers_t);
	Headers->Type = HeadersT;
	Headers->Handle = evhttp_request_get_input_headers(Request->Handle);
	RETURN(Headers);
}

METHOD("output_headers", TYP, RequestT) {
	http_request_t *Request = (http_request_t *)Args[0].Val;
	headers_t *Headers = new(headers_t);
	Headers->Type = HeadersT;
	Headers->Handle = evhttp_request_get_output_headers(Request->Handle);
	RETURN(Headers);
}

METHOD("output_buffer", TYP, RequestT) {
	http_request_t *Request = (http_request_t *)Args[0].Val;
	Util$Event$Buffer$t *Buffer = new(Util$Event$Buffer$t);
	Buffer->Type = Util$Event$Buffer$T;
	Buffer->Handle = evhttp_request_get_output_buffer(Request->Handle);
	RETURN(Buffer);
}

METHOD("uri", TYP, RequestT) {
	http_request_t *Request = (http_request_t *)Args[0].Val;
	RETURN(Std$String$new(evhttp_request_get_uri(Request->Handle)))
}
