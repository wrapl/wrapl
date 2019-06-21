#include <Riva/Memory.h>
#include <Std.h>
#include <Agg/List.h>
#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>
#include <pthread.h>

static GVC_t *Context;
static pthread_mutex_t Mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

typedef struct object_record_t {
	Agrec_t Header;
	void *Object;
} object_record_t;

typedef struct object_t {
	const Std$Type$t *Type;
	void *Handle;
} object_t;

TYPE(ObjectT);

typedef struct graph_t {
	const Std$Type$t *Type;
	Agraph_t *Handle;
} _graph_t;

TYPE(GraphT, ObjectT);

typedef struct node_t {
	const Std$Type$t *Type;
	Agnode_t *Handle;
} _node_t;

TYPE(NodeT, ObjectT);

typedef struct edge_t {
	const Std$Type$t *Type;
	Agedge_t *Handle;
} _edge_t;

TYPE(EdgeT, ObjectT);

METHOD("get", TYP, ObjectT, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	char *Value = agget(Object->Handle, Std$String$flatten(Args[1].Val));
	if (Value) {
		RETURN(Std$String$new(Value));
	} else {
		FAIL;
	}
}

METHOD(".", TYP, ObjectT, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	char *Value = agget(Object->Handle, Std$String$flatten(Args[1].Val));
	if (Value) {
		RETURN(Std$String$new(Value));
	} else {
		FAIL;
	}
}

METHOD("set", TYP, ObjectT, TYP, Std$String$T, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	agsafeset(Object->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val), Std$String$flatten(Args[2].Val));
	RETURN0;
}

AMETHOD(Std$String$Of, TYP, ObjectT) {
	object_t *Object = (object_t *)Args[0].Val;
	RETURN(Std$String$new(agnameof(Object->Handle)));
}

typedef struct type_t {
	const Std$Type$t *Type;
	Agdesc_t *Value;
} _type_t;

TYPE(TypeT);

_type_t GraphDefault[1] = {{TypeT, &Agundirected}};
_type_t GraphStrict[1] = {{TypeT, &Agstrictundirected}};
_type_t GraphDirected[1] = {{TypeT, &Agdirected}};
_type_t GraphDirectedStrict[1] = {{TypeT, &Agstrictdirected}};

GLOBAL_FUNCTION(New, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, TypeT);
	_graph_t *Graph = new(_graph_t);
	Graph->Type = GraphT;
	Graph->Handle = agopen(Std$String$flatten(Args[0].Val), ((_type_t *)Args[1].Val)->Value[0], 0);
	object_record_t *Record = agbindrec(Graph->Handle, "riva", sizeof(object_record_t), 0);
	Record->Object = Graph;
	Result->Val = (Std$Object$t *)Graph;
	return SUCCESS;
}

GLOBAL_FUNCTION(Parse, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	_graph_t *Graph = new(_graph_t);
	Graph->Type = GraphT;
	Graph->Handle = agmemread(Std$String$flatten(Args[0].Val));
	if (!Graph->Handle) SEND(Std$String$new("Parse error!"));
	object_record_t *Record = agbindrec(Graph->Handle, "riva", sizeof(object_record_t), 0);
	Record->Object = Graph;
	Result->Val = (Std$Object$t *)Graph;
	return SUCCESS;
}

GLOBAL_FUNCTION(Plugins, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	int Size;
	const char **Names = gvPluginList(Context, Std$String$flatten(Args[0].Val), &Size, "");
	Std$Object$t *List = Agg$List$new0();
	for (int I = 0; I < Size; ++I) {
		Agg$List$put(List, Std$String$new(Names[I]));
	}
	RETURN(List);
}

Std$Integer$smallt KindGraph[] = {{Std$Integer$SmallT, AGRAPH}};
Std$Integer$smallt KindNode[] = {{Std$Integer$SmallT, AGNODE}};
Std$Integer$smallt KindInEdge[] = {{Std$Integer$SmallT, AGINEDGE}};
Std$Integer$smallt KindOutEdge[] = {{Std$Integer$SmallT, AGOUTEDGE}};
Std$Integer$smallt KindEdge[] = {{Std$Integer$SmallT, AGEDGE}};

METHOD("get", TYP, GraphT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	agattr(Graph->Handle, Std$Integer$get_small(Args[1].Val), Std$String$flatten(Args[2].Val), 0);
	RETURN0;
}

METHOD("set", TYP, GraphT, TYP, Std$Integer$SmallT, TYP, Std$String$T, TYP, Std$String$T) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	char *Value = agattr(Graph->Handle, Std$Integer$get_small(Args[1].Val), Std$String$flatten(Args[2].Val), Std$String$flatten(Args[3].Val));
	if (Value) {
		RETURN(Std$String$new(Value));
	} else {
		FAIL;
	}

}

METHOD("node", TYP, GraphT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	char *Name;
	if (Count > 1) {
		Name = Std$String$flatten(Args[1].Val);
	} else {
		asprintf(&Name, "%s:N%d", agnameof(Graph->Handle), agnnodes(Graph->Handle));
	}
	Agnode_t *N = agnode(Graph->Handle, Name, 1);
	object_record_t *Record = aggetrec(N, "riva", 0);
	if (!Record) {
		_node_t *Node = new(_node_t);
		Node->Type = NodeT;
		Node->Handle = N;
		Record = agbindrec(Node->Handle, "riva", sizeof(object_record_t), 0);
		Record->Object = Node;
	}
	RETURN(Record->Object);
};

