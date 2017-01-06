#ifndef AGG_TYPETABLE_H
#define AGG_TYPETABLE_H

#include <Std/Type.h>

#define RIVA_MODULE Agg$TypeTable
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	Std$Type_t const *Type;
	unsigned long Size, Space;
	void *Entries;
};

extern Std$Type_t Agg$TypeTable$T[];

#define Agg$TypeTable$INIT {Agg$TypeTable$T, 0, 0, 0}

RIVA_CFUN(void, init, Agg$TypeTable_t *Table);
RIVA_CFUN(void, put, Agg$TypeTable_t *Table, const Std$Type_t *Key, const void *Value);
RIVA_CFUN(void *, get, const Agg$TypeTable_t *Table, const Std$Type_t *Key);

#undef RIVA_MODULE

#endif
