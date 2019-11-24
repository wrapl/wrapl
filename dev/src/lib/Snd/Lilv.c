#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <Agg/Table.h>
#include <Agg/StringTable.h>
#include <Snd/Lilv.h>
#include <string.h>

static LilvWorld *World;
Agg$StringTable$t PluginsTable[1] = {Agg$StringTable$INIT};

TYPE(PluginClassT);

TYPE(PluginT);

TYPE(PortT);

TYPE(InstanceT);

static LilvNode *riva_to_lilv(LilvWorld *World, Std$Object$t *Value) {
	if (Value->Type == Std$Integer$SmallT) {
		return lilv_new_int(World, Std$Integer$get_small(Value));
	} else if (Value->Type == Std$Real$T) {
		return lilv_new_float(World, Std$Real$get_value(Value));
	} else if (Value->Type == Std$String$T) {
		return lilv_new_string(World, Std$String$flatten(Value));
	} else {
		return 0;
	};
};

GLOBAL_FUNCTION(LoadAll, 0) {
	lilv_world_load_all(World);
	return SUCCESS;
};

GLOBAL_FUNCTION(GetAllPlugins, 0) {
	const LilvPlugins *Plugins = lilv_world_get_all_plugins(World);
	Std$Object$t *List = Agg$List$new0();
	for (LilvIter *Iter = lilv_plugins_begin(Plugins); !lilv_plugins_is_end(Plugins, Iter); Iter = lilv_plugins_next(Plugins, Iter)) {
		const LilvPlugin *Handle = lilv_plugins_get(Plugins, Iter);
		const char *Name = lilv_node_as_string(lilv_plugin_get_uri(Handle));
		Snd$Lilv$plugin_t *Plugin = new(Snd$Lilv$plugin_t);
		Plugin->Type = PluginT;
		Plugin->Handle = lilv_plugins_get(Plugins, Iter);
		Agg$List$put(List, (Std$Object$t *)Plugin);
		Agg$StringTable$put(PluginsTable, Name, strlen(Name), (Std$Object$t *)Plugin);
	};
	Result->Val = List;
	return SUCCESS;
};

static const LV2_Feature **Features;

void _add_feature(const char *Uri, void *Data) {
	LV2_Feature *Feature = new(LV2_Feature);
	Feature->URI = Uri;
	Feature->data = Data;
	size_t Count = 0;
	while (Features[Count]) ++Count;
	Features = (const LV2_Feature **)Riva$Memory$realloc(Features, (Count + 2) * sizeof(LV2_Feature *));
	Features[Count] = Feature;
};

METHOD("name", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Result->Val = Std$String$new(lilv_node_as_string(lilv_plugin_get_name(Plugin)));
	return SUCCESS;
};

METHOD("uri", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Result->Val = Std$String$new(lilv_node_as_string(lilv_plugin_get_uri(Plugin)));
	return SUCCESS;
};

typedef struct plugin_class_t {
	const Std$Type$t *Type;
	const LilvPluginClass *Handle;
} plugin_class_t;

METHOD("label", TYP, PluginClassT) {
	const LilvPluginClass *Class = ((plugin_class_t *)Args[0].Val)->Handle;
	Result->Val = Std$String$new(lilv_node_as_string(lilv_plugin_class_get_label(Class)));
	return SUCCESS;
};

METHOD("class", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	plugin_class_t *Class = new(plugin_class_t);
	Class->Type = PluginClassT;
	Class->Handle = lilv_plugin_get_class(Plugin);
	Result->Val = (Std$Object$t *)Class;
	return SUCCESS;
};

