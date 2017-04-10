#ifndef CACHE_H
#define CACHE_H

#include "map.h"
#include <nettle/sha2.h>

void cache_open(const char *RootPath);
void cache_close();

int cache_hash_get(const char *Id, uint8_t Digest[SHA256_DIGEST_SIZE]);
void cache_hash_set(const char *Id, uint8_t Digest[SHA256_DIGEST_SIZE], int Version);

int cache_updated_get(const char *Id);
void cache_updated_set(const char *Id);

struct map_t *cache_depends_get(const char *Id);
void cache_depends_set(const char *Id, struct map_t *Scans);

struct map_t *cache_scan_get(const char *Id);
void cache_scan_set(const char *Id, struct map_t *Scans);

extern int CurrentVersion;

#endif
