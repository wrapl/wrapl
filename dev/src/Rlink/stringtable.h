#ifndef STRINGTABLE_H
#define STRINGTABLE_H

typedef void *stringtable_t[4];

void stringtable_init(stringtable_t Table);
void stringtable_put(stringtable_t Table, const char *Key, void *Value);
void *stringtable_get(stringtable_t Table, const char *Key);

#endif
