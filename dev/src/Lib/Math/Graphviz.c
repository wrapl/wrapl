#include <Riva/Memory.h>
#include <Std.h>
#include <graphviz/gvc.h>
#include <pthread.h>

static GVC_t *Context;
static pthread_mutex_t Mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

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
	Result->Val = Std$String$new(agget(Object->Handle, Std$String$flatten(Args[1].Val)));
	return SUCCESS;
};

METHOD("set", TYP, ObjectT, TYP, Std$String$T, TYP, Std$String$T) {
	object_t *Object = (object_t *)Args[0].Val;
	agsafeset(Object->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val), Std$String$flatten(Args[2].Val));
	return SUCCESS;
};

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
	Result->Val = (Std$Object$t *)Graph;
	return SUCCESS;
};

METHOD("node", TYP, GraphT, TYP, Std$String$T) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	_node_t *Node = new(_node_t);
	Node->Type = NodeT;
	Node->Handle = agnode(Graph->Handle, Std$String$flatten(Args[1].Val), 1);
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
};

METHOD("edge", TYP, GraphT, TYP, NodeT, TYP, NodeT) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	_edge_t *Edge = new(_edge_t);
	char *Name;
	if (Count > 3) {
		Name = Std$String$flatten(Args[3].Val);
	} else {
		Name = asprintf("E%d", agnedges(Graph->Handle));
	};
	Edge->Type = EdgeT;
	Edge->Handle = agedge(Graph->Handle,
		((_node_t *)Args[2].Val)->Handle,
		((_node_t *)Args[1].Val)->Handle,
		Name,
		1
	);
	Result->Val = (Std$Object$t *)Edge;
	return SUCCESS;
};

METHOD("render", TYP, GraphT, TYP, Std$String$T) {
	_graph_t *Graph = (_graph_t *)Args[0].Val;
	const char *Layout = Std$String$flatten(Args[1].Val);
	pthread_mutex_lock(Mutex);
	gvLayout(Context, Graph->Handle, Layout);
	gvRender(Context, Graph->Handle, "dot", stdout);
	gvFreeLayout(Context, Graph->Handle);
	pthread_mutex_unlock(Mutex);
	Result->Arg = Args[0];
	return SUCCESS;
};

INITIAL() {
	//aginit();
	Context = gvContext();
};