METHOD("nodes", TYP, GraphT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	Std$Object$t *Nodes = Agg$List$new0();
	Agnode_t *N = agfstnode(Graph->Handle);
	while (N) {
		object_record_t *Record = aggetrec(N, "riva", 0);
		if (!Record) {
			_node_t *Node = new(_node_t);
			Node->Type = NodeT;
			Node->Handle = N;
			Record = agbindrec(N, "riva", sizeof(object_record_t), 0);
			Record->Object = Node;
		}
		Agg$List$put(Nodes, Record->Object);
		N = agnxtnode(Graph->Handle, N);
	}
	RETURN(Nodes);
}

METHOD("edge", TYP, GraphT, TYP, NodeT, TYP, NodeT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	_node_t *Head = (_node_t *)Args[1].Val;
	_node_t *Tail = (_node_t *)Args[2].Val;
	char *Name;
	if (Count > 3) {
		Name = Std$String$flatten(Args[3].Val);
	} else {
		asprintf(&Name, "%s->%s", agnameof(Head->Handle), agnameof(Tail->Handle));
	}
	Agedge_t *E = agedge(Graph->Handle, Tail->Handle, Head->Handle, Name, 1);
	object_record_t *Record = aggetrec(E, "riva", 0);
	if (!Record) {
		_edge_t *Edge = new(_edge_t);
		Edge->Type = EdgeT;
		Edge->Handle = E;
		Record = agbindrec(E, "riva", sizeof(object_record_t), 0);
		Record->Object = Edge;
	}
	RETURN(Record->Object);
}

METHOD("edges", TYP, GraphT, TYP, NodeT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	_node_t *Node = (_node_t *)Args[1].Val;
	Std$Object$t *Edges = Agg$List$new0();
	Agedge_t *E = agfstedge(Graph->Handle, Node->Handle);
	while (E) {
		object_record_t *Record = aggetrec(E, "riva", 0);
		if (!Record) {
			_edge_t *Edge = new(_edge_t);
			Edge->Type = EdgeT;
			Edge->Handle = E;
			Record = agbindrec(E, "riva", sizeof(object_record_t), 0);
			Record->Object = Edge;
		}
		Agg$List$put(Edges, Record->Object);
		E = agnxtedge(Graph->Handle, E, Node->Handle);
	}
	RETURN(Edges);
}

METHOD("subgraph", TYP, GraphT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	char *Name;
	if (Count > 1) {
		Name = Std$String$flatten(Args[1].Val);
	} else {
		asprintf(&Name, "%s:G%d", agnameof(Graph->Handle), agnsubg(Graph->Handle));
	}
	Agraph_t *G = agsubg(Graph->Handle, Name, 1);
	object_record_t *Record = aggetrec(G, "riva", 0);
	if (!Record) {
		_graph_t *SubGraph = new(_graph_t);
		SubGraph->Type = GraphT;
		SubGraph->Handle = G;
		Record = agbindrec(G, "riva", sizeof(object_record_t), 0);
		Record->Object = SubGraph;
	}
	RETURN(Record->Object);
}

METHOD("subgraphs", TYP, GraphT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	Std$Object$t *SubGraphs = Agg$List$new0();
	Agraph_t *G = agfstsubg(Graph->Handle);
	while (G) {
		object_record_t *Record = aggetrec(G, "riva", 0);
		if (!Record) {
			_graph_t *SubGraph = new(_graph_t);
			SubGraph->Type = GraphT;
			SubGraph->Handle = G;
			Record = agbindrec(G, "riva", sizeof(object_record_t), 0);
			Record->Object = SubGraph;
		}
		Agg$List$put(SubGraphs, Record->Object);
		G = agnxtnode(Graph->Handle, G);
	}
	RETURN(SubGraphs);
}

METHOD("delete", TYP, GraphT, TYP, ObjectT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	object_t *Object = (object_t *)Args[1].Val;
	agdelete(Graph->Handle, Object->Handle);
	RETURN0;
}

METHOD("render", TYP, GraphT, TYP, Std$String$T, TYP, Std$String$T) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	const char *Layout = Std$String$flatten(Args[1].Val);
	const char *Render = Std$String$flatten(Args[2].Val);
	pthread_mutex_lock(Mutex);
	gvLayout(Context, Graph->Handle, Layout);
	attach_attrs(Graph->Handle);
	char *Data;
	int Length;
	gvRenderData(Context, Graph->Handle, Render, &Data, &Length);
	gvFreeLayout(Context, Graph->Handle);
	pthread_mutex_unlock(Mutex);
	RETURN(Std$String$new_length(Data, Length));
}

extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
extern gvplugin_library_t gvplugin_core_LTX_library;
extern gvplugin_library_t gvplugin_pango_LTX_library;
extern gvplugin_library_t gvplugin_rsvg_LTX_library;

INITIAL() {
	//aginit();
	Context = gvContext();
	gvAddLibrary(Context, &gvplugin_dot_layout_LTX_library);
	gvAddLibrary(Context, &gvplugin_neato_layout_LTX_library);
	gvAddLibrary(Context, &gvplugin_core_LTX_library);
	gvAddLibrary(Context, &gvplugin_pango_LTX_library);
	gvAddLibrary(Context, &gvplugin_rsvg_LTX_library);
}
