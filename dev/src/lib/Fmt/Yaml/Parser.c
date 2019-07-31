#include <Riva/Memory.h>
#include <Std.h>
#include <Agg.h>
#include <IO/Stream.h>
#include <Riva/Module.h>
#include <Fmt/Yaml/Event.h>

typedef struct parser_t {
	const Std$Type$t *Type;
	yaml_parser_t Handle[1];
} parser_t;

TYPE(T);

static int read_stream(IO$Stream$t *Stream, char *Buffer, int Size, int *Length) {
	int Bytes = *Length = IO$Stream$read(Stream, Buffer, Size, 0);
	return (Bytes == -1) ? 0 : 1;
}

GLOBAL_FUNCTION(New, 1) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	yaml_parser_initialize(Parser->Handle);
	yaml_parser_set_input(Parser->Handle, read_stream, Args[0].Val);
	RETURN(Parser);
}

METHOD("next", TYP, T) {
	static const Std$Type$t *EventTypes[] = {
		[YAML_NO_EVENT] = Fmt$Yaml$Event$NoT,
		[YAML_STREAM_START_EVENT] = Fmt$Yaml$Event$StreamStartT,
		[YAML_STREAM_END_EVENT] = Fmt$Yaml$Event$StreamEndT,
		[YAML_DOCUMENT_START_EVENT] = Fmt$Yaml$Event$DocumentStartT,
		[YAML_DOCUMENT_END_EVENT] = Fmt$Yaml$Event$DocumentEndT,
		[YAML_ALIAS_EVENT] = Fmt$Yaml$Event$AliasT,
		[YAML_SCALAR_EVENT] = Fmt$Yaml$Event$ScalarT,
		[YAML_SEQUENCE_START_EVENT] = Fmt$Yaml$Event$SequenceStartT,
		[YAML_SEQUENCE_END_EVENT] = Fmt$Yaml$Event$SequenceEndT,
		[YAML_MAPPING_START_EVENT] = Fmt$Yaml$Event$MappingStartT,
		[YAML_MAPPING_END_EVENT] = Fmt$Yaml$Event$MappingEndT
	};
	parser_t *Parser = Args[0].Val;
	Fmt$Yaml$Event$t *Event = new(Fmt$Yaml$Event$t);
	if (Parser->Handle->stream_end_produced) return FAILURE;
	if (!yaml_parser_parse(Parser->Handle, Event->Handle)) {
		SEND(Std$String$copy(Parser->Handle->problem));
	};
	Event->Type = EventTypes[Event->Handle->type];
	RETURN(Event);
}
