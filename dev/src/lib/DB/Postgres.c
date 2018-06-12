#include <Std.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Alg/UUID.h>
#include <Sys/Time.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <libpq-fe.h>
#include <postgres_fe.h>
#include <catalog/pg_type.h>

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
	Options->Keywords[Index] = Std$String$flatten(Key);
	Options->Values[Index] = Std$String$flatten(Value);
	Options->Index = Index + 1;
	return 0;
};

GLOBAL_FUNCTION(Open, 1) {
	CHECK_ARG_TYPE(0, Agg$Table$T);
	Std$Object$t *Settings = Args[0].Val;
	size_t NumSettings = Agg$Table$size(Settings);
	options_t Options = {
		(const char **)Riva$Memory$alloc((NumSettings + 1) * sizeof(char *)),
		(const char **)Riva$Memory$alloc((NumSettings + 1) * sizeof(char *)),
		0
	};
	Agg$Table$foreach(Settings, parse_option, &Options);
	connection_t *Conn = new(connection_t);
	Conn->Type = T;
	Conn->Handle = PQconnectdbParams(Options.Keywords, Options.Values, 1);
	Result->Val = (Std$Object_t *)Conn;
	return SUCCESS;
};

METHOD("close", TYP, T) {
};

static void result_finalize(result_t *Results, void *Data) {
	if (Results->Handle) {
		PQclear(Results->Handle);
		Results->Handle = 0;
	};
};

