#ifndef UTIL_INTEGERTABLE_H
#define UTIL_INTEGERTABLE_H

#include <Std/Object.h>

#define RIVA_MODULE Agg$IntegerTable
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	Std$Type$t const *Type;
	unsigned long Size, Space;
	void *Entries;
};

RIVA_TYPE(T);

#define Agg$IntegerTable$INIT {Agg$IntegerTable$T, 0, 0, 0}

RIVA_CFUN(void, init, Agg$IntegerTable$t *Table);
RIVA_CFUN(void, put, Agg$IntegerTable$t *Table, unsigned long Key, void *Value);
RIVA_CFUN(void **, slot, Agg$IntegerTable$t *Table, unsigned long Key, void *Value);
RIVA_CFUN(void *, get, const Agg$IntegerTable$t *Table, unsigned long Key);

#undef RIVA_MODULE

#endif
