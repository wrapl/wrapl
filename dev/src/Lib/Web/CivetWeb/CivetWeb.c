#include <Std.h>
#include <Riva/Memory.h>
#include <Agg/Table.h>
#include <Agg/StringTable.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>	
#include <civetweb.h>
#include <ctype.h>

typedef struct context_t {
	const Std$Type$t *Type;
	struct mg_context *Handle;
	Std$Object$t *Upload;
} context_t;

TYPE(T);

typedef struct connection_t {
	const Std$Type$t *Type;
	struct mg_connection *Handle;
	Std$Object$t *Data;
} connection_t;

TYPE(ConnectionT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

typedef struct request_info_t {
	const Std$Type$t *Type;
	const struct mg_request_info *Handle;
} request_info_t;

TYPE(RequestInfoT);

static int parse_option(Std$Object$t *Key, Std$Object$t *Value, const char ***Option) {
	**Option = Std$String$flatten(Key);
	++*Option;
	**Option = Std$String$flatten(Value);
	++*Option;
	return 0;
};

/*
SYMBOL($begin_request, "begin_request");

static int riva_begin_request(struct mg_connection *Connection) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	if (!ConnectionObject) {
		ConnectionObject = new(connection_t);
		ConnectionObject->Type = ConnectionT;
		ConnectionObject->Handle = Connection;
		ConnectionObject->Data = Std$Object$Nil;
		mg_set_user_connection_data(Connection, ConnectionObject);
	};
	context_t *Context = (context_t *)mg_get_user_data(mg_get_context(Connection));
	Std$Function$result Result;
	switch (Std$Function$call($begin_request, 2, &Result, Context->Data, 0, ConnectionObject, 0)) {
	case SUSPEND:
	case SUCCESS:
		return Std$Integer$get_small(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return -1;
	};
};

static char *riva_open_file(const struct mg_connection *Connection, const char *Path, size_t *DataLength) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	if (!ConnectionObject) {
		ConnectionObject = new(connection_t);
		ConnectionObject->Type = ConnectionT;
		ConnectionObject->Handle = Connection;
		ConnectionObject->Data = Std$Object$Nil;
		mg_set_user_connection_data(Connection, ConnectionObject);
	};
	context_t *Context = (context_t *)mg_get_user_data(mg_get_context(Connection));
	
};
*/

static void riva_upload(struct mg_connection *Connection, const char *FileName) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	if (!ConnectionObject) {
		ConnectionObject = new(connection_t);
		ConnectionObject->Type = ConnectionT;
		ConnectionObject->Handle = Connection;
		ConnectionObject->Data = Std$Object$Nil;
		mg_set_user_connection_data(Connection, ConnectionObject);
	};
	context_t *Context = (context_t *)mg_get_user_data(mg_get_context(Connection));
	if (Context->Upload != Std$Object$Nil) {
		Std$Function$result Result;
		Std$Function$call(Context->Upload, 2, &Result, ConnectionObject, 0, Std$String$new(FileName), 0);
	};
};

GLOBAL_FUNCTION(New, 1) {
	CHECK_ARG_TYPE(0, Agg$Table$T);
	context_t *Context = new(context_t);
	Context->Type = T;
	Context->Upload = Std$Object$Nil;
	struct mg_callbacks Callbacks = {0,};
	//Callbacks.upload = riva_upload;
	Std$Object$t *OptionsTable = Args[0].Val;
	const char **Options = (const char **)Riva$Memory$alloc((2 * Agg$Table$size(OptionsTable) + 1) * sizeof(const char *));
	const char **Option = Options;
	Agg$Table$foreach(OptionsTable, (void *)parse_option, &Option);
	*Option = 0;
	Context->Handle = mg_start(&Callbacks, Context, Options);
	Result->Val = (Std$Object$t *)Context;
	return SUCCESS;
};

METHOD("upload", TYP, T) {
	context_t *Context = (context_t *)Args[0].Val;
	Result->Val = *(Result->Ref = &Context->Upload);
	return SUCCESS;
};


static int riva_request_handler(struct mg_connection *Connection, Std$Object$t *Callback) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	if (!ConnectionObject) {
		ConnectionObject = new(connection_t);
		ConnectionObject->Type = ConnectionT;
		ConnectionObject->Handle = Connection;
		ConnectionObject->Data = Std$Object$Nil;
		mg_set_user_connection_data(Connection, ConnectionObject);
	};
	Std$Function$result Result;
	switch (Std$Function$call(Callback, 1, &Result, ConnectionObject, 0)) {
	case SUSPEND:
	case SUCCESS:
		return Std$Integer$get_small(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return -1;
	}
};

