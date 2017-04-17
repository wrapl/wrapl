#ifndef CACHE_H
#define CACHE_H

#include "map.h"
#include <nettle/sha2.h>

void cache_open(const char *RootPath);
void cache_close();

void cache_hash_get(const char *Id, int *LastUpdated, int *LastChecked, uint8_t Digest[SHA256_DIGEST_SIZE]);
void cache_hash_set(const char *Id, uint8_t Digest[SHA256_DIGEST_SIZE]);
void cache_last_check_set(const char *Id);

struct map_t *cache_depends_get(const char *Id);
void cache_depends_set(const char *Id, struct map_t *Scans);

struct map_t *cache_scan_get(const char *Id);
void cache_scan_set(const char *Id, struct map_t *Scans);

extern int CurrentVersion;

#endif
