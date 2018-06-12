#include <Riva.h>
#include <Std.h>
#include <Agg.h>

#include <mysql/mysql.h>

typedef struct {
	const Std$Type_t *Type;
	int Open;
	MYSQL Handle[1];
} connection_t;

TYPE(T);

typedef struct {
	const Std$Type_t *Type;
	MYSQL_RES *Handle;
	int FieldCount;
	MYSQL_FIELD *Fields;
	Agg$List_t *FieldList;
	Std$Symbol_t **FieldSymbols;
} result_t;

TYPE(ResultT);

static void connection_finalize(connection_t *Connection, void *Data) {
	if (Connection->Open) {
		mysql_close(Connection->Handle);
		Connection->Open = 0;
	};
};

GLOBAL_FUNCTION(Open, 4) {
//@host : Std$String$T
//@user : Std$String$T
//@password : Std$String$T
//@database : Std$String$T = NIL
	connection_t *Connection = new(connection_t);
	Connection->Type = T;
	mysql_init(Connection->Handle);
	int Port = 0;
	const char *Socket = 0;
	if (Count > 4) {
		if (Args[4].Val->Type == Std$String$T) {
			Socket = Std$String$flatten(Args[4].Val);
		} else if (Args[4].Val->Type == Std$Integer$SmallT) {
			Port = ((Std$Integer_smallt *)Args[4].Val)->Value;
		};
	};
	if (mysql_real_connect(Connection->Handle,
		Std$String$flatten(Args[0].Val),
		Std$String$flatten(Args[1].Val),
		Std$String$flatten(Args[2].Val),
		Std$String$flatten(Args[3].Val),
		Port,
		Socket,
		0
	) == 0) {
		Result->Val = Std$String$new(mysql_error(Connection->Handle));
		return MESSAGE;
	};
	Connection->Open = 1;
	Riva$Memory$register_finalizer((void *)Connection, (void *)connection_finalize, 0, 0, 0);
	Result->Val = Connection;
	return SUCCESS;
};

METHOD("close", TYP, T) {
	connection_t *Connection = Args[0].Val;
	mysql_close(Connection->Handle);
	Connection->Open = 0;
	Riva$Memory$register_finalizer((void *)Connection, 0, 0, 0, 0);
	return SUCCESS;
};

