#ifndef UTIL_STRINGTABLE_H
#define UTIL_STRINGTABLE_H

#include <Std/Type.h>

#define RIVA_MODULE Agg$StringTable
#include <Riva-Header.h>

RIVA_STRUCT(node) {
	const char *Key;
	unsigned long Length, Hash;
	void *Value;
};

RIVA_STRUCT(t) {
	Std$Type$t const *Type;
	unsigned long Size, Space;
	Agg$StringTable$node *Entries;
};

RIVA_TYPE(T);

#define Agg$StringTable$INIT {Agg$StringTable$T, 0, 0, 0}

RIVA_CFUN(void, init, Agg$StringTable$t *Table);
RIVA_CFUN(void, put, Agg$StringTable$t *Table, const char *Key, int Length, void *Value);
RIVA_CFUN(void **, slot, Agg$StringTable$t *Table, const char *Key, int Length, void *Value);
RIVA_CFUN(int, inc, Agg$StringTable$t *Table, const char *Key, int Length, int Value);
RIVA_CFUN(void *, def, Agg$StringTable$t *Table, const char *Key, int Length, void *Value);
RIVA_CFUN(void *, get, const Agg$StringTable$t *Table, const char *Key, int Length);

#undef RIVA_MODULE

#endif
