#include <Std.h>
#include <Riva.h>
#include <Fmt/Yaml/Event.h>
#include <Util/Enum.h>

extern int Riva$Symbol[];

TYPE(T);

TYPE(NoT, T);

TYPE(StreamStartT, T);

TYPE(StreamEndT, T);

TYPE(DocumentStartT, T);

TYPE(DocumentEndT, T);

TYPE(AliasT, T);

METHOD("anchor", TYP, AliasT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	Result->Val = Std$String$new(Event->Handle->data.alias.anchor);
	return SUCCESS;
};

TYPE(ScalarT, T);

METHOD("anchor", TYP, ScalarT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	const char *Anchor = Event->Handle->data.sequence_start.anchor;
	if (Anchor) {
		RETURN(Std$String$new(Anchor));
	} else {
		FAIL;
	}
};

METHOD("tag", TYP, ScalarT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.scalar.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

METHOD("value", TYP, ScalarT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.value) {
		Result->Val = Std$String$new_length(Event->Handle->data.scalar.value, Event->Handle->data.scalar.length);
	};
	return SUCCESS;
};

ENUM_TYPE(ScalarStyleT);

TYPE(SequenceStartT, T);

METHOD("anchor", TYP, SequenceStartT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	const char *Anchor = Event->Handle->data.sequence_start.anchor;
	if (Anchor) {
		RETURN(Std$String$new(Anchor));
	} else {
		FAIL;
	}
};

METHOD("tag", TYP, SequenceStartT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.sequence_start.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

TYPE(SequenceEndT, T);

TYPE(MappingStartT, T);

METHOD("anchor", TYP, MappingStartT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	const char *Anchor = Event->Handle->data.sequence_start.anchor;
	if (Anchor) {
		RETURN(Std$String$new(Anchor));
	} else {
		FAIL;
	}
};

METHOD("tag", TYP, MappingStartT) {
	Fmt$Yaml$Event$t *Event = Args[0].Val;
	if (Event->Handle->data.scalar.tag) {
		int Type; void *Value;
		const char *Tag = Event->Handle->data.mapping_start.tag;
		while (*Tag == '!') ++Tag;
		Riva$Module$import(Riva$Symbol, Tag, &Type, &Value);
		Result->Val = Value;
	};
	return SUCCESS;
};

TYPE(MappingEndT, T);