TYPE(TypeT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

Std$Integer$smallt NULLTYPE[1] = {{TypeT, 0}};
Std$Integer$smallt INT4TYPE[1] = {{TypeT, INT4OID}};
Std$Integer$smallt FLOAT8TYPE[1] = {{TypeT, FLOAT8OID}};
Std$Integer$smallt VARCHARTYPE[1] = {{TypeT, VARCHAROID}};
Std$Integer$smallt BOOLTYPE[1] = {{TypeT, BOOLOID}};
Std$Integer$smallt TIMESTAMPTYPE[1] = {{TypeT, TIMESTAMPOID}};
Std$Integer$smallt BYTEATYPE[1] = {{TypeT, BYTEAOID}};
Std$Integer$smallt UUIDTYPE[1] = {{TypeT, UUIDOID}};

typedef struct send_t {
	const Std$Type$t *Type;
	Std$Object$t *Oid;
	Std$Object$t *Value;
	Std$Object$t *Binary;
} send_t;

TYPE(SendT);

METHOD("type", TYP, SendT) {
	send_t *Send = (send_t *)Args[1].Val;
	Result->Val = *(Result->Ref = &Send->Oid);
	return SUCCESS;
};

METHOD("value", TYP, SendT) {
	send_t *Send = (send_t *)Args[1].Val;
	Result->Val = *(Result->Ref = &Send->Value);
	return SUCCESS;
};

METHOD("binary", TYP, SendT) {
	send_t *Send = (send_t *)Args[1].Val;
	Result->Val = *(Result->Ref = &Send->Binary);
	return SUCCESS;
};

ASYMBOL(Send);

AMETHOD(Send, VAL, Std$Object$Nil, TYP, SendT) {
	send_t *Send = (send_t *)Args[1].Val;
	Send->Oid = NULLTYPE;
	Send->Value = Std$String$Nil;
	return SUCCESS;
};

STRING(StringTrue, "t");
STRING(StringFalse, "f");

AMETHOD(Send, VAL, $true, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = BOOLTYPE;
	SendValue->Value = StringTrue;
	return SUCCESS;
};

AMETHOD(Send, VAL, $false, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = BOOLTYPE;
	SendValue->Value = StringFalse;
	return SUCCESS;
};

AMETHOD(Send, TYP, Std$Integer$SmallT, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = INT4TYPE;
	char *Temp;
	int Length = asprintf(&Temp, "%d", Std$Integer$get_small(Args[0].Val));
	SendValue->Value = Std$String$new_length(Temp, Length);
	return SUCCESS;
};

AMETHOD(Send, TYP, Std$Real$T, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = FLOAT8TYPE;
	char *Temp;
	int Length = asprintf(&Temp, "%g", Std$Real$get_value(Args[0].Val));
	SendValue->Value = Std$String$new_length(Temp, Length);
	return SUCCESS;
};

AMETHOD(Send, TYP, Std$String$T, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = BYTEATYPE;
	SendValue->Value = Args[0].Val;
	SendValue->Binary = $true;
	return SUCCESS;
};

AMETHOD(Send, TYP, Sys$Time$T, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = TIMESTAMPTYPE;
	struct tm TM[1];
	localtime_r(&((Sys$Time$t *)Args[0].Val)->Value, TM);
	char *Temp = Riva$Memory$alloc_atomic(40);
	int Length = strftime(Temp, 40, "%F %T%z", TM);
	SendValue->Value = Std$String$new_length(Temp, Length);
	return SUCCESS;
};

AMETHOD(Send, TYP, Sys$Time$PreciseT, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	Sys$Time$precise_t *Time = (Sys$Time$precise_t *)Args[0].Val;
	SendValue->Oid = TIMESTAMPTYPE;
	struct tm TM[1];
	localtime_r(&Time->Value.tv_sec, TM);
	char *Temp = Riva$Memory$alloc_atomic(40);
	char *End = Temp;
	End += strftime(End, 40, "%F %T", TM);
	End += sprintf(End, ".%06ld", Time->Value.tv_usec);
	End += strftime(End, 40, "%z", TM);
	SendValue->Value = Std$String$new_length(Temp, End - Temp);
	return SUCCESS;
};

AMETHOD(Send, TYP, Alg$UUID$T, TYP, SendT) {
	send_t *SendValue = (send_t *)Args[1].Val;
	SendValue->Oid = UUIDTYPE;
	char *Temp = Riva$Memory$alloc_atomic(37);
	uuid_unparse(((Alg$UUID$t *)Args[0].Val)->Value, Temp);
	SendValue->Value = Std$String$new_length(Temp, 36);
	SendValue->Binary = $true;
	return SUCCESS;
};

static Std$Object$t *recv_nil(char *Data, int Length) {
	return Std$Object$Nil;
};

static Std$Object$t *recv_bool_text(char *Data, int Length) {
	if (Data[0] == 't') return $true;
	if (Data[0] == 'f') return $false;
	return Std$Object$Nil;
};

static Std$Object$t *recv_int4_text(char *Data, int Length) {
	return Std$Integer$new_small(strtol(Data, 0, 0));
};

static Std$Object$t *recv_float8_text(char *Data, int Length) {
	return Std$Real$new(strtod(Data, 0));
};

static Std$Object$t *recv_varchar_text(char *Data, int Length) {
	return Std$String$copy_length(Data, Length);
};

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
	};
	if (Data[0] == '+') strptime(Data, "%z", TM);
	Sys$Time$precise_t *Time = new(Sys$Time$precise_t);
	Time->Type = Sys$Time$PreciseT;
	Time->Value.tv_sec = mktime(TM);
	Time->Value.tv_usec = US;
	return (Std$Object$t *)Time;
};

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
		default:
			printf("Unknown OID: %d\n", PQftype(Res, Col));
			return recv_nil;
		};
	case 1:
		return recv_nil;
	};
};

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
			send_t SendValue[1] = {{SendT, NULLTYPE, Std$String$Nil, $false}};
			switch (Std$Function$call(Send, 2, Result, Args[I + 2].Val, 0, SendValue, 0)) {
			case SUSPEND: case SUCCESS:
				if (SendValue->Oid->Type != TypeT) {
					Result->Val = Std$String$new("Postgres exec: Invalid type");
					return MESSAGE;
				};
				if (SendValue->Value->Type != Std$String$T) {
					Result->Val = Std$String$new("Postgres exec: Invalid value");
					return MESSAGE;
				}
				ParamTypes[I] = Std$Integer$get_small(SendValue->Oid);
				ParamValues[I] = Std$String$flatten(SendValue->Value);
				ParamLengths[I] = Std$String$get_length(SendValue->Value);
				ParamFormats[I] = SendValue->Binary == $true;
				break;
			case FAILURE:
				return FAILURE;
			case MESSAGE:
				return MESSAGE;
			};
		};
	};
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
		};
		Riva$Memory$register_finalizer((void *)Results, (void *)result_finalize, 0, 0, 0);
		Result->Val = (Std$Object$t *)Results;
		return SUCCESS;
	};
	case PGRES_FATAL_ERROR:
		Result->Val = Std$String$copy(PQresultErrorMessage(Res));
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Postgres exec: Unknown result type");
		return MESSAGE;
	};
};

METHOD("rows", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Results->RowCount);
	return SUCCESS;
};

METHOD("fields", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	Result->Val = (Std$Object$t *)Results->FieldList;
	return SUCCESS;
};

METHOD("list", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	if (Results->Row >= Results->RowCount) return FAILURE;
	Std$Object$t *List = Agg$List$new0();
	for (int Field = 0; Field < Results->FieldCount; ++Field) {
		if (PQgetisnull(Results->Handle, Results->Row, Field)) {
			Agg$List$put(List, Std$Object$Nil);
		} else {
			Agg$List$put(List, Results->FieldFuncs[Field](PQgetvalue(Results->Handle, Results->Row, Field), PQgetlength(Results->Handle, Results->Row, Field)));
		};
	};
	++Results->Row;
	Result->Val = List;
	return SUCCESS;
};

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
		};
	};
	++Results->Row;
	Result->Val = Table;
	return SUCCESS;
};

METHOD("close", TYP, ResultT) {
	result_t *Results = (result_t *)Args[0].Val;
	if (Results->Handle) {
		PQclear(Results->Handle);
		Results->Handle = 0;
	};
	return SUCCESS;
};
