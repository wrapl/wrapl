#ifndef BITSET_H
#define BITSET_H

#include <Riva/Memory.h>

#include <stdint.h>

struct bitset_internal_t;

struct bitset_t {
	bitset_internal_t *Next;
	bitset_t();
	bitset_t(int I) {};
	bitset_t(bitset_t *Copy);
	bitset_t(bitset_t *A, bitset_t *B);
	void reserve(int N);
	void reserve(uint32_t Lo, uint32_t Hi);
	void reserve(const bitset_t *B);
	uint32_t below(uint32_t Max);
	uint32_t allocate();
	uint32_t allocate(const bitset_t *With);
	uint32_t allocate(uint32_t Count);
	uint32_t allocate(uint32_t Count, const bitset_t *With);
	uint32_t size();
	void update(bitset_t *With);
};

#endif
