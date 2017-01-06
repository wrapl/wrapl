#ifndef INTEGERTABLE_H
#define INTEGERTABLE_H

extern "C" {

#include <stdint.h>

typedef struct {
	uint32_t Key;
	void *Value;
} integertable_node;

typedef struct {
	uint32_t Size, Space;
	integertable_node *Entries;
} integertable_t[1];

void integertable_init(integertable_t Table);
void integertable_put(integertable_t Table, uint32_t Key, void *Value);
void *integertable_get(const integertable_t Table, uint32_t Key);

};

#endif
