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

typedef struct decoder_t decoder_t;
typedef struct entry_t entry_t;
typedef Std$Function$status (*decode_fn_t)(decoder_t *Decoder, Std$Function$result *Result, void *Data);

struct entry_t {
	decode_fn_t decode;
	void *Data;
};

struct decoder_t {
	const Std$Type$t *Type;
	Std$Object$t *Base;
	IO$Stream$readfn read;
	Agg$IntegerTable$t Entries[1];
};

TYPE(T);

static void decoder_register(decoder_t *Decoder, size_t Index, decode_fn_t decode, void *Data) {
	entry_t *Entry = new(entry_t);
	Entry->decode = decode;
	Entry->Data = Data;
	Agg$IntegerTable$put(Decoder->Entries, Index, Entry);
};

static Std$Function$status decode_riva(decoder_t *Decoder, Std$Function$result *Result, Std$Object$t *Function) {
	return Std$Function$call(Function, 2, Result, Decoder->Base, 0, Decoder, 0);
};

METHOD("register", TYP, T, TYP, Std$Integer$SmallT, ANY) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, (decode_fn_t)decode_riva, Args[2].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status decoder_read(decoder_t *Decoder, Std$Function$result *Result) {
	size_t Index;
	if (Decoder->read(Decoder->Base, (char *)&Index, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	//printf("Decoding type %d\n", Index);
	entry_t *Entry = (entry_t *)Agg$IntegerTable$get(Decoder->Entries, Index);
	if (!Entry) {
		Result->Val = Std$String$new("Invalid type");
		return MESSAGE;
	};
	Std$Function$status Status = Entry->decode(Decoder, Result, Entry->Data);
	return Status;
};

GLOBAL_METHOD(Read, 1, "read", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decoder_read(Decoder, Result);
};

Std$Object$t *_read(decoder_t *Decoder) {
	Std$Function$result Result;
	if (decoder_read(Decoder, &Result) < FAILURE) {
		return Result.Val;
	} else {
		return 0;
	}
}

static Std$Function$status decode_nil(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	Result->Val = Std$Object$Nil;
	return SUCCESS;
};

METHOD("register_nil", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_nil, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Function$status decode_small(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	int32_t Value = 0;
	int Size = (int)Data;
	if (Decoder->read(Decoder->Base, (char *)&Value, Size, 1) != Size) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Value);
	return SUCCESS;
};

METHOD("register_small", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	if (Count > 1) {
		int Size = Std$Integer$get_small(Args[1].Val);
		if ((Size < 1) || (Size > 4)) {
			Result->Val = Std$String$new("Invalid number of bytes");
			return MESSAGE;
		}
		decoder_register(Decoder, Index, decode_small, (void *)Size);
	} else {
		decoder_register(Decoder, Index, decode_small, (void *)4);
	}
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadSmall, 1, "read_small", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	if (Count > 1) {
		int Size = Std$Integer$get_small(Args[1].Val);
		if ((Size < 1) || (Size > 4)) {
			Result->Val = Std$String$new("Invalid number of bytes");
			return MESSAGE;
		}
		return decode_small(Decoder, Result, (void *)Size);
	} else {
		return decode_small(Decoder, Result, (void *)4);
	}
};

static Std$Function$status decode_real(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	double Value;
	if (Decoder->read(Decoder->Base, (char *)&Value, 8, 1) != 8) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$Real$new(Value);
	return SUCCESS;
};

METHOD("register_real", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_real, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadReal, 1, "read_real", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_real(Decoder, Result, 0);
};

SYMBOL($read, "read");

static Std$Function$status decode_string(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	size_t Length;
	if (Decoder->read(Decoder->Base, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	//printf("String length = %d\n", Length);
	/*char *String = Riva$Memory$alloc(Length);
	if (Decoder->read(Decoder->Base, String, Length, 1) != Length) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Result->Val = Std$String$new_length(String, Length);
	return SUCCESS;*/
	return Std$Function$call($read, 2, Result, Decoder->Base, 0, Std$Integer$new_small(Length), 0);
};

METHOD("register_string", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_string, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadString, 1, "read_string", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_string(Decoder, Result, 0);
};

static Std$Function$status decode_list(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	int Length;
	if (Decoder->read(Decoder->Base, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Std$Object$t *List = Agg$List$new0();
	while (--Length >= 0) {
		switch (decoder_read(Decoder, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Agg$List$put(List, Result->Val);
		};
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("register_list", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_list, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadList, 1, "read_list", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_list(Decoder, Result, 0);
};

static Std$Function$status decode_table(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	int Length;
	if (Decoder->read(Decoder->Base, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	Std$Object$t *Table = Agg$Table$new(0, 0);
	while (--Length >= 0) {
		Std$Object$t *Key;
		switch (decoder_read(Decoder, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Key = Result->Val;
		};
		switch (decoder_read(Decoder, Result)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND: Agg$Table$insert(Table, Key, Result->Val);
		};
	};
	Result->Val = Table;
	return SUCCESS;
};

METHOD("register_table", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_table, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadTable, 1, "read_table", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_table(Decoder, Result, 0);
};

extern Riva$Module$t Riva$Symbol[];

static Std$Function$status decode_symbol(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	size_t Length;
	if (Decoder->read(Decoder->Base, (char *)&Length, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	char *Name = Riva$Memory$alloc(Length);
	if (Decoder->read(Decoder->Base, Name, Length, 1) != Length) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	int Type;
	Riva$Module$import(Riva$Symbol, Name, &Type, (void **)&Result->Val);
	return SUCCESS;
};

METHOD("register_symbol", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_symbol, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadSymbol, 1, "read_symbol", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_symbol(Decoder, Result, 0);
};

static Std$Function$status decode_big(decoder_t *Decoder, Std$Function$result *Result, void *Data) {
	int Count;
	if (Decoder->read(Decoder->Base, (char *)&Count, 4, 1) != 4) {
		Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
		return MESSAGE;
	};
	mpz_t Value;
	mpz_init(Value);
	if (Count < 0) {
		Count = -Count;
		char *Buffer = Riva$Memory$alloc_atomic(4 * Count);
		if (Decoder->read(Decoder->Base, Buffer, 4 * Count, 1) != 4 * Count) {
			Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
			return MESSAGE;
		};
		mpz_import(Value, Count, -1, 4, -1, 0, Buffer);
		mpz_neg(Value, Value);
	} else {
		char *Buffer = Riva$Memory$alloc_atomic(4 * Count);
		if (Decoder->read(Decoder->Base, Buffer, 4 * Count, 1) != 4 * Count) {
			Result->Val = (Std$Object$t *)IO$Stream$ReadMessage;
			return MESSAGE;
		};
		mpz_import(Value, Count, -1, 4, -1, 0, Buffer);
	};
	Result->Val = Std$Integer$new_big(Value);
	return SUCCESS;
};

METHOD("register_big", TYP, T, TYP, Std$Integer$SmallT) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	size_t Index = Std$Integer$get_small(Args[1].Val);
	decoder_register(Decoder, Index, decode_big, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(ReadBig, 1, "read_big", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	return decode_big(Decoder, Result, 0);
};

METHOD("base", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->Base);
	return SUCCESS;
};

METHOD("base", TYP, T, TYP, IO$Stream$T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	Decoder->Base = Args[1].Val;
	Decoder->read = Util$TypedFunction$get(IO$Stream$read, Decoder->Base->Type);
	Result->Val = Decoder->Base;
	return SUCCESS;
};

decoder_t *decoder_new(Std$Object$t *Base) {
	decoder_t *Decoder = new(decoder_t);
	Decoder->Type = T;
	Decoder->Base = Base;
	Decoder->read = Util$TypedFunction$get(IO$Stream$read, Decoder->Base->Type);
	decoder_register(Decoder, 0, decode_nil, 0);
	decoder_register(Decoder, 1, decode_small, 0);
	decoder_register(Decoder, 2, decode_real, 0);
	decoder_register(Decoder, 3, decode_string, 0);
	decoder_register(Decoder, 4, decode_list, 0);
	decoder_register(Decoder, 5, decode_table, 0);
	decoder_register(Decoder, 6, decode_symbol, 0);
	decoder_register(Decoder, 7, decode_big, 0);
	return Decoder;
};

GLOBAL_FUNCTION(New, 1) {
	Result->Val = (Std$Object$t *)decoder_new(Args[0].Val);
	return SUCCESS;
};
