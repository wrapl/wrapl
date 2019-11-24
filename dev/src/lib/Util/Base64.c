#include <Std.h>
#include <Riva/Memory.h>
#include <stdio.h>

#define BlockSize 16

static const char Base64Chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t Base64Value[128] = {0,};

void _encode(char *OutChars, const char *InChars, size_t InLength) {
	int NumBlocks = InLength / 3;
	int Remainder = InLength % 3;
	while (--NumBlocks >= 0) {
		unsigned char Char1 = *InChars++;
		unsigned char Char2 = *InChars++;
		unsigned char Char3 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
		OutChars[2] = Base64Chars[((Char2 << 2) & 63) + (Char3 >> 6)];
		OutChars[3] = Base64Chars[Char3 & 63];
		OutChars += 4;
	};
	if (Remainder == 1) {
		unsigned char Char1 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[(Char1 << 4) & 63];
		OutChars[2] = '=';
		OutChars[3] = '=';
	} else if (Remainder == 2) {
		unsigned char Char1 = *InChars++;
		unsigned char Char2 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
		OutChars[2] = Base64Chars[(Char2 << 2) & 63];
		OutChars[3] = '=';
	};
};

GLOBAL_FUNCTION(Encode, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Std$String$t *InString = (Std$String$t *)Args[0].Val;
	int TotalInLength = InString->Length.Value;
	
	Std$String$t *OutString = Std$String$alloc((TotalInLength + BlockSize * 3 - 1) / (BlockSize * 3));
	int TotalOutLength = 0;
	Std$Address$t *OutBlock = OutString->Blocks;
	
	Std$Address$t *InBlock = InString->Blocks;
	size_t InLength = InBlock->Length.Value;
	const unsigned char *InChars = InBlock->Value;
	
	unsigned char *OutChars;
	while (TotalInLength > 3 * BlockSize) {
		int NumBlocks = BlockSize;
		OutBlock->Length.Value = BlockSize * 4;
		OutBlock->Value = OutChars = Riva$Memory$alloc_atomic(BlockSize * 4);
		TotalOutLength += BlockSize * 4;
		TotalInLength -= BlockSize * 3;
		while (--NumBlocks >= 0) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char1 = *InChars++;
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char2 = *InChars++;
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char3 = *InChars++;
			--InLength;
			OutChars[0] = Base64Chars[Char1 >> 2];
			OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
			OutChars[2] = Base64Chars[((Char2 << 2) & 63) + (Char3 >> 6)];
			OutChars[3] = Base64Chars[Char3 & 63];
			OutChars += 4;
		};
		++OutBlock;
	};
	if (TotalInLength) {
		int NumBlocks = TotalInLength / 3;
		int Remainder = TotalInLength % 3;
		int OutLength = NumBlocks * 4;
		if (Remainder) OutLength += 4;
		OutBlock->Length.Value = OutLength;
		OutBlock->Value = OutChars = Riva$Memory$alloc_atomic(OutLength);
		TotalOutLength += OutLength;
		while (--NumBlocks >= 0) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char1 = *InChars++;
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char2 = *InChars++;
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char3 = *InChars++;
			--InLength;
			OutChars[0] = Base64Chars[Char1 >> 2];
			OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
			OutChars[2] = Base64Chars[((Char2 << 2) & 63) + (Char3 >> 6)];
			OutChars[3] = Base64Chars[Char3 & 63];
			OutChars += 4;
		};
		if (Remainder == 1) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char1 = *InChars++;
			OutChars[0] = Base64Chars[Char1 >> 2];
			OutChars[1] = Base64Chars[(Char1 << 4) & 63];
			OutChars[2] = '=';
			OutChars[3] = '=';
		} else if (Remainder == 2) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char1 = *InChars++;
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			unsigned char Char2 = *InChars++;
			OutChars[0] = Base64Chars[Char1 >> 2];
			OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
			OutChars[2] = Base64Chars[(Char2 << 2) & 63];
			OutChars[3] = '=';
		};
	};
	
	OutString->Length.Value = TotalOutLength;
	Std$String$freeze(OutString);
	Result->Val = (Std$Object$t *)OutString;
	return SUCCESS;
};

