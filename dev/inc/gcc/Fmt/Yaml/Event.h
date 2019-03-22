#ifndef FMT_YAML_EVENT_H
#define FMT_YAML_EVENT_H

#include <Std/Object.h>
#include <yaml.h>

#define RIVA_MODULE Fmt$Yaml$Event
#include <Riva-Header.h>

RIVA_STRUCT(t) {
	const Std$Type_t *Type;
	yaml_event_t Handle[1];
};

RIVA_TYPE(T);
RIVA_TYPE(NoT);
RIVA_TYPE(StreamStartT);
RIVA_TYPE(StreamEndT);
RIVA_TYPE(DocumentStartT);
RIVA_TYPE(DocumentEndT);
RIVA_TYPE(AliasT);
RIVA_TYPE(ScalarT);
RIVA_TYPE(SequenceStartT);
RIVA_TYPE(SequenceEndT);
RIVA_TYPE(MappingStartT);
RIVA_TYPE(MappingEndT);

#undef RIVA_MODULE

#endif