METHOD("commit", TYP, T) {
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	if (mysql_commit(Handle)) {
		Result->Val = Std$String$new(mysql_error(Handle));
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("escape", TYP, T, TYP, Std$String$T) {
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	Std$String$t *In = (Std$String$t *)Args[1].Val;
	char Out[In->Length.Value * 2 + 1];
	Result->Val = Std$String$copy_length(Out, mysql_real_escape_string(Handle, Out, Std$String$content(In), In->Length.Value));
	return SUCCESS;
};

static void result_finalize(result_t *Result, void *Data) {
	if (Result->Handle) {
		mysql_free_result(Result->Handle);
		Result->Handle = 0;
	};
};

METHOD("exec", TYP, T, TYP, Std$String$T) {
//@connection
//@query
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	if (mysql_real_query(Handle, Std$String$flatten(Args[1].Val), ((Std$String_t *)Args[1].Val)->Length.Value)) {
		Result->Val = Std$String$new(mysql_error(Handle));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("insert_id", TYP, T) {
//@connection
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	Result->Val = Std$Integer$new_u64(mysql_insert_id(Handle));
	return SUCCESS;
}

METHOD("fetch", TYP, T, TYP, Std$String$T) {
//@connection
//@mode
//@query
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	if (mysql_real_query(Handle, Std$String$flatten(Args[1].Val), ((Std$String_t *)Args[1].Val)->Length.Value)) {
		Result->Val = Std$String$new(mysql_error(Handle));
		return MESSAGE;
	};
	MYSQL_RES *ResultHandle = mysql_store_result(Handle);
	int FieldCount = mysql_field_count(Handle);
	if (ResultHandle == 0) {
		if (FieldCount) {
			Result->Val = Std$String$new(mysql_error(Handle));
			return MESSAGE;
		} else {
			return FAILURE;
		};
	};
	result_t *Res = new(result_t);
	Res->Type = ResultT;
	Res->Handle = ResultHandle;
	Res->FieldCount = FieldCount;
	Std$Object_t *FieldList = Res->FieldList = Agg$List$new(0);
	MYSQL_FIELD *Fields = Res->Fields = mysql_fetch_fields(ResultHandle);
	for (int I = 0; I < FieldCount; ++I) {
		Agg$List$put(FieldList, Std$String$copy_length(Fields[I].name, Fields[I].name_length));
	};
	Result->Val = Res;
	Riva$Memory$register_finalizer((void *)Res, (void *)result_finalize, 0, 0, 0);
	return SUCCESS;
};

SYMBOL($copy, "copy");
SYMBOL($read, "read");

METHOD("query", TYP, T, TYP, Std$Symbol$T, TYP, Std$String$T) {
//@connection
//@mode
//@query
	MYSQL *Handle = ((connection_t *)Args[0].Val)->Handle;
	if (mysql_real_query(Handle, Std$String$flatten(Args[2].Val), ((Std$String_t *)Args[2].Val)->Length.Value)) {
		Result->Val = Std$String$new(mysql_error(Handle));
		return MESSAGE;
	};
	if (Args[1].Val == $copy) {
		MYSQL_RES *ResultHandle = mysql_store_result(Handle);
		int FieldCount = mysql_field_count(Handle);
		if (ResultHandle == 0) {
			if (FieldCount) {
				Result->Val = Std$String$new(mysql_error(Handle));
				return MESSAGE;
			} else {
				return FAILURE;
			};
		};
		result_t *Res = new(result_t);
		Res->Type = ResultT;
		Res->Handle = ResultHandle;
		Res->FieldCount = FieldCount;
		Std$Object_t *FieldList = Res->FieldList = Agg$List$new(0);
		MYSQL_FIELD *Fields = Res->Fields = mysql_fetch_fields(ResultHandle);
		for (int I = 0; I < FieldCount; ++I) {
			Agg$List$put(FieldList, Std$String$copy_length(Fields[I].name, Fields[I].name_length));
		};
		Result->Val = Res;
		Riva$Memory$register_finalizer((void *)Res, (void *)result_finalize, 0, 0, 0);
		return SUCCESS;
	} else if (Args[1].Val == $read) {
		MYSQL_RES *ResultHandle = mysql_use_result(Handle);
		if (ResultHandle == 0) {
			Result->Val = Std$String$new(mysql_error(Handle));
			return MESSAGE;
		};
		result_t *Res = new(result_t);
		Res->Type = ResultT;
		Res->Handle = ResultHandle;
		int FieldCount = Res->FieldCount = mysql_field_count(Handle);
		Std$Object_t *FieldList = Res->FieldList = Agg$List$new(0);
		MYSQL_FIELD *Fields = Res->Fields = mysql_fetch_fields(ResultHandle);
		for (int I = 0; I < FieldCount; ++I) {
			Agg$List$put(FieldList, Std$String$copy_length(Fields[I].name, Fields[I].name_length));
		};
		Result->Val = Res;
		Riva$Memory$register_finalizer((void *)Res, (void *)result_finalize, 0, 0, 0);
		return SUCCESS;
	};
	return SUCCESS;
};

METHOD("rows", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	Result->Val = Std$Integer$new_small(mysql_num_rows(Res->Handle));
	return SUCCESS;
};

METHOD("fields", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	Result->Val = Res->FieldList;
	return SUCCESS;
};

static inline Std$Object_t *convert_field(int Type, void *Data, unsigned long Length) {
	if (Data == 0) return Std$Object$Nil;
	switch (Type) {
	case MYSQL_TYPE_TINY:
	case MYSQL_TYPE_SHORT:
	case MYSQL_TYPE_LONG:
	case MYSQL_TYPE_INT24:
	case MYSQL_TYPE_LONGLONG: return Std$Integer$new_string(Data);
	case MYSQL_TYPE_DECIMAL:
	case MYSQL_TYPE_NEWDECIMAL:
	case MYSQL_TYPE_FLOAT:
	case MYSQL_TYPE_DOUBLE: return Std$Real$new_string(Data);
	case MYSQL_TYPE_BIT:
	case MYSQL_TYPE_TIMESTAMP:
	case MYSQL_TYPE_DATE:
	case MYSQL_TYPE_TIME:
	case MYSQL_TYPE_DATETIME:
	case MYSQL_TYPE_YEAR:
	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_BLOB: return Std$String$copy_length(Data, Length);
	case MYSQL_TYPE_SET:
	case MYSQL_TYPE_ENUM:
	case MYSQL_TYPE_GEOMETRY:
	case MYSQL_TYPE_NULL: return Std$Object$Nil;
	};
};

extern Riva$Module_t Riva$Symbol[];

METHOD("list", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	MYSQL_ROW *Row = mysql_fetch_row(Res->Handle);
	if (Row == 0) return FAILURE;
	unsigned long *Lengths = mysql_fetch_lengths(Res->Handle);
	MYSQL_FIELD *Fields = Res->Fields;
	int FieldCount = Res->FieldCount;
	
	Std$Object_t *List = Agg$List$new(0);
	for (int I = 0; I < FieldCount; ++I) {
		Agg$List$put(List, convert_field(Fields[I].type, Row[I], Lengths[I]));
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("table", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	MYSQL_ROW *Row = mysql_fetch_row(Res->Handle);
	if (Row == 0) return FAILURE;
	unsigned long *Lengths = mysql_fetch_lengths(Res->Handle);
	MYSQL_FIELD *Fields = Res->Fields;
	int FieldCount = Res->FieldCount;
	
	Std$Object_t *Table = Agg$Table$new(0, 0);
	int I = 0;
	for (Agg$List_node *Node = Res->FieldList->Head; Node; Node = Node->Next, ++I) {
		Agg$Table$insert(Table, Node->Value, convert_field(Fields[I].type, Row[I], Lengths[I]));
	};
	Result->Val = Table;
	return SUCCESS;
};

METHOD("fill", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	MYSQL_ROW *Row = mysql_fetch_row(Res->Handle);
	if (Row == 0) return FAILURE;
	unsigned long *Lengths = mysql_fetch_lengths(Res->Handle);
	MYSQL_FIELD *Fields = Res->Fields;

	int FieldCount = Res->FieldCount;
	
	if (FieldCount > Count - 1) FieldCount = Count - 1;
	
	for (int I = 0; I < FieldCount; ++I) {
		if (Args[1 + I].Ref) {
			Args[1 + I].Ref[0] = convert_field(Fields[I].type, Row[I], Lengths[I]);
		};
	};

	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("call", TYP, ResultT, ANY) {
	result_t *Res = Args[0].Val;
	MYSQL_ROW *Row = mysql_fetch_row(Res->Handle);
	if (Row == 0) return FAILURE;
	unsigned long *Lengths = mysql_fetch_lengths(Res->Handle);
	MYSQL_FIELD *Fields = Res->Fields;
	int FieldCount = Res->FieldCount;
	
	int I = 0;
	for (Agg$List_node *Node = Res->FieldList->Head; Node; Node = Node->Next, ++I) {
		switch (Std$Function$call(Args[1].Val, 2, Result, Node->Value, 0, convert_field(Fields[I].type, Row[I], Lengths[I]), 0)) {
		case SUSPEND: case SUCCESS: break;
		case FAILURE: return FAILURE;
		case MESSAGE: return MESSAGE;
		};
	};
	return SUCCESS;
};

METHOD("free", TYP, ResultT) {
	result_t *Res = Args[0].Val;
	mysql_free_result(Res->Handle);
	Res->Handle = 0;
	Riva$Memory$register_finalizer((void *)Res, 0, 0, 0, 0);
	return SUCCESS;
};

INITIAL() {
	mysql_library_init(0, 0, 0);
};
