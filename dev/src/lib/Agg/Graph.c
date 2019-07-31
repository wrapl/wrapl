#include <Std.h>
#include <Riva.h>
#include <Agg.h>
#include <Agg/IntegerTable.h>
#include <Agg/ObjectTable.h>

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void * __attribute__((__regparm__ (1))) ___tls_get_addr(tls_index *ti) {
};

TYPE(T);

TYPE(VertexT);
TYPE(EdgeT);

TYPE(VertexSelectionT);
TYPE(EdgeSelectionT);

#undef TYPE
#include <igraph/igraph.h>

SYMBOL($out, "out");
SYMBOL($in, "in");
SYMBOL($all, "all");

typedef struct graph_t graph_t;
typedef struct vertex_t vertex_t;
typedef struct edge_t edge_t;
typedef struct vertex_selection_t vertex_selection_t;
typedef struct edge_selection_t edge_selection_t;

struct graph_t {
	const Std$Type$t *Type;
	Std$Object$t *VertexList;
	Std$Object$t *EdgeList;
	Agg$ObjectTable$t VertexTable[1];
	Agg$ObjectTable$t EdgeTable[1];
	igraph_t Handle[1];
};

struct vertex_selection_t {
	const Std$Type$t *Type;
	igraph_vs_t Handle[1];
};

struct edge_selection_t {
	const Std$Type$t *Type;
	igraph_es_t Handle[1];
};

static inline igraph_integer_t get_id(Agg$ObjectTable$t *Table, Std$Object$t *Object) {
	return ((Std$Real$t *)Agg$ObjectTable$get(Table, Object))->Value;
};

static inline void set_id(Agg$ObjectTable$t *Table, Std$Object$t *Object, igraph_integer_t ID) {
	Agg$ObjectTable$put(Table, Object, Std$Real$new(ID));
};

GLOBAL_FUNCTION(New, 0) {
	graph_t *Graph = new(graph_t);
	Graph->Type = T;
	igraph_empty_attrs(Graph->Handle, 0, 0, Graph);
	Graph->VertexList= Agg$List$new(0);
	Graph->EdgeList = Agg$List$new(0);
	Result->Val = (Std$Object$t *)Graph;
	return SUCCESS;
};

GLOBAL_FUNCTION(DirectedNew, 0) {
	graph_t *Graph = new(graph_t);
	Graph->Type = T;
	igraph_empty_attrs(Graph->Handle, 0, 1, Graph);
	Graph->VertexList= Agg$List$new(0);
	Graph->EdgeList = Agg$List$new(0);
	Result->Val = (Std$Object$t *)Graph;
	return SUCCESS;
};

