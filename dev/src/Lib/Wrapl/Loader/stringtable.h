#ifndef STRINGTABLE_H
#define STRINGTABLE_H

extern "C" {

typedef struct {
	char *Key;
	unsigned long Hash, Incr;
	void *Value;
} stringtable_node;

typedef struct {
	unsigned long Size, Space;
	stringtable_node *Entries;
} stringtable_t[1];

void stringtable_init(stringtable_t Table);
void stringtable_put(stringtable_t Table, const char *Key, void *Value);
void *stringtable_get(const stringtable_t Table, const char *Key);

};

#endif
