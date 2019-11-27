#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include "minicbor.h"

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

typedef struct decoder_t {
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
	minicbor_reader_t Reader[1];
} decoder_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

static Std$Object$t IsByteString[1];
static Std$Object$t IsString[1];
static Std$Object$t IsList[1];

static void value_handler(decoder_t *Decoder, Std$Object$t *Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	for (tag_t *Tag = Decoder->Tags; Tag; Tag = Tag->Prev) {
		//printf("%s:%d\n", __func__, __LINE__);
		Std$Function$result Result;
		int Status = Std$Function$call(Tag->Handler, 2, &Result, Decoder->UserData, &Decoder->UserData, Value, 0);
		if (Status < FAILURE) Value = Result.Val;
	}
	Decoder->Tags = 0;
	collection_t *Collection = Decoder->Collection;
	if (!Collection) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Decoder->FinalHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->FinalHandler, 2, &Result, Decoder->UserData, &Decoder->UserData, Value, 0);
		}
	} else if (Collection->Key == IsList) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Decoder->ArrayValueHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayValueHandler, 2, &Result, Decoder->UserData, &Decoder->UserData, Value, 0);
		}
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			if (Decoder->ArrayEndHandler != Std$Object$Nil) {
				Std$Function$result Result;
				int Status = Std$Function$call(Decoder->ArrayEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
				if (Status < FAILURE) value_handler(Decoder, Result.Val);
			}
		}
	} else if (Collection->Key) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Decoder->MapPairHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapPairHandler, 3, &Result, Decoder->UserData, &Decoder->UserData, Decoder->Collection->Key, 0, Value, 0);
		}
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			if (Decoder->MapEndHandler != Std$Object$Nil) {
				Std$Function$result Result;
				int Status = Std$Function$call(Decoder->MapEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
				if (Status < FAILURE) value_handler(Decoder, Result.Val);
			}
		} else {
			Collection->Key = 0;
		}
	} else {
		//printf("%s:%d\n", __func__, __LINE__);
		Collection->Key = Value;
	}
}

void riva_cbor_read_positive_fn(decoder_t *Decoder, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Decoder, Std$Integer$new_small((uint32_t)Value));
	} else {
		value_handler(Decoder, Std$Integer$new_u64(Value));
	}
}

void riva_cbor_read_negative_fn(decoder_t *Decoder, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Decoder, Std$Integer$new_small(~(uint32_t)Value));
	} else {
		mpz_t Temp;
		mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
		mpz_mul_2exp(Temp, Temp, 32);
		mpz_add_ui(Temp, Temp, (uint32_t)Value);
		mpz_com(Temp, Temp);
		value_handler(Decoder, Std$Integer$new_big(Temp));
	}
}

void riva_cbor_read_bytes_fn(decoder_t *Decoder, int Size) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Size) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Key = IsByteString;
		Collection->Remaining = 0;
		Collection->Blocks = 0;
		Decoder->Collection = Collection;
	} else {
		value_handler(Decoder, Std$Address$new(NULL, 0));
	}
}

void riva_cbor_read_bytes_piece_fn(decoder_t *Decoder, const void *Bytes, int Size, int Final) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = Decoder->Collection;
	if (Final) {
		Decoder->Collection = Collection->Prev;
		Decoder->Tags = Collection->Tags;
		char *Buffer = Riva$Memory$alloc_atomic(Collection->Remaining + Size);
		Buffer += Collection->Remaining;
		memcpy(Buffer, Bytes, Size);
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Length;
			memcpy(Buffer, B->Data, B->Length);
		}
		value_handler(Decoder, Std$Address$new(Buffer, Collection->Remaining + Size));
	} else {
		block_t *Block = new(block_t);
		Block->Prev = Collection->Blocks;
		Block->Data = Bytes;
		Block->Length = Size;
		Collection->Blocks = Block;
		Collection->Remaining += Size;
	}
}

