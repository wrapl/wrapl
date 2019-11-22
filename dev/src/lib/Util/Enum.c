#include <Std.h>
#include <Riva/Memory.h>
#include <Util/Enum.h>

TYPE(T);

METHOD("+", TYP, T, TYP, T) {
	CHECK_EXACT_ARG_TYPE(1, Args[0].Val);
	Util$Enum$t *Enum1 = (Util$Enum$t *)Args[0].Val;
	Util$Enum$t *Enum2 = (Util$Enum$t *)Args[1].Val;

	Util$Enum$t *Enum3 = Riva$Memory$alloc(sizeof(Util$Enum$t) + (Enum1->Count + Enum2->Count) * sizeof(const char *));
	Enum3->Type = Enum1->Type;
	Enum3->Value = Enum1->Value | Enum2->Value;
	char *Names = Enum3->Names;
	for (int I = 0; I < Enum1->Count; ++I) *(Names++) = Enum1->Names[I];
	for (int I = 0; I < Enum2->Count; ++I) *(Names++) = Enum2->Names[I];
	RETURN(Enum3);
}

AMETHOD(Std$String$Of, TYP, T) {
	Util$Enum$t *Enum = (Util$Enum$t *)Args[0].Val;
	int Length = strlen(Enum->Names[0]);
	for (int I = 1; I < Enum->Count; ++I) Length += 1 + strlen(Enum->Names[I]);
	char *Buffer = Riva$Memory$alloc_atomic(Length + 1);
	char *P = stpcpy(Buffer, Enum->Names[0]);
	for (int I = 1; I < Enum->Count; ++I) {
		*P++ = '|';
		P = stpcpy(P, Enum->Names[I]);
	}
	RETURN(Std$String$new_length(Buffer, Length));
}
