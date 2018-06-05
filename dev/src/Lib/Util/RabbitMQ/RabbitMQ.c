#include <Std.h>
#include <Riva/Memory.h>
#include <Agg/Table.h>
#include <Agg/List.h>
#include <Sys/Time.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>
#include <amqp_framing.h>

SYMBOL($passive, "passive");
SYMBOL($durable, "durable");
SYMBOL($exclusive, "exclusive");
SYMBOL($auto_delete, "auto_delete");
SYMBOL($no_local, "no_local");
SYMBOL($no_ack, "no_ack");
SYMBOL($mandatory, "mandatory");
SYMBOL($immediate, "immediate");

typedef struct connection_state_t {
	const Std$Type$t *Type;
	amqp_connection_state_t *Handle;
} connection_state_t;

TYPE(T);

typedef struct channel_t {
	const Std$Type$t *Type;
	amqp_connection_state_t *Connection;
	int Index;
} channel_t;

TYPE(ChannelT);

GLOBAL_FUNCTION(New, 0) {
	connection_state_t *Connection = new(connection_state_t);
	Connection->Type = T;
	Connection->Handle = amqp_new_connection();
	Result->Val = (Std$Object$t *)Connection;
	return SUCCESS;
}

METHOD("destroy", TYP, T) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	amqp_destroy_connection(Connection->Handle);
	Connection->Handle = 0;
	return SUCCESS;
}

METHOD("login", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	Result->Arg = Args[0];
	const char *VHost = Std$String$flatten(Args[1].Val);
	int ChannelMax = Std$Integer$get_small(Args[2].Val);
	int FrameMax = Std$Integer$get_small(Args[3].Val);
	int HeartBeat = Std$Integer$get_small(Args[4].Val);
	int SaslMethod = AMQP_SASL_METHOD_PLAIN;
	const char *UserName = Std$String$flatten(Args[5].Val);
	const char *Password = Std$String$flatten(Args[6].Val);
	amqp_login(Connection->Handle, VHost, ChannelMax, FrameMax, HeartBeat, SaslMethod, UserName, Password);
	return SUCCESS;
}

METHOD("get_rpc_reply", TYP, T) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	amqp_get_rpc_reply(Connection->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

typedef struct envelope_t {
	const Std$Type$t *Type;
	amqp_envelope_t Value[1];
} envelope_t;

TYPE(EnvelopeT);

METHOD("consume_message", TYP, T) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	envelope_t *Envelope = new(envelope_t);
	Envelope->Type = EnvelopeT;
	amqp_rpc_reply_t Reply = amqp_consume_message(Connection->Handle, Envelope->Value, NULL, 0);
	Result->Val = (Std$Object$t *)Envelope;
	return SUCCESS;
}

METHOD("consumer_tag", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	Result->Val = Std$String$copy_length(Envelope->Value->consumer_tag.bytes, Envelope->Value->consumer_tag.len);
	return SUCCESS;
}

METHOD("delivery_tag", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	Result->Val = Std$Integer$new_u64(Envelope->Value->delivery_tag);
	return SUCCESS;
}

METHOD("redelivered", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	Result->Arg = Args[0];
	return Envelope->Value->redelivered ? SUCCESS : FAILURE;
}

METHOD("exchange", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	Result->Val = Std$String$copy_length(Envelope->Value->exchange.bytes, Envelope->Value->exchange.len);
	return SUCCESS;
}

METHOD("routing_key", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	Result->Val = Std$String$copy_length(Envelope->Value->routing_key.bytes, Envelope->Value->routing_key.len);
	return SUCCESS;
}

typedef struct message_t {
	const Std$Type$t *Type;
	amqp_message_t Value[1];
} message_t;

TYPE(MessageT);

METHOD("message", TYP, EnvelopeT) {
	envelope_t *Envelope = (envelope_t *)Args[0].Val;
	message_t *Message = new(message_t);
	Message->Type = MessageT;
	Message->Value[0] = Envelope->Value->message;
	Result->Val = (Std$Object$t *)Message;
	return SUCCESS;
}

METHOD("body", TYP, MessageT) {
	message_t *Message = (message_t *)Args[0].Val;
	Result->Val = Std$String$copy_length(Message->Value->body.bytes, Message->Value->body.len);
	return SUCCESS;
}

