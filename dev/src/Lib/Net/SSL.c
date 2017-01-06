#include <Std.h>
#include <Riva/Memory.h>
#include <openssl/ssl.h>

typedef struct context_t {
	const Std$Type$t *Type;
	SSL_CTX *Handle;
} context_t;

TYPE(ContextT);

typedef struct method_t {
	const Std$Type$t *Type;
	SSL_METHOD *Handle;
} method_t;

TYPE(MethodT);

GLOBAL_FUNCTION(MethodSSLv23, 0) {
	method_t *Method = new(method_t);
	Method->Type = MethodT;
	Method->Handle = SSLv23_method();
	Result->Val = (Std$Object$t *)Method;
	return SUCCESS;
}

GLOBAL_FUNCTION(ContextNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, MethodT);
	context_t *Context = new(context_t);
	Context->Type = ContextT;
	Context->Handle = SSL_CTX_new(((method_t *)Args[0].Val)->Handle);
	Result->Val = (Std$Object$t *)Context;
	return SUCCESS;
};