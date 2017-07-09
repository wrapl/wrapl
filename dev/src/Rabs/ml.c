#include "minilang.h"
#include "map.h"
#include <stdio.h>
#include <gc.h>

static map_void_t Globals[1];

static ml_value_t *global_get(ml_t *ML, void *Data, const char *Name) {
	ml_value_t **Address = (ml_value_t **)map_get(Globals, Name);
	if (Address) return Address[0];
	return 0;
}

static ml_value_t *global_print(ml_t *ML, void *Data, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = Args[I];
		fputs(Value->Type->to_string(ML, Value), stdout);
	}
	fflush(stdout);
	return Nil;
}

int main(int Argc, const char *Argv[]) {
	map_init(Globals);
	map_set(Globals, "print", ml_function(0, global_print));
	ml_t *ML = ml_new(0, global_get);
	ml_value_t *Closure = ml_load(ML, Argv[1]);
	ml_call(ML, Closure, 0, 0);
	return 0;
}