METHOD("channel_open", TYP, T, TYP, Std$Integer$SmallT) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	amqp_channel_open(Connection->Handle, Index);
	channel_t *Channel = new(channel_t);
	Channel->Type = ChannelT;
	Channel->Connection = Connection->Handle;
	Channel->Index = Index;
	Result->Val = (Std$Object$t *)Channel;
	return SUCCESS;
}

METHOD("channel_close", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_channel_close(Channel->Connection, Channel->Index, AMQP_REPLY_SUCCESS);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get_rpc_reply", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_get_rpc_reply(Channel->Connection);
	Result->Arg = Args[0];
	return SUCCESS;
}

typedef struct basic_properties_t {
	const Std$Type$t *Type;
	amqp_basic_properties_t Value[1];
} basic_properties_t;

TYPE(BasicPropertiesT);

/*
#define AMQP_BASIC_CONTENT_TYPE_FLAG (1 << 15)
#define AMQP_BASIC_CONTENT_ENCODING_FLAG (1 << 14)
#define AMQP_BASIC_HEADERS_FLAG (1 << 13)
#define AMQP_BASIC_DELIVERY_MODE_FLAG (1 << 12)
#define AMQP_BASIC_PRIORITY_FLAG (1 << 11)
#define AMQP_BASIC_CORRELATION_ID_FLAG (1 << 10)
#define AMQP_BASIC_REPLY_TO_FLAG (1 << 9)
#define AMQP_BASIC_EXPIRATION_FLAG (1 << 8)
#define AMQP_BASIC_MESSAGE_ID_FLAG (1 << 7)
#define AMQP_BASIC_TIMESTAMP_FLAG (1 << 6)
#define AMQP_BASIC_TYPE_FLAG (1 << 5)
#define AMQP_BASIC_USER_ID_FLAG (1 << 4)
#define AMQP_BASIC_APP_ID_FLAG (1 << 3)
#define AMQP_BASIC_CLUSTER_ID_FLAG (1 << 2)
typedef struct amqp_basic_properties_t_ {
  amqp_flags_t _flags;
  uint8_t priority;
} amqp_basic_properties_t;
*/

GLOBAL_FUNCTION(BasicPropertiesNew, 0) {
	basic_properties_t *Properties = new(basic_properties_t);
	Properties->Type = BasicPropertiesT;
	Result->Val = (Std$Object$t *)Properties;
	return SUCCESS;
}