void riva_cbor_read_string_fn(decoder_t *Decoder, int Size) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Size) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Key = IsString;
		Collection->Remaining = 0;
		Collection->Blocks = 0;
		Decoder->Collection = Collection;
	} else {
		value_handler(Decoder, Std$String$Empty);
	}
}

void riva_cbor_read_string_piece_fn(decoder_t *Decoder, const void *Bytes, int Size, int Final) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = Decoder->Collection;
	if (Final) {
		if (Size) {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			Std$String$t *String = Std$String$alloc(Collection->Remaining + 1);
			Std$Address$t *Block = String->Blocks + Collection->Remaining;
			char *Chars = Block->Value = Riva$Memory$alloc_atomic(Size + 1);
			memcpy(Chars, Bytes, Size);
			Chars[Size] = 0;
			int Total = Block->Length.Value = Size;
			for (block_t *B = Collection->Blocks; B; B = B->Prev) {
				--Block;
				Block->Value = B->Data;
				Block->Length.Value = B->Length;
				Total += B->Length;
			}
			String->Length.Value = Total;
			value_handler(Decoder, Std$String$freeze(String));
		} else {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			Std$String$t *String = Std$String$alloc(Collection->Remaining);
			Std$Address$t *Block = String->Blocks + Collection->Remaining;
			int Total = 0;
			for (block_t *B = Collection->Blocks; B; B = B->Prev) {
				--Block;
				Block->Value = B->Data;
				Block->Length.Value = B->Length;
				Total += B->Length;
			}
			String->Length.Value = Total;
			value_handler(Decoder, Std$String$freeze(String));
		}
	} else if (Size) {
		block_t *Block = new(block_t);
		Block->Prev = Collection->Blocks;
		char *Chars = Block->Data = Riva$Memory$alloc_atomic(Size + 1);
		memcpy(Chars, Bytes, Size);
		Chars[Size] = 0;
		Block->Length = Size;
		Collection->Blocks = Block;
		++Collection->Remaining;
	}
}

void riva_cbor_read_array_fn(decoder_t *Decoder, int Size) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Size < 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = 0;
		Collection->Key = IsList;
		Decoder->Collection = Collection;
		if (Decoder->ArrayStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
	} else if (Size == 0) {
		if (Decoder->ArrayStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
		if (Decoder->ArrayEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
			if (Status < FAILURE) value_handler(Decoder, Result.Val);
		}
	} else {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = Size;
		Collection->Key = IsList;
		Decoder->Collection = Collection;
		if (Decoder->ArrayStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
	}
}

void riva_cbor_read_map_fn(decoder_t *Decoder, int Size) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Size < 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = 0;
		Collection->Key = 0;
		Decoder->Collection = Collection;
		if (Decoder->MapStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
	} else if (Size == 0) {
		if (Decoder->MapStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
		if (Decoder->MapEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
			if (Status < FAILURE) value_handler(Decoder, Result.Val);
		}
	} else {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = Size;
		Collection->Key = 0;
		Decoder->Collection = Collection;
		if (Decoder->MapStartHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapStartHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
		}
	}
}

void riva_cbor_read_tag_fn(decoder_t *Decoder, uint64_t Tag) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Decoder->TagHandler != Std$Object$Nil) {
		Std$Function$result Result;
		int Status = Std$Function$call(Decoder->TagHandler, 2, &Result, Decoder->UserData, &Decoder->UserData, Std$Integer$new_u64(Tag), 0);
		if (Status < FAILURE) {
			tag_t *Tag = new(tag_t);
			Tag->Prev = Decoder->Tags;
			Tag->Handler = Result.Val;
			Decoder->Tags = Tag;
		}
	}
}

void riva_cbor_read_float_fn(decoder_t *Decoder, double Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, Std$Real$new(Value));
}

