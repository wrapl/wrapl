#include <Std.h>
#include <Riva.h>
#include <Num/Array.h>
#include <Num/Bitset.h>

METHOD("[]", TYP, Num$Array$T, TYP, Num$Bitset$T) {
	Num$Array$t *Array = (Num$Array$t *)Args[0].Val;
	Num$Bitset$t *Indices = (Num$Bitset$t *)Args[1].Val;
	RETURN0;
}
