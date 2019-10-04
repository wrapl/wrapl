#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <cbor.h>

typedef struct block_t {
	struct block_t *Prev;
	void *Data;
	size_t Length;
} block_t;

typedef struct collection_t {
	struct collection_t *Prev;
	struct tag_t *Tags;
	Std$Object$t *Key;
	block_t *Blocks;
	size_t Remaining;
} collection_t;

typedef struct tag_t {
	struct tag_t *Prev;
	Std$Function$t *Handler;
} tag_t;

typedef struct parser_t {
	const Std$Type$t *Type;
	Std$Object$t *UserData;
	collection_t *Collection;
	tag_t *Tags;
	Std$Function$t *FinalHandler;
	Std$Function$t *MapStartHandler;
	Std$Function$t *MapPairHandler;
	Std$Function$t *MapEndHandler;
	Std$Function$t *ArrayStartHandler;
	Std$Function$t *ArrayValueHandler;
	Std$Function$t *ArrayEndHandler;
	Std$Function$t *TagHandler;
} parser_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

static Std$Object$t IsByteString[1];
static Std$Object$t IsString[1];
static Std$Object$t IsList[1];

static void value_handler(parser_t *Parser, Std$Object$t *Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	for (tag_t *Tag = Parser->Tags; Tag; Tag = Tag->Prev) {
		//printf("%s:%d\n", __func__, __LINE__);
		Std$Function$result Result;
		int Status = Std$Function$call(Tag->Handler, 2, &Result, Parser->UserData, &Parser->UserData, Value, 0);
		if (Status < FAILURE) Value = Result.Val;
	}
	Parser->Tags = 0;
	collection_t *Collection = Parser->Collection;
	if (!Collection) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Parser->FinalHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->FinalHandler, 2, &Result, Parser->UserData, &Parser->UserData, Value, 0);
		}
	} else if (Collection->Key == IsList) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Parser->ArrayValueHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->ArrayValueHandler, 2, &Result, Parser->UserData, &Parser->UserData, Value, 0);
		}
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Parser->Collection = Collection->Prev;
			Parser->Tags = Collection->Tags;
			if (Parser->ArrayEndHandler != Std$Object$Nil) {
				Std$Function$result Result;
				int Status = Std$Function$call(Parser->ArrayEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
				if (Status < FAILURE) value_handler(Parser, Result.Val);
			}
		}
	} else if (Collection->Key) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Parser->MapPairHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->MapPairHandler, 3, &Result, Parser->UserData, &Parser->UserData, Parser->Collection->Key, 0, Value, 0);
		}
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Parser->Collection = Collection->Prev;
			Parser->Tags = Collection->Tags;
			if (Parser->MapEndHandler != Std$Object$Nil) {
				Std$Function$result Result;
				int Status = Std$Function$call(Parser->MapEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
				if (Status < FAILURE) value_handler(Parser, Result.Val);
			}
		} else {
			Collection->Key = 0;
		}
	} else {
		//printf("%s:%d\n", __func__, __LINE__);
		Collection->Key = Value;
	}
}

static void riva_uint8_cb(parser_t *Parser, uint8_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Std$Integer$new_small(Value));
}

static void riva_uint16_cb(parser_t *Parser, uint16_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Std$Integer$new_small(Value));
}

static void riva_uint32_cb(parser_t *Parser, uint32_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFF) {
		value_handler(Parser, Std$Integer$new_small(Value));
	} else {
		mpz_t Temp;
		mpz_init_set_ui(Temp, Value);
		value_handler(Parser, Std$Integer$new_big(Value));
	}
}

static void riva_uint64_cb(parser_t *Parser, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Parser, Std$Integer$new_small((uint32_t)Value));
	} else {
		value_handler(Parser, Std$Integer$new_u64(Value));
	}
}

static void riva_int8_cb(parser_t *Parser, uint8_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Parser, Std$Integer$new_small(~Value0));
}

static void riva_int16_cb(parser_t *Parser, uint16_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Parser, Std$Integer$new_small(~Value0));
}

static void riva_int32_cb(parser_t *Parser, uint32_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Parser, Std$Integer$new_small(~Value));
}

static void riva_int64_cb(parser_t *Parser, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Parser, Std$Integer$new_small(~(uint32_t)Value));
	} else {
		mpz_t Temp;
		mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
		mpz_mul_2exp(Temp, Temp, 32);
		mpz_add_ui(Temp, Temp, (uint32_t)Value);
		mpz_com(Temp, Temp);
		value_handler(Parser, Std$Integer$new_big(Temp));
	}
}

static void riva_byte_string_start_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Parser->Collection;
	Collection->Tags = Parser->Tags;
	Parser->Tags = 0;
	Collection->Key = IsByteString;
	Collection->Remaining = 0;
	Collection->Blocks = 0;
	Parser->Collection = Collection;
}