void riva_cbor_read_simple_fn(decoder_t *Decoder, int Special) {
	//printf("%s:%d\n", __func__, __LINE__);
	switch (Special) {
	case CBOR_SIMPLE_FALSE:
		value_handler(Decoder, $false);
		break;
	case CBOR_SIMPLE_TRUE:
		value_handler(Decoder, $true);
		break;
	default:
		value_handler(Decoder, Std$Object$Nil);
		break;
	}
}

void riva_cbor_read_break_fn(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = Decoder->Collection;
	Decoder->Collection = Collection->Prev;
	Decoder->Tags = Collection->Tags;
	if (Collection->Key == IsList) {
		if (Decoder->ArrayEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->ArrayEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
			if (Status < FAILURE) value_handler(Decoder, Result.Val);
		}
	} else {
		if (Decoder->MapEndHandler != Std$Object$Nil) {
			Std$Function$result Result;
			int Status = Std$Function$call(Decoder->MapEndHandler, 1, &Result, Decoder->UserData, &Decoder->UserData);
			if (Status < FAILURE) value_handler(Decoder, Result.Val);
		}
	}
}

void riva_cbor_read_error_fn(decoder_t *Decoder, int Position, const char *Message) {
}

GLOBAL_FUNCTION(New, 0) {
	decoder_t *Decoder = new(decoder_t);
	Decoder->Type = T;
	Decoder->UserData = Std$Object$Nil;
	Decoder->Collection = 0;
	Decoder->Tags = 0;
	Decoder->FinalHandler = Std$Object$Nil;
	Decoder->MapStartHandler = Std$Object$Nil;
	Decoder->MapPairHandler = Std$Object$Nil;
	Decoder->MapEndHandler = Std$Object$Nil;
	Decoder->ArrayStartHandler = Std$Object$Nil;
	Decoder->ArrayValueHandler = Std$Object$Nil;
	Decoder->ArrayEndHandler = Std$Object$Nil;
	Decoder->TagHandler = Std$Object$Nil;
	riva_cbor_reader_init(Decoder->Reader);
	Decoder->Reader->UserData = Decoder;
	Result->Val = Decoder;
	return SUCCESS;
}

METHOD("parse", TYP, T, TYP, Std$String$T) {
	//printf("%s:%d\n", __func__, __LINE__);
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	Std$String$t *String = Args[1].Val;
	for (Std$Address$t *Block = String->Blocks; Block->Length.Value; ++Block) {
		riva_cbor_read(Decoder->Reader, Block->Value, Block->Length.Value);
	}
	RETURN0;
}

METHOD("parse", TYP, T, TYP, Std$Address$T) {
	//printf("%s:%d\n", __func__, __LINE__);
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	void *Data = Std$Address$get_value(Args[1].Val);
	size_t Length = Std$Address$get_length(Args[1].Val);
	riva_cbor_read(Decoder->Reader, Data, Length);
	RETURN0;
}

TYPED_INSTANCE(int, IO$Stream$write, T, decoder_t *Stream, const char *Buffer, int Count, int Blocks) {
	riva_cbor_read(Stream->Reader, Buffer, Count);
	return Count;
}

METHOD("userdata", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->UserData);
	return SUCCESS;
}

METHOD("onfinal", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->FinalHandler);
	return SUCCESS;
}

METHOD("onmapstart", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->MapStartHandler);
	return SUCCESS;
}

METHOD("onmappair", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->MapPairHandler);
	return SUCCESS;
}

METHOD("onmapend", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->MapEndHandler);
	return SUCCESS;
}

METHOD("onarraystart", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->ArrayStartHandler);
	return SUCCESS;
}

METHOD("onarrayvalue", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->ArrayValueHandler);
	return SUCCESS;
}

METHOD("onarrayend", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->ArrayEndHandler);
	return SUCCESS;
}

METHOD("ontag", TYP, T) {
	decoder_t *Decoder = Args[0].Val;
	Result->Val = *(Result->Ref = &Decoder->TagHandler);
	return SUCCESS;
}
