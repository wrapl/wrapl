#include <Riva/Memory.h>
#include <Std.h>
#include <Agg.h>
#include <IO/Stream.h>
#include <Riva/Module.h>
#include <yaml.h>

extern int Riva$Symbol[];

typedef struct parser_t {
	const Std$Type_t *Type;
	yaml_parser_t Handle[1];
} parser_t;

TYPE(T);

typedef struct event_t {
	const Std$Type_t *Type;
	yaml_event_t Handle[1];
} event_t;

TYPE(EventT);

TYPE(NoEventT, EventT);

TYPE(StreamStartEventT, EventT);

TYPE(StreamEndEventT, EventT);

TYPE(DocumentStartEventT, EventT);

TYPE(DocumentEndEventT, EventT);

TYPE(AliasEventT, EventT);

METHOD("anchor", TYP, AliasEventT) {
	event_t *Event = Args[0].Val;
	Result->Val = Std$String$new(Event->Handle->data.alias.anchor);
	return SUCCESS;
};

TYPE(ScalarEventT, EventT);

METHOD("anchor", TYP, ScalarEventT) {
	event_t *Event = Args[0].Val;
	Result->Val = Std$String$new(Event->Handle->data.scalar.anchor);
	return SUCCESS;
};

METHOD("tag", TYP, ScalarEventT) {
	event_t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.scalar.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

METHOD("value", TYP, ScalarEventT) {
	event_t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.value) {
		Result->Val = Std$String$new_length(Event->Handle->data.scalar.value, Event->Handle->data.scalar.length);
	};
	return SUCCESS;
};

TYPE(SequenceStartEventT, EventT);

METHOD("anchor", TYP, SequenceStartEventT) {
	event_t *Event = Args[0].Val;
	Result->Val = Std$String$new(Event->Handle->data.sequence_start.anchor);
	return SUCCESS;
};

METHOD("tag", TYP, SequenceStartEventT) {
	event_t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.sequence_start.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

TYPE(SequenceEndEventT, EventT);

TYPE(MappingStartEventT, EventT);

METHOD("anchor", TYP, MappingStartEventT) {
	event_t *Event = Args[0].Val;
	Result->Val = Std$String$new(Event->Handle->data.mapping_start.anchor);
	return SUCCESS;
};

METHOD("tag", TYP, MappingStartEventT) {
	event_t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.mapping_start.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

TYPE(MappingEndEventT, EventT);

static int read_stream(IO$Stream_t *Stream, char *Buffer, int Size, int *Length) {
	int Bytes = *Length = IO$Stream$read(Stream, Buffer, Size, 0);
	return (Bytes == -1) ? 0 : 1;
};

GLOBAL_FUNCTION(New, 1) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	yaml_parser_initialize(Parser->Handle);
	yaml_parser_set_input(Parser->Handle, read_stream, Args[0].Val);
	Result->Val = Parser;
	return SUCCESS;
};

METHOD("next", TYP, T) {
	static const Std$Type_t *EventTypes[] = {
		[YAML_NO_EVENT] = NoEventT,
		[YAML_STREAM_START_EVENT] = StreamStartEventT,
		[YAML_STREAM_END_EVENT] = StreamEndEventT,
		[YAML_DOCUMENT_START_EVENT] = DocumentStartEventT,
		[YAML_DOCUMENT_END_EVENT] = DocumentEndEventT,
		[YAML_ALIAS_EVENT] = AliasEventT,
		[YAML_SCALAR_EVENT] = ScalarEventT,
		[YAML_SEQUENCE_START_EVENT] = SequenceStartEventT,
		[YAML_SEQUENCE_END_EVENT] = SequenceEndEventT,
		[YAML_MAPPING_START_EVENT] = MappingStartEventT,
		[YAML_MAPPING_END_EVENT] = MappingEndEventT
	};
	parser_t *Parser = Args[0].Val;
	event_t *Event = new(event_t);
	if (Parser->Handle->eof) return FAILURE;
	if (!yaml_parser_parse(Parser->Handle, Event->Handle)) {
		Result->Val = Std$String$copy(Parser->Handle->problem);
		return MESSAGE;
	};
	Event->Type = EventTypes[Event->Handle->type];
	Result->Val = Event;
	return SUCCESS;
};