static void riva_byte_string_cb(parser_t *Parser, cbor_data Data, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Parser->Collection && Parser->Collection->Key == IsByteString) {
		block_t *Block = new(block_t);
		Block->Prev = Parser->Collection->Blocks;
		Block->Data = Data;
		Block->Length = Length;
		Parser->Collection->Blocks = Block;
		Parser->Collection->Remaining += Length;
	} else {
		value_handler(Parser, Std$Address$new_sized(Data, Length));
	}
}

static void riva_string_start_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Parser->Collection;
	Collection->Tags = Parser->Tags;
	Parser->Tags = 0;
	Collection->Key = IsString;
	Collection->Remaining = 0;
	Collection->Blocks = 0;
	Parser->Collection = Collection;
}

static void riva_string_cb(parser_t *Parser, cbor_data Data, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Parser->Collection && Parser->Collection->Key == IsString) {
		block_t *Block = new(block_t);
		Block->Prev = Parser->Collection->Blocks;
		Block->Data = Data;
		Block->Length = Length;
		Parser->Collection->Blocks = Block;
		++Parser->Collection->Remaining;
	} else {
		value_handler(Parser, Std$String$new_length(Data, Length));
	}
}

static void riva_indef_array_start_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Parser->Collection;
	Collection->Tags = Parser->Tags;
	Parser->Tags = 0;
	Collection->Remaining = 0;
	Collection->Key = IsList;
	Parser->Collection = Collection;
	if (Parser->ArrayStartHandler != Std$Object$Nil) {
		Std$Function$result Result;
		int Status = Std$Function$call(Parser->ArrayStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
	}
}

static void riva_array_start_cb(parser_t *Parser, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Length > 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Parser->Collection;
		Collection->Tags = Parser->Tags;
		Parser->Tags = 0;
		Collection->Remaining = Length;
		Collection->Key = IsList;
		Parser->Collection = Collection;
		if (Parser->ArrayStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->ArrayStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		}
	} else {
		if (Parser->ArrayStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->ArrayStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		}
		if (Parser->ArrayEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->ArrayEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
			if (Status < FAILURE) value_handler(Parser, Result.Val);
		}
	}
}

static void riva_indef_map_start_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Parser->Collection;
	Collection->Tags = Parser->Tags;
	Parser->Tags = 0;
	Collection->Remaining = 0;
	Collection->Key = 0;
	Parser->Collection = Collection;
	if (Parser->MapStartHandler != Std$Object$Nil) {
		Std$Function$result Result;
		int Status = Std$Function$call(Parser->MapStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
	}
}

static void riva_map_start_cb(parser_t *Parser, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Length > 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Parser->Collection;
		Collection->Tags = Parser->Tags;
		Parser->Tags = 0;
		Collection->Remaining = Length;
		Collection->Key = 0;
		Parser->Collection = Collection;
		if (Parser->MapStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->MapStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		}
	} else {
		if (Parser->MapStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->MapStartHandler, 1, &Result, Parser->UserData, &Parser->UserData);
		}
		if (Parser->MapEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->MapEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
			if (Status < FAILURE) value_handler(Parser, Result.Val);
		}
	}
}

static void riva_tag_cb(parser_t *Parser, uint64_t Tag) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Parser->TagHandler != Std$Object$Nil) {
		Std$Function$result Result;
		int Status = Std$Function$call(Parser->TagHandler, 2, &Result, Parser->UserData, &Parser->UserData, Std$Integer$new_u64(Tag), 0);
		if (Status < FAILURE) {
			tag_t *Tag = new(tag_t);
			Tag->Prev = Parser->Tags;
			Tag->Handler = Result.Val;
			Parser->Tags = Tag;
		}
	}
}

static void riva_float_cb(parser_t *Parser, float Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Std$Real$new(Value));
}

static void riva_double_cb(parser_t *Parser, double Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Std$Real$new(Value));
}

static void riva_null_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Std$Object$Nil);
}

static void riva_boolean_cb(parser_t *Parser, bool Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Parser, Value ? $true : $false);
}

static void riva_indef_break_cb(parser_t *Parser) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = Parser->Collection;
	Parser->Collection = Collection->Prev;
	Parser->Tags = Collection->Tags;
	if (Collection->Key == IsByteString) {
		char *Buffer = Riva$Memory$alloc_atomic(Collection->Remaining);
		Buffer += Collection->Remaining;
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Length;
			memcpy(Buffer, B->Data, B->Length);
		}
		value_handler(Parser, Std$Address$new_sized(Buffer, Collection->Remaining));
	} else if (Collection->Key == IsString) {
		Std$String$t *String = Std$String$alloc(Collection->Remaining);
		Std$String$block *Block = String->Blocks + Collection->Remaining - 1;
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Block->Chars.Value = B->Data;
			Block->Length.Value = B->Length;
			--Block;
		}
		value_handler(Parser, Std$String$freeze(String));
	} else if (Collection->Key == IsList) {
		if (Parser->ArrayEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->ArrayEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
			if (Status < FAILURE) value_handler(Parser, Result.Val);
		}
	} else {
		if (Parser->MapEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Parser->MapEndHandler, 1, &Result, Parser->UserData, &Parser->UserData);
			if (Status < FAILURE) value_handler(Parser, Result.Val);
		}
	}
}

