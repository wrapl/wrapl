#include <Riva/Memory.h>
#include <Std.h>
#include <Agg.h>
#include <IO/Stream.h>
#include <Riva/Module.h>
#include <Fmt/Yaml/Event.h>

typedef struct emitter_t {
	const Std$Type$t *Type;
	yaml_emitter_t Handle[1];
} emitter_t;

TYPE(T);

GLOBAL_FUNCTION(New, 1) {
	emitter_t *Emitter = new(emitter_t);
	Emitter->Type = T;
	yaml_emitter_initialize(Emitter->Handle);
	//yaml_parser_set_input(Parser->Handle, read_stream, Args[0].Val);
	RETURN(Emitter);
}

METHOD("emit", TYP, T, TYP, Fmt$Yaml$Event$T) {
	emitter_t *Emitter = (emitter_t *)Args[0].Val;
	Fmt$Yaml$Event$t *Event = (Fmt$Yaml$Event$t *)Args[1].Val;
	yaml_emitter_emit(Emitter->Handle, Event->Handle);
	RETURN0;
}
