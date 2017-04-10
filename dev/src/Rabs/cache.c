#include "cache.h"
#include "util.h"
#include "target.h"
#include "rabs.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

static sqlite3 *Cache;
static sqlite3_stmt *HashGetStatement;
static sqlite3_stmt *HashSetStatement;
static sqlite3_stmt *UpdatedGetStatement;
static sqlite3_stmt *UpdatedSetStatement;
static sqlite3_stmt *DependsGetStatement;
static sqlite3_stmt *DependsDeleteStatement;
static sqlite3_stmt *DependsInsertStatement;
static sqlite3_stmt *ScanGetStatement;
static sqlite3_stmt *ScanDeleteStatement;
static sqlite3_stmt *ScanInsertStatement;
int CurrentVersion = 1;

static int version_callback(void *Data, int NumCols, char **Values, char **Names) {
	CurrentVersion = atoi(Values[0] ?: "0");
	return 0;
}

void cache_open(const char *RootPath) {
	const char *CacheFileName = concat(RootPath, "/_build_.db", 0);
	if (sqlite3_open_v2(CacheFileName, &Cache, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_exec(Cache, "CREATE TABLE IF NOT EXISTS info(key TEXT PRIMARY KEY, value);", 0, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_exec(Cache, "CREATE TABLE IF NOT EXISTS hashes(id TEXT PRIMARY KEY, version INTEGER, updated INTEGER, hash BLOB);", 0, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_exec(Cache, "CREATE TABLE IF NOT EXISTS scans(id TEXT, scan TEXT);", 0, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_exec(Cache, "CREATE TABLE IF NOT EXISTS depends(id TEXT, depend TEXT);", 0, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "SELECT hash, version FROM hashes WHERE id = ?", -1, &HashGetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "REPLACE INTO hashes(id, hash, version, updated) VALUES(?, ?, ?, ?)", -1, &HashSetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "SELECT updated FROM hashes WHERE id = ?", -1, &UpdatedGetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "UPDATE hashes SET updated = ? WHERE id = ?", -1, &UpdatedSetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "SELECT scan FROM scans WHERE id = ?", -1, &ScanGetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "DELETE FROM scans WHERE id = ?", -1, &ScanDeleteStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "INSERT INTO scans(id, scan) VALUES(?, ?)", -1, &ScanInsertStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "SELECT depend FROM depends WHERE id = ?", -1, &DependsGetStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "DELETE FROM depends WHERE id = ?", -1, &DependsDeleteStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_prepare_v2(Cache, "INSERT INTO depends(id, depend) VALUES(?, ?)", -1, &DependsInsertStatement, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	if (sqlite3_exec(Cache, "SELECT MAX(version) FROM hashes", version_callback, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	++CurrentVersion;
	printf("CurrentVersion = %d\n", CurrentVersion);
	atexit(cache_close);
}

void cache_close() {
	char Buffer[100];
	sprintf(Buffer, "REPLACE INTO info(key, value) VALUES('version', %d)", CurrentVersion);
	if (sqlite3_exec(Cache, Buffer, 0, 0, 0) != SQLITE_OK) {
		printf("Sqlite error: %s\n", sqlite3_errmsg(Cache));
		exit(1);
	}
	sqlite3_finalize(HashGetStatement);
	sqlite3_finalize(HashSetStatement);
	sqlite3_finalize(DependsGetStatement);
	sqlite3_finalize(DependsDeleteStatement);
	sqlite3_finalize(DependsInsertStatement);
	sqlite3_finalize(ScanGetStatement);
	sqlite3_finalize(ScanDeleteStatement);
	sqlite3_finalize(ScanInsertStatement);
	sqlite3_close(Cache);
}


int cache_hash_get(const char *Id, uint8_t Digest[SHA256_DIGEST_SIZE]) {
	sqlite3_bind_text(HashGetStatement, 1, Id, -1, SQLITE_STATIC);
	int Version = 0;
	if (sqlite3_step(HashGetStatement) == SQLITE_ROW) {
		memcpy(Digest, sqlite3_column_blob(HashGetStatement, 0), SHA256_DIGEST_SIZE);
		Version = sqlite3_column_int(HashGetStatement, 1);;
	}
	sqlite3_reset(HashGetStatement);
	return Version;
}

void cache_hash_set(const char *Id, uint8_t Digest[SHA256_DIGEST_SIZE], int Version) {
	sqlite3_bind_text(HashSetStatement, 1, Id, -1, SQLITE_STATIC);
	sqlite3_bind_blob(HashSetStatement, 2, Digest, SHA256_DIGEST_SIZE, SQLITE_STATIC);
	sqlite3_bind_int(HashSetStatement, 3, Version);
	sqlite3_bind_int(HashSetStatement, 4, CurrentVersion);
	sqlite3_step(HashSetStatement);
	sqlite3_reset(HashSetStatement);
}

int cache_updated_get(const char *Id) {
	sqlite3_bind_text(UpdatedGetStatement, 1, Id, -1, SQLITE_STATIC);
	int Updated = 0;
	if (sqlite3_step(UpdatedGetStatement) == SQLITE_ROW) {
		Updated = sqlite3_column_int(UpdatedGetStatement, 0);
	}
	sqlite3_reset(UpdatedGetStatement);
	return Updated;
}

void cache_updated_set(const char *Id) {
	sqlite3_bind_int(UpdatedSetStatement, 1, CurrentVersion);
	sqlite3_bind_text(UpdatedSetStatement, 2, Id, -1, SQLITE_STATIC);
	sqlite3_step(UpdatedSetStatement);
	sqlite3_reset(UpdatedSetStatement);
}

struct map_t *cache_depends_get(const char *Id) {
	sqlite3_bind_text(DependsGetStatement, 1, Id, -1, SQLITE_STATIC);
	struct map_t *Depends = 0;
	while (sqlite3_step(DependsGetStatement) == SQLITE_ROW) {
		if (Depends == 0) Depends = new_map();
		const char *DependId = concat(sqlite3_column_text(DependsGetStatement, 0), 0);
		map_set(Depends, DependId, target_find(DependId));
	}
	sqlite3_reset(DependsGetStatement);
	return Depends;
}

void cache_depends_set(const char *Id, struct map_t *Depends) {
	sqlite3_bind_text(DependsDeleteStatement, 1, Id, -1, SQLITE_STATIC);
	sqlite3_step(DependsDeleteStatement);
	sqlite3_reset(DependsDeleteStatement);
	sqlite3_bind_text(DependsInsertStatement, 1, Id, -1, SQLITE_STATIC);
	for (struct map_node_t *Node = Depends->head; Node; Node = Node->next) {
		sqlite3_bind_text(DependsInsertStatement, 2, Node->key, -1, SQLITE_STATIC);
		sqlite3_step(DependsInsertStatement);
		sqlite3_reset(DependsInsertStatement);
	}
}

struct map_t *cache_scan_get(const char *Id) {
	sqlite3_bind_text(ScanGetStatement, 1, Id, -1, SQLITE_STATIC);
	struct map_t *Scans = 0;
	while (sqlite3_step(ScanGetStatement) == SQLITE_ROW) {
		if (Scans == 0) Scans = new_map();
		const char *ScanId = concat(sqlite3_column_text(ScanGetStatement, 0), 0);
		map_set(Scans, ScanId, target_find(ScanId));
	}
	sqlite3_reset(ScanGetStatement);
	return Scans;
}

void cache_scan_set(const char *Id, struct map_t *Scans) {
	sqlite3_bind_text(ScanDeleteStatement, 1, Id, -1, SQLITE_STATIC);
	sqlite3_step(ScanDeleteStatement);
	sqlite3_reset(ScanDeleteStatement);
	sqlite3_bind_text(ScanInsertStatement, 1, Id, -1, SQLITE_STATIC);
	for (struct map_node_t *Node = Scans->head; Node; Node = Node->next) {
		sqlite3_bind_text(ScanInsertStatement, 2, Node->key, -1, SQLITE_STATIC);
		sqlite3_step(ScanInsertStatement);
		sqlite3_reset(ScanInsertStatement);
	}
}
