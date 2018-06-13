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
SYMBOL($empty, "empty")
SYMBOL($unused, "unused")
SYMBOL($requeue, "requeue")
SYMBOL($internal, "internal")

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

METHOD("close", TYP, T) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	amqp_connection_close(Connection->Handle, AMQP_RESPONSE_NORMAL);
	Connection->Handle = 0;
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
	amqp_rpc_reply_t Reply = amqp_get_rpc_reply(Connection->Handle);
	switch (Reply.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		Result->Arg = Args[0];
		return SUCCESS;
	case AMQP_RESPONSE_SERVER_EXCEPTION:
		Result->Val = Std$String$new_format("Server exception for method %d", Reply.reply.id);
		return MESSAGE;
	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		Result->Val = Std$String$copy(amqp_error_string(Reply.library_error));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Invalid rpc reply");
		return MESSAGE;
	}
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

METHOD("consume_message", TYP, T, TYP, Sys$Time$PreciseT) {
	connection_state_t *Connection = (connection_state_t *)Args[0].Val;
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[1].Val;
	envelope_t *Envelope = new(envelope_t);
	Envelope->Type = EnvelopeT;
	amqp_rpc_reply_t Reply = amqp_consume_message(Connection->Handle, Envelope->Value, &Time->Value, 0);
	switch (Reply.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		Result->Val = (Std$Object$t *)Envelope;
		return SUCCESS;
	case AMQP_RESPONSE_SERVER_EXCEPTION:
		Result->Val = Std$String$new_format("Server exception for method %d", Reply.reply.id);
		return MESSAGE;
	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		Result->Val = Std$String$copy(amqp_error_string(Reply.library_error));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Invalid rpc reply");
		return MESSAGE;
	}
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

METHOD("close", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_channel_close(Channel->Connection, Channel->Index, AMQP_REPLY_SUCCESS);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("get_rpc_reply", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_rpc_reply_t Reply = amqp_get_rpc_reply(Channel->Connection);
	switch (Reply.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		Result->Arg = Args[0];
		return SUCCESS;
	case AMQP_RESPONSE_SERVER_EXCEPTION:
		Result->Val = Std$String$new_format("Server exception for method %d", Reply.reply.id);
		return MESSAGE;
	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		Result->Val = Std$String$copy(amqp_error_string(Reply.library_error));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Invalid rpc reply");
		return MESSAGE;
	}
}

METHOD("read_message", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	message_t *Message = new(message_t);
	Message->Type = MessageT;
	amqp_rpc_reply_t Reply = amqp_read_message(Channel->Connection, Channel->Index, Message->Value, 0);;
	switch (Reply.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		Result->Val = (Std$Object$t *)Message;
		return SUCCESS;
	case AMQP_RESPONSE_SERVER_EXCEPTION:
		Result->Val = Std$String$new_format("Server exception for method %d", Reply.reply.id);
		return MESSAGE;
	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		Result->Val = Std$String$copy(amqp_error_string(Reply.library_error));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Invalid rpc reply");
		return MESSAGE;
	}
}

typedef struct basic_properties_t {
	const Std$Type$t *Type;
	amqp_basic_properties_t Value[1];
} basic_properties_t;

TYPE(BasicPropertiesT);

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

static inline int table_append_riva(Std$Object$t *Key, Std$Object$t *Value, amqp_table_t *Table);

static inline int riva_to_field(Std$Object$t *Value, amqp_field_value_t *Field) {
	if (Value == Std$Object$Nil) {
		Field->kind = AMQP_FIELD_KIND_VOID;
		return 0;
	} else if (Value == $true) {
		Field->kind = AMQP_FIELD_KIND_BOOLEAN;
		Field->value.boolean = 1;
		return 0;
	} else if (Value == $false) {
		Field->kind = AMQP_FIELD_KIND_BOOLEAN;
		Field->value.boolean = 0;
		return 0;
	} else if (Value->Type == Std$Integer$SmallT) {
		Field->kind = AMQP_FIELD_KIND_I32;
		Field->value.i32 = Std$Integer$get_small(Value);
		return 0;
	} else if (Value->Type == Std$Integer$BigT) {
		Field->kind = AMQP_FIELD_KIND_U64;
		Field->value.i32 = Std$Integer$get_u64(Value);
		return 0;
	} else if (Value->Type == Std$Real$T) {
		Field->kind = AMQP_FIELD_KIND_F64;
		Field->value.i32 = Std$Real$get_value(Value);
		return 0;
	} else if (Value->Type == Std$String$T) {
		Field->kind = AMQP_FIELD_KIND_UTF8;
		Field->value.bytes.len = Std$String$get_length(Value);
		Field->value.bytes.bytes = Std$String$flatten(Value);
		return 0;
	} else if (Value->Type == Sys$Time$T) {
		Field->kind = AMQP_FIELD_KIND_TIMESTAMP;
		Field->value.u64 = ((Sys$Time$t *)Value)->Value;
		return 0;
	} else if (Value->Type == Agg$List$T) {
		Field->kind = AMQP_FIELD_KIND_ARRAY;
		int NumEntries = Field->value.array.num_entries = Agg$List$length(Value);
		amqp_field_value_t *Entry = Field->value.array.entries = (amqp_field_value_t *)Riva$Memory$alloc(NumEntries * sizeof(amqp_field_value_t));
		for (Agg$List$node *Node = ((Agg$List$t *)Value)->Head; Node; Node = Node->Next) {
			if (riva_to_field(Node->Value, Entry++)) return 1;
		}
		return 0;
	} else if (Value->Type == Agg$Table$T) {
		Field->kind = AMQP_FIELD_KIND_TABLE;
		Field->value.table.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Value) * sizeof(amqp_table_entry_t));
		if (Agg$Table$foreach(Value, table_append_riva, &Field->value.table)) return 1;
		return 0;
	}
	return 1;
}

