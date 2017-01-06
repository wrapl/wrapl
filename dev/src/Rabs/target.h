#ifndef TARGET_H
#define TARGET_H

#include <lua.h>
#include <lauxlib.h>

typedef struct target_t target_t;

void target_init();

int target_dir_new(lua_State *L);
int target_file_new(lua_State *L);
int target_meta_new(lua_State *L);

target_t *target_symb_new(const char *Name);

int target_tostring(lua_State *L);
void target_depends_add(target_t *Target, target_t *Depend);
void target_update(target_t *Target);
void target_depends_auto(target_t *Depend);
target_t *target_find(const char *Id);
void target_push(target_t *Target);

#endif
