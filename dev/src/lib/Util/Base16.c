#include <Std.h>
#include <Riva/Memory.h>

GLOBAL_FUNCTION(Encode, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	static const char HexDigits[] = "0123456789ABCDEF";
	Std$String$t *String = (Std$String$t *)Args[0].Val;
	int Length = String->Length.Value * 2;
	char *Hex = Riva$Memory$alloc_atomic(Length + 1);
	char *HP = Hex;
	for (Std$Address$t *Block = String->Blocks; Block->Length.Value; ++Block) {
		char *CP = Block->Value;
		for (int I = Block->Length.Value; --I >= 0;) {
			char C = *CP++;
			*HP++ = HexDigits[(C >> 4) & 15];
			*HP++ = HexDigits[C & 15];
		}
	}
	*HP = 0;
	RETURN(Std$String$new_length(Hex, Length));
}

GLOBAL_FUNCTION(Decode, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Std$String$t *String = (Std$String$t *)Args[0].Val;
	int Length = (String->Length.Value + 1) / 2;
	char *Str = Riva$Memory$alloc_atomic(Length + 1);
	char *SP = Str;
	int Odd = 1;
	for (Std$Address$t *Block = String->Blocks; Block->Length.Value; ++Block) {
		char *CP = Block->Value;
		for (int I = Block->Length.Value; --I >= 0;) {
			char C = *CP++;
			if (C >= 'a') {
				C -= ('a' - 10);
			} else if (C >= 'A') {
				C -= ('A' - 10);
			} else {
				C -= '0';
			}
			if (Odd) {
				*SP = C << 4;
				Odd = 0;
			} else {
				*SP++ += C;
				Odd = 1;
			}
		}
	}
	if (!Odd) SP++;
	*SP = 0;
	RETURN(Std$String$new_length(Str, Length));
}