METHOD("addv", TYP, T, TYP, Agg$List$T) {
//@graph
//@vertices
//:T
// adds <code>vertices:length</code> new vertices to <var>graph</var>
	graph_t *Graph = (graph_t *)Args[0].Val;
	Agg$List$t *Vertices = (Agg$List$t *)Args[1].Val;
	if (Vertices->Length) {
		int N0 = igraph_vcount(Graph->Handle);
		int N1 = N0 + Vertices->Length;
		igraph_add_vertices(Graph->Handle, Vertices->Length, 0);
		Agg$List$node *Node = Vertices->Head;
		for (int ID = N0; ID < N1; ++ID, Node = Node->Next) {
			Std$Object$t *Value = Node->Value;
// 			printf("Adding Vertex: ID = %d, Object = %x\n", ID, Value);
			set_id(Graph->VertexTable, Value, ID);
			Agg$List$put(Graph->VertexList, Value);
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("join", TYP, T, ANY, ANY, ANY) {
//@graph
//@from
//@to
//@edge
//:T
// adds edges to <var>graph</var>
	graph_t *Graph = (graph_t *)Args[0].Val;
	igraph_integer_t VertexID1 = get_id(Graph->VertexTable, Args[1].Val);
	igraph_integer_t VertexID2 = get_id(Graph->VertexTable, Args[2].Val);
	Std$Object$t *Value = Args[3].Val;
	int ID = igraph_ecount(Graph->Handle);
	igraph_add_edge(Graph->Handle, VertexID1, VertexID2);
	set_id(Graph->EdgeTable, Value, ID);
	Agg$List$put(Graph->EdgeList, Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

static Std$Object$t *vertex_vector_to_list(graph_t *Graph, igraph_vector_t *Vector) {
	Std$Object$t *List = Agg$List$new0();
	for (int I = 0; I < igraph_vector_size(Vector); ++I) {
		int ID = VECTOR(Vector[0])[I];
		Std$Object$t *Object = Agg$List$find_node(Graph->VertexList, ID + 1)->Value;
		Agg$List$put(List, Object);
	};
	return List;
};

static void vertex_list_to_vector(graph_t *Graph, Agg$List$t *List, igraph_vector_t *Vector) {
	igraph_vector_resize(Vector, List->Length);
	int I = 0;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		VECTOR(Vector[0])[I] = get_id(Graph->VertexTable, Node->Value);
	};
};

static Std$Object$t *edge_vector_to_list(graph_t *Graph, igraph_vector_t *Vector) {
	Std$Object$t *List = Agg$List$new0();
	for (int I = 0; I < igraph_vector_size(Vector); ++I) {
		int ID = VECTOR(Vector[0])[I];
		Std$Object$t *Object = Agg$List$find_node(Graph->EdgeList, ID + 1)->Value;
		Agg$List$put(List, Object);
	};
	return List;
};

/*static void edge_weights_by_function(graph_t *Graph, Std$Function$t *Function, igraph_vector_t *Vector) {
	igraph_vector_resize(Vector, igraph_ecount(Graph->Handle));
	for (int I = 0; I < igraph_ecount(Graph->Handle); ++I) {
		Std$Function$result Result;
		Std$Function$call(Function, 1, &Result, VECTOR(Graph->EdgeVector[0])[I], 0);
		if (Result.Val->Type == Std$Integer$SmallT) {
			VECTOR(Vector[0])[I] = (double)((Std$Integer$smallt *)Result.Val)->Value;
		} else if (Result.Val->Type == Std$Real$T) {
			VECTOR(Vector[0])[I] = ((Std$Real$t *)Result.Val)->Value;
		};
	};
};

static void edge_weights_by_list(graph_t *Graph, Agg$List$t *List, igraph_vector_t *Vector) {
	igraph_vector_resize(Vector, List->Length);
	int I = 0;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) {
		Std$Object$t *Value = Node->Value;
		if (Value->Type == Std$Integer$SmallT) {
			VECTOR(Vector[0])[I] = (double)((Std$Integer$smallt *)Value)->Value;
		} else if (Value->Type == Std$Real$T) {
			VECTOR(Vector[0])[I] = ((Std$Real$t *)Value)->Value;
		};
	};
};*/

METHOD("vs", TYP, T, ANY) {
	graph_t *Graph = (graph_t *)Args[0].Val;
	igraph_integer_t ID = get_id(Graph->VertexTable, Args[1].Val);
	if (ID == -1) {
		Result->Val = Std$String$new("object is not a vertex");
		return MESSAGE;
	};
	vertex_selection_t *Selection = new(vertex_selection_t);
	Selection->Type = VertexSelectionT;
	igraph_vs_1(Selection->Handle, ID);
	Result->Val = (Std$Object$t *)Selection;
	return SUCCESS;
};

METHOD("vs", TYP, T, TYP, Agg$List$T) {
	graph_t *Graph = (graph_t *)Args[0].Val;
	igraph_vector_t *Vector = new(igraph_vector_t);
	igraph_vector_init(Vector, 0);
	vertex_list_to_vector(Graph, (Agg$List$t *)Args[1].Val, Vector);
	vertex_selection_t *Selection = new(vertex_selection_t);
	Selection->Type = VertexSelectionT;
	igraph_vs_vector(Selection->Handle, Vector);
	Result->Val = (Std$Object$t *)Selection;
	return SUCCESS;
};

METHOD("vs", TYP, T, TYP, Std$Symbol$T) {
	vertex_selection_t *Selection = new(vertex_selection_t);
	Selection->Type = VertexSelectionT;
	if (Args[1].Val == $all) {
		igraph_vs_all(Selection->Handle);
	} else {
		igraph_vs_none(Selection->Handle);
	};
	Result->Val = (Std$Object$t *)Selection;
	return SUCCESS;
};

METHOD("neighbours", TYP, T, ANY) {
	graph_t *Graph = (graph_t *)Args[0].Val;
	igraph_integer_t ID = get_id(Graph->VertexTable, Args[1].Val);
	igraph_vector_t Res[1];
	igraph_vector_init(Res, 0);
	igraph_neighbors(Graph->Handle, Res, ID, IGRAPH_ALL);
	Result->Val = vertex_vector_to_list(Graph, Res);
	return SUCCESS;
};

METHOD("neighbours", TYP, T, ANY, TYP, Std$Symbol$T) {
	graph_t *Graph = (graph_t *)Args[0].Val;
	igraph_integer_t ID = get_id(Graph->VertexTable, Args[1].Val);
	igraph_vector_t Res[1];
	igraph_vector_init(Res, 0);
	igraph_neimode_t Mode;
	if (Args[2].Val == $out) {
		Mode = IGRAPH_OUT;
	} else if (Args[2].Val == $in) {
		Mode = IGRAPH_IN;
	} else if (Args[2].Val == $all) {
		Mode = IGRAPH_ALL;
	};
	igraph_neighbors(Graph->Handle, Res, ID, Mode);
	Result->Val = vertex_vector_to_list(Graph, Res);
	return SUCCESS;
};

static int graph_init(igraph_t *Handle, graph_t *Graph) {
	Handle->attr = Graph;
	return IGRAPH_SUCCESS;
};

static void graph_destroy(igraph_t *Handle) {
};

static int graph_copy(igraph_t *ToHandle, const igraph_t *FromHandle, igraph_bool_t GA, igraph_bool_t VA, igraph_bool_t EA) {
	return IGRAPH_SUCCESS;
};

static int graph_add_vertices(igraph_t *Handle, long int NV, igraph_vector_ptr_t *Attrs) {
	return IGRAPH_SUCCESS;
};

static void permute(const igraph_vector_t *Permute, Agg$List$t *List, Agg$ObjectTable$t *Table) {
	Agg$List$empty(List);
	Agg$ObjectTable$init(Table);
	if (igraph_vector_size(Permute)) {
		int NoOfNodes = igraph_vector_max(Permute);
		Agg$List$node *Nodes[NoOfNodes];
		Agg$List$node *Node = List->Head;
		for (int Old = 0; Old < igraph_vector_size(Permute); ++Old, Node = Node->Next) {
			int New = VECTOR(*Permute)[Old];
			if (New) {
				Nodes[New - 1] = Node;
				set_id(Table, Node->Value, New - 1);
			};
		};
		List->Head = List->Cache = Node = Nodes[0];
		List->Index = 1;
		Node->Prev = 0;
		for (int I = 1; I < NoOfNodes; ++I) {
			Node->Next = Nodes[I];
			Nodes[I]->Prev = Node;
			Node = Nodes[I];
		};
		Node->Next = 0;
		List->Tail = Node;
		List->Length = NoOfNodes;

	};
};

static void graph_delete_vertices(igraph_t *Handle, const igraph_vector_t *Edges, const igraph_vector_t *Vertices) {
	graph_t *Graph = Handle->attr;
	permute(Edges, Graph->EdgeList, Graph->EdgeTable);
	permute(Vertices, Graph->VertexList, Graph->VertexTable);
};

static int graph_add_edges(igraph_t *Handle, const igraph_vector_t *Edges, igraph_vector_ptr_t *Attr) {
	return IGRAPH_SUCCESS;
};

static void graph_delete_edges(igraph_t *Handle, const igraph_vector_t *Edges) {
	graph_t *Graph = Handle->attr;
	permute(Edges, Graph->EdgeList, Graph->EdgeTable);
};

static void graph_permute_edges(igraph_t *Handle, const igraph_vector_t *Edges) {
	//graph_t *Graph = Handle->attr;
	//permute(Edges, Graph->EdgeList, Graph->EdgeTable);
};

static int graph_get_info(const igraph_t *Graph, igraph_strvector_t *GNames, igraph_vector_t *GTypes, igraph_strvector_t *VNames,
					igraph_vector_t *VTypes, igraph_strvector_t *ENames, igraph_vector_t *ETypes) {
	return IGRAPH_SUCCESS;
};

static igraph_bool_t graph_has_attr(const igraph_t *graph, igraph_attribute_elemtype_t type, const char *name) {
	return 0;
};

static int graph_gettype(const igraph_t *graph, igraph_attribute_type_t *type, igraph_attribute_elemtype_t elemtype, const char *name) {
	return IGRAPH_SUCCESS;
};

INITIAL() {
 	static igraph_attribute_table_t AttributeTable = {
 		.init = graph_init,
 		.destroy = graph_destroy,
 		.copy = graph_copy,
 		.add_vertices = graph_add_vertices,
 		.delete_vertices = graph_delete_vertices,
 		.add_edges = graph_add_edges,
 		.delete_edges = graph_delete_edges,
 		.permute_edges = graph_permute_edges,
 		.get_info = graph_get_info,
 		.has_attr = graph_has_attr,
 		.gettype = graph_gettype
 	};
 	igraph_i_set_attribute_table(&AttributeTable);
};
