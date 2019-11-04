#include <Riva/Memory.h>
#include <stdio.h>
#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Agg/TypeTable.h>

typedef struct serializer_t serializer_t;
typedef struct entry_t entry_t;
typedef union {void *Data; size_t Index;} data_t;
typedef Std$Function$status (*serialize_fn_t)(serializer_t *Serializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data, Std$Object$t *Value);

struct entry_t {
	serialize_fn_t serialize;
	size_t Index;
	void *Data;
};

struct serializer_t {
	const Std$Type$t *Type;
	Agg$TypeTable$t Entries[1];
};

TYPE(T);

static void serializer_register(serializer_t *Serializer, const Std$Type$t *Type, size_t Index, serialize_fn_t serialize, void *Data) {
	entry_t *Entry = new(entry_t);
	Entry->serialize = serialize;
	Entry->Index = Index;
	Entry->Data = Data;
	Agg$TypeTable$put(Serializer->Entries, Type, Entry);
};

static Std$Function$status serialize_riva(serializer_t *Serializer, IO$Stream$t *Stream, Std$Object$t *Value, Std$Function$result *Result, Std$Object$t *Function) {
	return Std$Function$call(Function, 3, Result, Stream, 0, Value, 0, Serializer, 0);
};

METHOD("register", TYP, T, TYP, Std$Type$T, TYP, Std$Integer$SmallT, ANY) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[2].Val);
	serializer_register(Serializer, (Std$Type$t *)Args[1].Val, Index, (serialize_fn_t)serialize_riva, Args[3].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status serializer_write(serializer_t *Serializer, IO$Stream$t *Stream, Std$Object$t *Value, Std$Function$result *Result) {
	entry_t *Entry = (entry_t *)Agg$TypeTable$get(Serializer->Entries, Value->Type);
	if (!Entry) {
		Result->Val = Std$String$new("Invalid type");
		return MESSAGE;
	};
	//printf("Serializing type %d\n", Entry->Index);
	if (IO$Stream$write(Stream, (char *)&Entry->Index, 1, 1) != 1) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return Entry->serialize(Serializer, Stream, Value, Result, Entry->Data);
};

GLOBAL_METHOD(Write, 3, "write", TYP, T, TYP, IO$Stream$WriterT, ANY) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	return serializer_write(Serializer, Args[1].Val, Args[2].Val, Result);
};

static Std$Function$status serialize_nil(serializer_t *Serializer, IO$Stream$t *Stream, Std$Object$t *Value, Std$Function$result *Result, void *Data) {
	Result->Val = Std$Object$Nil;
	return SUCCESS;
};

