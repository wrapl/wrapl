#include <Riva/Memory.h>
#include <stdio.h>
#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Agg/TypeTable.h>

typedef struct encoder_t encoder_t;
typedef struct entry_t entry_t;
typedef union {void *Data; size_t Index;} data_t;
typedef Std$Function$status (*encode_fn_t)(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Object$t *Value);

struct entry_t {
	encode_fn_t encode;
	size_t Index;
	void *Data;
};

struct encoder_t {
	const Std$Type$t *Type;
	Std$Object$t *Base;
	IO$Stream$writefn write;
	Agg$TypeTable$t Entries[1];
};

TYPE(T);

static void encoder_register(encoder_t *Encoder, const Std$Type$t *Type, size_t Index, encode_fn_t encode, void *Data) {
	entry_t *Entry = new(entry_t);
	Entry->encode = encode;
	Entry->Index = Index;
	Entry->Data = Data;
	Agg$TypeTable$put(Encoder->Entries, Type, Entry);
};

static Std$Function$status encode_riva(encoder_t *Encoder, Std$Function$result *Result, Std$Object$t *Function, Std$Object$t *Value) {
	return Std$Function$call(Function, 3, Result, Value, 0, Encoder->Base, 0, Encoder, 0);
};

METHOD("register", TYP, T, TYP, Std$Type$T, TYP, Std$Integer$SmallT, ANY) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[2].Val);
	encoder_register(Encoder, (Std$Type$t *)Args[1].Val, Index, (encode_fn_t)encode_riva, Args[3].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status encoder_write(encoder_t *Encoder, Std$Function$result *Result, Std$Object$t *Value) {
	entry_t *Entry = (entry_t *)Agg$TypeTable$get(Encoder->Entries, Value->Type);
	if (!Entry) {
		Result->Val = Std$String$new("Invalid type");
		return MESSAGE;
	};
	//printf("Encoding type %d\n", Entry->Index);
	if (Encoder->write(Encoder->Base, (char *)&Entry->Index, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return Entry->encode(Encoder, Result, Entry->Data, Value);
};

GLOBAL_METHOD(Write, 2, "write", TYP, T, ANY) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encoder_write(Encoder, Result, Args[1].Val);
};

void _write(encoder_t *Encoder, Std$Object$t *Value) {
	Std$Function$result Result;
	encoder_write(Encoder, &Result, Value);
}

static Std$Function$status encode_nil(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Object$t *Value) {
	Result->Val = Std$Object$Nil;
	return SUCCESS;
};

METHOD("register_nil", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Std$Object$T, Index, encode_nil, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status encode_small(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Integer$smallt *Value) {
	int Size = (int)Data;
	if (Encoder->write(Encoder->Base, (char *)&Value->Value, Size, 1) != Size) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_small", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	if (Count > 1) {
		int Size = Std$Integer$get_small(Args[1].Val);
		if ((Size < 1) || (Size > 4)) {
			Result->Val = Std$String$new("Invalid number of bytes");
			return MESSAGE;
		}
		encoder_register(Encoder, Std$Integer$SmallT, Index, (encode_fn_t)encode_small, (void *)Size);
	} else {
		encoder_register(Encoder, Std$Integer$SmallT, Index, (encode_fn_t)encode_small, (void *)4);
	}
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteSmall, 2, "write_small", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	if (Count > 2) {
		int Size = Std$Integer$get_small(Args[1].Val);
		if ((Size < 1) || (Size > 4)) {
			Result->Val = Std$String$new("Invalid number of bytes");
			return MESSAGE;
		}
		return encode_small(Encoder, Result, (void *)Size, (Std$Integer$smallt *)Args[1].Val);
	} else {
		return encode_small(Encoder, Result, (void *)4, (Std$Integer$smallt *)Args[1].Val);
	}
};

static Std$Function$status encode_real(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Real$t *Value) {
	if (Encoder->write(Encoder->Base, (char *)&Value->Value, 8, 1) != 8) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_real", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Std$Real$T, Index, (encode_fn_t)encode_real, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteReal, 2, "write_real", TYP, T, TYP, Std$Real$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_real(Encoder, Result, 0, (Std$Real$t *)Args[1].Val);
};

SYMBOL($write, "write");

static Std$Function$status encode_string(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$String$t *Value) {
	//printf("Encoded string size = %d\n", Value->Length.Value);
	if (Encoder->write(Encoder->Base, (char *)&Value->Length.Value, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return Std$Function$call($write, 2, Result, Encoder->Base, 0, Value, 0);
};

METHOD("register_string", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Std$String$T, Index, (encode_fn_t)encode_string, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteString, 2, "write_string", TYP, T, TYP, Std$String$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_string(Encoder, Result, 0, (Std$String$t *)Args[1].Val);
};

static Std$Function$status encode_list(encoder_t *Encoder, Std$Function$result *Result, void *Data, Agg$List$t *Value) {
	if (Encoder->write(Encoder->Base, (char *)&Value->Length, 4, 1) != 4) {
		Result->Val = Std$String$new("Read error");
		return MESSAGE;
	};
	for (Agg$List$node *Node = Value->Head; Node; Node = Node->Next) {
		switch (encoder_write(Encoder, Result, Node->Value)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: break;
		};
	};
	return SUCCESS;
};

METHOD("register_list", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Agg$List$T, Index, (encode_fn_t)encode_list, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteList, 2, "write_list", TYP, T, TYP, Agg$List$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_list(Encoder, Result, 0, (Agg$List$t *)Args[1].Val);
};

static Std$Function$status encode_table(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Object$t *Value) {
	int Length = Agg$Table$size(Value);
	if (Encoder->write(Encoder->Base, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	for (Std$Object$t *Node = Agg$Table$trav_first(Trav, Value); Node; Node = Agg$Table$trav_next(Trav)) {
		switch (encoder_write(Encoder, Result, Agg$Table$node_key(Node))) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: break;
		};
		switch (encoder_write(Encoder, Result, Agg$Table$node_value(Node))) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: break;
		};
	};
	return SUCCESS;
};

METHOD("register_table", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Agg$Table$T, Index, (encode_fn_t)encode_table, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteTable, 2, "write_table", TYP, T, TYP, Agg$Table$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_table(Encoder, Result, 0, (Std$Object$t *)Args[1].Val);
};

static Std$Function$status encode_symbol(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Symbol$t *Value) {
	Std$String$t *String = Value->Name;
	if (Encoder->write(Encoder->Base, (char *)&String->Length.Value, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return Std$Function$call($write, 2, Result, Encoder->Base, 0, String, 0);
};

METHOD("register_symbol", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Std$Symbol$T, Index, (encode_fn_t)encode_symbol, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteSymbol, 2, "write_symbol", TYP, T, TYP, Std$Symbol$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_symbol(Encoder, Result, 0, (Std$Symbol$t *)Args[1].Val);
};

static Std$Function$status encode_big(encoder_t *Encoder, Std$Function$result *Result, void *Data, Std$Integer$bigt *Value) {
	int Count = (mpz_sizeinbase(Value->Value, 2) + 31) / 32;
	char *Buffer = Riva$Memory$alloc_atomic(4 * Count + 4);
	if (mpz_sgn(Value->Value) < 0) {
		*(int *)Buffer = -Count;
	} else {
		*(int *)Buffer = Count;
	};
	mpz_export(Buffer + 4, NULL, -1, 4, -1, 0, Value->Value);
	if (Encoder->write(Encoder->Base, Buffer, 4 * Count + 4, 1) != 4 * Count + 4) {
		Result->Val = (Std$Object$t *)IO$Stream$WriteMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

METHOD("register_big", TYP, T, TYP, Std$Integer$SmallT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	encoder_register(Encoder, Std$Integer$BigT, Index, (encode_fn_t)encode_big, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(WriteBig, 2, "write_big", TYP, T, TYP, Std$Integer$BigT) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	return encode_big(Encoder, Result, 0, (Std$Integer$bigt *)Args[1].Val);
};

METHOD("base", TYP, T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Result->Val = Encoder->Base;
	return SUCCESS;
};

METHOD("base", TYP, T, TYP, IO$Stream$T) {
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Encoder->Base = Args[1].Val;
	Encoder->write = Util$TypedFunction$get(IO$Stream$write, Encoder->Base->Type);
	Result->Val = Encoder->Base;
	return SUCCESS;
};

encoder_t *encoder_new(Std$Object$t *Base) {
	encoder_t *Encoder = new(encoder_t);
	Encoder->Type = T;
	Encoder->Base = Base;
	Encoder->write = Util$TypedFunction$get(IO$Stream$write, Encoder->Base->Type);
	encoder_register(Encoder, Std$Object$T, 0, encode_nil, 0);
	encoder_register(Encoder, Std$Integer$SmallT, 1, (encode_fn_t)encode_small, 0);
	encoder_register(Encoder, Std$Real$T, 2, (encode_fn_t)encode_real, 0);
	encoder_register(Encoder, Std$String$T, 3, (encode_fn_t)encode_string, 0);
	encoder_register(Encoder, Agg$List$T, 4, (encode_fn_t)encode_list, 0);
	encoder_register(Encoder, Agg$Table$T, 5, (encode_fn_t)encode_table, 0);
	encoder_register(Encoder, Std$Symbol$T, 6, (encode_fn_t)encode_symbol, 0);
	encoder_register(Encoder, Std$Integer$BigT, 7, (encode_fn_t)encode_big, 0);
	return Encoder;
};

GLOBAL_FUNCTION(New, 1) {
	Result->Val = (Std$Object$t *)encoder_new(Args[0].Val);
	return SUCCESS;
};
