#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <cbor.h>

typedef struct item_t {
	const Std$Type$t *Type;
	cbor_item_t *Handle;
} item_t;

TYPE(T);

ASYMBOL(Build);

TYPED_FUNCTION(cbor_item_t *, build, Std$Object$t *Value) {
	Std$Function$result Result[1];
	if (Std$Function$call(Build, 1, Result, Value, 0) < FAILURE) {
		if (Result->Val->Type == T) return Result->Val;
	}
	return 0;
}

GLOBAL_FUNCTION(BuildInt8, 1) {
	//printf("%s()\n", __func__);
	int32_t Value = Std$Integer$get_small(Args[0].Val);
	item_t *Item = new(item_t);
	Item->Type = T;
	if (Value < 0) {
		Item->Handle = cbor_build_uint8(~Value);
		cbor_mark_negint(Item->Handle);
	} else {
		Item->Handle = cbor_build_uint8(Value);
	}
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildInt16, 1) {
	//printf("%s()\n", __func__);
	int32_t Value = Std$Integer$get_small(Args[0].Val);
	item_t *Item = new(item_t);
	Item->Type = T;
	if (Value < 0) {
		Item->Handle = cbor_build_uint16(~Value);
		cbor_mark_negint(Item->Handle);
	} else {
		Item->Handle = cbor_build_uint16(Value);
	}
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildInt32, 1) {
	//printf("%s()\n", __func__);
	int32_t Value = Std$Integer$get_small(Args[0].Val);
	item_t *Item = new(item_t);
	Item->Type = T;
	if (Value < 0) {
		Item->Handle = cbor_build_uint32(~Value);
		cbor_mark_negint(Item->Handle);
	} else {
		Item->Handle = cbor_build_uint32(Value);
	}
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildInt64, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	if (Args[0].Val->Type == Std$Integer$SmallT) {
		int64_t Value = Std$Integer$get_small(Args[0].Val);
		if (Value < 0) {
			Item->Handle = cbor_build_uint64(~Value);
			cbor_mark_negint(Item->Handle);
		} else {
			Item->Handle = cbor_build_uint64(Value);
		}
		RETURN(Item);
	} else if (Args[0].Val->Type == Std$Integer$BigT) {
		Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[0].Val;
		mpz_t Temp;
		mpz_init_set(Temp, Arg->Value);
		int Neg = mpz_sgn(Temp) < 0;
		if (Neg) mpz_com(Temp, Temp);
		mpz_tdiv_r_2exp(Temp, Temp, 64);
		uint64_t Value = 0;
		mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
		Item->Handle = cbor_build_uint64(Value);
		if (Neg) cbor_mark_negint(Item->Handle);
		RETURN(Item);
	} else {
		CHECK_EXACT_ARG_TYPE(0, Std$Integer$T);
		RETURN(Std$Object$Nil);
	}
}

GLOBAL_FUNCTION(BuildInteger, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	if (Args[0].Val->Type == Std$Integer$SmallT) {
		int32_t Value = Std$Integer$get_small(Args[0].Val);
		if (Value < 0) {
			Value = ~Value;
			if (Value < 256) {
				Item->Handle = cbor_build_uint8(Value);
			} else if (Value < 65536) {
				Item->Handle = cbor_build_uint16(Value);
			} else {
				Item->Handle = cbor_build_uint32(Value);
			}
			cbor_mark_negint(Item->Handle);
		} else {
			if (Value < 256) {
				Item->Handle = cbor_build_uint8(Value);
			} else if (Value < 65536) {
				Item->Handle = cbor_build_uint16(Value);
			} else {
				Item->Handle = cbor_build_uint32(Value);
			}
		}
	} else if (Args[0].Val->Type == Std$Integer$BigT) {
		Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[0].Val;
		mpz_t Temp;
		mpz_init_set(Temp, Arg->Value);
		int Neg = mpz_sgn(Temp) < 0;
		if (Neg) mpz_com(Temp, Temp);
		size_t BitCount = mpz_sizeinbase(Temp, 2);
		if (BitCount > 64) {
			size_t ByteCount = (BitCount + 7) / 8;
			void *Bytes = Riva$Memory$alloc_atomic(ByteCount);
			mpz_export(Bytes, 0, 1, 1, 0, 0, Temp);
			cbor_item_t *Handle = Item->Handle = cbor_new_tag(2 + Neg);
			cbor_tag_set_item(Handle, cbor_build_bytestring(Bytes, ByteCount));
		} else {
			mpz_tdiv_r_2exp(Temp, Temp, 64);
			uint64_t Value = 0;
			mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
			Item->Handle = cbor_build_uint64(Value);
			if (Neg) cbor_mark_negint(Item->Handle);
		}
	} else {
		CHECK_EXACT_ARG_TYPE(0, Std$Integer$T);
	}
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildByteString, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Std$Address$sizedt *Address = (Std$Address$sizedt *)Args[0].Val;
	Item->Handle = cbor_build_bytestring(Address->Value, Address->Length.Value);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildString, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Std$String$t *String = (Std$String$t *)Args[0].Val;
	if (String->Count == 0) {
		cbor_item_t *Handle = Item->Handle = cbor_new_definite_string();
	} else if (String->Count == 1) {
		cbor_item_t *Handle = Item->Handle = cbor_build_stringn(String->Blocks[0].Chars.Value, String->Blocks[0].Length.Value);
	} else {
		cbor_item_t *Handle = Item->Handle = cbor_new_indefinite_string();
		for (Std$String$block *Block = String->Blocks; Block->Length.Value; ++Block) {
			cbor_item_t *Chunk = cbor_build_stringn(Block->Chars.Value, Block->Length.Value);
			cbor_string_add_chunk(Handle, Chunk);
		}
	}
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildArray, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	cbor_item_t *Handle = Item->Handle = cbor_new_definite_array(Agg$List$length(Args[0].Val));
	for (Agg$List$node *Node = Agg$List$head(Args[0].Val); Node; Node = Node->Next) {
		item_t *Child = build(Node->Value);
		if (!Child) SEND(Std$String$new("Unable to build cbor value"));
		cbor_array_push(Handle, Child->Handle);
	}
	RETURN(Item);
}