METHOD("register_nil", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$Object$T, Index, serialize_nil, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status serialize_small(serializer_t *Serializer, IO$Stream$t *Stream, Std$Integer$smallt *Value, Std$Function$result *Result, void *Data) {
	if (IO$Stream$write(Stream, (char *)&Value->Value, 4, 1) != 4) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_small", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$Integer$SmallT, Index, (serialize_fn_t)serialize_small, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteSmall, 2) {
	return serialize_small(0, Args[0].Val, (Std$Integer$smallt *)Args[1].Val, Result, 0);
};

static Std$Function$status serialize_real(serializer_t *Serializer, IO$Stream$t *Stream, Std$Real$t *Value, Std$Function$result *Result, void *Data) {
	if (IO$Stream$write(Stream, (char *)&Value->Value, 8, 1) != 8) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_real", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$Real$T, Index, (serialize_fn_t)serialize_real, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteReal, 2) {
	return serialize_real(0, Args[0].Val, (Std$Real$t *)Args[1].Val, Result, 0);
};

SYMBOL($write, "write");

static Std$Function$status serialize_string(serializer_t *Serializer, IO$Stream$t *Stream, Std$String$t *Value, Std$Function$result *Result, void *Data) {
	//printf("Serialized string size = %d\n", Value->Length.Value);
	if (IO$Stream$write(Stream, (char *)&Value->Length.Value, 4, 1) != 4) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return Std$Function$call($write, 2, Result, Stream, 0, Value, 0);
};

METHOD("register_string", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$String$T, Index, (serialize_fn_t)serialize_string, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteString, 2) {
	return serialize_string(0, Args[0].Val, (Std$String$t *)Args[1].Val, Result, 0);
};

static Std$Function$status serialize_list(serializer_t *Serializer, IO$Stream$t *Stream, Agg$List$t *Value, Std$Function$result *Result, void *Data) {
	if (IO$Stream$write(Stream, (char *)&Value->Length, 4, 1) != 4) {
		Result->Val = Std$String$new("Read error");
		return MESSAGE;
	};
	for (Agg$List$node *Node = Value->Head; Node; Node = Node->Next) {
		switch (serializer_write(Serializer, Stream, Node->Value, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: case SUCCESS: case SUSPEND: break;
		};
	};
	return SUCCESS;
};

METHOD("register_list", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Agg$List$T, Index, (serialize_fn_t)serialize_list, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteList, 3) {
	serializer_t *Serializer = (serializer_t *)Args[2].Val;
	return serialize_list(Serializer, Args[0].Val, (Agg$List$t *)Args[1].Val, Result, 0);
};

static Std$Function$status serialize_table(serializer_t *Serializer, IO$Stream$t *Stream, Std$Object$t *Value, Std$Function$result *Result, void *Data) {
	int Length = Agg$Table$size(Value);
	if (IO$Stream$write(Stream, (char *)&Length, 4, 1) != 4) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Value); Node; Node = Agg$Table$trav_next(Trav)) {
		switch (serializer_write(Serializer, Stream, Agg$Table$node_key(Node), Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: case SUCCESS: case SUSPEND: break;
		};
		switch (serializer_write(Serializer, Stream, Agg$Table$node_value(Node), Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: case SUCCESS: case SUSPEND: break;
		};
	};
	return SUCCESS;
};

METHOD("register_table", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Agg$Table$T, Index, (serialize_fn_t)serialize_table, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteTable, 3) {
	serializer_t *Serializer = (serializer_t *)Args[2].Val;
	return serialize_table(Serializer, Args[0].Val, (Std$Object$t *)Args[1].Val, Result, 0);
};

static Std$Function$status serialize_symbol(serializer_t *Serializer, IO$Stream$t *Stream, Std$Symbol$t *Value, Std$Function$result *Result, void *Data) {
	Std$String$t *String = Value->Name;
	if (IO$Stream$write(Stream, (char *)&String->Length.Value, 4, 1) != 4) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return Std$Function$call($write, 2, Result, Stream, 0, String, 0);
};

METHOD("register_symbol", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$Symbol$T, Index, (serialize_fn_t)serialize_symbol, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteSymbol, 2) {
	return serialize_symbol(0, Args[0].Val, (Std$Symbol$t *)Args[1].Val, Result, 0);
};

static Std$Function$status serialize_big(serializer_t *Serializer, IO$Stream$t *Stream, Std$Integer$bigt *Value, Std$Function$result *Result, void *Data) {
	int Count = (mpz_sizeinbase(Value->Value, 2) + 31) / 32;
	char *Buffer = Riva$Memory$alloc_atomic(4 * Count + 4);
	if (mpz_sgn(Value->Value) < 0) {
		*(int *)Buffer = -Count;
	} else {
		*(int *)Buffer = Count;
	};
	mpz_export(Buffer + 4, NULL, -1, 4, -1, 0, Value->Value);
	if (IO$Stream$write(Stream, Buffer, 4 * Count + 4, 1) != 4 * Count + 4) {
		Result->Val = IO$Stream$Message$new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_big", TYP, T, TYP, Std$Integer$SmallT) {
	serializer_t *Serializer = (serializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	serializer_register(Serializer, Std$Integer$BigT, Index, (serialize_fn_t)serialize_big, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(WriteBig, 2) {
	return serialize_big(0, Args[0].Val, (Std$Integer$bigt *)Args[1].Val, Result, 0);
};

Std$Integer$smallt TypeNil[1] = {{Std$Integer$SmallT, 0}};
Std$Integer$smallt TypeInteger[1] = {{Std$Integer$SmallT, -1}};
Std$Integer$smallt TypeReal[1] = {{Std$Integer$SmallT, -2}};
Std$Integer$smallt TypeString[1] = {{Std$Integer$SmallT, -3}};
Std$Integer$smallt TypeList[1] = {{Std$Integer$SmallT, -4}};
Std$Integer$smallt TypeTable[1] = {{Std$Integer$SmallT, -5}};
Std$Integer$smallt TypeSymbol[1] = {{Std$Integer$SmallT, -6}};
Std$Integer$smallt TypeBigInteger[1] = {{Std$Integer$SmallT, -7}};

serializer_t *serializer_new() {
	serializer_t *Serializer = new(serializer_t);
	Serializer->Type = T;
	serializer_register(Serializer, Std$Object$T, 0, serialize_nil, 0);
	serializer_register(Serializer, Std$Integer$SmallT, -1, (serialize_fn_t)serialize_small, 0);
	serializer_register(Serializer, Std$Real$T, -2, (serialize_fn_t)serialize_real, 0);
	serializer_register(Serializer, Std$String$T, -3, (serialize_fn_t)serialize_string, 0);
	serializer_register(Serializer, Agg$List$T, -4, (serialize_fn_t)serialize_list, 0);
	serializer_register(Serializer, Agg$Table$T, -5, (serialize_fn_t)serialize_table, 0);
	serializer_register(Serializer, Std$Symbol$T, -6, (serialize_fn_t)serialize_symbol, 0);
	serializer_register(Serializer, Std$Integer$BigT, -7, (serialize_fn_t)serialize_big, 0);
	return Serializer;
};

GLOBAL_FUNCTION(New, 0) {
	Result->Val = (Std$Object$t *)serializer_new();
	return SUCCESS;
};
