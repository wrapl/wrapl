#include <Std.h>
#include <Riva/Memory.h>
#include <Agg.h>
#include <lib/libwebsockets.h>

typedef struct context_t {
	const Std$Type$t *Type;
	struct lws_context *Handle;
} context_t;

static int parse_protocol(Std$Object$t *Key, Std$Object$t *Value, struct lws_protocols *Protocol) {
	const char *KeyString = Std$String$flatten(Key);
	if (!strcmp(KeyString, "name")) {
		Protocol->name = Std$String$flatten(Value);
		return 0;
	};
	if (!strcmp(KeyString, "callback")) {
		
		return 0;
	}
	return 1;
};

static int protocol_callback(const struct lws *WSI, enum lws_callback_reasons Reason, void *User, void *In, size_t Length) {
	
};

static int parse_info(Std$Object$t *Key, Std$Object$t *Value, struct lws_context_creation_info *Info) {
	const char *KeyString = Std$String$flatten(Key);
	if (!strcmp(KeyString, "port")) {
		Info->port = Std$Integer$get_small(Value);
		return 0;
	};
	if (!strcmp(KeyString, "iface")) {
		Info->iface = Std$String$flatten(Value);
		return 0;
	};
	if (!strcmp(KeyString, "protocols")) {
		Agg$List$t *ProtocolsList = (Agg$List$t *)Value;
		struct lws_protocols *Protocols = Riva$Memory$alloc((ProtocolsList->Length + 1) * sizeof(struct lws_protocols));
		struct lws_protocols *Protocol = Protocols;
		for (Agg$List$node *Node = ProtocolsList->Head; Node; Node = Node->Next) {
			Protocol->callback = protocol_callback;
			Agg$Table$foreach(Node->Value, parse_protocol, Protocol);
			++Protocol;
		}
		Info->protocols = Protocols;
		return 0;
	};
	if (!strcmp(KeyString, "ssl_cert_filepath")) {
		Info->ssl_cert_filepath = Std$String$flatten(Value);
		return 0;
	}
	if (!strcmp(KeyString, "ssl_private_key_filepath")) {
		Info->ssl_private_key_filepath = Std$String$flatten(Value);
		return 0;
	}
	return 1;
};

GLOBAL_FUNCTION(Init, 1) {
	CHECK_ARG_TYPE(0, Agg$Table$T);
	struct lws_context_creation_info Info = {0,};
	Agg$Table$foreach(Args[0].Val, parse_info, &Info);
	return SUCCESS;
};