static inline int table_append_riva(Std$Object$t *Key, Std$Object$t *Value, amqp_table_t *Table) {
	if (Key->Type != Std$String$T) return 1;
	amqp_table_entry_t *Entry = &Table->entries[Table->num_entries++];
	Entry->key.len = Std$String$get_length(Key);
	Entry->key.bytes = Std$String$flatten(Key);
	riva_to_field(Value, &Entry->value);
	return 0;
}

static Std$Object$t *table_to_riva(amqp_table_t *Table);
static Std$Object$t *array_to_riva(amqp_array_t *Array);

static inline Std$Object$t *field_to_value(amqp_field_value_t *Value) {
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
		return table_to_riva(&Value->value.table);
	case AMQP_FIELD_KIND_ARRAY:
		return array_to_riva(&Value->value.array);
	}
	return 0;
}

static Std$Object$t *table_to_riva(amqp_table_t *Table) {
	Std$Object$t *Result = Agg$Table$new(Std$String$Compare, Std$String$Hash);
	for (int I = 0; I < Table->num_entries; ++I) {
		amqp_table_entry_t *Entry = &Table->entries[I];
		Std$Object$t *Key = Std$String$copy_length(Entry->key.bytes, Entry->key.len);
		Std$Object$t *Value = field_to_value(&Entry->value);
		Agg$Table$insert(Result, Key, Value);
	}
	return Result;
}

static Std$Object$t *array_to_riva(amqp_array_t *Array) {
	Std$Object$t *Result = Agg$List$new0();
	for (int I = 0; I < Array->num_entries; ++I) {
		Std$Object$t *Value = field_to_value(&Array->entries[I]);
		Agg$List$put(Result, Value);
	}
	return Result;
}

#define STRING_TO_BYTES(Val) (amqp_bytes_t){Std$String$get_length(Val), Std$String$flatten(Val)}
#define BYTES_TO_STRING(Val) Std$String$copy_length(Val.bytes Val.len)

METHOD("headers", TYP, BasicPropertiesT) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	if (!(Properties->Value->_flags & AMQP_BASIC_HEADERS_FLAG)) return FAILURE;
	Result->Val = table_to_riva(&Properties->Value->headers);
	return SUCCESS;
}