static struct cbor_callbacks Callbacks = {
	.uint8 = riva_uint8_cb,
	.uint16 = riva_uint16_cb,
	.uint32 = riva_uint32_cb,
	.uint64 = riva_uint64_cb,
	.negint64 = riva_int64_cb,
	.negint32 = riva_int32_cb,
	.negint16 = riva_int16_cb,
	.negint8 = riva_int8_cb,
	.byte_string_start = riva_byte_string_start_cb,
	.byte_string = riva_byte_string_cb,
	.string = riva_string_cb,
	.string_start = riva_string_start_cb,
	.indef_array_start = riva_indef_array_start_cb,
	.array_start = riva_array_start_cb,
	.indef_map_start = riva_indef_map_start_cb,
	.map_start = riva_map_start_cb,
	.tag = riva_tag_cb,
	.float2 = riva_float_cb,
	.float4 = riva_float_cb,
	.float8 = riva_double_cb,
	.undefined = riva_null_cb,
	.null = riva_null_cb,
	.boolean = riva_boolean_cb,
	.indef_break = riva_indef_break_cb
};

GLOBAL_FUNCTION(New, 0) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	Parser->UserData = Std$Object$Nil;
	Parser->Collection = 0;
	Parser->Tags = 0;
	Parser->FinalHandler = Std$Object$Nil;
	Parser->MapStartHandler = Std$Object$Nil;
	Parser->MapPairHandler = Std$Object$Nil;
	Parser->MapEndHandler = Std$Object$Nil;
	Parser->ArrayStartHandler = Std$Object$Nil;
	Parser->ArrayValueHandler = Std$Object$Nil;
	Parser->ArrayEndHandler = Std$Object$Nil;
	Parser->TagHandler = Std$Object$Nil;
	Result->Val = Parser;
	return SUCCESS;
}

METHOD("parse", TYP, T, TYP, Std$String$T) {
	//printf("%s:%d\n", __func__, __LINE__);
	parser_t *Parser = (parser_t *)Args[0].Val;
	Std$String$t *String = Args[1].Val;
	for (Std$String$block *Block = String->Blocks; Block->Length.Value; ++Block) {
		cbor_data Data = Block->Chars.Value;
		size_t Length = Block->Length.Value;
		while (Length) {
			struct cbor_decoder_result DecoderResult = cbor_stream_decode(
				Data, Length, &Callbacks, Parser
			);
			if (DecoderResult.status != CBOR_DECODER_FINISHED) {
				SEND(Std$String$new("Cbor parse error"));
			}
			Length -= DecoderResult.read;
			Data += DecoderResult.read;
		}
	}
	RETURN0;
}

METHOD("parse", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	//printf("%s:%d\n", __func__, __LINE__);
	parser_t *Parser = (parser_t *)Args[0].Val;
	cbor_data Data = Std$Address$get_value(Args[1].Val);
	size_t Length = Std$Integer$get_small(Args[2].Val);
	while (Length) {
		struct cbor_decoder_result DecoderResult = cbor_stream_decode(
			Data, Length, &Callbacks, Parser
		);
		if (DecoderResult.status != CBOR_DECODER_FINISHED) {
			SEND(Std$String$new("Cbor parse error"));
		}
		Length -= DecoderResult.read;
		Data += DecoderResult.read;
	}
	RETURN0;
}

TYPED_INSTANCE(int, IO$Stream$write, T, parser_t *Stream, const char *Buffer, int Count, int Blocks) {
	struct cbor_decoder_result DecoderResult = cbor_stream_decode(
		Buffer, Count, &Callbacks, Stream
	);
	if (DecoderResult.status != CBOR_DECODER_FINISHED) {
		return -1;
	} else {
		return DecoderResult.read;
	}
}

METHOD("userdata", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->UserData);
	return SUCCESS;
}

METHOD("onfinal", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->FinalHandler);
	return SUCCESS;
}

METHOD("onmapstart", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapStartHandler);
	return SUCCESS;
}

METHOD("onmappair", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapPairHandler);
	return SUCCESS;
}

METHOD("onmapend", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->MapEndHandler);
	return SUCCESS;
}

METHOD("onarraystart", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ArrayStartHandler);
	return SUCCESS;
}

METHOD("onarrayvalue", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ArrayValueHandler);
	return SUCCESS;
}

METHOD("onarrayend", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->ArrayEndHandler);
	return SUCCESS;
}

METHOD("ontag", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->TagHandler);
	return SUCCESS;
}

static void nop_free(void *Ptr) {}

INIT() {
	cbor_set_allocs(Riva$Memory$alloc, Riva$Memory$realloc, nop_free);
}