#define BASIC_PROPERTY_GETTER(Name, FlagName) \
METHOD(#Name, TYP, BasicPropertiesT) {\
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;\
	if (!(Properties->Value->_flags & AMQP_BASIC_ ## FlagName ## _FLAG)) return FAILURE;\
	Result->Val = Std$String$copy_length(\
		Properties->Value->Name.bytes,\
		Properties->Value->Name.len\
	);\
	return SUCCESS;\
}\

#define BASIC_PROPERTY_SETTER(Name, FlagName)\
METHOD("set_" #Name, TYP, BasicPropertiesT, TYP, Std$String$T) {\
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;\
	Properties->Value->Name.bytes = Std$String$flatten(Args[1].Val);\
	Properties->Value->Name.len = Std$String$get_length(Args[1].Val);\
	Properties->Value->_flags |= AMQP_BASIC_ ## FlagName ## _FLAG;\
	Result->Arg = Args[0];\
	return SUCCESS;\
}

BASIC_PROPERTY_GETTER(content_type, CONTENT_TYPE);
BASIC_PROPERTY_GETTER(content_encoding, CONTENT_ENCODING);
BASIC_PROPERTY_GETTER(correlation_id, CORRELATION_ID);
BASIC_PROPERTY_GETTER(reply_to, REPLY_TO);
BASIC_PROPERTY_GETTER(expiration, EXPIRATION);
BASIC_PROPERTY_GETTER(message_id, MESSAGE_ID);
BASIC_PROPERTY_GETTER(type, TYPE);
BASIC_PROPERTY_GETTER(user_id, USER_ID);
BASIC_PROPERTY_GETTER(app_id, APP_ID);
BASIC_PROPERTY_GETTER(cluster_id, CLUSTER_ID);

BASIC_PROPERTY_SETTER(content_type, CONTENT_TYPE);
BASIC_PROPERTY_SETTER(content_encoding, CONTENT_ENCODING);
BASIC_PROPERTY_SETTER(correlation_id, CORRELATION_ID);
BASIC_PROPERTY_SETTER(reply_to, REPLY_TO);
BASIC_PROPERTY_SETTER(expiration, EXPIRATION);
BASIC_PROPERTY_SETTER(message_id, MESSAGE_ID);
BASIC_PROPERTY_SETTER(type, TYPE);
BASIC_PROPERTY_SETTER(user_id, USER_ID);
BASIC_PROPERTY_SETTER(app_id, APP_ID);
BASIC_PROPERTY_SETTER(cluster_id, CLUSTER_ID);

static int amqp_table_append(Std$Object$t *Key, Std$Object$t *Value, amqp_table_t *Table) {
	if (Key->Type != Std$String$T) return 1;
	amqp_table_entry_t *Entry = &Table->entries[Table->num_entries++];
	Entry->key.len = Std$String$get_length(Key);
	Entry->key.bytes = Std$String$flatten(Key);
	if (Value == Std$Object$Nil) {
		Entry->value.kind = AMQP_FIELD_KIND_VOID;
	} else if (Value == $true) {
		Entry->value.kind = AMQP_FIELD_KIND_BOOLEAN;
		Entry->value.value.boolean = 1;
	} else if (Value == $false) {
		Entry->value.kind = AMQP_FIELD_KIND_BOOLEAN;
		Entry->value.value.boolean = 0;
	} else if (Value->Type == Std$Integer$SmallT) {
		Entry->value.kind = AMQP_FIELD_KIND_I32;
		Entry->value.value.i32 = Std$Integer$get_small(Value);
	} else if (Value->Type == Std$Integer$BigT) {
		Entry->value.kind = AMQP_FIELD_KIND_U64;
		Entry->value.value.i32 = Std$Integer$get_u64(Value);
	} else if (Value->Type == Std$Real$T) {
		Entry->value.kind = AMQP_FIELD_KIND_F64;
		Entry->value.value.i32 = Std$Real$get_value(Value);
	} else if (Value->Type == Std$String$T) {
		Entry->value.kind = AMQP_FIELD_KIND_UTF8;
		Entry->value.value.bytes.len = Std$String$get_length(Value);
		Entry->value.value.bytes.bytes = Std$String$flatten(Value);
	} else if (Value->Type == Sys$Time$T) {
		Entry->value.kind = AMQP_FIELD_KIND_TIMESTAMP;
		Entry->value.value.u64 = ((Sys$Time$t *)Value)->Value;
	} else if (Value->Type == Agg$List$T) {
		Entry->value.kind = AMQP_FIELD_KIND_ARRAY;
		Entry->value.value.array.num_entries = 0;
	} else if (Value->Type == Agg$Table$T) {
		Entry->value.kind = AMQP_FIELD_KIND_TABLE;
		Entry->value.value.table.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Value) * sizeof(amqp_table_entry_t));
		if (Agg$Table$foreach(Value, amqp_table_append, &Entry->value.value.table)) return 1;
	}
	return 0;
}

static Std$Object$t *amqp_table_convert(amqp_table_t *Table);

static Std$Object$t *amqp_field_convert(amqp_field_value_t *Value) {
	switch (Value->kind) {
	case AMQP_FIELD_KIND_VOID:
		return Std$Object$Nil;
	case AMQP_FIELD_KIND_BOOLEAN:
		return Value->value.boolean ? $true : $false;
	case AMQP_FIELD_KIND_I8:
		return Std$Integer$new_small(Value->value.i8);
	case AMQP_FIELD_KIND_U8:
		return Std$Integer$new_small(Value->value.u8);
	case AMQP_FIELD_KIND_I16:
		return Std$Integer$new_small(Value->value.i16);
	case AMQP_FIELD_KIND_U16:
		return Std$Integer$new_small(Value->value.u16);
	case AMQP_FIELD_KIND_I32:
		return Std$Integer$new_small(Value->value.i32);
	case AMQP_FIELD_KIND_U32:
		return Std$Integer$new_u64(Value->value.u32);
	case AMQP_FIELD_KIND_I64:
		return Std$Integer$new_s64(Value->value.i64);
	case AMQP_FIELD_KIND_U64:
		return Std$Integer$new_u64(Value->value.u64);
	case AMQP_FIELD_KIND_F32:
		return Std$Real$new(Value->value.f32);
	case AMQP_FIELD_KIND_F64:
		return Std$Real$new(Value->value.f64);
	case AMQP_FIELD_KIND_UTF8:
	case AMQP_FIELD_KIND_BYTES:
		return Std$String$copy_length(Value->value.bytes.bytes, Value->value.bytes.len);
	case AMQP_FIELD_KIND_TIMESTAMP:
		return Sys$Time$new(Value->value.u64);
	case AMQP_FIELD_KIND_TABLE:
		return amqp_table_convert(&Value->value.table);
	case AMQP_FIELD_KIND_ARRAY:
		Value->value.array;
		return Agg$List$new(0);
	}
	return 0;
}

