#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

typedef struct redis_t {
	const Std$Type$t *Type;
	Std$Function$status (*Invoke)(FUNCTION_PARAMS);
	redisContext *Handle;
} redis_t;

FUNCTIONAL_TYPE(T, Std$Function$CT);

typedef struct redis_async_t {
	const Std$Type$t *Type;
	redisAsyncContext *Handle;
} redis_async_t;

TYPE(AsyncT);

TYPE(StatusT, Std$String$T);
TYPE(ErrorT, Std$String$T);

static Std$Object$t *reply_to_riva(redisReply *Reply) {
	switch (Reply->type) {
	case REDIS_REPLY_STATUS: {
		Std$Object$t *Status = Std$String$copy_length(Reply->str, Reply->len);
		Status->Type = StatusT;
		return Status;
	};
	case REDIS_REPLY_ERROR: {
		Std$Object$t *Error = Std$String$copy_length(Reply->str, Reply->len);
		Error->Type = StatusT;
		return Error;
	};
	case REDIS_REPLY_INTEGER: return Std$Integer$new_s64(Reply->integer);
	case REDIS_REPLY_NIL: return Std$Object$Nil;
	case REDIS_REPLY_STRING: return Std$String$copy_length(Reply->str, Reply->len);
	case REDIS_REPLY_ARRAY: {
		Std$Object$t *Array = Agg$List$new0();
		for (size_t I = 0; I < Reply->elements; ++I) Agg$List$put(Array, reply_to_riva(Reply->element[I]));
		return Array;
	};
	};
};

static Std$Function$status redis_invoke(FUNCTION_PARAMS) {
	redis_t *Redis = (redis_t *)Fun;
	int Argc = Count;
	const char **Argv = (const char **)Riva$Memory$alloc(Argc * sizeof(const char *));
	size_t *ArgvLen = (size_t *)Riva$Memory$alloc(Argc * sizeof(size_t));
	for (size_t I = 0; I < Argc; ++I) {
		Std$Function$result Result0;
		switch (Std$Function$call(Std$String$Of, 1, &Result0, Args[I].Val, Args[I].Ref)) {
		case SUSPEND: case SUCCESS:
			Argv[I] = Std$String$flatten(Result0.Val);
			ArgvLen[I] = ((Std$String$t *)Result0.Val)->Length.Value;
			break;
		case FAILURE: case MESSAGE:
			Result->Val = Std$String$new("Could not convert parameter to string");
			return MESSAGE;
		};
	};
	redisReply *Reply = redisCommandArgv(Redis->Handle, Argc, Argv, ArgvLen);
	if (Reply) {
		Result->Val = reply_to_riva(Reply);
		freeReplyObject(Reply);
		return SUCCESS;
	} else {
		Result->Val = Std$String$copy(Redis->Handle->errstr);
		return MESSAGE;
	};
};

GLOBAL_FUNCTION(New, 2) {
	redis_t *Redis = new(redis_t);
	Redis->Type = T;
	Redis->Invoke = redis_invoke;
	Redis->Handle = redisConnect(Std$String$flatten(Args[0].Val), Std$Integer$get_small(Args[1].Val));
	Result->Val = (Std$Object$t *)Redis;
	return SUCCESS;
};

METHOD("command", TYP, T, TYP, Std$String$T) {
	redis_t *Redis = (redis_t *)Args[0].Val;
	int Argc = Count - 1;
	const char **Argv = (const char **)Riva$Memory$alloc(Argc * sizeof(const char *));
	size_t *ArgvLen = (size_t *)Riva$Memory$alloc(Argc * sizeof(size_t));
	for (size_t I = 0; I < Argc; ++I) {
		Std$Function$result Result0;
		switch (Std$Function$call(Std$String$Of, 1, &Result0, Args[I + 1].Val, Args[I + 1].Ref)) {
		case SUSPEND: case SUCCESS:
			Argv[I] = Std$String$flatten(Result0.Val);
			ArgvLen[I] = ((Std$String$t *)Result0.Val)->Length.Value;
			break;
		case FAILURE: case MESSAGE:
			Result->Val = Std$String$new("Could not convert parameter to string");
			return MESSAGE;
		};
	};
	redisReply *Reply = redisCommandArgv(Redis->Handle, Argc, Argv, ArgvLen);
	if (Reply) {
		Result->Val = reply_to_riva(Reply);
		freeReplyObject(Reply);
		return SUCCESS;
	} else {
		Result->Val = Std$String$copy(Redis->Handle->errstr);
		return MESSAGE;
	};
};

GLOBAL_FUNCTION(NewAsync, 2) {
	redis_async_t *Redis = new(redis_async_t);
	Redis->Type = AsyncT;
	Redis->Handle = redisAsyncConnect(Std$String$flatten(Args[0].Val), Std$Integer$get_small(Args[1].Val));
	if (Redis->Handle->err) {
		Result->Val = Std$String$copy(Redis->Handle->errstr);
		return MESSAGE;
	} else {
		Redis->Handle->data = Redis;
		Result->Val = (Std$Object$t *)Redis;
		return SUCCESS;
	};
};

static void async_callback(redisAsyncContext *Context, redisReply *Reply, Std$Object$t *Function) {
	printf("At least we get here!\n");
	Std$Function$result Result;
	if (Reply == 0) {
		Std$Object$t *Error = Std$String$copy(Context->errstr);
		Error->Type = ErrorT;
		Std$Function$call(Function, 2, &Result, Error, 0, Context->data, 0);
	} else {
		Std$Function$call(Function, 2, &Result, reply_to_riva(Reply), 0, Context->data, 0);
		freeReplyObject(Reply);
	};
};

METHOD("command", TYP, AsyncT, ANY, TYP, Std$String$T) {
	redis_async_t *Redis = (redis_async_t *)Args[0].Val;
	int Argc = Count - 2;
	const char **Argv = (const char **)Riva$Memory$alloc(Argc * sizeof(const char *));
	size_t *ArgvLen = (size_t *)Riva$Memory$alloc(Argc * sizeof(size_t));
	for (size_t I = 0; I < Argc; ++I) {
		Std$Function$result Result0;
		switch (Std$Function$call(Std$String$Of, 1, &Result0, Args[I + 2].Val, Args[I + 2].Ref)) {
		case SUSPEND: case SUCCESS:
			Argv[I] = Std$String$flatten(Result0.Val);
			ArgvLen[I] = ((Std$String$t *)Result0.Val)->Length.Value;
			break;
		case FAILURE: case MESSAGE:
			Result->Val = Std$String$new("Could not convert parameter to string");
			return MESSAGE;
		};
	};
	Result->Arg = Args[0];
	int Status;
	if (Args[1].Val == Std$Object$Nil) {
		Status = redisAsyncCommandArgv(Redis->Handle, 0, 0, Argc, Argv, ArgvLen);
	} else {
		Status = redisAsyncCommandArgv(Redis->Handle, (void *)async_callback, Args[1].Val, Argc, Argv, ArgvLen);
	};
	return (Status == REDIS_OK) ? SUCCESS : FAILURE;
};

METHOD("disconnect", TYP, AsyncT) {
	redis_async_t *Redis = (redis_async_t *)Args[0].Val;
	redisAsyncDisconnect(Redis->Handle);
	return SUCCESS;
};