METHOD("set_headers", TYP, BasicPropertiesT, TYP, Agg$Table$T) {
	basic_properties_t *Properties = (basic_properties_t *)Args[0].Val;
	Properties->Value->headers.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Args[1].Val) * sizeof(amqp_table_entry_t));
	if (Agg$Table$foreach(Args[1].Val, table_append_riva, &Properties->Value->headers)) {
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

METHOD("queue_declare", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Passive = 0;
	amqp_boolean_t Durable = 0;
	amqp_boolean_t Exclusive = 0;
	amqp_boolean_t AutoDelete = 0;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 2; I < Count; ++I) {
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
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_queue_declare_ok_t *Ok = amqp_queue_declare(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), Passive, Durable, Exclusive, AutoDelete, Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("queue_bind", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 4; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_queue_bind_ok_t *Ok = amqp_queue_bind(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), STRING_TO_BYTES(Args[3].Val), Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("queue_purge", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_queue_purge_ok_t *Ok = amqp_queue_purge(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val)
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("queue_delete", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t IfUnused = 0;
	amqp_boolean_t IfEmpty = 0;
	for (int I = 2; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $unused) {
			IfUnused = 1;
		} else if (Arg == $empty) {
			IfEmpty = 1;
		}
	}
	amqp_queue_delete_ok_t *Ok = amqp_queue_delete(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), IfUnused, IfEmpty
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("queue_unbind", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 4; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_queue_unbind_ok_t *Ok = amqp_queue_unbind(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), STRING_TO_BYTES(Args[3].Val), Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("basic_publish", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Mandatory = 0;
	amqp_boolean_t Immediate = 0;
	amqp_basic_properties_t *Properties = 0;
	for (int I = 4; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $mandatory) {
			Mandatory = 1;
		} else if (Arg == $immediate) {
			Immediate = 1;
		} else if (Arg->Type == BasicPropertiesT) {
			Properties = ((basic_properties_t *)Arg)->Value;
		}
	}
	amqp_status_enum Status = amqp_basic_publish(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), Mandatory, Immediate, Properties, STRING_TO_BYTES(Args[3].Val)
	);
	if (Status == AMQP_STATUS_OK) {
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		Result->Val = Std$String$new("AMQP Error");
		return MESSAGE;
	}
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
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_basic_consume_ok_t *Ok = amqp_basic_consume(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), NoLocal, NoAck, Exclusive, Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("basic_cancel", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_basic_cancel_ok_t *Ok = amqp_basic_cancel(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val)
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("basic_recover", TYP, ChannelT) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Requeue = 0;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $requeue) {
			Requeue = 1;
		}
	}
	amqp_basic_recover_ok_t *Ok = amqp_basic_recover(Channel->Connection, Channel->Index,
		Requeue
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("exchange_declare", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t Passive = 0;
	amqp_boolean_t Durable = 0;
	amqp_boolean_t AutoDelete = 0;
	amqp_boolean_t Internal = 0;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 3; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $passive) {
			Passive = 1;
		} else if (Arg == $durable) {
			Durable = 1;
		} else if (Arg == $internal) {
			Internal = 1;
		} else if (Arg == $auto_delete) {
			AutoDelete = 1;
		} else if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_exchange_declare_ok_t *Ok = amqp_exchange_declare(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), Passive, Durable, AutoDelete, Internal, Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("exchange_bind", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 4; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_exchange_bind_ok_t *Ok = amqp_exchange_bind(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), STRING_TO_BYTES(Args[3].Val), Arguments
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("exchange_delete", TYP, ChannelT, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_boolean_t IfUnused = 0;
	for (int I = 2; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg == $unused) {
			IfUnused = 1;
		}
	}
	amqp_exchange_delete_ok_t *Ok = amqp_exchange_delete(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), IfUnused
	);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("exchange_unbind", TYP, ChannelT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
	channel_t *Channel = (channel_t *)Args[0].Val;
	amqp_table_t Arguments = amqp_empty_table;
	for (int I = 4; I < Count; ++I) {
		Std$Object$t *Arg = Args[I].Val;
		if (Arg->Type == Agg$Table$T) {
			Arguments.entries = (amqp_table_entry_t *)Riva$Memory$alloc(Agg$Table$size(Arg) * sizeof(amqp_table_entry_t));
			if (Agg$Table$foreach(Arg, table_append_riva, &Arguments)) {
				Result->Val = Std$String$new("Error converting arguments into table");
				return MESSAGE;
			}
		}
	}
	amqp_exchange_unbind_ok_t *Ok = amqp_exchange_unbind(Channel->Connection, Channel->Index,
		STRING_TO_BYTES(Args[1].Val), STRING_TO_BYTES(Args[2].Val), STRING_TO_BYTES(Args[3].Val), Arguments
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
