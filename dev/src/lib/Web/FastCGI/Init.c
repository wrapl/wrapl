#include <Std.h>
#include <Riva.h>
#include <Agg/Table.h>
#include <Util/TypedFunction.h>
#include <IO/Stream.h>
#include <fcgiapp.h>
#include <string.h>

typedef struct stream_t {
	Std$Type$t *Type;
	FCGX_Stream *Handle;
} stream_t;

TYPE(StreamT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

static inline stream_t *stream_new(FCGX_Stream *Handle) {
	stream_t *Stream = new(stream_t);
	Stream->Type = StreamT;
	Stream->Handle = Handle;
	return Stream;
};

static int fcgi_read(stream_t *Stream, char *Buffer, int Count, int Block) {
	if (Stream->Handle == 0) printf("\e[31m******** Stream(0x%x)->Handle = 0 ********\e[0m\n", Stream);
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = FCGX_GetStr(Buffer, Count, Stream->Handle);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return FCGX_GetStr(Buffer, Count, Stream->Handle);
	};
};

METHOD("read", TYP, StreamT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	stream_t *Stream = (stream_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesRead = fcgi_read(Stream, Buffer, Size, Count > 3 && Args[3].Val == $true);
	if (BytesRead < 0) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

static int fcgi_close(stream_t *Stream, int Mode) {
	FCGX_FClose(Stream->Handle);
};

static int fcgi_flush(stream_t *Stream) {
	FCGX_FFlush(Stream->Handle);
};

static int fcgi_write(stream_t *Stream, char *Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = FCGX_PutStr(Buffer, Count, Stream->Handle);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return FCGX_PutStr(Buffer, Count, Stream->Handle);
	};
};

METHOD("write", TYP, StreamT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	stream_t *Stream = (stream_t *)Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	int Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	int BytesWritten = fcgi_write(Stream, Buffer, Size, Count > 3 && Args[3].Val == $true);
	if (BytesWritten < 0) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

METHOD("close", TYP, StreamT) {
	stream_t *Stream = Args[0].Val;
	FCGX_FClose(Stream->Handle);
	return SUCCESS;
};

METHOD("flush", TYP, StreamT) {
	stream_t *Stream = Args[0].Val;
	FCGX_FFlush(Stream->Handle);
	return SUCCESS;
};

typedef struct socket_t {
	const Std$Type$t *Type;
	int Handle;
} socket_t;

TYPE(SocketT);

GLOBAL_FUNCTION(SocketNew, 1) {
	int BackLog = 10;
	if (Count > 1) BackLog = Std$Integer$get_small(Args[1].Val);
	int Handle = FCGX_OpenSocket(Std$String$flatten(Args[0].Val), BackLog);
	if (Handle == -1) {
		Result->Val = Std$String$new("Error creating socket\n");
		return MESSAGE;
	} else {
		socket_t *Socket = new(socket_t);
		Socket->Type = SocketT;
		Socket->Handle = Handle;
		Result->Val = (Std$Object$t *)Socket;
		return SUCCESS;
	};
};

typedef struct request_t {
	const Std$Type$t *Type;
	stream_t *In, *Out, *Err;
	FCGX_Request Handle[1];
} request_t;

TYPE(RequestT);

GLOBAL_FUNCTION(RequestNew, 0) {
//@socket:Std$String$T=0
// Creates a new <id>RequestT</id> object for receiving requests.
	int Socket = 0;
	if (Count > 0) Socket = ((socket_t *)Args[0].Val)->Handle;
	request_t *Request = new(request_t);
	Request->Type = RequestT;
	if (FCGX_InitRequest(Request->Handle, Socket, 0) == -1) {
		Result->Val = Std$String$new("Error initializing request\n");
		return MESSAGE;
	};
	Request->In = Std$Object$Nil;
	Request->Out = Std$Object$Nil;
	Request->Err = Std$Object$Nil;
	Result->Val = Request;
	return SUCCESS;
};

METHOD("accept", TYP, RequestT) {
//@request
// Accepts a new request.
	request_t *Request = Args[0].Val;
	int Errno = FCGX_Accept_r(Request->Handle);
	if (Errno) {
		printf("\e[31mErrno = %d\e[0m\n", Errno);
		exit(-1);
		Result->Val = Std$String$new_format("Error accepting request\n");
		return MESSAGE;
	};
	Request->In = stream_new(Request->Handle->in);
	Request->Out = stream_new(Request->Handle->out);
	Request->Err = stream_new(Request->Handle->err);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("finish", TYP, RequestT) {
//@request
// Finishes the current request.
	request_t *Request = Args[0].Val;
	FCGX_Finish_r(Request->Handle);
	Request->In = Std$Object$Nil;
	Request->Out = Std$Object$Nil;
	Request->Err = Std$Object$Nil;
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("free", TYP, RequestT, TYP, Std$Symbol$T) {
//@request
// Frees the current request.
	request_t *Request = Args[0].Val;
	FCGX_Free(Request->Handle, Args[1].Val == $true);
	return SUCCESS;
};

METHOD("in", TYP, RequestT) {
//@request
//:StreamT
// Returns the input stream for the current request. POST data should be read from this stream.
	request_t *Request = Args[0].Val;
	//if (Request->In->Handle == 0) printf("\e[31m******** Request->In->Handle = 0 ********\e[0m\n");
	Result->Val = Request->In;
	return SUCCESS;
};

METHOD("out", TYP, RequestT) {
//@request
//:StreamT
// Returns the output stream for the current request. The generated output (including headers) should be written to this stream.
	request_t *Request = Args[0].Val;
	//if (Request->Out->Handle == 0) printf("\e[31m******** Request->Out->Handle = 0 ********\e[0m\n");
	Result->Val = Request->Out;
	return SUCCESS;
};

METHOD("err", TYP, RequestT) {
//@request
//:StreamT
// Returns the error stream for the current request.
	request_t *Request = Args[0].Val;
	//if (Request->Err->Handle == 0) printf("\e[31m******** Request->Err->Handle = 0 ********\e[0m\n");
	Result->Val = Request->Err;
	return SUCCESS;
};

METHOD("env", TYP, RequestT, TYP, Std$String$T) {
//@request
//@name
//:Std$String$T
// Returns the value of the environment variable <var>name</var> in the current request.
	request_t *Request = Args[0].Val;
	const char *Key = Std$String$flatten(Args[1].Val);
	const char *Value = FCGX_GetParam(Key, Request->Handle->envp);
	if (Value == 0) return FAILURE;
	Result->Val = Std$String$copy(Value);
	return SUCCESS;
};

METHOD("envs", TYP, RequestT) {
//@request
//:Agg$Table$T
// Returns a table of all environment variables in the current request.
	request_t *Request = Args[0].Val;
	Std$Object$t *Env = Agg$Table$new(0, 0);
	if (Request->Handle->envp) for (char **Ptr = Request->Handle->envp; *Ptr; ++Ptr) {
		char *Key = *Ptr;
		char *Value = strchr(Key, '=');
		Agg$Table$insert(Env, Std$String$copy_length(Key, Value - Key), Std$String$copy(Value + 1));
	};
	Result->Val = Env;
	return SUCCESS;
};

INITIAL() {
	FCGX_Init();
	Util$TypedFunction$set(IO$Stream$close, StreamT, fcgi_close);
//	Util$TypedFunction$set(IO$Stream$eoi, StreamT, fcgi_eoi);
	Util$TypedFunction$set(IO$Stream$read, StreamT, fcgi_read);
//	Util$TypedFunction$set(IO$Stream$readx, StreamT, fcgi_readx);
//	Util$TypedFunction$set(IO$Stream$readi, StreamT, fcgi_readi);
//	Util$TypedFunction$set(IO$Stream$readc, StreamT, fcgi_readc);
//	Util$TypedFunction$set(IO$Stream$readl, StreamT, fcgi_readl);
	Util$TypedFunction$set(IO$Stream$flush, StreamT, fcgi_flush);
	Util$TypedFunction$set(IO$Stream$write, StreamT, fcgi_write);
//	Util$TypedFunction$set(IO$Stream$writec, StreamT, fcgi_writec);
//	Util$TypedFunction$set(IO$Stream$writes, StreamT, fcgi_writes);
//	Util$TypedFunction$set(IO$Stream$writef, StreamT, fcgi_writef);
//	Util$TypedFunction$set(IO$Stream$seek, StreamT, fcgi_seek);
//	Util$TypedFunction$set(IO$Stream$tell, StreamT, fcgi_tell);
};
