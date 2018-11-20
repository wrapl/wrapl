#include <Std.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Alg/UUID.h>
#include <Sys/Time.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <libpq-fe.h>
#include <catalog/pg_type_d.h>

typedef struct {
	const Std$Type$t *Type;
	PGconn *Handle;
} connection_t;

TYPE(T);

typedef Std$Object$t *(*recv_value_fn)(char *Data, int Length);

typedef struct {
	const Std$Type$t *Type;
	PGresult *Handle;
	int FieldCount, RowCount, Row;
	recv_value_fn *FieldFuncs;
	Agg$List$t *FieldList;
} result_t;

TYPE(ResultT);

typedef struct {
	const char **Keywords;
	const char **Values;
	int Index;
} options_t;

static int parse_option(Std$Object$t *Key, Std$Object$t *Value, options_t *Options) {
	int Index = Options->Index;
	if (Key->Type != Std$String$T || Value->Type != Std$String$T) return 1;
	Options->Keywords[Index] = Std$String$flatten(Key);
	Options->Values[Index] = Std$String$flatten(Value);
	Options->Index = Index + 1;
	return 0;
}

GLOBAL_FUNCTION(Open, 1) {
	CHECK_ARG_TYPE(0, Agg$Table$T);
	Std$Object$t *Settings = Args[0].Val;
	size_t NumSettings = Agg$Table$size(Settings);
	options_t Options = {
		(const char **)Riva$Memory$alloc((NumSettings + 1) * sizeof(char *)),
		(const char **)Riva$Memory$alloc((NumSettings + 1) * sizeof(char *)),
		0
	};
	if (Agg$Table$foreach(Settings, parse_option, &Options)) {
		SEND(Std$String$new("Invalid parameters"));
	}
	connection_t *Conn = new(connection_t);
	Conn->Type = T;
	Conn->Handle = PQconnectdbParams(Options.Keywords, Options.Values, 1);
	if (PQstatus(Conn->Handle) == CONNECTION_BAD) {
		SEND(Std$String$copy(PQerrorMessage(Conn->Handle)));
	} else {
		RETURN(Conn);
	}
}

METHOD("close", TYP, T) {
	RETURN0;
}

static void result_finalize(result_t *Results, void *Data) {
	if (Results->Handle) {
		PQclear(Results->Handle);
		Results->Handle = 0;
	}
}

typedef struct send_t {
	const Std$Type$t *Type;
	char *Value;
	int Length, Binary;
	Oid Oid;
} send_t;

TYPE(SendT);

ASYMBOL(Send);

TYPED_FUNCTION(Std$Object$t *, send_value, Std$Object$t *Value, send_t *Send) {
	if (Value != Std$Object$Nil) {
		Std$Function$result Result[1];
		switch (Std$Function$call(Send, 2, Result, Value, 0, Send, 0)) {
		case SUSPEND: case SUCCESS: return 0;
		case FAILURE: return Std$String$new("Send failed");
		case MESSAGE: return Result->Val;
		}
	}
	Send->Oid = 0;
	Send->Value = 0;
	Send->Length = 0;
	Send->Binary = 0;
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Std$Symbol$T, Std$Object$t *Value, send_t *Send) {
	if (Value == $true) {
		Send->Oid = BOOLOID;
		Send->Value = "t";
		Send->Length = 1;
		return 0;
	} else if (Value == $false) {
		Send->Oid = BOOLOID;
		Send->Value = "f";
		Send->Length = 1;
		return 0;
	} else {
		return Std$String$new("Unknown symbol for send");
	}
}

