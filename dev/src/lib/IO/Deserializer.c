#include <Riva/Memory.h>
#include <Riva/Module.h>
#include <Riva/System.h>
#include <stdio.h>
#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Agg/IntegerTable.h>

typedef struct deserializer_t deserializer_t;
typedef struct entry_t entry_t;
typedef Std$Function$status (*deserialize_fn_t)(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data);

struct entry_t {
	deserialize_fn_t deserialize;
	void *Data;
};

struct deserializer_t {
	const Std$Type$t *Type;
	Agg$IntegerTable$t Entries[1];
};

TYPE(T);

static void deserializer_register(deserializer_t *Deserializer, size_t Index, deserialize_fn_t deserialize, void *Data) {
	entry_t *Entry = new(entry_t);
	Entry->deserialize = deserialize;
	Entry->Data = Data;
	Agg$IntegerTable$put(Deserializer->Entries, Index, Entry);
};

static Std$Function$status deserialize_riva(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, Std$Object$t *Function) {
	return Std$Function$call(Function, 2, Result, Stream, 0, Deserializer, 0);
};

METHOD("register", TYP, T, TYP, Std$Integer$SmallT, ANY) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, (deserialize_fn_t)deserialize_riva, Args[2].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status deserializer_read(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result) {
	size_t Index = 0;
	if (IO$Stream$read(Stream, (char *)&Index, 1, 1) != 1) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	//printf("Deserializing type %d\n", Index);
	entry_t *Entry = (entry_t *)Agg$IntegerTable$get(Deserializer->Entries, Index);
	if (!Entry) {
		Result->Val = Std$String$new("Invalid type");
		return MESSAGE;
	};
	Std$Function$status Status = Entry->deserialize(Deserializer, Stream, Result, Entry->Data);
	return Status;
};

GLOBAL_METHOD(Read, 2, "read", TYP, T, TYP, IO$Stream$ReaderT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	return deserializer_read(Deserializer, Args[1].Val, Result);
};

static Std$Function$status deserialize_nil(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	Result->Val = Std$Object$Nil;
	return SUCCESS;
};

