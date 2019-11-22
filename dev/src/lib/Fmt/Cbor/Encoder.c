#include <Std.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include "minicbor.h"

typedef struct encoder_t {
	const Std$Type$t *Type;
	Std$Object$t *Stream;
	minicbor_write_fn WriteFn;
} encoder_t;

TYPE(T);

GLOBAL_FUNCTION(New, 1) {
	encoder_t *Encoder = new(encoder_t);
	Encoder->Type = T;
	Encoder->Stream = Args[0].Val;
	Encoder->WriteFn = Util$TypedFunction$get(IO$Stream$write, Args[0].Val->Type);
	RETURN(Encoder);
}

METHOD("write_positive", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Value = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_positive(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

METHOD("write_positive", TYP, T, TYP, Std$Integer$BigT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[1].Val;
	mpz_t Temp;
	mpz_init_set(Temp, Arg->Value);
	mpz_tdiv_r_2exp(Temp, Temp, 64);
	uint64_t Value = 0;
	mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
	riva_cbor_write_positive(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

METHOD("write_negative", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Value = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_negative(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

METHOD("write_negative", TYP, T, TYP, Std$Integer$BigT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[1].Val;
	mpz_t Temp;
	mpz_init_set(Temp, Arg->Value);
	mpz_tdiv_r_2exp(Temp, Temp, 64);
	uint64_t Value = 0;
	mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
	riva_cbor_write_negative(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

METHOD("write_bytes", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Size = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_bytes(Encoder->Stream, Encoder->WriteFn, Size);
	RETURN0;
}

METHOD("write_indef_bytes", TYP, T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_indef_bytes(Encoder->Stream, Encoder->WriteFn);
	RETURN0;
}

METHOD("write_string", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Size = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_string(Encoder->Stream, Encoder->WriteFn, Size);
	RETURN0;
}

METHOD("write_indef_string", TYP, T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_indef_string(Encoder->Stream, Encoder->WriteFn);
	RETURN0;
}

METHOD("write_array", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Size = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_array(Encoder->Stream, Encoder->WriteFn, Size);
	RETURN0;
}

METHOD("write_indef_array", TYP, T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_indef_array(Encoder->Stream, Encoder->WriteFn);
	RETURN0;
}

METHOD("write_map", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Size = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_map(Encoder->Stream, Encoder->WriteFn, Size);
	RETURN0;
}

METHOD("write_indef_map", TYP, T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_indef_map(Encoder->Stream, Encoder->WriteFn);
	RETURN0;
}

METHOD("write_float2", TYP, T, TYP, Std$Real$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_float2(Encoder->Stream, Encoder->WriteFn, Std$Real$get_value(Args[0].Val));
	RETURN0;
}

METHOD("write_float4", TYP, T, TYP, Std$Real$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_float4(Encoder->Stream, Encoder->WriteFn, Std$Real$get_value(Args[0].Val));
	RETURN0;
}

METHOD("write_float8", TYP, T, TYP, Std$Real$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_float8(Encoder->Stream, Encoder->WriteFn, Std$Real$get_value(Args[0].Val));
	RETURN0;
}

METHOD("write_simple", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	unsigned char Simple = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_simple(Encoder->Stream, Encoder->WriteFn, Simple);
	RETURN0;
}

METHOD("write_break", TYP, T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_break(Encoder->Stream, Encoder->WriteFn);
	RETURN0;
}

METHOD("write_tag", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Value = Std$Integer$get_small(Args[1].Val);
	riva_cbor_write_tag(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

METHOD("write_tag", TYP, T, TYP, Std$Integer$BigT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[1].Val;
	mpz_t Temp;
	mpz_init_set(Temp, Arg->Value);
	mpz_tdiv_r_2exp(Temp, Temp, 64);
	uint64_t Value = 0;
	mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
	riva_cbor_write_tag(Encoder->Stream, Encoder->WriteFn, Value);
	RETURN0;
}

SYMBOL($write, "write");

METHOD("write", TYP, T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int32_t Value = Std$Integer$get_small(Args[1].Val);
	if (Value < 0) {
		riva_cbor_write_negative(Encoder->Stream, Encoder->WriteFn, ~Value);
	} else {
		riva_cbor_write_positive(Encoder->Stream, Encoder->WriteFn, Value);
	}
	RETURN0;
}

METHOD("write", TYP, T, TYP, Std$Integer$BigT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	Std$Integer$bigt *Arg = (Std$Integer$bigt *)Args[1].Val;
	mpz_t Temp;
	mpz_init_set(Temp, Arg->Value);
	int Neg = mpz_sgn(Temp) < 0;
	if (Neg) mpz_com(Temp, Temp);
	size_t BitCount = mpz_sizeinbase(Temp, 2);
	if (BitCount > 64) {
		size_t ByteCount = (BitCount + 7) / 8;
		void *Bytes = Riva$Memory$alloc_atomic(ByteCount);
		mpz_export(Bytes, 0, 1, 1, 0, 0, Temp);
		riva_cbor_write_tag(Encoder->Stream, Encoder->WriteFn, 2 + Neg);
		riva_cbor_write_bytes(Encoder->Stream, Encoder->WriteFn, ByteCount);
		Encoder->WriteFn(Encoder->Stream, Bytes, ByteCount);
	} else {
		mpz_tdiv_r_2exp(Temp, Temp, 64);
		uint64_t Value = 0;
		mpz_export(&Value, 0, -1, 1, 0, 0, Temp);
		if (Neg) {
			riva_cbor_write_negative(Encoder->Stream, Encoder->WriteFn, Value);
		} else {
			riva_cbor_write_positive(Encoder->Stream, Encoder->WriteFn, Value);
		}
	}
	RETURN0;
}

METHOD("write", TYP, T, TYP, Std$Real$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_float8(Encoder->Stream, Encoder->WriteFn, Std$Real$get_value(Args[0].Val));
	RETURN0;
}

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int Size = Std$Integer$get_small(Args[2].Val);
	riva_cbor_write_bytes(Encoder->Stream, Encoder->WriteFn, Size);
	Encoder->WriteFn(Encoder->Stream, Std$Address$get_value(Args[1].Val), Size);
	RETURN0;
}

METHOD("write", TYP, T, TYP, Std$Address$SizedT) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int Size = Std$Address$get_size(Args[1].Val);
	riva_cbor_write_bytes(Encoder->Stream, Encoder->WriteFn, Size);
	Encoder->WriteFn(Encoder->Stream, Std$Address$get_value(Args[1].Val), Size);
	RETURN0;
}

METHOD("write", TYP, T, TYP, Std$String$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	int Size = Std$String$get_length(Args[1].Val);
	riva_cbor_write_string(Encoder->Stream, Encoder->WriteFn, Size);
	for (Std$String$block *Block = Std$String$blocks(Args[1].Val); Block->Length.Value; ++Block) {
		Encoder->WriteFn(Encoder->Stream, Block->Chars.Value, Block->Length.Value);
	}
	RETURN0;
}

METHOD("write", TYP, T, TYP, Agg$List$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_array(Encoder->Stream, Encoder->WriteFn, Agg$List$length(Args[1].Val));
	Std$Function$argument SubArgs[2];
	SubArgs[0].Val = Args[0].Val;
	SubArgs[0].Ref = 0;
	SubArgs[1].Ref = 0;
	for (Agg$List$node *Node = Agg$List$head(Args[1].Val); Node; Node = Node->Next) {
		SubArgs[1].Val = Node->Value;
		if (Std$Function$invoke($write, 2, Result, SubArgs) == MESSAGE) return MESSAGE;
	}
	RETURN0;
}

static int write_map_pair(Std$Object$t *Key, Std$Object$t *Value, encoder_t *Encoder) {
	Std$Function$argument SubArgs[2];
	SubArgs[0].Val = (Std$Object$t *)Encoder;
	SubArgs[0].Ref = 0;
	SubArgs[1].Val = Key;
	SubArgs[1].Ref = 0;
	Std$Function$result Result[1];
	if (Std$Function$invoke($write, 2, Result, SubArgs) == MESSAGE) return 1;
	SubArgs[1].Val = Value;
	if (Std$Function$invoke($write, 2, Result, SubArgs) == MESSAGE) return 1;
	return 0;
}

METHOD("write", TYP, T, TYP, Agg$Table$T) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_map(Encoder->Stream, Encoder->WriteFn, Agg$Table$size(Args[1].Val));
	Agg$Table$foreach(Args[1].Val, write_map_pair, Encoder);
	RETURN0;
}

METHOD("write", TYP, T, VAL, Std$Object$Nil) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_simple(Encoder->Stream, Encoder->WriteFn, CBOR_SIMPLE_NULL);
	RETURN0;
}

METHOD("write", TYP, T, VAL, $false) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_simple(Encoder->Stream, Encoder->WriteFn, CBOR_SIMPLE_FALSE);
	RETURN0;
}

METHOD("write", TYP, T, VAL, $true) {
	//printf("%s()\n", __func__);
	encoder_t *Encoder = (encoder_t *)Args[0].Val;
	riva_cbor_write_simple(Encoder->Stream, Encoder->WriteFn, CBOR_SIMPLE_TRUE);
	RETURN0;
}
