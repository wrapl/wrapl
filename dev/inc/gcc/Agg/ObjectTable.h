#ifndef UTIL_OBJECTTABLE_H
#define UTIL_OBJECTTABLE_H

#include <Std/Type.h>

#define RIVA_MODULE Agg$ObjectTable
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	Std$Type_t const *Type;
	unsigned long Size, Space;
	void *Entries;
};

RIVA_TYPE(T);

#define Agg$ObjectTable$INIT {Agg$ObjectTable$T, 0, 0, 0}

RIVA_CFUN(void, init, Agg$ObjectTable_t *Table);
RIVA_CFUN(void, put, Agg$ObjectTable_t *Table, Std$Object_t *Key, void *Value);
RIVA_CFUN(void *, get, const Agg$ObjectTable_t *Table, Std$Object_t *Key);

#undef RIVA_MODULE

#endif