GLOBAL_METHOD(SetRequestHandler, 3, "set_request_handler", TYP, T, TYP, Std$String$T, ANY) {
	context_t *Context = (context_t *)Args[0].Val;
	const char *Uri = Std$String$flatten(Args[1].Val);
	mg_set_request_handler(Context->Handle, Uri, (void *)riva_request_handler, Args[2].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(SetAuthHandler, 3, "set_auth_handler", TYP, T, TYP, Std$String$T, ANY) {
	context_t *Context = (context_t *)Args[0].Val;
	const char *Uri = Std$String$flatten(Args[1].Val);
	mg_set_auth_handler(Context->Handle, Uri, (void *)riva_request_handler, Args[2].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

typedef struct websocket_callbacks_t {
	Std$Object$t *ConnectHandler;
	Std$Object$t *ReadyHandler;
	Std$Object$t *DataHandler;
	Std$Object$t *CloseHandler;
} websocket_callbacks_t;

static int riva_websocket_connect_handler(struct mg_connection *Connection, websocket_callbacks_t *Callbacks) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	if (!ConnectionObject) {
		ConnectionObject = new(connection_t);
		ConnectionObject->Type = ConnectionT;
		ConnectionObject->Handle = Connection;
		ConnectionObject->Data = Std$Object$Nil;
		mg_set_user_connection_data(Connection, ConnectionObject);
	};
	Std$Function$result Result;
	switch (Std$Function$call(Callbacks->ConnectHandler, 1, &Result, ConnectionObject, 0)) {
	case SUSPEND:
	case SUCCESS:
		return 0;
	case FAILURE:
	case MESSAGE:
		return 1;
	};
};

static void riva_websocket_ready_handler(struct mg_connection *Connection, websocket_callbacks_t *Callbacks) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	Std$Function$result Result;
	Std$Function$call(Callbacks->ReadyHandler, 1, &Result, ConnectionObject, 0);
};

static int riva_websocket_data_handler(struct mg_connection *Connection, int Type, char *Data, size_t Size, websocket_callbacks_t *Callbacks) {
	if (Type & WEBSOCKET_OPCODE_CONNECTION_CLOSE) return 0;
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	Std$Function$result Result;
	switch (Std$Function$call(Callbacks->DataHandler, 2, &Result, ConnectionObject, 0, Std$String$new_length(Data, Size))) {
	case SUSPEND:
	case SUCCESS:
		return 1;
	case FAILURE:
	case MESSAGE:
		return 0;
	};
};

static void riva_websocket_close_handler(const struct mg_connection *Connection, websocket_callbacks_t *Callbacks) {
	connection_t *ConnectionObject = (connection_t *)mg_get_user_connection_data(Connection);
	Std$Function$result Result;
	Std$Function$call(Callbacks->CloseHandler, 1, &Result, ConnectionObject, 0);
};

GLOBAL_METHOD(SetWebSocketHandler, 3, "set_websocket_handler", TYP, T, TYP, Std$String$T, ANY, ANY, ANY, ANY) {
	context_t *Context = (context_t *)Args[0].Val;
	const char *Uri = Std$String$flatten(Args[1].Val);
	websocket_callbacks_t *Callbacks = new(websocket_callbacks_t);
	Callbacks->ConnectHandler = Args[2].Val;
	Callbacks->ReadyHandler = Args[3].Val;
	Callbacks->DataHandler = Args[4].Val;
	Callbacks->CloseHandler = Args[5].Val;
	mg_set_websocket_handler(Context->Handle, Uri,
		(void *)riva_websocket_connect_handler,
		(void *)riva_websocket_ready_handler,
		(void *)riva_websocket_data_handler,
		(void *)riva_websocket_close_handler,
		Callbacks
	);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(GetRequestInfo, 1, "info", TYP, ConnectionT) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	request_info_t *RequestInfo = new(request_info_t);
	RequestInfo->Type = RequestInfoT;
	RequestInfo->Handle = mg_get_request_info(Connection->Handle);
	Result->Val = (Std$Object$t *)RequestInfo;
	return SUCCESS;
};

METHOD("data", TYP, ConnectionT) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	Result->Val = *(Result->Ref = &Connection->Data);
	return SUCCESS;
};

METHOD("send_file", TYP, ConnectionT, TYP, Std$String$T) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	mg_send_file(Connection->Handle, Std$String$flatten(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("send_file", TYP, ConnectionT, TYP, Std$String$T, TYP, Std$String$T) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	mg_send_mime_file(Connection->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

/*METHOD("upload", TYP, ConnectionT, TYP, Std$String$T) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(mg_upload(Connection->Handle, Std$String$flatten(Args[1].Val)));
	return SUCCESS;
};*/

static int riva_field_found(const char *Key, const char *FileName, char *Path, size_t PathLen, void *UserData) {
	
};

static int riva_field_get(const char *Key, const char *Value, size_t *ValueLen, void *UserData) {
	
};

METHOD("handle_form_request", TYP, ConnectionT) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	struct mg_form_data_handler FormDataHandler = {riva_field_found, riva_field_get, 0};
	Result->Val = Std$Integer$new_small(mg_handle_form_request(Connection, &FormDataHandler));
	return SUCCESS;
}

TYPE(OpcodeT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

Std$Integer_smallt OPCODE_CONTINUATION[] = {{OpcodeT, WEBSOCKET_OPCODE_CONTINUATION}};
Std$Integer_smallt OPCODE_TEXT[] = {{OpcodeT, WEBSOCKET_OPCODE_TEXT}};
Std$Integer_smallt OPCODE_BINARY[] = {{OpcodeT, WEBSOCKET_OPCODE_BINARY}};
Std$Integer_smallt OPCODE_CONNECTION_CLOSE[] = {{OpcodeT, WEBSOCKET_OPCODE_CONNECTION_CLOSE}};
Std$Integer_smallt OPCODE_PING[] = {{OpcodeT, WEBSOCKET_OPCODE_PING}};
Std$Integer_smallt OPCODE_PONG[] = {{OpcodeT, WEBSOCKET_OPCODE_PONG}};

METHOD("write", TYP, ConnectionT, TYP, OpcodeT, TYP, Std$String$T) {
	connection_t *Connection = (connection_t *)Args[0].Val;
	int Written = mg_websocket_write(Connection->Handle, Std$Integer$get_small(Args[1].Val), Std$String$flatten(Args[2].Val), Std$String$get_length(Args[2].Val));
	if (Written == 0) return FAILURE;
	if (Written == -1) {
		Result->Val = Std$String$new("Write Error");
		return MESSAGE;
	}
	Result->Val = Std$Integer$new_small(Written);
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$read, ConnectionT, connection_t *Connection, char *Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = mg_read(Connection->Handle, Buffer, Count);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return mg_read(Connection->Handle, Buffer, Count);
	};
};

TYPED_INSTANCE(int, IO$Stream$write, ConnectionT, connection_t *Connection, char *Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = mg_write(Connection->Handle, Buffer, Count);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return mg_write(Connection->Handle, Buffer, Count);
	};
};

METHOD("method", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$String$new(RequestInfo->Handle->request_method);
	return SUCCESS;
};

METHOD("uri", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$String$new(RequestInfo->Handle->request_uri);
	return SUCCESS;
};

METHOD("local", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	if (RequestInfo->Handle->local_uri) {
		Result->Val = Std$String$new(RequestInfo->Handle->local_uri);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("version", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$String$new(RequestInfo->Handle->http_version);
	return SUCCESS;
};

METHOD("query", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$String$new(RequestInfo->Handle->query_string);
	return SUCCESS;
};

METHOD("user", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	if (RequestInfo->Handle->remote_user) {
		Result->Val = Std$String$new(RequestInfo->Handle->remote_user);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("remote", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$String$new(RequestInfo->Handle->remote_addr);
	return SUCCESS;
};

METHOD("length", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$Integer$new_s64(RequestInfo->Handle->content_length);
	return SUCCESS;
};

METHOD("port", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(RequestInfo->Handle->remote_port);
	return SUCCESS;
};

METHOD("ssl", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	if (RequestInfo->Handle->is_ssl) {
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("headers", TYP, RequestInfoT) {
	request_info_t *RequestInfo = (request_info_t *)Args[0].Val;
	Std$Object$t *Headers = Agg$Table$new(0, 0);
	const struct mg_header *Header = RequestInfo->Handle->http_headers;
	char NameBuffer[256];
	for (int I = RequestInfo->Handle->num_headers; --I >= 0;) {
		char *Ptr = NameBuffer;
		const char *Name = Header->name;
		int NameLength = Header->value - Name - 2;
		if (NameLength > 255) NameLength = 255;
		for (int J = NameLength; --J >= 0;) {
			*Ptr = tolower(*Name);
			++Ptr;
			++Name;
		};
		*Ptr = 0;
		Agg$Table$insert(Headers, Std$String$copy_length(NameBuffer, NameLength), Std$String$new(Header->value));
		++Header;
	};
	Result->Val = Headers;
	return SUCCESS;
};

INITIAL() {
	mg_init_library(0);
}