static int build_map_pair(Std$Object$t *Key, Std$Object$t *Value, cbor_item_t *Handle) {
	item_t *KeyItem = build(Key);
	if (!KeyItem) return 1;
	item_t *ValueItem = build(Value);
	if (!ValueItem) return 1;
	struct cbor_pair Pair = {KeyItem->Handle, ValueItem->Handle};
	cbor_map_add(Handle, Pair);
	return 0;
}

GLOBAL_FUNCTION(BuildMap, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	cbor_item_t *Handle = Item->Handle = cbor_new_definite_map(Agg$Table$size(Args[0].Val));
	Agg$Table$foreach(Args[0].Val, build_map_pair, Handle);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildTag, 2) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	cbor_item_t *Handle = Item->Handle = cbor_new_tag(Std$Integer$get_u64(Args[0].Val));
	cbor_tag_set_item(Handle, ((item_t *)Args[1].Val)->Handle);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildFloat2, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_float2(Std$Real$get_value(Args[0].Val));
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildFloat4, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_float4(Std$Real$get_value(Args[0].Val));
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildFloat8, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_float8(Std$Real$get_value(Args[0].Val));
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildNil, 0) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_ctrl(CBOR_CTRL_NULL);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildTrue, 0) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_ctrl(CBOR_CTRL_TRUE);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildFalse, 0) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_ctrl(CBOR_CTRL_FALSE);
	RETURN(Item);
}

GLOBAL_FUNCTION(BuildSymbol, 1) {
	//printf("%s()\n", __func__);
	item_t *Item = new(item_t);
	Item->Type = T;
	Item->Handle = cbor_build_ctrl(Args[0].Val == $true ? CBOR_CTRL_TRUE : CBOR_CTRL_FALSE);
	RETURN(Item);
}

SET_AMETHOD(Build, BuildInteger, TYP, Std$Integer$T);
SET_AMETHOD(Build, BuildFloat8, TYP, Std$Real$T);
SET_AMETHOD(Build, BuildByteString, TYP, Std$Address$SizedT);
SET_AMETHOD(Build, BuildString, TYP, Std$String$T);
SET_AMETHOD(Build, BuildArray, TYP, Agg$List$T);
SET_AMETHOD(Build, BuildMap, TYP, Agg$Table$T);
SET_AMETHOD(Build, BuildNil, VAL, Std$Object$Nil);
SET_AMETHOD(Build, BuildTrue, VAL, $true);
SET_AMETHOD(Build, BuildFalse, VAL, $false);

METHOD("encode", TYP, T) {
	item_t *Item = (item_t *)Args[0].Val;
	size_t Size = 16;
	for (int I = 0; I < 31; ++I) {
		cbor_data Buffer = Riva$Memory$alloc_atomic(Size);
		size_t Length = cbor_serialize(Item->Handle, Buffer, Size);
		if (Length) RETURN(Std$Address$new_sized(Buffer, Length));
		Size *= 2;
	}
	SEND(Std$String$new("Error encoding cbor item"));
}

static void nop_free(void *Ptr) {}

INIT() {
	cbor_set_allocs(Riva$Memory$alloc, Riva$Memory$realloc, nop_free);
}