TYPED_INSTANCE(Std$Object$t *, send_value, Std$Integer$SmallT, Std$Object$t *Value, send_t *Send) {
	Send->Oid = INT4OID;
	Send->Length = asprintf(&Send->Value, "%d", Std$Integer$get_small(Value));
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Std$Real$T, Std$Object$t *Value, send_t *Send) {
	Send->Oid = FLOAT8OID;
	Send->Length = asprintf(&Send->Value, "%g", Std$Real$get_value(Value));
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Std$String$T, Std$Object$t *Value, send_t *Send) {
	Send->Oid = VARCHAROID;
	Send->Value = Std$String$flatten(Value);
	Send->Length = Std$String$get_length(Value);
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Sys$Time$T, Std$Object$t *Value, send_t *Send) {
	Send->Oid = TIMESTAMPOID;
	struct tm TM[1];
	localtime_r(&((Sys$Time$t *)Value)->Value, TM);
	Send->Value = Riva$Memory$alloc_atomic(40);
	Send->Length = strftime(Send->Value, 40, "%F %T%z", TM);
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Sys$Time$PreciseT, Std$Object$t *Value, send_t *Send) {
	Send->Oid = TIMESTAMPOID;
	struct tm TM[1];
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Value;
	localtime_r(&Time->Value.tv_sec, TM);
	char *Buffer = Send->Value = Riva$Memory$alloc_atomic(40);
	Buffer += strftime(Send->Value, 40, "%F %T%z", TM);
	Buffer += sprintf(Buffer, ".%06ld", Time->Value.tv_usec);
	Buffer += strftime(Buffer, 40, "%z", TM);
	Send->Length = Buffer - Send->Value;
	return 0;
}

TYPED_INSTANCE(Std$Object$t *, send_value, Alg$UUID$T, Std$Object$t *Value, send_t *Send) {
	Send->Oid = UUIDOID;
	Send->Value = Riva$Memory$alloc_atomic(37);
	uuid_unparse(((Alg$UUID$t *)Value)->Value, Send->Value);
	Send->Length = 36;
	return 0;
}

static Std$Object$t *recv_nil(char *Data, int Length) {
	return Std$Object$Nil;
}

static Std$Object$t *recv_bool_text(char *Data, int Length) {
	if (Data[0] == 't') return $true;
	if (Data[0] == 'f') return $false;
	return Std$Object$Nil;
}

static Std$Object$t *recv_int4_text(char *Data, int Length) {
	return Std$Integer$new_small(strtol(Data, 0, 0));
}

static Std$Object$t *recv_float8_text(char *Data, int Length) {
	return Std$Real$new(strtod(Data, 0));
}

static Std$Object$t *recv_varchar_text(char *Data, int Length) {
	return Std$String$copy_length(Data, Length);
}

static Std$Object$t *recv_timestamp_text(char *Data, int Length) {
	struct tm TM[1];
	Data = strptime(Data, "%Y-%m-%d %H:%M:%S", TM);
	unsigned int US = 0;
	if (Data[0] == '.') {
		++Data;
		char *End;
		US = strtol(Data, &End, 10);
		int Size = End - Data;
		while (Size > 6) { US /= 10; --Size; }
		while (Size < 6) { US *= 10; ++Size; }
	}
	if (Data[0] == '+') strptime(Data, "%z", TM);
	Sys$Time$precise_t *Time = new(Sys$Time$precise_t);
	Time->Type = Sys$Time$PreciseT;
	Time->Value.tv_sec = mktime(TM);
	Time->Value.tv_usec = US;
	return (Std$Object$t *)Time;
}

recv_value_fn lookup_recv_value_fn(const PGresult *Res, int Col) {
	switch (PQfformat(Res, Col)) {
	case 0:
		switch (PQftype(Res, Col)) {
		case BOOLOID:
			return recv_bool_text;
		case INT4OID:
			return recv_int4_text;
		case FLOAT4OID:
		case FLOAT8OID:
			return recv_float8_text;
		case BPCHAROID:
		case VARCHAROID:
		case TEXTOID:
		case BYTEAOID:
			return recv_varchar_text;
		case TIMESTAMPOID:
		case TIMESTAMPTZOID:
			return recv_timestamp_text;
		}
		printf("Unknown OID: %d\n", PQftype(Res, Col));
		return recv_nil;
	case 1:
		return recv_nil;
	default:
		return recv_nil;
	}
}

