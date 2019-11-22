#ifndef SND_LILV_H
#define SND_LILV_H

#include <Std/Type.h>

#define RIVA_MODULE Snd$Lilv
#include <Riva-Header.h>

#include <lilv/lilv.h>

RIVA_STRUCT(plugin_t) {
	const Std$Type$t *Type;
	const LilvPlugin *Handle;
};

RIVA_TYPE(PluginT);

RIVA_STRUCT(port_t) {
	const Std$Type$t *Type;
	const LilvPlugin *Plugin;
	const LilvPort *Handle;
};

RIVA_TYPE(PortT);

RIVA_STRUCT(instance_t) {
	const Std$Type$t *Type;
	LilvInstance *Handle;
};

RIVA_TYPE(InstanceT);

RIVA_CFUN(void, add_feature, const char *Uri, void *Data);

#undef RIVA_MODULE

#endif