size_t _decode(char *OutChars, const char *InChars, size_t InLength) {
	int NumBlocks = InLength / 4;
	size_t OutLength = 0;
	while (--NumBlocks >= 0) {
		uint8_t Byte1 = Base64Value[*InChars++];
		if (Byte1 == 255) return OutLength;
		uint8_t Byte2 = Base64Value[*InChars++];
		if (Byte2 == 255) return OutLength;
		uint8_t Byte3 = Base64Value[*InChars++];
		if (Byte3 == 255) {
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			return OutLength + 1;
		};
		uint8_t Byte4 = Base64Value[*InChars++];
		if (Byte4 == 255) {
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
			return OutLength + 2;
		};
		OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
		OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
		OutChars[2] = (Byte3 << 6) + Byte4;
		OutChars += 3;
		OutLength += 3;
	};
	return OutLength;
};

GLOBAL_FUNCTION(Decode, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Std$String$t *InString = (Std$String$t *)Args[0].Val;
	int TotalInLength = InString->Length.Value;
	
	Std$String$t *OutString = Std$String$alloc((TotalInLength + BlockSize * 4 - 1) / (BlockSize * 4));
	int TotalOutLength = 0;
	Std$Address$t *OutBlock = OutString->Blocks;
	
	Std$Address$t *InBlock = InString->Blocks;
	size_t InLength = InBlock->Length.Value;
	const unsigned char *InChars = InBlock->Value;
	
	unsigned char *OutChars;
	while (TotalInLength > 4 * BlockSize) {
		int NumBlocks = BlockSize;
		OutBlock->Length.Value = BlockSize * 3;
		OutBlock->Value = OutChars = Riva$Memory$alloc_atomic(BlockSize * 3);
		TotalOutLength += BlockSize * 3;
		TotalInLength -= BlockSize * 4;
		while (--NumBlocks >= 0) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte1 = Base64Value[*InChars++];
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte2 = Base64Value[*InChars++];
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte3 = Base64Value[*InChars++];
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte4 = Base64Value[*InChars++];
			--InLength;
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
			OutChars[2] = (Byte3 << 6) + Byte4;
			OutChars += 3;
		};
		++OutBlock;
	};
	if (TotalInLength) {
		int NumBlocks = TotalInLength / 4;
		int OutLength = NumBlocks * 3;
		OutBlock->Length.Value = OutLength;
		OutBlock->Value = OutChars = Riva$Memory$alloc_atomic(OutLength);
		TotalOutLength += OutLength;
		while (--NumBlocks >= 0) {
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte1 = Base64Value[*InChars++];
			if (Byte1 == 255) {
				OutBlock->Length.Value -= 3;
				TotalOutLength -= 3;
				break;
			};
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte2 = Base64Value[*InChars++];
			if (Byte2 == 255) {
				OutBlock->Length.Value -= 3;
				TotalOutLength -= 3;
				break;
			};
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte3 = Base64Value[*InChars++];
			if (Byte3 == 255) {
				OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
				OutBlock->Length.Value -= 2;
				TotalOutLength -= 2;
				break;
			};
			--InLength;
			while (InLength == 0) { ++InBlock; InChars = InBlock->Value; InLength = InBlock->Length.Value; };
			uint8_t Byte4 = Base64Value[*InChars++];
			if (Byte4 == 255) {
				OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
				OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
				OutBlock->Length.Value -= 1;
				TotalOutLength -= 1;
				break;
			};
			--InLength;
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
			OutChars[2] = (Byte3 << 6) + Byte4;
			OutChars += 3;
		};
	};
finished:
	OutString->Length.Value = TotalOutLength;
	Std$String$freeze(OutString);
	Result->Val = (Std$Object$t *)OutString;
	return SUCCESS;
};

INITIAL() {
	for (int I = 0; I < 64; ++I) Base64Value[Base64Chars[I]] = I;
	Base64Value['='] = 255;
};