METHOD("extensions", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Std$Object$t *List = Agg$List$new0();
	LilvNodes *Nodes = lilv_plugin_get_extension_data(Plugin);
	for (LilvIter *Iter = lilv_nodes_begin(Nodes); !lilv_nodes_is_end(Nodes, Iter); Iter = lilv_nodes_next(Nodes, Iter)) {
		const LilvNode *Node = lilv_nodes_get(Nodes, Iter);
		Agg$List$put(List, Std$String$new(lilv_node_as_string(Node)));
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("required_features", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Std$Object$t *List = Agg$List$new0();
	LilvNodes *Nodes = lilv_plugin_get_required_features(Plugin);
	for (LilvIter *Iter = lilv_nodes_begin(Nodes); !lilv_nodes_is_end(Nodes, Iter); Iter = lilv_nodes_next(Nodes, Iter)) {
		const LilvNode *Node = lilv_nodes_get(Nodes, Iter);
		Agg$List$put(List, Std$String$new(lilv_node_as_string(Node)));
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("optional_features", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Std$Object$t *List = Agg$List$new0();
	LilvNodes *Nodes = lilv_plugin_get_optional_features(Plugin);
	for (LilvIter *Iter = lilv_nodes_begin(Nodes); !lilv_nodes_is_end(Nodes, Iter); Iter = lilv_nodes_next(Nodes, Iter)) {
		const LilvNode *Node = lilv_nodes_get(Nodes, Iter);
		Agg$List$put(List, Std$String$new(lilv_node_as_string(Node)));
	};
	Result->Val = List;
	return SUCCESS;
};

METHOD("num_ports", TYP, PluginT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	Result->Val = Std$Integer$new_small(lilv_plugin_get_num_ports(Plugin));
	return SUCCESS;
};

METHOD("[]", TYP, PluginT, TYP, Std$Integer$SmallT) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	const LilvPort *Handle = lilv_plugin_get_port_by_index(Plugin, Std$Integer$get_small(Args[1].Val));
	if (Handle) {
		Snd$Lilv$port_t *Port = new(Snd$Lilv$port_t);
		Port->Type = PortT;
		Port->Plugin = Plugin;
		Port->Handle = Handle;
		Result->Val = (Std$Object$t *)Port;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("[]", TYP, PluginT, TYP, Std$String$T) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	const LilvPort *Handle = lilv_plugin_get_port_by_symbol(Plugin, lilv_new_uri(World, Std$String$flatten(Args[1].Val)));
	if (Handle) {
		Snd$Lilv$port_t *Port = new(Snd$Lilv$port_t);
		Port->Type = PortT;
		Port->Plugin = Plugin;
		Port->Handle = Handle;
		Result->Val = (Std$Object$t *)Port;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD(".", TYP, PluginT, TYP, Std$String$T) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	const LilvPort *Handle = lilv_plugin_get_port_by_symbol(Plugin, lilv_new_uri(World, Std$String$flatten(Args[1].Val)));
	if (Handle) {
		Snd$Lilv$port_t *Port = new(Snd$Lilv$port_t);
		Port->Type = PortT;
		Port->Plugin = Plugin;
		Port->Handle = Handle;
		Result->Val = (Std$Object$t *)Port;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("name", TYP, PortT) {
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[0].Val;
	Result->Val = Std$String$new(lilv_node_as_string(lilv_port_get_name(Port->Plugin, Port->Handle)));
	return SUCCESS;
};

METHOD("index", TYP, PortT) {
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(lilv_port_get_index(Port->Plugin, Port->Handle));
	return SUCCESS;
};

METHOD("range", TYP, PortT, ANY, ANY, ANY) {
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[0].Val;
	LilvNode *Default, *Min, *Max;
	lilv_port_get_range(Port->Plugin, Port->Handle, &Default, &Min, &Max);
	if (Args[1].Ref) Args[1].Ref[0] = Default ? Std$Real$new(lilv_node_as_float(Default)) : Std$Object$Nil;
	if (Args[2].Ref) Args[2].Ref[0] = Min ? Std$Real$new(lilv_node_as_float(Min)) : Std$Object$Nil;
	if (Args[3].Ref) Args[3].Ref[0] = Max ? Std$Real$new(lilv_node_as_float(Max)) : Std$Object$Nil;
	return SUCCESS;
};

METHOD("classes", TYP, PortT) {
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[0].Val;
	const LilvNodes *Classes = lilv_port_get_classes(Port->Plugin, Port->Handle);
	Std$Object$t *List = Agg$List$new0();
	for (LilvIter *Iter = lilv_nodes_begin(Classes); !lilv_nodes_is_end(Classes, Iter); Iter = lilv_nodes_next(Classes, Iter)) {
		Agg$List$push(List, Std$String$new(lilv_node_as_string((lilv_nodes_get(Classes, Iter)))));
	};
	Result->Val = (Std$Object$t *)List;
	return SUCCESS;
};

METHOD("properties", TYP, PortT) {
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[0].Val;
	const LilvNodes *Properties = lilv_port_get_properties(Port->Plugin, Port->Handle);
	Std$Object$t *List = Agg$List$new0();
	for (LilvIter *Iter = lilv_nodes_begin(Properties); !lilv_nodes_is_end(Properties, Iter); Iter = lilv_nodes_next(Properties, Iter)) {
		Agg$List$push(List, Std$String$new(lilv_node_as_string((lilv_nodes_get(Properties, Iter)))));
	};
	Result->Val = (Std$Object$t *)List;
	return SUCCESS;
};

METHOD("instantiate", TYP, PluginT, TYP, Std$Number$T) {
	const LilvPlugin *Plugin = ((Snd$Lilv$plugin_t *)Args[0].Val)->Handle;
	double SampleRate;
	if (Args[1].Val->Type == Std$Integer$SmallT) {
		SampleRate = Std$Integer$get_small(Args[1].Val);
	} else if (Args[1].Val->Type == Std$Real$T) {
		SampleRate = Std$Real$get_value(Args[1].Val);
	} else {
		SampleRate = 48000;
	};
	LilvInstance *Handle = lilv_plugin_instantiate(Plugin, SampleRate, Features);
	if (Handle) {
		Snd$Lilv$instance_t *Instance = new(Snd$Lilv$instance_t);
		Instance->Type = InstanceT;
		Instance->Handle = Handle;
		Result->Val = (Std$Object$t *)Instance;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("connect", TYP, InstanceT, TYP, Std$Integer$SmallT, TYP, Std$Address$T) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	uint32_t PortIndex = Std$Integer$get_small(Args[1].Val);
	void *DataLocation = Std$Address$get_value(Args[2].Val);
	printf("Connecting lv2 port #%d to 0x%x\n", PortIndex, DataLocation);
	lilv_instance_connect_port(Instance, PortIndex, DataLocation);
	return SUCCESS;
};

METHOD("connect", TYP, InstanceT, TYP, PortT, TYP, Std$Address$T) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	Snd$Lilv$port_t *Port = (Snd$Lilv$port_t *)Args[1].Val;
	uint32_t PortIndex = lilv_port_get_index(Port->Plugin, Port->Handle);
	void *DataLocation = Std$Address$get_value(Args[2].Val);
	printf("Connecting lv2 port #%d to 0x%x\n", PortIndex, DataLocation);
	lilv_instance_connect_port(Instance, PortIndex, DataLocation);
	return SUCCESS;
};

METHOD("extension", TYP, InstanceT, TYP, Std$String$T) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	const void *Data = lilv_instance_get_extension_data(Instance, Std$String$flatten(Args[1].Val));
	if (Data) {
		Result->Val = Std$Address$new(Data, 0);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("activate", TYP, InstanceT) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	lilv_instance_activate(Instance);
	return SUCCESS;
};

METHOD("run", TYP, InstanceT, TYP, Std$Integer$SmallT) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	uint32_t SampleCount = Std$Integer$get_small(Args[1].Val);
	//printf("Running lv2 instance for %d samples \n", SampleCount);
	lilv_instance_run(Instance, SampleCount);
	return SUCCESS;
};

METHOD("deactivate", TYP, InstanceT) {
	LilvInstance *Instance = ((Snd$Lilv$instance_t *)Args[0].Val)->Handle;
	lilv_instance_deactivate(Instance);
	return SUCCESS;
};

INITIAL() {
	World = lilv_world_new();
	Features = (const LV2_Feature **)Riva$Memory$alloc(sizeof(LV2_Feature *));
};