static Std$Object$t *amqp_table_convert(amqp_table_t *Table) {
	Std$Object$t *Result = Agg$Table$new(Std$String$Compare, Std$String$Hash);
	for (int I = 0; I < Table->num_entries; ++I) {
		amqp_table_entry_t *Entry = &Table->entries[I];
		Std$Object$t *Key = Std$String$copy_length(Entry->key.bytes, Entry->key.len);
		Std$Object$t *Value = amqp_field_convert(&Entry->value);
		Agg$Table$insert(Result, Key, Value);
	}
	return Result;
}

METHOD("headers", TYP, BasicPropertiesT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	if (!(Properties->Value->_flags & AMQP_BASIC_HEADERS_FLAG)) return FAILURE;
	Result->Val = amqp_table_convert(&Properties->Value->headers);
	return SUCCESS;
}

METHOD("set_headers", TYP, BasicPropertiesT, TYP, Agg$Table$T) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	Properties->Value->headers.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Args[1].Val) * sizeof(amqp_table_entry_t));
	if (Agg$Table$foreach(Args[1].Val, amqp_table_append, &Properties->Value->headers)) {
		Result->Val = Std$String$new("Error converting arguments to table");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("priority", TYP, BasicPropertiesT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	if (!(Properties->Value->_flags & AMQP_BASIC_PRIORITY_FLAG)) return FAILURE;
	Result->Val = Std$Integer$new_small(Properties->Value->priority);
	return SUCCESS;
}

METHOD("set_timestamp", TYP, BasicPropertiesT, TYP, Std$Integer$SmallT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	Properties->Value->priority = Std$Integer$get_small(Args[1].Val);
	Properties->Value->_flags |= AMQP_BASIC_PRIORITY_FLAG;
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("timestamp", TYP, BasicPropertiesT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	if (!(Properties->Value->_flags & AMQP_BASIC_TIMESTAMP_FLAG)) return FAILURE;
	Result->Val = Sys$Time$new(Properties->Value->timestamp);
	return SUCCESS;
}

METHOD("set_timestamp", TYP, BasicPropertiesT, TYP, Sys$Time$T) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	Properties->Value->timestamp = ((Sys$Time$t *)Args[1].Val)->Value;
	Properties->Value->_flags |= AMQP_BASIC_TIMESTAMP_FLAG;
	Result->Arg = Args[0];
	return SUCCESS;
}

TYPE(DeliveryModeT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

const Std$Integer$smallt DeliveryModePersistent[1] = {{DeliveryModeT, AMQP_DELIVERY_PERSISTENT}};
const Std$Integer$smallt DeliveryModeNonPersistent[1] = {{DeliveryModeT, AMQP_DELIVERY_NONPERSISTENT}};

METHOD("delivery_mode", TYP, BasicPropertiesT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	if (!(Properties->Value->_flags & AMQP_BASIC_DELIVERY_MODE_FLAG)) return FAILURE;
	switch (Properties->Value->delivery_mode) {
	case AMQP_DELIVERY_PERSISTENT:
		Result->Val = (Std$Object$t *)DeliveryModePersistent;
		return SUCCESS;
	case AMQP_DELIVERY_NONPERSISTENT:
		Result->Val = (Std$Object$t *)DeliveryModeNonPersistent;
		return SUCCESS;
	default:
		Result->Val = Std$String$new("Unknown delivery mode");
		return MESSAGE;
	}
}

METHOD("set_delivery_mode", TYP, BasicPropertiesT, TYP, DeliveryModeT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	Properties->Value->delivery_mode = Std$Integer$get_small(Args[1].Val);
	Properties->Value->_flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("basic_publish", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Mandatory = 0;
	amqp_boolean_t Immediate = 0;
	amqp_basic_properties_t *Properties = 0;
	for (int I = 5; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $mandatory) {
			Mandatory = 1;
		} else if (Arg == $immediate) {
			Immediate = 1;
		} else if (Arg->Type == BasicPropertiesT) {
			Properties = ((basic_properties_t *)Arg)->Value;
		}
	}
	amqp_status_enum Status = amqp_basic_publish(
		Channel->Connection, Channel->Index,
		(amqp_bytes_t){Std$String$get_length(Args[2].Val), Std$String$flatten(Args[2].Val)},
		(amqp_bytes_t){Std$String$get_length(Args[3].Val), Std$String$flatten(Args[3].Val)},
		Mandatory, Immediate, Properties,
		(amqp_bytes_t){Std$String$get_length(Args[3].Val), Std$String$flatten(Args[4].Val)}
	);
	if (Status == AMQP_STATUS_OK) {
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		Result->Val = Std$String$new("AMQP Error");
		return MESSAGE;
	}
}

METHOD("queue_declare", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Passive = 0;
	amqp_boolean_t Durable = 0;
	amqp_boolean_t Exclusive = 0;
	amqp_boolean_t AutoDelete = 0;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $passive) {
			Passive = 1;
		} else if (Arg == $durable) {
			Durable = 1;
		} else if (Arg == $exclusive) {
			Exclusive = 1;
		} else if (Arg == $auto_delete) {
			AutoDelete = 1;
		} else if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, amqp_table_append, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_queue_declare_ok_t *Ok = amqp_queue_declare(Channel->Connection, Channel->Index,
		(amqp_bytes_t){Std$String$get_length(Args[2].Val), Std$String$flatten(Args[2].Val)},
		Passive, Durable, Exclusive, AutoDelete, Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("queue_bind", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, amqp_table_append, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_queue_bind_ok_t *Ok = amqp_queue_bind(Channel->Connection, Channel->Index,
		(amqp_bytes_t){Std$String$get_length(Args[2].Val), Std$String$flatten(Args[2].Val)},
		(amqp_bytes_t){Std$String$get_length(Args[3].Val), Std$String$flatten(Args[3].Val)},
		(amqp_bytes_t){Std$String$get_length(Args[4].Val), Std$String$flatten(Args[4].Val)},
		Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("basic_consume", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t NoLocal = 0;
	amqp_boolean_t NoAck = 0;
	amqp_boolean_t Exclusive = 0;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $no_local) {
			NoLocal = 1;
		} else if (Arg == $no_ack) {
			NoAck = 1;
		} else if (Arg == $exclusive) {
			Exclusive = 1;
		} else if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, amqp_table_append, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_basic_consume_ok_t *Ok = amqp_basic_consume(Channel->Connection, Channel->Index,
		(amqp_bytes_t){Std$String$get_length(Args[2].Val), Std$String$flatten(Args[2].Val)},
		(amqp_bytes_t){Std$String$get_length(Args[3].Val), Std$String$flatten(Args[3].Val)},
		NoLocal, NoAck, Exclusive, Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

typedef struct socket_t {
	const Std$Type$t *Type;
	amqp_socket_t *Handle;
} socket_t;

TYPE(SocketT);

METHOD("tcp_socket", TYP, T) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	socket_t *Socket = new(socket_t);
	Socket->Type = SocketT;
	Socket->Handle = amqp_tcp_socket_new(Connection->Handle);
	Result->Val = (Std$Object$t *)Socket;
	return SUCCESS;
}

METHOD("open", TYP, SocketT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	socket_t *Socket = (socket_t *)Args[0].Val;
	const char *HostName = Std$String$flatten(Args[1].Val);
	int Port = Std$Integer$get_small(Args[2].Val);
	if (amqp_socket_open(Socket->Handle, HostName, Port)) {
		Result->Val = Std$String$new("Failed to open socket");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}