METHOD("register_nil", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_nil, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status deserialize_small(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	int32_t Value;
	if (IO$Stream$read(Stream, (char *)&Value, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Value);
	return SUCCESS;
};

METHOD("register_small", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_small, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadSmall, 1) {
	return deserialize_small(0, Args[0].Val, Result, 0);
};

static Std$Function$status deserialize_real(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	double Value;
	if (IO$Stream$read(Stream, (char *)&Value, 8, 1) != 8) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Real$new(Value);
	return SUCCESS;
};

METHOD("register_real", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_real, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadReal, 1) {
	return deserialize_real(0, Args[0].Val, Result, 0);
};

SYMBOL($read, "read");

static Std$Function$status deserialize_string(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	size_t Length;
	if (IO$Stream$read(Stream, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	//printf("String length = %d\n", Length);
	/*char *String = Riva$Memory$alloc(Length);
	if (IO$Stream$read(Stream, String, Length, 1) != Length) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$String$new_length(String, Length);
	return SUCCESS;*/
	return Std$Function$call($read, 2, Result, Stream, 0, Std$Integer$new_small(Length), 0);
};

METHOD("register_string", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_string, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadString, 1) {
	return deserialize_string(0, Args[0].Val, Result, 0);
};

static Std$Function$status deserialize_list(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	int Length;
	if (IO$Stream$read(Stream, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Std$Object$t *List = Agg$List$new0();
	while (--Length >= 0) {
		switch (deserializer_read(Deserializer, Stream, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Agg$List$put(List, Result->Val);
		};
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("register_list", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_list, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadList, 2) {
	deserializer_t *Deserializer = (deserializer_t *)Args[1].Val;
	return deserialize_list(Deserializer, Args[0].Val, Result, 0);
};

static Std$Function$status deserialize_table(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	int Length;
	if (IO$Stream$read(Stream, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Std$Object$t *Table = Agg$Table$new(0, 0);
	while (--Length >= 0) {
		Std$Object$t *Key;
		switch (deserializer_read(Deserializer, Stream, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Key = Result->Val;
		};
		switch (deserializer_read(Deserializer, Stream, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Agg$Table$insert(Table, Key, Result->Val);
		};
	};
	Result->Val = Table;
	return SUCCESS;
};

METHOD("register_table", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_table, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadTable, 2) {
	deserializer_t *Deserializer = (deserializer_t *)Args[1].Val;
	return deserialize_table(Deserializer, Args[0].Val, Result, 0);
};

extern Riva$Module$t Riva$Symbol[];

static Std$Function$status deserialize_symbol(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	size_t Length;
	if (IO$Stream$read(Stream, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	char *Name = Riva$Memory$alloc(Length);
	if (IO$Stream$read(Stream, Name, Length, 1) != Length) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	int Type;
	Riva$Module$import(Riva$Symbol, Name, &Type, (void **)&Result->Val);
	return SUCCESS;
};

METHOD("register_symbol", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_symbol, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadSymbol, 1) {
	return deserialize_symbol(0, Args[0].Val, Result, 0);
};

static Std$Function$status deserialize_big(deserializer_t *Deserializer, IO$Stream$t *Stream, Std$Function$result *Result, void *Data) {
	int Count;
	if (IO$Stream$read(Stream, (char *)&Count, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	mpz_t Value;
	mpz_init(Value);
	if (Count < 0) {
		Count = -Count;
		char *Buffer = Riva$Memory$alloc_atomic(4 * Count);
		if (IO$Stream$read(Stream, Buffer, 4 * Count, 1) != 4 * Count) {
			Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
			return MESSAGE;
		};
		mpz_import(Value, Count, -1, 4, -1, 0, Buffer);
		mpz_neg(Value, Value);
	} else {
		char *Buffer = Riva$Memory$alloc_atomic(4 * Count);
		if (IO$Stream$read(Stream, Buffer, 4 * Count, 1) != 4 * Count) {
			Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
			return MESSAGE;
		};
		mpz_import(Value, Count, -1, 4, -1, 0, Buffer);
	};
	Result->Val = Std$Integer$new_big(Value);
	return SUCCESS;
};

METHOD("register_big", TYP, T, TYP, Std$Integer$SmallT) {
	deserializer_t *Deserializer = (deserializer_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	deserializer_register(Deserializer, Index, deserialize_big, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(ReadBig, 1) {
	return deserialize_big(0, Args[0].Val, Result, 0);
};

Std$Integer$smallt TypeNil[1] = {{Std$Integer$SmallT, 0}};
Std$Integer$smallt TypeInteger[1] = {{Std$Integer$SmallT, -1}};
Std$Integer$smallt TypeReal[1] = {{Std$Integer$SmallT, -2}};
Std$Integer$smallt TypeString[1] = {{Std$Integer$SmallT, -3}};
Std$Integer$smallt TypeList[1] = {{Std$Integer$SmallT, -4}};
Std$Integer$smallt TypeTable[1] = {{Std$Integer$SmallT, -5}};
Std$Integer$smallt TypeSymbol[1] = {{Std$Integer$SmallT, -6}};
Std$Integer$smallt TypeBigInteger[1] = {{Std$Integer$SmallT, -7}};

deserializer_t *deserializer_new() {
	deserializer_t *Deserializer = new(deserializer_t);
	Deserializer->Type = T;
	deserializer_register(Deserializer, 0, deserialize_nil, 0);
	deserializer_register(Deserializer, -1, deserialize_small, 0);
	deserializer_register(Deserializer, -2, deserialize_real, 0);
	deserializer_register(Deserializer, -3, deserialize_string, 0);
	deserializer_register(Deserializer, -4, deserialize_list, 0);
	deserializer_register(Deserializer, -5, deserialize_table, 0);
	deserializer_register(Deserializer, -6, deserialize_symbol, 0);
	deserializer_register(Deserializer, -7, deserialize_big, 0);
	return Deserializer;
};

GLOBAL_FUNCTION(New, 0) {
	Result->Val = (Std$Object$t *)deserializer_new();
	return SUCCESS;
};
