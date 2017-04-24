#ifndef CACHE_H
#define CACHE_H

#include <libHX/map.h>
#include <nettle/sha2.h>
#include <time.h>

void cache_open(const char *RootPath);
void cache_close();

void cache_hash_get(const char *Id, int *LastUpdated, int *LastChecked, time_t *FileTime, uint8_t Digest[SHA256_DIGEST_SIZE]);
void cache_hash_set(const char *Id, time_t FileTime, uint8_t Digest[SHA256_DIGEST_SIZE]);
void cache_last_check_set(const char *Id);

struct HXmap *cache_depends_get(const char *Id);
void cache_depends_set(const char *Id, struct HXmap *Scans);

struct HXmap *cache_scan_get(const char *Id);
void cache_scan_set(const char *Id, struct HXmap *Scans);

extern int CurrentVersion;

#endif