METHOD("exec", TYP, T, TYP, Std$String$T) {
	connection_t *Conn = (connection_t *)Args[0].Val;
	const char *Command = Std$String$flatten(Args[1].Val);
	int ParamCount = Count - 2;
	Oid *ParamTypes = 0;
	const char **ParamValues = 0;
	int *ParamLengths = 0;
	int *ParamFormats = 0;
	if (ParamCount > 0) {
		ParamTypes = (Oid *)Riva$Memory$alloc(ParamCount * sizeof(Oid));
		ParamValues = (const char **)Riva$Memory$alloc(ParamCount * sizeof(const char *));
		ParamLengths = (int *)Riva$Memory$alloc(ParamCount * sizeof(int));
		ParamFormats = (int *)Riva$Memory$alloc(ParamCount * sizeof(int));
		for (int I = 0; I < ParamCount; ++I) {
			send_t SendValue[1] = {{SendT, 0, 0, 0, 0}};
			Std$Object$t *Message = send_value(Args[I + 2].Val, SendValue);
			if (Message) SEND(Message);
			ParamTypes[I] = SendValue->Oid;
			ParamValues[I] = SendValue->Value;
			ParamLengths[I] = SendValue->Length;
			ParamFormats[I] = SendValue->Binary;
		}
	}
	// TODO: Change 0 -> 1 in next line to switch to binary format, once implemented.
	PGresult *Res = PQexecParams(Conn->Handle, Command, ParamCount, ParamTypes, ParamValues, ParamLengths, ParamFormats, 0);
	switch (PQresultStatus(Res)) {
	case PGRES_EMPTY_QUERY:
		PQclear(Res);
		return FAILURE;
	case PGRES_COMMAND_OK:
		PQclear(Res);
		Result->Arg = Args[0];
		return SUCCESS;
	case PGRES_TUPLES_OK:
	case PGRES_SINGLE_TUPLE: {
		result_t *Results = new(result_t);
		Results->Type = ResultT;
		Results->Handle = Res;
		Results->RowCount = PQntuples(Res);
		Results->Row = 0;
		int FieldCount = Results->FieldCount = PQnfields(Res);
		Agg$List$t *FieldList = Results->FieldList = (Agg$List$t *)Agg$List$new0();
		recv_value_fn *FieldFuncs = Results->FieldFuncs = (recv_value_fn *)Riva$Memory$alloc(FieldCount * sizeof(recv_value_fn));
		for (int I = 0; I < FieldCount; ++I) {
			FieldFuncs[I] = lookup_recv_value_fn(Res, I);
			Agg$List$put((Std$Object$t *)FieldList, Std$String$copy(PQfname(Res, I)));
		}
		Riva$Memory$register_finalizer((void *)Results, (void *)result_finalize, 0, 0, 0);
		Result->Val = (Std$Object$t *)Results;
		return SUCCESS;
	}
	case PGRES_FATAL_ERROR:
		Result->Val = Std$String$copy(PQresultErrorMessage(Res));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Postgres exec: Unknown result type");
		return MESSAGE;
	}
}

METHOD("rows", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Results->RowCount);
	return SUCCESS;
}

METHOD("fields", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Results->FieldList;
	return SUCCESS;
}

METHOD("list", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	if (Results->Row >= Results->RowCount) return FAILURE;
	Std$Object$t *List = Agg$List$new0();
	for (int Field = 0; Field < Results->FieldCount; ++Field) {
		if (PQgetisnull(Results->Handle, Results->Row, Field)) {
			Agg$List$put(List, Std$Object$Nil);
		} else {
			Agg$List$put(List, Results->FieldFuncs[Field](PQgetvalue(Results->Handle, Results->Row, Field), PQgetlength(Results->Handle, Results->Row, Field)));
		}
	}
	++Results->Row;
	Result->Val = List;
	return SUCCESS;
}

METHOD("table", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	if (Results->Row >= Results->RowCount) return FAILURE;
	Std$Object$t *Table = Agg$Table$new(0, 0);
	Agg$List$node *Node = Results->FieldList->Head;
	for (int Field = 0; Field < Results->FieldCount; ++Field) {
		if (PQgetisnull(Results->Handle, Results->Row, Field)) {
			Agg$Table$insert(Table, Node->Value, Std$Object$Nil);
		} else {
			Agg$Table$insert(Table, Node->Value, Results->FieldFuncs[Field](PQgetvalue(Results->Handle, Results->Row, Field), PQgetlength(Results->Handle, Results->Row, Field)));
		}
	}
	++Results->Row;
	Result->Val = Table;
	return SUCCESS;
}

METHOD("close", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	if (Results->Handle) {
		PQclear(Results->Handle);
		Results->Handle = 0;
	}
	return SUCCESS;
}
