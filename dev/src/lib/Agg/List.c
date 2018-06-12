#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>

#include <string.h>
#include <stdio.h>

#ifdef THREADED

#include <pthread.h>

typedef struct _list {
	const Std$Type_t *Type;
	Agg$List_node *Head, *Tail, *Cache, **Array;
	unsigned long Length;
	unsigned long Index, Lower, Upper, Access;
	pthread_rwlock_t Lock[1];
} _list;

#define RDLOCK(L) pthread_rwlock_rdlock(L->Lock)
#define WRLOCK(L) pthread_rwlock_wrlock(L->Lock)
#define UNLOCK(L) pthread_rwlock_unlock(L->Lock)
#define INITLOCK(L) pthread_rwlock_init(L->Lock, 0)

#else

typedef struct Agg$List$t _list;

#define RDLOCK(L) 0
#define WRLOCK(L) 0
#define UNLOCK(L) 0
#define INITLOCK(L) 0

#endif

typedef struct Agg$List$node _node;

TYPE(T);
// A general purpose extensible list type. Lists are indexed starting at <code>1</code>.

#ifdef THREADED
GLOBAL_FUNCTION(New, 0) {
//@length : Std$Integer$T := 0
//:T
// Returns a new list with Length elements
	_list *List = new(_list);
	List->Type = T;
	if (Count > 0) {
		long I, Size = Std$Integer$get_small(Args[0].Val);
		Std$Object_t *Value = Std$Object$Nil;
		_node *Node = new(_node);
		if (Count > 1) Value = Args[1].Val;
		Node->Value = Value;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (I = 1; I < Size; ++I) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Value;
		};
		List->Tail = Node;
		List->Length = Size;
	} else {
		//GC_ZEROED: List->Length = 0;
	};
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	return SUCCESS;
};
#endif

Std$Object_t *_new0() {
	_list *List = new(_list);
	List->Type = T;
	//GC_ZEROED: List->Length = 0;
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	return (Std$Object_t *)List;
};

Std$Object_t *_new(long Count, Std$Object_t *First, ...) {
	Std$Object_t **Values = &First;
	_list *List = new(_list);
	List->Type = T;
	if (Count > 0) {
		_node *Node = new(_node);
		Node->Value = Values[0];
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (int I = 1; I < Count; ++I) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Values[I];
		};
		List->Tail = Node;
		List->Length = Count;
	} else {
		//GC_ZEROED: List->Length = 0;
	};
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	return (Std$Object_t *)List;
};

Std$Object_t *_newv(long Count, Std$Object_t **Values) {
	_list *List = new(_list);
	List->Type = T;
	if (Count > 0) {
		_node *Node = new(_node);
		Node->Value = Values[0];
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (int I = 1; I < Count; ++I) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Values[I];
		};
		List->Tail = Node;
		List->Length = Count;
	} else {
		//GC_ZEROED: List->Length = 0;
	};
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	return (Std$Object_t *)List;
};

#ifdef THREADED
GLOBAL_FUNCTION(Make, 0) {
//@value&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;value&lt;sub&gt;k&lt;/sub&gt;
//:T
// returns a list with <var>value<sub>1</sub></var>, ... , <var>value<sub>k</sub></var> as its elements
	_list *List = new(_list);
	List->Type = T;
	if (Count > 0) {
		_node *Node = new(_node);
		Node->Value = Args[0].Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (int I = 1; I < Count; ++I) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Args[I].Val;
		};
		List->Tail = Node;
		List->Length = Count;
	} else {
		//GC_ZEROED: List->Length = 0;
	};
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	return SUCCESS;
};
#endif

 METHOD("_check", TYP, T) {
// internal function
    _list *List = (_list *)Args[0].Val;
    if (List->Array == 0) {
        printf("List has no array.\n");
        return SUCCESS;
    } else {
        printf("List has array [%d - %d]...", (int)List->Lower, (int)List->Upper);
        _node *Node = List->Head;
        for (int I = 1; I < List->Lower; ++I) Node = Node->Next;
        _node **Array = List->Array;
        for (int I = List->Lower; I <= List->Upper; ++I) {
            if (*(Array++) != Node) {
                printf("\n\tArray is incorrect at %d...", I);
            };
            Node = Node->Next;
        };
        printf("done.\n");
        return SUCCESS;
    };
};

 METHOD("_validate", TYP, T) {
// internal function
	_list *List = (_list *)Args[0].Val;
	_node **Nodes = (_node **)Riva$Memory$alloc(List->Length * sizeof(_node *));
	_node **Slot = Nodes;
	for (_node *Node = List->Head; Node; Node = Node->Next) *Slot++ = Node;
	for (_node *Node = List->Tail; Node; Node = Node->Prev) {
		if (*--Slot != Node) {
			Result->Val = Std$String$new("List pointers are broken internally!");
			return MESSAGE;
		}
	};
	if (List->Index && Nodes[List->Index - 1] != List->Cache) {
		Result->Val = Std$String$new("List cache is broken internally!");
		return MESSAGE;
	};
	if (List->Array) {
		 for (size_t I = List->Lower; I <= List->Upper; ++I) {
		 	if (List->Array[I - List->Lower] != Nodes[I - 1]) {
		 		Result->Val = Std$String$new("List array is broken internally!");
				return MESSAGE;
			};
		};
	};
	return SUCCESS;
};

 METHOD("_cache", TYP, T) {
	_list *List = (_list *)Args[0].Val;
	Result->Val = Std$Integer$new_small(List->Index);
	return SUCCESS;
};

SYMBOL($HASH, "#");
SYMBOL($COMP, "?");

METHOD("#", TYP, T) {
	Agg$ObjectTable$t TempCache[1] = {Agg$ObjectTable$INIT};
	Agg$ObjectTable$t *Cache;
	if (Count > 1) {
		Cache = (Agg$ObjectTable_t *)Args[1].Val;
		Std$Object_t *Prior = Agg$ObjectTable$get(Cache, Args[0].Val);
		if (Prior != (void *)0xFFFFFFFF) {
			Result->Val = Prior;
			return SUCCESS;
		};
	} else {
		Cache = TempCache;
	};
	_list *List = (_list *)Args[0].Val;
	int32_t Hash = List->Length;
	for (_node *Node = List->Head; Node; Node = Node->Next) {
		switch (Std$Function$call($HASH, 2, Result, Node->Value, 0, Cache, 0)) {
		case MESSAGE: return MESSAGE;
		case FAILURE: return FAILURE;
		case SUCCESS: case SUSPEND:
			Hash <<= 2;
			Hash ^= Std$Integer$get_small(Result->Val);
		};
	};
	Result->Val = Std$Integer$new_small(Hash);
	return SUCCESS;
};

METHOD("?", TYP, T, TYP, T) {
	_list *A = (_list *)Args[0].Val;
	_list *B = (_list *)Args[1].Val;
	Agg$ObjectTable$t TempCache[1] = {Agg$ObjectTable$INIT};
	Agg$ObjectTable$t *Cache, *SubCache;
	if (Count > 2) {
		Cache = (Agg$ObjectTable_t *)Args[2].Val;
		SubCache = Agg$ObjectTable$get(Cache, A);
		if (SubCache != (void *)0xFFFFFFFF) {
			Std$Object$t *Prior = Agg$ObjectTable$get(SubCache, B);
			if (Prior != (void *)0xFFFFFFFF) {
				Result->Val = Prior;
				return SUCCESS;
			};
		} else {
			SubCache = new(Agg$ObjectTable$t);
			SubCache->Type = Agg$ObjectTable$T;
			Agg$ObjectTable$put(Cache, A, SubCache);
		};
	} else {
		Cache = TempCache;
		SubCache = new(Agg$ObjectTable$t);
		SubCache->Type = Agg$ObjectTable$T;
		Agg$ObjectTable$put(Cache, A, SubCache);
	};
	for (_node *NodeA = A->Head, *NodeB = B->Head;; NodeA = NodeA->Next, NodeB = NodeB->Next) {
		if (NodeA == 0) {
			if (NodeB == 0) {
				Result->Val = Std$Object$Equal;
			} else {
				Result->Val = Std$Object$Less;
			}
			break;
		} else if (NodeB == 0) {
			Result->Val = Std$Object$Greater;
			break;
		} else {
			switch (Std$Function$call($COMP, 3, Result, NodeA->Value, 0, NodeB->Value, 0, Cache, 0)) {
			case MESSAGE: return MESSAGE;
			case FAILURE: return FAILURE;
			case SUCCESS: case SUSPEND: break;
			};
			if (Result->Val != Std$Object$Equal) break;
		};
	};
	Agg$ObjectTable$put(SubCache, B, Result->Val);
	return SUCCESS;
};

METHOD("empty", TYP, T) {
//@list
//:T
// empties <var>list</var> and returns it
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	List->Head = List->Tail = List->Cache = 0;
	List->Index = List->Lower = List->Upper = 0;
	List->Array = 0;
	List->Length = 0;
	List->Access = 4;
	UNLOCK(List);
	Result->Val = (Std$Object_t *)List;
	return SUCCESS;
};

Std$Object_t *_empty(_list *List) {
	WRLOCK(List);
	List->Head = List->Tail = List->Cache = 0;
	List->Index = List->Lower = List->Upper = 0;
	List->Array = 0;
	List->Length = 0;
	List->Access = 4;
	UNLOCK(List);
	return (Std$Object_t *)List;
};

METHOD("copy", TYP, T) {
//@list
//:T
// returns a shallow copy of <var>list</var>
	_list *List0 = (_list *)Args[0].Val;
	_list *List = new(_list);
	List->Type = T;
	RDLOCK(List0);
	if (List0->Head) {
		_node *Temp = List0->Head;
		_node *Node = new(_node);
		List->Head = Node;
		Node->Value = Temp->Value;
		while (Temp = Temp->Next) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Temp->Value;
		};
		List->Cache = Node;
		List->Index = List0->Length;
		List->Tail = Node;
	};
	List->Length = List0->Length;
	UNLOCK(List0);
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
//@a
//@b
//:T
// returns the concatenation of <var>a</var> and <var>b</var>
	_list *List0 = (_list *)Args[0].Val;
	_list *List1 = (_list *)Args[1].Val;
	RDLOCK(List0);
	RDLOCK(List1);
	_list *List = new(_list);
	List->Type = T;
	if (List0->Head) {
		_node *Temp = List0->Head;
		_node *Node = new(_node);
		List->Head = Node;
		Node->Value = Temp->Value;
		while (Temp = Temp->Next) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Temp->Value;
		};
		List->Cache = Node;
		List->Index = List0->Length;
		for (Temp = List1->Head; Temp; Temp = Temp->Next) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Temp->Value;
		};
		List->Tail = Node;
	} else if (List1->Head) {
		_node *Temp = List1->Head;
		_node *Node = new(_node);
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		Node->Value = Temp->Value;
		while (Temp = Temp->Next) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Temp->Value;
		};
		List->Tail = Node;
	};
	List->Length = List0->Length + List1->Length;
	UNLOCK(List1);
	UNLOCK(List0);
	//GC_ZEROED: List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	return SUCCESS;
};

#ifdef THREADED
METHOD("push", TYP, T, SKP) {
//@list
//@value
//@...
//:T
// inserts <code>value, ...</code> onto the start <var>list</var> and returns <var>list</var>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	for (int I = 1; I < Count; ++I) {
		_node *Node = new(_node);
		Node->Value = Args[I].Val;
		Node->Next = List->Head;
		if (List->Head) List->Head->Prev = Node; else List->Tail = Node;
		List->Head = Node;
		++List->Length;
		if (List->Array) {++List->Lower; ++List->Upper;};
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	Result->Arg = Args[0];
	return SUCCESS;
};


METHOD("put", TYP, T, SKP) {
//@list
//@value
//@...
//:T
// inserts <code>value, ...</code> onto the end <var>list</var> and returns <var>list</var>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	for (int I = 1; I < Count; ++I) {
		_node *Node = new(_node);
		Node->Value = Args[I].Val;
		Node->Prev = List->Tail;
		if (List->Tail) List->Tail->Next = Node; else List->Head = Node;
		List->Tail = Node;
		++List->Length;
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("pop", TYP, T) {
//@list
//:ANY
// removes and returns the first element of <var>list</var>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	_node *Node = List->Head;
	if (!Node) {
		UNLOCK(List);
		return FAILURE;
	};
	if (List->Head = Node->Next) List->Head->Prev = 0; else List->Tail = 0;
	--List->Length;
	if (List->Array) {
		if (List->Lower > 1) --List->Lower; else ++List->Array;
		--List->Upper;
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	Result->Val = Node->Value;
	return SUCCESS;
};

METHOD("pull", TYP, T) {
//@list
//:ANY
// removes and returns the last element of <var>list</var>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	_node *Node = List->Tail;
	if (!Node) {
		UNLOCK(List);
		return FAILURE;
	};
	if (List->Tail = Node->Prev) List->Tail->Next = 0; else List->Head = 0;
	--List->Length;
	if (List->Array) {
		if (List->Upper > List->Length) --List->Upper;
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	Result->Val = Node->Value;
	return SUCCESS;
};
#endif

void _push(_list *List, Std$Object_t *Value) {
	WRLOCK(List);
	_node *Node = new(_node);
	Node->Value = Value;
	Node->Next = List->Head;
	if (List->Head) List->Head->Prev = Node; else List->Tail = Node;
	List->Head = Node;
	++List->Length;
	if (List->Array) {++List->Lower; ++List->Upper;};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
};

void _put(_list *List, Std$Object_t *Value) {
	WRLOCK(List);
	_node *Node = new(_node);
	Node->Value = Value;
	Node->Prev = List->Tail;
	if (List->Tail) List->Tail->Next = Node; else List->Head = Node;
	List->Tail = Node;
	++List->Length;
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
};

Std$Object_t *_pop(_list *List) {
	WRLOCK(List);
	_node *Node = List->Head;
	if (!Node) {
		UNLOCK(List);
		return 0;
	};
	if (List->Head = Node->Next) List->Head->Prev = 0; else List->Tail = 0;
	--List->Length;
	if (List->Array) {
		if (List->Lower > 1) --List->Lower; else ++List->Array;
		--List->Upper;
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	return Node->Value;
};

Std$Object_t *_pull(_list *List) {
	WRLOCK(List);
	_node *Node = List->Tail;
	if (!Node) {
		UNLOCK(List);
		return 0;
	};
	if (List->Tail = Node->Prev) List->Tail->Next = 0; else List->Head = 0;
	--List->Length;
	if (List->Array) {
		if (List->Upper > List->Length) --List->Upper;
	};
	List->Index = 1; List->Cache = List->Head;
	List->Access = 4;
	UNLOCK(List);
	return Node->Value;
};

static void build_index_array(_list *List) {
	_node **Array = (_node **)Riva$Memory$alloc(sizeof(_node *) * List->Length);
	_node **Address = Array;
	_node *Node = List->Head;
	while (Node) Node = (*Address++ = Node)->Next;
	List->Array = Array;
	List->Lower = 1;
	List->Upper = List->Length;
};

#ifdef THREADED
METHOD("[]", TYP, T, TYP, Std$Integer$SmallT) {
//@list
//@n
//:ANY
// returns an assignable reference to the <var>n</var><sup>th</sup> element of <var>list</var>
// negative indices are taken from the end of the list
// fails if <var>n</var> is outside the range <code>-</code><var>list</var><code>:length</code> .. <var>list</var><code>:length</code>
	_list *List = (_list *)Args[0].Val;
	RDLOCK(List);
	long Index = Std$Integer$get_small(Args[1].Val);
	long Cache = List->Index;
	long Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if ((Index < 1) || (Length < Index)) {UNLOCK(List); return FAILURE;};
	if (Index == 1) {Result->Val = *(Result->Ref = &(List->Head->Value)); UNLOCK(List); return SUCCESS;};
	if (Index == Length) {Result->Val = *(Result->Ref = &(List->Tail->Value)); UNLOCK(List); return SUCCESS;};
	UNLOCK(List);
	WRLOCK(List);
	switch (Index - Cache) {
	case -1: {
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = List->Cache->Prev)->Value);
		UNLOCK(List);
		return SUCCESS;
	};
	case 0: {
		Result->Val = *(Result->Ref = &List->Cache->Value);
		UNLOCK(List);
		return SUCCESS;
	};
	case 1: {
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = List->Cache->Next)->Value);
		UNLOCK(List);
		return SUCCESS;
	};
	};
	if (List->Array && (List->Lower <= Index) && (Index <= List->Upper)) {
		Result->Val = *(Result->Ref = &List->Array[Index - List->Lower]->Value);
		UNLOCK(List);
		return SUCCESS;
	} else if (--List->Access == 0) {
		build_index_array(List);
		Result->Val = *(Result->Ref = &List->Array[Index - 1]->Value);
		UNLOCK(List);
		return SUCCESS;
	} else if (2 * Index < Cache) {
		_node *Node = List->Head;
		long Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = Node)->Value);
		UNLOCK(List);
		return SUCCESS;
	} else if (Index < Cache) {
		_node *Node = List->Cache;
		long Steps = Cache - Index;
		do Node = Node->Prev; while (--Steps);
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = Node)->Value);
		UNLOCK(List);
		return SUCCESS;
	} else if (2 * Index < Cache + Length) {
		_node *Node = List->Cache;
		long Steps = Index - Cache;
		do Node = Node->Next; while (--Steps);
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = Node)->Value);
		UNLOCK(List);
		return SUCCESS;
	} else {
		_node *Node = List->Tail;
		long Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
		List->Index = Index;
		Result->Val = *(Result->Ref = &(List->Cache = Node)->Value);
		UNLOCK(List);
		return SUCCESS;
	};
};
#endif

_node *_find_node(_list *List, long Index) {
	long Cache = List->Index, Length = List->Length;
	if (Index == 1) return List->Head;
	if (Index == Length) return List->Tail;

	switch (Index - Cache) {
	case -1: {List->Index = Index; return (List->Cache = List->Cache->Prev);};
	case 0: {return List->Cache;};
	case 1: {List->Index = Index; return (List->Cache = List->Cache->Next);};
	};
	if (List->Array && (List->Lower <= Index) && (Index <= List->Upper)) {
		return List->Array[Index - List->Lower];
	} else if (--List->Access == 0) {
		build_index_array(List); return List->Array[Index - 1];
	} else if (2 * Index < Cache) {
		_node *Node = List->Head; long Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
		List->Index = Index;
		return (List->Cache = Node);
	} else if (Index < Cache) {
		_node *Node = List->Cache; long Steps = Cache - Index;
		do Node = Node->Prev; while (--Steps);
		List->Index = Index;
		return (List->Cache = Node);
	} else if (2 * Index < Cache + Length) {
		_node *Node = List->Cache; long Steps = Index - Cache;
		do Node = Node->Next; while (--Steps);
		List->Index = Index;
		return (List->Cache = Node);
	} else {
		_node *Node = List->Tail; long Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
		List->Index = Index;
		return (List->Cache = Node);
	};
};

METHOD("[]", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@list
//@m
//@n
//:ANY
// returns the sublist of <var>list</var> from the <var>m</var><sup>th</sup> to the <var>n - 1</var><sup>th</sup> element inclusively
// fails if either <var>m</var> or <var>n</var> is outside the range of the list
// returns an empty list if <code>m &gt; n</code>
	_list *List = (_list *)Args[0].Val;
	RDLOCK(List);
	long Length = List->Length;
	long Index0 = Std$Integer$get_small(Args[1].Val);
	if (Index0 <= 0) Index0 += Length + 1;
	if ((Index0 < 1) || (Length < Index0)) {UNLOCK(List); return FAILURE;};
	long Index1 = Std$Integer$get_small(Args[2].Val);
	if (Index1 <= 0) Index1 += Length + 1;
	if ((Index1 < Index0) || (Length + 1 < Index1)) {UNLOCK(List); return FAILURE;};
	if (Index0 == Index1) {
		UNLOCK(List);
		_list *List2 = new(_list);
		List2->Type = T;
		//GC_ZEROED: List2->Length = 0;
		//GC_ZEROED: List2->Lower = List2->Upper = 0;
		List2->Access = 4;
		INITLOCK(List2);
		Result->Val = (Std$Object_t *)List2;
		return SUCCESS;
	};

	UNLOCK(List);
	WRLOCK(List);

	_node *Head = _find_node(List, Index0);
	_node *Tail = Index1 <= Length ? _find_node(List, Index1) : 0;

	_list *List2 = new(_list);
	List2->Type = T;
	_node *Node = new(_node);
	Node->Value = Head->Value;
	List2->Head = Node;
	List2->Cache = Node;
	List2->Index = 1;
	List2->Length = Index1 - Index0;
	if (Head != Tail) {
		for (_node *Temp = Head->Next; Temp != Tail; Temp = Temp->Next) {
			_node *Prev = Node;
			Node = new(_node);
			(Node->Prev = Prev)->Next = Node;
			Node->Value = Temp->Value;
		};
	};
	UNLOCK(List);
	List2->Tail = Node;
	List2->Access = 4;
	INITLOCK(List2);
	Result->Val = (Std$Object_t *)List2;
	return SUCCESS;
};

METHOD("separate", TYP, T, TYP, Std$Function$T) {
	_list *List1 = (_list *)Args[0].Val;
	Std$Object$t *Separator = Args[1].Val;
	WRLOCK(List1);
	_list *List2 = new(_list);
	List2->Type = T;
	_node **Slot1 = &List1->Head;
	_node **Slot2 = &List2->Head;
	_node *Prev1 = 0;
	_node *Prev2 = 0;
	List1->Index = 1;
	List1->Cache = List1->Head;
	List1->Array = 0;
	for (_node *Node = List1->Head; Node; Node = Node->Next) {
		switch (Std$Function$call(Separator, 1, Result, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			Node->Prev = Prev2;
			Slot2[0] = Node;
			Slot2 = &Node->Next;
			--List1->Length;
			++List2->Length;
			Prev2 = Node;
			break;
		case FAILURE:
			Node->Prev = Prev1;
			Slot1[0] = Node;
			Slot1 = &Node->Next;
			Prev1 = Node;
			break;
		case MESSAGE:
			Node->Prev = Prev1;
			Slot1[0] = Node;
			UNLOCK(List1);
			return MESSAGE;
		}
	}
	List2->Tail = Prev2;
	if (Prev2) Prev2->Next = 0; else List2->Head = 0;
	List1->Tail = Prev1;
	if (Prev1) Prev1->Next = 0; else List1->Head = 0;
	UNLOCK(List1);
	Result->Val = List2;
	return SUCCESS;
};

METHOD("split", TYP, T, TYP, Std$Integer$SmallT) {
//@list
//@from
//:T
// Trims the list to <code>list[1, from - 1]</code> and returns <code>list[from, 0]</code>.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	long Index = Std$Integer$get_small(Args[1].Val);
	long Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if ((Index < 1) || (Length + 1 < Index)) {UNLOCK(List); return FAILURE;};
	_list *List2 = new(_list);
	if (Index == 1) {
		*List2 = *List;
		List->Head = List->Tail = List->Cache = 0;
		List->Index = List->Lower = List->Upper = 0;
		List->Array = 0;
		List->Length = 0;
		List->Access = 4;
	} else {
		List2->Type = T;
		if (List2->Length = Length - Index + 1) {
			_node *Node = _find_node(List, Index);
			List2->Tail = List->Tail;
			(List->Tail = Node->Prev)->Next = 0;
			(List2->Head = Node)->Prev = 0;
			List->Length -= List2->Length;
		};
		//GC_ZEROED: List2->Lower = List2->Upper = 0;
		List2->Access = 4;
		List2->Cache = List2->Head;
		List->Index = List2->Index = 1;
		List->Cache = List->Head;
	};
	UNLOCK(List);
	INITLOCK(List2);
	Result->Val = (Std$Object_t *)List2;
	return SUCCESS;
};

METHOD("split", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@list
//@from
//@to
//:T
// Trims the list to <code>list[1, from - 1] + list[to, 0]</code> and returns <code>list[from, to - 1]</code>.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	long Length = List->Length;
	long Index0 = Std$Integer$get_small(Args[1].Val);
	if (Index0 <= 0) Index0 += Length + 1;
	if ((Index0 < 1) || (Length < Index0)) {UNLOCK(List); return FAILURE;};
	long Index1 = Std$Integer$get_small(Args[2].Val);
	if (Index1 <= 0) Index1 += Length + 1;
	if ((Index1 < Index0) || (Length + 1 < Index1)) {UNLOCK(List); return FAILURE;};
	_list *List2 = new(_list);
	if (Index0 == 1) {
		if (Index1 == Length + 1) {
			*List2 = *List;
			List->Head = List->Tail = List->Cache = 0;
			List->Index = List->Lower = List->Upper = 0;
			List->Array = 0;
			List->Length = 0;
			List->Access = 4;
		} else {
			List2->Type = T;
			if (List2->Length = Index1 - 1) {
				_node *Node = _find_node(List, Index1);
				List2->Head = List->Head;
				(List2->Tail = Node->Prev)->Next = 0;
				(List->Head = Node)->Prev = 0;
				List->Length -= List2->Length;
				List2->Cache = List2->Head;
				List->Index = List2->Index = 1;
				List->Cache = List->Head;
				List->Array = 0;
			};
			//GC_ZEROED: List2->Lower = List2->Upper = 0;
			List2->Access = 4;
		}
	} else if (Index1 == Length + 1) {
		List2->Type = T;
		if (List2->Length = Length - Index0 + 1) {
			_node *Node = _find_node(List, Index0);
			List2->Tail = List->Tail;
			(List->Tail = Node->Prev)->Next = 0;
			(List2->Head = Node)->Prev = 0;
			List->Length -= List2->Length;
			List2->Cache = List2->Head;
			List->Index = List2->Index = 1;
			List->Cache = List->Head;
			List->Array = 0;
		};
		//GC_ZEROED: List2->Lower = List2->Upper = 0;
		List2->Access = 4;
	} else {
		List2->Type = T;
		if (List2->Length = Index1 - Index0) {
			_node *Node0 = _find_node(List, Index0);
			_node *Node1 = _find_node(List, Index1);
			Node0->Prev->Next = Node1;
			List2->Head = Node0;
			(List2->Tail = Node1->Prev)->Next = 0;
			Node1->Prev = Node0->Prev;
			List2->Head->Prev = 0;
			List->Length -= List2->Length;
			List2->Cache = List2->Head;
			List->Index = List2->Index = 1;
			List->Cache = List->Head;
			List->Array = 0;
		};
		//GC_ZEROED: List2->Lower = List2->Upper = 0;
		List2->Access = 4;
	};
	UNLOCK(List);
	INITLOCK(List2);
	Result->Val = (Std$Object_t *)List2;
	return SUCCESS;
};

static void shift_array_right(_list *List, long Lower, long Upper) {
    if (List->Array == 0) return;
    if ((List->Lower > Upper) || (List->Upper < Lower)) return;
    if ((List->Lower > Lower) || (List->Upper < Upper)) {
        List->Access = 4;
        List->Array = 0;
    };
    _node **Array = List->Array + (Lower - List->Lower);
    _node *Temp = Array[Upper - Lower];
    memmove(Array + 1, Array, (Upper - Lower) * sizeof(_node *));
    Array[0] = Temp;
};

static void shift_array_left(_list *List, long Lower, long Upper) {
    if (List->Array == 0) return;
    if ((List->Lower > Upper) || (List->Upper < Lower)) return;
    if ((List->Lower > Lower) || (List->Upper < Upper)) {
        List->Access = 4;
        List->Array = 0;
    };
    _node **Array = List->Array + (Lower - List->Lower);
    _node *Temp = Array[0];
    memmove(Array, Array + 1, (Upper - Lower) * sizeof(_node *));
    Array[Upper - Lower] = Temp;
};

METHOD("shift", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@list
//@n
//@r
//:T
// shifts the <var>n</var><sup>th</sup> element by <var>r</var> positions within <var>list</var>
    _list *List = (_list *)Args[0].Val;
    WRLOCK(List);
    long Index = Std$Integer$get_small(Args[1].Val);
    long Shift = Std$Integer$get_small(Args[2].Val);
    long Length = List->Length;
    if (Index <= 0) Index += Length + 1;
    if ((Index < 1) || (Length < Index)) {UNLOCK(List); return FAILURE;};
    if (Shift < 0) {
        if (Index + Shift <= 1) {
            if (Index > 1) {
                List->Access = 4;
                _node *Node = _find_node(List, Index);
                if (Node->Prev->Next = Node->Next) {
                    Node->Next->Prev = Node->Prev;
                } else {
                    List->Tail = Node->Prev;
                };
                (Node->Next = List->Head)->Prev = Node;
                Node->Prev = 0;
                List->Head = Node;
                //List->Array = 0;
                //List->Access = 4;
                shift_array_right(List, 1, Index);
            };
        } else {
            List->Access = 4;
            _node *Node1 = _find_node(List, Index);
            _node *Node2 = _find_node(List, Index + Shift);
            if (Node1->Prev->Next = Node1->Next) {
                Node1->Next->Prev = Node1->Prev;
            } else {
                List->Tail = Node1->Prev;
            };
            (Node1->Prev = Node2->Prev)->Next = Node1;
            (Node2->Prev = Node1)->Next = Node2;
            //List->Array = 0;
            //List->Access = 4;
            shift_array_right(List, Index + Shift, Index);
        };
    } else if (Shift > 0) {
        if (Index + Shift >= Length) {
            if (Index < Length) {
                List->Access = 4;
                _node *Node = _find_node(List, Index);
                if (Node->Next->Prev = Node->Prev) {
                    Node->Prev->Next = Node->Next;
                } else {
                    List->Head = Node->Next;
                };
                (Node->Prev = List->Tail)->Next = Node;
                Node->Next = 0;
                List->Tail = Node;
                //List->Array = 0;
                //List->Access = 4;
                shift_array_left(List, Index, Length);
            };
        } else {
            List->Access = 4;
            _node *Node1 = _find_node(List, Index);
            _node *Node2 = _find_node(List, Index + Shift);
            if (Node1->Next->Prev = Node1->Prev) {
                Node1->Prev->Next = Node1->Next;
            } else {
                List->Head = Node1->Next;
            };
            (Node1->Next = Node2->Next)->Prev = Node1;
            (Node2->Next = Node1)->Prev = Node2;
            //List->Array = 0;
            //List->Access = 4;
            shift_array_left(List, Index, Index + Shift);
        };
    };
    UNLOCK(List);
    Result->Val = (Std$Object_t *)List;
    return SUCCESS;
};

static Std$Object_t *delete_node(_list *List, _node *Node) {
	(Node->Prev->Next = Node->Next)->Prev = Node->Prev;
	--List->Length;
	List->Index = 1;
	List->Cache = List->Head;
	List->Array = 0;
	List->Access = 4;
	return Node->Value;
};

METHOD("delete", TYP, T, TYP, Std$Integer$SmallT) {
//@list
//@n
//:ANY
// removes the <var>n</var><sup>th</sup> element from <var>list</var>. Returns the removed value.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	long Index = Std$Integer$get_small(Args[1].Val);
	long Cache = List->Index;
	long Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if ((Index < 1) || (Length < Index)) {UNLOCK(List); return FAILURE;};
	if (Index == 1) {
		_node *Node = List->Head;
		if (List->Head = Node->Next) List->Head->Prev = 0; else List->Tail = 0;
		--List->Length;
		if (List->Array) {
			if (List->Lower > 1) --List->Lower; else ++List->Array;
			--List->Upper;
		};
		List->Index = 1; List->Cache = List->Head;
		List->Access = 4;
		UNLOCK(List);
		Result->Val = Node->Value;
		return SUCCESS;
	};
	if (Index == Length) {
		_node *Node = List->Tail;
		if (List->Tail = Node->Prev) List->Tail->Next = 0; else List->Head = 0;
		--List->Length;
		if (List->Array) {
			if (List->Upper > List->Length) --List->Upper;
		};
		List->Index = 1; List->Cache = List->Head;
		List->Access = 4;
		UNLOCK(List);
		Result->Val = Node->Value;
		return SUCCESS;
	};
	switch (Index - Cache) {
	case -1: {
		Result->Val = delete_node(List, List->Cache->Prev);
		UNLOCK(List);
		return SUCCESS;
	};
	case 0: {
		Result->Val = delete_node(List, List->Cache);
		UNLOCK(List);
		return SUCCESS;
	};
	case 1: {
		Result->Val = delete_node(List, List->Cache->Next);
		UNLOCK(List);
		return SUCCESS;
	};
	};
	if (List->Array && (List->Lower <= Index) && (Index <= List->Upper)) {
		_node *Node = List->Array[Index - List->Lower];
		Result->Val = delete_node(List, Node);
		UNLOCK(List);
		return SUCCESS;
	} else if (2 * Index < Cache) {
		_node *Node = List->Head;
		long Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
		Result->Val = delete_node(List, Node);
		UNLOCK(List);
		return SUCCESS;
	} else if (Index < Cache) {
		_node *Node = List->Cache;
		long Steps = Cache - Index;
		do Node = Node->Prev; while (--Steps);
		Result->Val = delete_node(List, Node);
		UNLOCK(List);
		return SUCCESS;
	} else if (2 * Index < Cache + Length) {
		_node *Node = List->Cache;
		long Steps = Index - Cache;
		do Node = Node->Next; while (--Steps);
		Result->Val = delete_node(List, Node);
		UNLOCK(List);
		return SUCCESS;
	} else {
		_node *Node = List->Tail;
		long Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
		Result->Val = delete_node(List, Node);
		UNLOCK(List);
		return SUCCESS;
	};
};

static void insert_nodes(_list *List, _node *Node, int Count, const Std$Function$argument *Args) {
	for (int I = Count; --I >= 0;) {
		_node *New = new(_node);
		(New->Prev = Node->Prev)->Next = New;
		(New->Next = Node)->Prev = New;
		New->Value = Args[I].Val;
		++List->Length;
		Node = New;
	};
	List->Array = 0;
	List->Access = 4;
	List->Index = 1; List->Cache = List->Head;
};

METHOD("insert", TYP, T, TYP, Std$Integer$SmallT, SKP) {
//@list
//@n
//@value...
//:T
// inserts <code>value...</code> into the <var>n</var><sup>th</sup> position in <var>list</var>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	long Index = Std$Integer$get_small(Args[1].Val);
	Std$Object_t *Value = Args[2].Val;
	long Cache = List->Index;
	long Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if ((Index < 1) || (Length + 1 < Index)) {UNLOCK(List); return FAILURE;};
	if (Index == 1) {
		for (int I = 2; I < Count; ++I) {
			_node *Node = new(_node);
			Node->Value = Args[I].Val;
			Node->Next = List->Head;
			if (List->Head) List->Head->Prev = Node; else List->Tail = Node;
			List->Head = Node;
			++List->Length;
			if (List->Array) {++List->Lower; ++List->Upper;};
		};
		List->Index = 1; List->Cache = List->Head;
		List->Access = 4;
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	if (Index == Length + 1) {
		for (int I = 2; I < Count; ++I) {
			_node *Node = new(_node);
			Node->Value = Args[I].Val;
			Node->Prev = List->Tail;
			if (List->Tail) List->Tail->Next = Node; else List->Head = Node;
			List->Tail = Node;
			++List->Length;
		};
		List->Index = 1; List->Cache = List->Head;
		List->Access = 4;
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	if (Index == Length) {
		insert_nodes(List, List->Tail, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	switch (Index - Cache) {
	case -1: {
		insert_nodes(List, List->Cache->Prev, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case 0: {
		insert_nodes(List, List->Cache, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case 1: {
		insert_nodes(List, List->Cache->Next, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	};
	if (List->Array && (List->Lower <= Index) && (Index <= List->Upper)) {
		_node *Node = List->Array[Index - List->Lower];
		insert_nodes(List, Node, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (2 * Index < Cache) {
		_node *Node = List->Head;
		long Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
		insert_nodes(List, Node, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (Index < Cache) {
		_node *Node = List->Cache;
		long Steps = Cache - Index;
		do Node = Node->Prev; while (--Steps);
		insert_nodes(List, Node, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (2 * Index < Cache + Length) {
		_node *Node = List->Cache;
		long Steps = Index - Cache;
		do Node = Node->Next; while (--Steps);
		insert_nodes(List, Node, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		_node *Node = List->Tail;
		long Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
		insert_nodes(List, Node, Count - 2, Args + 2);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
};

static void splice_nodes(_list *List, _node *Next, int Length, _node *Head, _node *Tail) {
	Head->Prev = Next->Prev;
	Next->Prev->Next = Head;
	Tail->Next = Next;
	Next->Prev = Tail;
	List->Array = 0;
	List->Index = 1;
	List->Cache = List->Head;
	List->Access = 4;
	List->Length += Length;
}

METHOD("splice", TYP, T, TYP, Std$Integer$SmallT, TYP, T) {
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	long Index = Std$Integer$get_small(Args[1].Val);
	long Cache = List->Index;
	long Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if ((Index < 1) || (Length + 1 < Index)) {UNLOCK(List); return FAILURE;};
	_list *Source = (_list *)Args[2].Val;
	WRLOCK(Source);
	long SourceLength = Source->Length;
	if (SourceLength == 0) {
		UNLOCK(Source);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	}
	_node *Head = Source->Head;
	_node *Tail = Source->Tail;
	Source->Head = Source->Tail = Source->Cache = 0;
	Source->Index = Source->Lower = Source->Upper = 0;
	Source->Array = 0;
	Source->Length = 0;
	Source->Access = 4;
	UNLOCK(Source);
	if (Index == 1) {
		Tail->Next = List->Head;
		List->Head->Prev = Tail;
		List->Head = Head;
		if (List->Array) {
			List->Lower += Length;
			List->Upper += Length;
		}
		List->Index = 1;
		List->Cache = Head;
		List->Access = 4;
		List->Length += SourceLength;
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	}
	if (Index == Length + 1) {
		Head->Prev = List->Tail;
		List->Tail->Next = Head;
		List->Tail = Tail;
		List->Index = 1;
		List->Cache = Head;
		List->Access = 4;
		List->Length += SourceLength;
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	if (Index == Length) {
		splice_nodes(List, List->Tail, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	switch (Index - Cache) {
	case -1: {
		splice_nodes(List, List->Cache->Prev, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case 0: {
		splice_nodes(List, List->Cache, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case 1: {
		splice_nodes(List, List->Cache->Next, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
	};
	if (List->Array && (List->Lower <= Index) && (Index <= List->Upper)) {
		_node *Node = List->Array[Index - List->Lower];
		splice_nodes(List, Node, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (2 * Index < Cache) {
		_node *Node = List->Head;
		long Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
		splice_nodes(List, Node, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (Index < Cache) {
		_node *Node = List->Cache;
		long Steps = Cache - Index;
		do Node = Node->Prev; while (--Steps);
		splice_nodes(List, Node, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else if (2 * Index < Cache + Length) {
		_node *Node = List->Cache;
		long Steps = Index - Cache;
		do Node = Node->Next; while (--Steps);
		splice_nodes(List, Node, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		_node *Node = List->Tail;
		long Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
		splice_nodes(List, Node, SourceLength, Head, Tail);
		UNLOCK(List);
		Result->Arg = Args[0];
		return SUCCESS;
	};
}

/*
METHOD("length", TYP, T) {
//@list
//:Std$Integer$SmallT
// returns the length of <var>list</var>
	const _list *List = (_list *)Args[0].Val;
	Result->Val = Std$Integer$new_small(List->Length);
	return SUCCESS;
};

METHOD("size", TYP, T) {
//@list
//:Std$Integer$SmallT
// returns the length of <var>list</var>
	const _list *List = (_list *)Args[0].Val;
	Result->Val = Std$Integer$new_small(List->Length);
	return SUCCESS;
};
*/

STRING(LeftBracket, "[");
STRING(RightBracket, "]");
STRING(CommaSpace, ", ");
STRING(LeftRightBracket, "[]");
STRING(ValueString, "<value>");

SYMBOL($AT, "@");

METHOD("@", TYP, T, VAL, Std$String$T) {
	_list *List = (_list *)Args[0].Val;
	const _node *Node = List->Head;
	RDLOCK(List);
	if (Node) {
		Std$Object_t *Final;
		Std$Function_result Buffer;
		if (Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0) < FAILURE) {
			Final = Std$String$add(LeftBracket, Buffer.Val);
		} else {
			Final = Std$String$add(LeftBracket, ValueString);
		};
		while (Node = Node->Next) {
			Final = Std$String$add(Final, CommaSpace);
			if (Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0) < FAILURE) {
				Final = Std$String$add(Final, Buffer.Val);
			} else {
				Final = Std$String$add(Final, ValueString);
			};
		};
		Result->Val = Std$String$add(Final, RightBracket);
		UNLOCK(List);
		return SUCCESS;
	} else {
		Result->Val = LeftRightBracket;
		UNLOCK(List);
		return SUCCESS;
	};
};

METHOD("@", TYP, T, VAL, Std$String$T, TYP, Std$String$T) {
//@list
//@_
//@sep
//:Std$String$T
// converts each element of <var>list</var> to a string and joins them separating elements with <var>sep</var>.
	_list *List = (_list *)Args[0].Val;
	const _node *Node = List->Head;
	Std$Object_t *Sep = Args[2].Val;
	RDLOCK(List);
	if (Node) {
		Std$Object_t *Final;
		Std$Function_result Buffer;
		if (Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0) < FAILURE) {
			Final = Buffer.Val;
		} else {
			Final = ValueString;
		};
		while (Node = Node->Next) {
			Final = Std$String$add(Final, Sep);
			if (Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0) < FAILURE) {
				Final = Std$String$add(Final, Buffer.Val);
			} else {
				Final = Std$String$add(Final, ValueString);
			};
		};
		Result->Val = Final;
		UNLOCK(List);
		return SUCCESS;
	} else {
		Result->Val = Std$String$Empty;
		UNLOCK(List);
		return SUCCESS;
	};
};

SYMBOL($EQUAL, "=");

METHOD("in", ANY, TYP, T) {
//@value
//@list
//:ANY
// Returns <var>value</var> if it is an element in <var>list</var>, fails otherwise.
	int Index = 0;
	for (const _node *Node = ((_list *)Args[1].Val)->Head; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[0].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Arg = Args[0];
			return SUCCESS;
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

typedef struct find_generator {
	Std$Function_cstate State;
	const _node *Current;
	long Index;
	const Std$Object_t *Value;
} find_generator;

typedef struct find_resume_data {
	find_generator *Generator;
	Std$Function_argument Result;
} find_resume_data;

static long resume_find_list(find_resume_data *Data) {
	find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Generator->Value, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				Generator->Current = Node->Next;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("find", TYP, T, ANY) {
//@list
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>list[n] = value</code>, if any.
	int Index = 0;
	for (const _node *Node = ((_list *)Args[0].Val)->Head; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				find_generator *Generator = new(find_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_list;
				Generator->Current = Node->Next;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

static long resume_find_object_list(find_resume_data *Data) {
	find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Next) {
		++Index;
		if (Node->Value == Generator->Value) {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				Generator->Current = Node->Next;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
	};
	return FAILURE;
};

 METHOD("find_object", TYP, T, ANY) {
//@list
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>list[n] = value</code>, if any.
	int Index = 0;
	for (const _node *Node = ((_list *)Args[0].Val)->Head; Node; Node = Node->Next) {
		++Index;
		if (Node->Value == Args[1].Val) {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				find_generator *Generator = new(find_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_object_list;
				Generator->Current = Node->Next;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
	};
	return FAILURE;
};

static long resume_rfind_list(find_resume_data *Data) {
	find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Generator->Value, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("rfind", TYP, T, ANY) {
//@list
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>list[n] = value</code> in reverse order, if any.
	int Index = ((_list *)Args[0].Val)->Length + 1;
	for (const _node *Node = ((_list *)Args[0].Val)->Tail; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				find_generator *Generator = new(find_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_rfind_list;
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

static long resume_rfind_object_list(find_resume_data *Data) {
	find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Prev) {
		--Index;
		if (Node->Value == Generator->Value) {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
	};
	return FAILURE;
};

 METHOD("rfind_object", TYP, T, ANY) {
//@list
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>list[n] = value</code> in reverse order, if any.
	int Index = ((_list *)Args[0].Val)->Length + 1;
	for (const _node *Node = ((_list *)Args[0].Val)->Tail; Node; Node = Node->Prev) {
		--Index;
		if (Node->Value == Args[1].Val) {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				find_generator *Generator = new(find_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_rfind_object_list;
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
	};
	return FAILURE;
};

typedef struct applied_find_generator {
	Std$Function_cstate State;
	const _node *Current;
	long Index;
	size_t NumFunctions;
	const Std$Object_t *Value;
	const Std$Object_t *Functions[];
} applied_find_generator;

typedef struct applied_find_resume_data {
	applied_find_generator *Generator;
	Std$Function_argument Result;
} applied_find_resume_data;

static long resume_applied_find_list(applied_find_resume_data *Data) {
	applied_find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		Result0.Val = Node->Value;
		Result0.Ref = 0;
		for (size_t I = 0; I < Generator->NumFunctions; ++I) {
			switch (Std$Function$call(Generator->Functions[I], 1, &Result0, Result0.Val, Result0.Ref)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				goto failed;
			case MESSAGE:
				Data->Result.Val = Result0.Val;
				return MESSAGE;
			};
		};
		switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, Result0.Ref, Generator->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				Generator->Current = Node->Next;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
		failed: 0;
	};
	return FAILURE;
};

METHOD("find", TYP, T, ANY, ANY) {
//@list
//@value
//@fn&lt;sub&gt;1&lt;/sub&gt;+,&#160;...,&#160;fn&lt;sub&gt;k&lt;/sub&gt;+
//:Std$Integer$SmallT
// Generates all <var>n</var> where <var>fn<sub>k</sub></var><code>( ... </code><var>fn<sub>1</sub></var><code>(list[n]) ... ) = value</code>, if any.
	int Index = 0;
	size_t NumFunctions = Count - 2;
	for (const _node *Node = ((_list *)Args[0].Val)->Head; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		Result0.Val = Node->Value;
		Result0.Ref = 0;
		for (size_t I = 0; I < NumFunctions; ++I) {
			switch (Std$Function$call(Args[2 + I].Val, 1, &Result0, Result0.Val, Result0.Ref)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				goto failed;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			};
		};
		switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, Result0.Ref, Args[1].Val, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				applied_find_generator *Generator = (applied_find_generator *)Riva$Memory$alloc(sizeof(applied_find_generator) + NumFunctions * sizeof(Std$Object$t *));
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_applied_find_list;
				Generator->Current = Node->Next;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Generator->NumFunctions = NumFunctions;
				for (size_t I = 0; I < NumFunctions; ++I) Generator->Functions[I] = Args[I + 2].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
		failed: 0;
	};
	return FAILURE;
};

static long resume_applied_rfind_list(applied_find_resume_data *Data) {
	applied_find_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		Result0.Val = Node->Value;
		Result0.Ref = 0;
		for (size_t I = 0; I < Generator->NumFunctions; ++I) {
			switch (Std$Function$call(Generator->Functions[I], 1, &Result0, Result0.Val, Result0.Ref)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				goto failed;
			case MESSAGE:
				Data->Result.Val = Result0.Val;
				return MESSAGE;
			};
		};
		switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, Result0.Ref, Generator->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
		failed: 0;
	};
	return FAILURE;
};

METHOD("rfind", TYP, T, ANY, ANY) {
//@list
//@value
//@fn&lt;sub&gt;1&lt;/sub&gt;+,&#160;...,&#160;fn&lt;sub&gt;k&lt;/sub&gt;+
//:Std$Integer$SmallT
// Generates all <var>n</var> where <var>fn<sub>k</sub></var><code>( ... </code><var>fn<sub>1</sub></var><code>(list[n]) ... ) = value</code> in reverse order, if any.
	int Index = ((_list *)Args[0].Val)->Length + 1;
	size_t NumFunctions = Count - 2;
	for (const _node *Node = ((_list *)Args[0].Val)->Tail; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		Result0.Val = Node->Value;
		Result0.Ref = 0;
		for (size_t I = 0; I < NumFunctions; ++I) {
			switch (Std$Function$call(Args[2 + I].Val, 1, &Result0, Result0.Val, Result0.Ref)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				goto failed;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			};
		};
		switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, Result0.Ref, Args[1].Val, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				applied_find_generator *Generator = (applied_find_generator *)Riva$Memory$alloc(sizeof(applied_find_generator) + NumFunctions * sizeof(Std$Object$t *));
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_applied_rfind_list;
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				Generator->Value = Args[1].Val;
				Generator->NumFunctions = NumFunctions;
				for (size_t I = 0; I < NumFunctions; ++I) Generator->Functions[I] = Args[I + 2].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
		failed: 0;
	};
	return FAILURE;
};

METHOD("bsearch", TYP, T, ANY) {
	_list *List = (_list *)Args[0].Val;
	Std$Object$t *Value = Args[1].Val;
	Std$Object$t *Compare = Count > 2 ? Args[2].Val : $COMP;
	build_index_array(List);
	_node **Nodes = List->Array;
	
	size_t Min = 0;
	size_t Max = List->Length - 1;
	while (Max >= Min) {
		size_t Mid = (Min + Max) / 2;
		switch (Std$Function$call(Compare, 2, Result, Nodes[Mid]->Value, 0, Value, 0)) {
		case SUSPEND: case SUCCESS: {
			if (Result->Val == Std$Object$Equal) {
				Result->Val = *(Result->Ref = &Nodes[Mid]->Value);
				return SUCCESS;
			} else if (Result->Val == Std$Object$Less) {
				Min = Mid + 1;
				break;
			} else if (Result->Val == Std$Object$Greater) {
				Max = Mid - 1;
				break;
			};
		};
		case FAILURE: Result->Val = Std$String$new("Comparison failed");
		case MESSAGE: return MESSAGE;
		};
	};
	return FAILURE;
};

typedef struct where_generator {
	Std$Function_cstate State;
	const _node *Current;
	long Index;
	const Std$Object_t *Test;
} where_generator;

typedef struct where_resume_data {
	where_generator *Generator;
	Std$Function_argument Result;
} where_resume_data;

static long resume_where_list(where_resume_data *Data) {
	where_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Generator->Test, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Next) {
				Generator->Current = Node->Next;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("where", TYP, T, TYP, Std$Function$T) {
//@list
//@predicate
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>predicate(list[n])</code> succeeds, if any.
	int Index = 0;
	_list *List = (_list *)Args[0].Val;
	for (const _node *Node = List->Head; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Args[1].Val, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Result->Val = Std$Integer$new_small(Index);
			List->Index = Index;
			List->Cache = Node;
			if (Node->Next) {
				where_generator *Generator = new(where_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_where_list;
				Generator->Current = Node->Next;
				Generator->Index = Index;
				Generator->Test = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

static long resume_rwhere_list(where_resume_data *Data) {
	where_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Generator->Test, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			Data->Result.Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("rwhere", TYP, T, TYP, Std$Function$T) {
//@list
//@predicate
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>predicate(list[n])</code> succeeds in reverse order, if any.
	_list *List = (_list *)Args[0].Val;
	int Index = List->Length + 1;
	for (const _node *Node = List->Tail; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Args[1].Val, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			List->Index = Index;
			List->Cache = Node;
			Result->Val = Std$Integer$new_small(Index);
			if (Node->Prev) {
				where_generator *Generator = new(where_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_rwhere_list;
				Generator->Current = Node->Prev;
				Generator->Index = Index;
				Generator->Test = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

typedef struct where_value_generator {
	Std$Function_cstate State;
	const _node *Current;
	long Index;
	const Std$Object_t *Test, *Value;
} where_value_generator;

typedef struct where_value_resume_data {
	where_value_generator *Generator;
	Std$Function_argument Result;
} where_value_resume_data;

static long resume_where_value_list(where_value_resume_data *Data) {
	where_value_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Generator->Test, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, 0, Generator->Value, 0)) {
			case SUSPEND: case SUCCESS: {
				Data->Result.Val = Std$Integer$new_small(Index);
				if (Node->Next) {
					Generator->Current = Node->Next;
					Generator->Index = Index;
					return SUSPEND;
				} else {
					return SUCCESS;
				};
			};
			case FAILURE: continue;
			case MESSAGE: {
				Data->Result.Val = Result0.Val;
				return MESSAGE;
			};
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("where", TYP, T, TYP, Std$Function$T, ANY) {
//@list
//@predicate
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>predicate(list[n]) = value</code>, if any.
	int Index = 0;
	_list *List = (_list *)Args[0].Val;
	for (const _node *Node = List->Head; Node; Node = Node->Next) {
		++Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Args[1].Val, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, 0, Args[2].Val, 0)) {
			case SUSPEND: case SUCCESS: {
				Result->Val = Std$Integer$new_small(Index);
				List->Index = Index;
				List->Cache = Node;
				if (Node->Next) {
					where_value_generator *Generator = new(where_value_generator);
					Generator->State.Run = Std$Function$resume_c;
					Generator->State.Invoke = (Std$Function_cresumefn)resume_where_value_list;
					Generator->Current = Node->Next;
					Generator->Index = Index;
					Generator->Test = Args[1].Val;
					Generator->Value = Args[2].Val;
					Result->State = Generator;
					return SUSPEND;
				} else {
					return SUCCESS;
				};
			};
			case FAILURE: continue;
			case MESSAGE: {
				Result->Val = Result0.Val;
				return MESSAGE;
			};
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

static long resume_rwhere_value_list(where_resume_data *Data) {
	where_value_generator *Generator = Data->Generator;
	const _node *Node = Generator->Current;
	int Index = Generator->Index;
	for (const _node *Node = Generator->Current; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Generator->Test, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, 0, Generator->Value, 0)) {
			case SUSPEND: case SUCCESS: {
				Data->Result.Val = Std$Integer$new_small(Index);
				if (Node->Prev) {
					Generator->Current = Node->Prev;
					Generator->Index = Index;
					return SUSPEND;
				} else {
					return SUCCESS;
				};
			};
			case FAILURE: continue;
			case MESSAGE: {
				Data->Result.Val = Result0.Val;
				return MESSAGE;
			};
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("rwhere", TYP, T, TYP, Std$Function$T, ANY) {
//@list
//@predicate
//@value
//:Std$Integer$SmallT
// Generates all <var>n</var> where <code>predicate(list[n]) = value</code> in reverse order, if any.
	_list *List = (_list *)Args[0].Val;
	int Index = List->Length + 1;
	for (const _node *Node = List->Tail; Node; Node = Node->Prev) {
		--Index;
		Std$Function_result Result0;
		switch (Std$Function$call(Args[1].Val, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			switch (Std$Function$call($EQUAL, 2, &Result0, Result0.Val, 0, Args[2].Val, 0)) {
			case SUSPEND: case SUCCESS: {
				List->Index = Index;
				List->Cache = Node;
				Result->Val = Std$Integer$new_small(Index);
				if (Node->Prev) {
					where_value_generator *Generator = new(where_value_generator);
					Generator->State.Run = Std$Function$resume_c;
					Generator->State.Invoke = (Std$Function_cresumefn)resume_rwhere_value_list;
					Generator->Current = Node->Prev;
					Generator->Index = Index;
					Generator->Test = Args[1].Val;
					Generator->Value = Args[2].Val;
					Result->State = Generator;
					return SUSPEND;
				} else {
					return SUCCESS;
				};
			};
			case FAILURE: continue;
			case MESSAGE: {
				Result->Val = Result0.Val;
			};
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	return FAILURE;
};

METHOD("remove", TYP, T, ANY) {
//@list
//@value
//:ANY
// Removes the first occurance of <var>value</var> from <var>list</var>.
// Returns <var>value</var> if <var>value</var> was present, fails otherwise.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	for (_node *Node = List->Head; Node; Node = Node->Next) {
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			if (Node->Next) {
				if (Node->Prev) {
					Result->Val = delete_node(List, Node);
				} else {
					_node *Node = List->Head;
					List->Head = Node->Next;
					List->Head->Prev = 0;
					--List->Length;
					if (List->Array) {
						if (List->Lower > 1) --List->Lower; else ++List->Array;
						--List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			} else {
				if (Node->Prev) {
					_node *Node = List->Tail;
					List->Tail = Node->Prev;
					List->Tail->Next = 0;
					--List->Length;
					if (List->Array) {
						if (List->Upper > List->Length) --List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				} else {
					List->Head = List->Tail = List->Cache = 0;
					List->Index = List->Lower = List->Upper = 0;
					List->Array = 0;
					List->Length = 0;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			};
			UNLOCK(List);
			return SUCCESS;
		};
		case FAILURE: continue;
		case MESSAGE: {
			UNLOCK(List);
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	UNLOCK(List);
	return FAILURE;
};

METHOD("remove", TYP, T, ANY, ANY) {
//@list
//@value
//:ANY
// Removes the first occurance where <var>fn<sub>k</sub></var><code>( ... </code><var>fn<sub>1</sub></var><code>(list[n]) ... ) = value</code> from <var>list</var>.
// Returns <var>value</var> if <var>value</var> was removed, fails otherwise.
	_list *List = (_list *)Args[0].Val;
	size_t NumFunctions = Count - 2;
	WRLOCK(List);
	for (_node *Node = List->Head; Node; Node = Node->Next) {
		Std$Function_result Result0;
		Result0.Val = Node->Value;
		Result0.Ref = 0;
		for (size_t I = 0; I < NumFunctions; ++I) {
			switch (Std$Function$call(Args[2 + I].Val, 1, &Result0, Result0.Val, Result0.Ref)) {
			case SUSPEND: case SUCCESS:
				break;
			case FAILURE:
				goto failed;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			};
		};
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			if (Node->Next) {
				if (Node->Prev) {
					Result->Val = delete_node(List, Node);
				} else {
					(List->Head = Node->Next)->Prev = 0;
					--List->Length;
					if (List->Array) {
						if (List->Lower > 1) --List->Lower; else ++List->Array;
						--List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			} else {
				if (Node->Prev) {
					(List->Tail = Node->Prev)->Next = 0;
					--List->Length;
					if (List->Array) {
						if (List->Upper > List->Length) --List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				} else {
					List->Head = List->Tail = List->Cache = 0;
					List->Index = List->Lower = List->Upper = 0;
					List->Array = 0;
					List->Length = 0;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			};
			UNLOCK(List);
			return SUCCESS;
		};
		case FAILURE: continue;
		case MESSAGE: {
			UNLOCK(List);
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
		failed: 0;
	};
	UNLOCK(List);
	return FAILURE;
};

typedef struct filter_generator {
	Std$Function_cstate State;
	_list *List;
	_node *Current;
	const Std$Object_t *Function;
} filter_generator;

typedef struct filter_resume_data {
	filter_generator *Generator;
	Std$Function_argument Result;
} filter_resume_data;

static long resume_filter_list(filter_resume_data *Data) {
	filter_generator *Generator = Data->Generator;
	_list *List = Generator->List;
	WRLOCK(List);
	for (_node *Node = Generator->Current; Node; Node = Node->Next) {
		Std$Function_result Result0;
		switch (Std$Function$call(Generator->Function, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			if (Node->Next) {
				if (Node->Prev) {
					Data->Result.Val = delete_node(List, Node);
				} else {
					(List->Head = Node->Next)->Prev = 0;
					--List->Length;
					if (List->Array) {
						if (List->Lower > 1) --List->Lower; else ++List->Array;
					--List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Data->Result.Val = Node->Value;
				};
			} else {
				if (Node->Prev) {
					(List->Tail = Node->Prev)->Next = 0;
					--List->Length;
					if (List->Array) {
						if (List->Upper > List->Length) --List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Data->Result.Val = Node->Value;
				} else {
					List->Head = List->Tail = List->Cache = 0;
					List->Index = List->Lower = List->Upper = 0;
					List->Array = 0;
					List->Length = 0;
					List->Access = 4;
					Data->Result.Val = Node->Value;
				};
			};
			UNLOCK(List);
			if (Node->Next) {
				Generator->Current = Node->Next;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			UNLOCK(List);
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	UNLOCK(List);
	return FAILURE;
};

METHOD("filter", TYP, T, TYP, Std$Function$T) {
//@list
//@func
//:ANY
// Removes each <var>value</var> from <var>list</var> for which <code>func(value)</code> succeeds, generating each value in turn.
// Returns <var>value</var> if such a <var>value</var> was found, fails otherwise.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	for (_node *Node = List->Head; Node; Node = Node->Next) {
		Std$Function_result Result0;
		switch (Std$Function$call(Args[1].Val, 1, &Result0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS: {
			if (Node->Next) {
				if (Node->Prev) {
					Result->Val = delete_node(List, Node);
				} else {
					(List->Head = Node->Next)->Prev = 0;
					--List->Length;
					if (List->Array) {
						if (List->Lower > 1) --List->Lower; else ++List->Array;
					--List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			} else {
				if (Node->Prev) {
					(List->Tail = Node->Prev)->Next = 0;
					--List->Length;
					if (List->Array) {
						if (List->Upper > List->Length) --List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				} else {
					List->Head = List->Tail = List->Cache = 0;
					List->Index = List->Lower = List->Upper = 0;
					List->Array = 0;
					List->Length = 0;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			};
			UNLOCK(List);
			if (Node->Next) {
				filter_generator *Generator = new(filter_generator);
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_filter_list;
				Generator->List = List;
				Generator->Current = Node->Next;
				Generator->Function = Args[1].Val;
				Result->State = Generator;
				return SUSPEND;
			} else {
				return SUCCESS;
			};
		};
		case FAILURE: continue;
		case MESSAGE: {
			UNLOCK(List);
			Result->Val = Result0.Val;
			return MESSAGE;
		};
		};
	};
	UNLOCK(List);
	return FAILURE;
};

 METHOD("remove_object", TYP, T, ANY) {
//@list
//@value
//:ANY
// Removes the first occurance of <var>value</var> from <var>list</var>.
// Returns <var>value</var> if <var>value</var> was present, fails otherwise.
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	for (_node *Node = List->Head; Node; Node = Node->Next) {
		if (Node->Value == Args[1].Val) {
			if (Node->Next) {
				if (Node->Prev) {
					Result->Val = delete_node(List, Node);
				} else {
					(List->Head = Node->Next)->Prev = 0;
					--List->Length;
					if (List->Array) {
						if (List->Lower > 1) --List->Lower; else ++List->Array;
						--List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			} else {
				if (Node->Prev) {
					(List->Tail = Node->Prev)->Next = 0;
					--List->Length;
					if (List->Array) {
						if (List->Upper > List->Length) --List->Upper;
					};
					List->Index = 1; List->Cache = List->Head;
					List->Access = 4;
					Result->Val = Node->Value;
				} else {
					List->Head = List->Tail = List->Cache = 0;
					List->Index = List->Lower = List->Upper = 0;
					List->Array = 0;
					List->Length = 0;
					List->Access = 4;
					Result->Val = Node->Value;
				};
			};
			UNLOCK(List);
			return SUCCESS;
		};
	};
	UNLOCK(List);
	return FAILURE;
};

SYMBOL($to, "to");

/*
METHOD("keys", TYP, T) {
//@list
//:Std$Integer$SmallT
// Equivalent to <code>1:to(list:length)</code>.
	Std$Integer_smallt To = {Std$Integer$SmallT, ((_list *)Args[0].Val)->Length};
	return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &To, 0);
};

typedef struct list_generator {
	Std$Function_cstate State;
	_node *Current;
} list_generator;

typedef struct list_resume_data {
	list_generator *Generator;
	Std$Function_argument Result;
} list_resume_data;

static long resume_values_list(list_resume_data *Data) {
	_node *Node = Data->Generator->Current->Next;
	if (Node != 0) {
		Data->Generator->Current = Node;
		Data->Result.Val = *(Data->Result.Ref = &Node->Value);
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("values", TYP, T) {
//@list
//:ANY
// Generates the values in <var>list</var>.
	_node *Node = ((_list *)Args[0].Val)->Head;
	if (Node != 0) {
		list_generator *Generator = new(list_generator);
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function_cresumefn)resume_values_list;
		Generator->Current = Node;
		Result->Val = *(Result->Ref = &Node->Value);
		Result->State = Generator;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};
*/

typedef struct list_loop_generator {
	Std$Function_cstate State;
	Std$Object_t **Key, **Value;
	size_t Index;
	_node *Current;
} list_loop_generator;

typedef struct list_loop_resume_data {
	list_loop_generator *Generator;
	Std$Function$argument Result;
} list_loop_resume_data;

static Std$Function$status resume_list_loop(list_loop_resume_data *Data) {
	list_loop_generator *Generator = Data->Generator;
	_node *Current = Generator->Current;
	Generator->Key[0] = Std$Integer$new_small(Generator->Index);
	Generator->Value[0] = Current->Value;
	Current = Current->Next;
	if (Current == 0) return SUCCESS;
	++Generator->Index;
	Generator->Current = Current;
	return SUSPEND;
};

METHOD("loop", TYP, T, ANY, ANY) {
	_node *Current = ((_list *)Args[0].Val)->Head;
	if (Current == 0) return FAILURE;
	Args[1].Ref[0] = Std$Integer$new_small(1);
	Args[2].Ref[0] = Current->Value;
	Current = Current->Next;
	if (Current == 0) return SUCCESS;
	list_loop_generator *Generator = new(list_loop_generator);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = (Std$Function_cresumefn)resume_list_loop;
	Generator->Index = 2;
	Generator->Current = Current;
	Generator->Key = Args[1].Ref;
	Generator->Value = Args[2].Ref;
	Result->Arg = Args[0];
	Result->State = Generator;
	return SUSPEND;
};

typedef struct list_fill_generator {
	Std$Function_cstate State;
	unsigned long NoOfRefs;
	_node *Current;
	Std$Object_t **Refs[];
} list_fill_generator;

typedef struct list_fill_resume_data {
	list_fill_generator *Generator;
	Std$Function_argument Result;
} list_fill_resume_data;

static long resume_fill_list(list_fill_resume_data *Data) {
	list_fill_generator *Gen = Data->Generator;
	_node *Current = Gen->Current;
	for (unsigned long I = 0; I < Gen->NoOfRefs; ++I) {
		if (Current == 0) return FAILURE;
		Gen->Refs[I][0] = Current->Value;
		Current = Current->Next;
	};
	if (Current == 0) return SUCCESS;
	Gen->Current = Current;
	return SUSPEND;
};

METHOD("fill", TYP, T, ANY) {
//@list
//@var&lt;sub&gt;1&lt;/sub&gt;+,&#160;...,&#160;var&lt;sub&gt;k&lt;/sub&gt;+
// Iterates through <var>list</var> assigning values to <var>var<sub>1</sub></var>, ..., <var>var<sub>k</sub></var>, <var>k</var> values at a time,
	_node *Current = ((_list *)Args[0].Val)->Head;
	for (unsigned long I = 0; I < Count - 1; ++I) {
		if (Current == 0) return FAILURE;
		Args[I + 1].Ref[0] = Current->Value;
		Current = Current->Next;
	};
	if (Current == 0) return SUCCESS;
	list_fill_generator *Generator = (list_fill_generator *)Riva$Memory$alloc(sizeof(list_fill_generator) + (Count - 1) * sizeof(Std$Object_t **));
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = (Std$Function_cresumefn)resume_fill_list;
	Generator->Current = Current;
	Generator->NoOfRefs = Count - 1;
	for (unsigned long I = 0; I < Generator->NoOfRefs; ++I) {
		Generator->Refs[I] = Args[I + 1].Ref;
	};
	Result->Arg = Args[0];
	Result->State = Generator;
	return SUSPEND;
};

METHOD("apply", TYP, T, TYP, Std$Function$T) {
//@list
//@func
//:ANY
// Returns the result of calling <var>func</var> with the elements in <var>list</var> as arguments.
// References to the elements of <var>list</var> are passed so if <var>func</var> expects reference parameters, it will modify <var>list</var>.
	const _list *List = (_list *)Args[0].Val;
	int Count0 = List->Length;
	Std$Function_argument *Args0 = (Std$Function_argument *)Riva$Memory$alloc(Count0 * sizeof(Std$Function_argument));
	_node *Node = List->Head;
	Std$Function_argument *Cur = Args0;
	for (; Node; Node = Node->Next, ++Cur) Cur->Val = *(Cur->Ref = &Node->Value);
	return Std$Function$invoke(Args[1].Val, Count0, Result, Args0);
};

GLOBAL_FUNCTION(Apply, 2) {
//@list
//@func
//:ANY
// Returns the result of calling <var>func</var> with the elements in <var>list</var> as arguments.
// References to the elements of <var>list</var> are passed so if <var>func</var> expects reference parameters, it will modify <var>list</var>.
	CHECK_EXACT_ARG_TYPE(0, T);
	const _list *List = (_list *)Args[0].Val;
	int Count0 = List->Length;
	Std$Function_argument *Args0 = (Std$Function_argument *)Riva$Memory$alloc(Count0 * sizeof(Std$Function_argument));
	_node *Node = List->Head;
	Std$Function_argument *Cur = Args0;
	for (; Node; Node = Node->Next, ++Cur) Cur->Val = *(Cur->Ref = &Node->Value);
	return Std$Function$invoke(Args[1].Val, Count0, Result, Args0);
};

METHOD("apply", TYP, Std$Function$T, TYP, T) {
//@func
//@list
//:ANY
// Returns the result of calling <var>func</var> with the elements in <var>list</var> as arguments.
// References to the elements of <var>list</var> are passed so if <var>func</var> expects reference parameters, it will modify <var>list</var>.
	const _list *List = (_list *)Args[1].Val;
	int Count0 = List->Length;
	Std$Function_argument *Args0 = (Std$Function_argument *)Riva$Memory$alloc(Count0 * sizeof(Std$Function_argument));
	_node *Node = List->Head;
	Std$Function_argument* Cur = Args0;
	for (; Node; Node = Node->Next, ++Cur) Cur->Val = *(Cur->Ref = &Node->Value);
	return Std$Function$invoke(Args[0].Val, Count0, Result, Args0);
};

METHOD("map", TYP, T, TYP, Std$Function$T) {
//@list
//@func
//:T
// Returns the list or results obtained by calling <var>func</var> with each element in <var>list</var>.
// Elements for which <code>func(value)</code> fail will not add any result to the list.
	Std$Object_t *Function = Args[1].Val;
	const _list *In = (_list *)Args[0].Val;
	_list *Out = new(_list);
	Out->Type = T;
	_node *Node = 0;
	long Length = 0;
	for (_node *Arg = In->Head; Arg; Arg = Arg->Next) {
		Std$Function_result Result;
		if (Std$Function$call(Function, 1, &Result, Arg->Value, 0) <= SUCCESS) {
			if (Node) {
				_node *Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
			} else {
				Node = new(_node);
				Out->Head = Out->Cache = Node;
				Out->Index = 1;
			};
			++Length;
			Node->Value = Result.Val;
		};
	};
	Out->Tail = Node;
	Out->Length = Length;
	Out->Lower = Out->Upper = 0;
	Out->Access = 4;
	Result->Val = (Std$Object_t *)Out;
	return SUCCESS;
};

METHOD("map", TYP, Std$Function$T, TYP, T) {
//@func
//@list
//:T
// Returns the list or results obtained by calling <var>func</var> with each element in <var>list</var>.
// Elements for which <code>func(value)</code> fail will not add any result to the list.
	Std$Object_t *Function = Args[0].Val;
	_list *Out = new(_list);
	Out->Type = T;
	_node *Node = 0;
	long Length = 0;
	long Count0 = Count - 1;
	Std$Function_argument *Args0 = (Std$Function_argument *)Riva$Memory$alloc(Count0 * sizeof(Std$Function_argument));
	_node **Nodes = (_node **)Riva$Memory$alloc(Count0 * sizeof(_node *));
	for (int I = 0; I < Count0; ++I) {
		if ((Nodes[I] = ((_list *)Args[I + 1].Val)->Head) == 0) goto finished;
		Args0[I].Val = *(Args0[I].Ref = &Nodes[I]->Value);
	};
	for (;;) {
		Std$Function_result Result;
		if (Std$Function$invoke(Function, Count0, &Result, Args0) <= SUCCESS) {
			if (Node) {
				_node *Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
			} else {
				Node = new(_node);
				Out->Head = Out->Cache = Node;
				Out->Index = 1;
			};
			++Length;
			Node->Value = Result.Val;
		};
		for (int I = 0; I < Count0; ++I) {
			if ((Nodes[I] = Nodes[I]->Next) == 0) goto finished;
			Args0[I].Val = *(Args0[I].Ref = &Nodes[I]->Value);
		};
	};
finished:
	Out->Tail = Node;
	Out->Length = Length;
	Out->Lower = Out->Upper = 0;
	Out->Access = 4;
	Result->Val = (Std$Object_t *)Out;
	return SUCCESS;
};

METHOD("foldl", TYP, Std$Function$T, TYP, T) {
	Std$Object_t *Function = Args[0].Val;
	const _list *List = (_list *)Args[1].Val;
	_node *Node = List->Head;
	if (Node == 0) return FAILURE;
	Result->Val = Node->Value;
	while (Node = Node->Next) {
		int Status = Std$Function$call(Function, 2, Result, Result->Val, 0, Node->Value, 0);
		if (Status > SUCCESS) return Status;
	};
	return SUCCESS;
};

METHOD("foldr", TYP, Std$Function$T, TYP, T) {
	Std$Object_t *Function = Args[0].Val;
	const _list *List = (_list *)Args[1].Val;
	_node *Node = List->Head;
	if (Node == 0) return FAILURE;
	Result->Val = Node->Value;
	while (Node = Node->Next) {
		int Status = Std$Function$call(Function, 2, Result, Node->Value, 0, Result->Val, 0);
		if (Status > SUCCESS) return Status;
	};
	return SUCCESS;
};

METHOD("foldl", TYP, T, TYP, Std$Function$T) {
	Std$Object_t *Function = Args[1].Val;
	const _list *List = (_list *)Args[0].Val;
	_node *Node = List->Head;
	if (Node == 0) return FAILURE;
	Result->Val = Node->Value;
	while (Node = Node->Next) {
		int Status = Std$Function$call(Function, 2, Result, Result->Val, 0, Node->Value, 0);
		if (Status > SUCCESS) return Status;
	};
	return SUCCESS;
};

METHOD("foldr", TYP, T, TYP, Std$Function$T) {
	Std$Object_t *Function = Args[1].Val;
	const _list *List = (_list *)Args[0].Val;
	_node *Node = List->Head;
	if (Node == 0) return FAILURE;
	Result->Val = Node->Value;
	while (Node = Node->Next) {
		int Status = Std$Function$call(Function, 2, Result, Node->Value, 0, Result->Val, 0);
		if (Status > SUCCESS) return Status;
	};
	return SUCCESS;
};

SYMBOL($LESS, "<");

METHOD("=", TYP, T, TYP, T, TYP, Agg$ObjectTable$T) {
	Agg$ObjectTable_t *Cache = (Agg$ObjectTable_t *)Args[2].Val;
	Std$Object_t *Prior = Agg$ObjectTable$get(Cache, Args[0].Val);
	if (Prior != (void *)0xFFFFFFFF) {
		if (Prior == Args[1].Val) {
			Result->Arg = Args[1];
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
	const _list *A = (_list *)Args[0].Val;
	const _list *B = (_list *)Args[1].Val;
	Agg$ObjectTable$put(Cache, A, B);
	if (A->Length != B->Length) return FAILURE;
	if (A->Length == 0) {
		Agg$ObjectTable$put(Cache, A, B);
		Result->Arg = Args[1];
		return SUCCESS;
	};
	const _node *NodeA = A->Head;
	const _node *NodeB = B->Head;
	for (int I = A->Length; --I >= 0;) {
		switch (Std$Function$call($EQUAL, 3, Result, NodeA->Value, 0, NodeB->Value, 0, Cache, 0)) {
		case SUSPEND:
		case SUCCESS:
			break;
		case FAILURE:
			return FAILURE;
		case MESSAGE:
			return MESSAGE;
		};
		NodeA = NodeA->Next;
		NodeB = NodeB->Next;
	};
	Result->Arg = Args[1];
	return SUCCESS;
};

METHOD("[]=", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, T) {
//@list
//@m
//@n
//@list2
//:T
// compares the sublist of <var>list</var> from the <var>m</var><sup>th</sup> to the <var>n - 1</var><sup>th</sup> element inclusively to <var>list2</var>
// fails if either <var>m</var> or <var>n</var> is outside the range of the list
// returns an empty list if <code>m &gt; n</code>
	_list *List = (_list *)Args[0].Val;
	RDLOCK(List);
	long Length = List->Length;
	long Index0 = Std$Integer$get_small(Args[1].Val);
	if (Index0 <= 0) Index0 += Length + 1;
	if ((Index0 < 1) || (Length < Index0)) {UNLOCK(List); return FAILURE;};
	long Index1 = Std$Integer$get_small(Args[2].Val);
	if (Index1 <= 0) Index1 += Length + 1;
	--Index1;
	if ((Index1 < 0) || (Length < Index1)) {UNLOCK(List); return FAILURE;};
	_list *List2 = (_list *)Args[3].Val;
	if (Index0 > Index1) {
		UNLOCK(List);
		if (List2->Length == 0) {
			Result->Arg = Args[3];
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
	Length = Index1 - Index0 + 1;
	if (Length != List2->Length) return FAILURE;
	_node *NodeA = _find_node(List, Index0);
	_node *NodeB = List2->Head;
	Agg$ObjectTable_t Cache[1] = {Agg$ObjectTable$INIT};
	for (int I = Length; --I >= 0;) {
		switch (Std$Function$call($EQUAL, 3, Result, NodeA->Value, 0, NodeB->Value, 0, Cache, 0)) {
		case SUSPEND:
		case SUCCESS:
			break;
		case FAILURE:
			return FAILURE;
		case MESSAGE:
			return MESSAGE;
		};
		NodeA = NodeA->Next;
		NodeB = NodeB->Next;
	};
	UNLOCK(List);
	Result->Arg = Args[3];
	return SUCCESS;
};

/*
static Std$Object_t *sort_list(Std$Object_t **First, Std$Object_t **Last) {
	if (First == Last) return 0;
	Std$Object_t **A = First;
	Std$Object_t **B = Last;
	Std$Object_t *S = *A;
	Std$Object_t *T = *B;

	while (A != B) {
		Std$Function_result Result;
		switch (Std$Function$call($LESS, 2, &Result, S, 0, T, 0)) {
		case SUSPEND: case SUCCESS: {
			*B = T; --B; T = *B;
			break;
		};
		case FAILURE: {
			*A = T; ++A; T = *A;
			break;
		};
		case MESSAGE: {
			return Result.Val;
		};
		};
	};
	*A = S;
	if (A != First) {
		Std$Object_t *Error = sort_list(First, A - 1);
		if (Error) return Error;
	};
	if (B != Last) {
		Std$Object_t *Error = sort_list(B + 1, Last);
		if (Error) return Error;
	};
	return 0;
};

METHOD("sort", TYP, T) {
//@list
//:T
// Sorts <var>list</var> in place using <code>:"&lt;"</code> to order the elements.
	_list *List = (_list *)Args[0].Val;
	Result->Arg = Args[0];
	if (List->Length == 0) return SUCCESS;
	if (List->Length < 1024) {
		Std$Object_t *First[List->Length];
		Std$Object_t **Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) *(++Last) = Node->Value;
		Std$Object_t *Error = sort_list(First, Last);
		if (Error) {
			Result->Val = Error;
			return MESSAGE;
		};
		Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) Node->Value = *(++Last);
		return SUCCESS;
	} else {
		Std$Object_t **First = Riva$Memory$alloc(List->Length * sizeof(Std$Object_t *));;
		Std$Object_t **Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) *(++Last) = Node->Value;
		Std$Object_t *Error = sort_list(First, Last);
		if (Error) {
			Result->Val = Error;
			return MESSAGE;
		};
		Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) Node->Value = *(++Last);
		return SUCCESS;
	};
};

static Std$Object_t *sort_list_f(Std$Object_t **First, Std$Object_t **Last, Std$Object_t *Compare) {
	if (First == Last) return 0;
	Std$Object_t **A = First;
	Std$Object_t **B = Last;
	Std$Object_t *S = *A;
	Std$Object_t *T = *B;

	while (A != B) {
		Std$Function_result Result;
		switch (Std$Function$call(Compare, 2, &Result, S, 0, T, 0)) {
		case SUSPEND: case SUCCESS:
                    *B = T; --B; T = *B;
                    break;
                case FAILURE:
                    *A = T; ++A; T = *A;
                    break;
		case MESSAGE:
                    return Result.Val;
		};
	};
	*A = S;
	if (A != First) {
		Std$Object_t *Error = sort_list_f(First, A - 1, Compare);
		if (Error) return Error;
	};
	if (B != Last) {
		Std$Object_t *Error = sort_list_f(B + 1, Last, Compare);
		if (Error) return Error;
	};
	return 0;
};

METHOD("sort", TYP, T, TYP, Std$Function$T) {
//@list
//@comp
//:T
// Sorts <var>list</var> in place with the ordering given by <code>a &lt; b</code> if <code>comp(a, b)</code> succeeds.
	_list *List = (_list *)Args[0].Val;
	Result->Arg = Args[0];
	if (List->Length == 0) return SUCCESS;
	if (List->Length < 1024) {
		Std$Object_t *First[List->Length];
		Std$Object_t **Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) *(++Last) = Node->Value;
		Std$Object_t *Error = sort_list_f(First, Last, Args[1].Val);
		if (Error) {
			Result->Val = Error;
			return MESSAGE;
		};
		Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) Node->Value = *(++Last);
		return SUCCESS;
	} else {
		Std$Object_t **First = Riva$Memory$alloc(List->Length * sizeof(Std$Object_t *));;
		Std$Object_t **Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) *(++Last) = Node->Value;
		Std$Object_t *Error = sort_list_f(First, Last, Args[1].Val);
		if (Error) {
			Result->Val = Error;
			return MESSAGE;
		};
		Last = First - 1;
		for (_node *Node = List->Head; Node; Node = Node->Next) Node->Value = *(++Last);
		return SUCCESS;
	};
};
*/

METHOD("sort2", TYP, T) {
//@list
//@comp
//:T
// Code based on public domain code by Philip J. Erdelsky, pje@acm.org
// http://www.alumni.caltech.edu/~pje/
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	if (List->Length <= 1) {
		UNLOCK(List);
		Result->Val = (Std$Object_t *)List;
		return SUCCESS;
	};

	struct _tape {
		_node *Head, *Tail;
		unsigned long Count;
	} Tape[4];

	unsigned int Base;
	unsigned long BlockSize;

	/* Distribute the records alternately to tape[0] and tape[1]. */
	_node *Node = List->Head;

	Tape[0].Count = Tape[1].Count = 0;
	Tape[0].Head = 0;
	Base = 0;
	while (Node) {
		_node *Next = Node->Next;
		Node->Next = Tape[Base].Head;
		Tape[Base].Head = Node;
		Tape[Base].Count++;
		Base ^= 1;
		Node = Next;
	};

	/* If the list is empty or contains only a single record, then */
	/* tape[1].count == 0L and this part is vacuous.               */

	for (Base = 0, BlockSize = 1; Tape[Base + 1].Count; Base ^= 2, BlockSize <<= 1) {
		struct _tape *Tape0 = Tape + Base;
		struct _tape *Tape1 = Tape + Base + 1;
		int Dest = Base ^ 2;
		Tape[Dest].Count = Tape[Dest + 1].Count = 0;
		for (; Tape0->Count; Dest ^= 1) {
			unsigned long N0, N1;
			struct _tape *OutputTape = Tape + Dest;
			N0 = N1 = BlockSize;
			while (1) {
				_node *ChosenNode;
				struct _tape *ChosenTape;
				if (N0 == 0 || Tape0->Count == 0) {
					if (N1 == 0 || Tape1->Count == 0) break;
					ChosenTape = Tape1;
					N1--;
				} else if (N1 == 0 || Tape1->Count == 0) {
					ChosenTape = Tape0;
					N0--;
				} else {
					Std$Function_result Result0;
					switch (Std$Function$call($LESS, 2, &Result0, Tape0->Head->Value, 0, Tape1->Head->Value, 0)) {
					case SUSPEND: case SUCCESS:
						ChosenTape = Tape0;
						N0--;
		                break;
	        	    case FAILURE:
						ChosenTape = Tape1;
						N1--;
		                break;
					case MESSAGE:
						UNLOCK(List);
						Result->Val = Result0.Val;
						return MESSAGE;
					};
				};
				ChosenTape->Count--;
				ChosenNode = ChosenTape->Head;
				ChosenTape->Head = ChosenNode->Next;
				if (OutputTape->Count == 0) {
					OutputTape->Head = ChosenNode;
				} else {
					OutputTape->Tail->Next = ChosenNode;
				};
				OutputTape->Tail = ChosenNode;
				OutputTape->Count++;
			};
		};
	};

	_node *Head = Tape[Base].Head;
	_node *Tail = Tape[Base].Tail;

	Tail->Next = 0;
	for (_node *Prev = 0, *Node = Head; Node; Prev = Node, Node = Node->Next) Node->Prev = Prev;

	List->Head = List->Cache = Head;
	List->Tail = Tail;
	List->Index = 1;
	List->Cache = Head;
	List->Access = 4;
	List->Array = 0;
	Result->Val = (Std$Object_t *)List;
	UNLOCK(List);
	return SUCCESS;
};

METHOD("sort", TYP, T) {
//@list
//@comp
//:T
// Sorts <var>list</var> in place with the ordering given by <code>a &lt; b</code> if <code>comp(a, b)</code> succeeds.
// Algorithm taken from <a href="http://www.chiark.greenend.org.uk/~sgtatham/algorithms/">Simon Tatham's Algorithms Page</a>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	if (List->Length <= 1) {
		UNLOCK(List);
		Result->Val = (Std$Object_t *)List;
		return SUCCESS;
	};
	_node *Head = List->Head;
	int InSize = 1;
	for (;;) {
		_node *P = Head;
		_node *Tail = Head = 0;

		int NMerges = 0;

		while (P) {
			NMerges++;
			_node *Q = P;
			int PSize = 0;
			for (int I = 0; I < InSize; I++) {
				PSize++;
				Q = Q->Next;
				if (!Q) break;
			};
			int QSize = InSize;
			_node *E;
			while (PSize > 0 || (QSize > 0 && Q)) {
				if (PSize == 0) {
					E = Q; Q = Q->Next; QSize--;
				} else if (QSize == 0 || !Q) {
					E = P; P = P->Next; PSize--;
				} else {
					Std$Function_result Result0;
					switch (Std$Function$call($LESS, 2, &Result0, P->Value, 0, Q->Value, 0)) {
					case SUSPEND: case SUCCESS:
						E = P; P = P->Next; PSize--;
	                    break;
            	    case FAILURE:
						E = Q; Q = Q->Next; QSize--;
	                    break;
					case MESSAGE:
						UNLOCK(List);
						Result->Val = Result0.Val;
						return MESSAGE;
					};
				};
				if (Tail) {
					Tail->Next = E;
				} else {
					Head = E;
				};
				E->Prev = Tail;
				Tail = E;
			};
			P = Q;
		};
		Tail->Next = 0;
		if (NMerges <= 1) {
			List->Head = Head;
			List->Tail = Tail;
			List->Index = 1;
			List->Cache = Head;
			List->Access = 4;
			List->Array = 0;
			Result->Val = (Std$Object_t *)List;
			UNLOCK(List);
			return SUCCESS;
		};
		InSize *= 2;
	};
};

METHOD("sort2", TYP, T, TYP, Std$Function$T) {
//@list
//@comp
//:T
// Code based on public domain code by Philip J. Erdelsky, pje@acm.org
// http://www.alumni.caltech.edu/~pje/
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	if (List->Length <= 1) {
		UNLOCK(List);
		Result->Val = (Std$Object_t *)List;
		return SUCCESS;
	};
	Std$Object_t *Compare = Args[1].Val;

	struct _tape {
		_node *Head, *Tail;
		unsigned long Count;
	} Tape[4];

	unsigned int Base;
	unsigned long BlockSize;

	/* Distribute the records alternately to tape[0] and tape[1]. */
	_node *Node = List->Head;

	Tape[0].Count = Tape[1].Count = 0;
	Tape[0].Head = 0;
	Base = 0;
	while (Node) {
		_node *Next = Node->Next;
		Node->Next = Tape[Base].Head;
		Tape[Base].Head = Node;
		Tape[Base].Count++;
		Base ^= 1;
		Node = Next;
	};

	/* If the list is empty or contains only a single record, then */
	/* tape[1].count == 0L and this part is vacuous.               */

	for (Base = 0, BlockSize = 1; Tape[Base + 1].Count; Base ^= 2, BlockSize <<= 1) {
		struct _tape *Tape0 = Tape + Base;
		struct _tape *Tape1 = Tape + Base + 1;
		int Dest = Base ^ 2;
		Tape[Dest].Count = Tape[Dest + 1].Count = 0;
		for (; Tape0->Count; Dest ^= 1) {
			unsigned long N0, N1;
			struct _tape *OutputTape = Tape + Dest;
			N0 = N1 = BlockSize;
			while (1) {
				_node *ChosenNode;
				struct _tape *ChosenTape;
				if (N0 == 0 || Tape0->Count == 0) {
					if (N1 == 0 || Tape1->Count == 0) break;
					ChosenTape = Tape1;
					N1--;
				} else if (N1 == 0 || Tape1->Count == 0) {
					ChosenTape = Tape0;
					N0--;
				} else {
					Std$Function_result Result0;
					switch (Std$Function$call(Compare, 2, &Result0, Tape0->Head->Value, 0, Tape1->Head->Value, 0)) {
					case SUSPEND: case SUCCESS:
						ChosenTape = Tape0;
						N0--;
		                break;
	        	    case FAILURE:
						ChosenTape = Tape1;
						N1--;
		                break;
					case MESSAGE:
						UNLOCK(List);
						Result->Val = Result0.Val;
						return MESSAGE;
					};
				};
				ChosenTape->Count--;
				ChosenNode = ChosenTape->Head;
				ChosenTape->Head = ChosenNode->Next;
				if (OutputTape->Count == 0) {
					OutputTape->Head = ChosenNode;
				} else {
					OutputTape->Tail->Next = ChosenNode;
				};
				OutputTape->Tail = ChosenNode;
				OutputTape->Count++;
			};
		};
	};

	_node *Head = Tape[Base].Head;
	_node *Tail = Tape[Base].Tail;

	Tail->Next = 0;
	for (_node *Prev = 0, *Node = Head; Node; Prev = Node, Node = Node->Next) Node->Prev = Prev;

	List->Head = List->Cache = Head;
	List->Tail = Tail;
	List->Index = 1;
	List->Cache = Head;
	List->Access = 4;
	List->Array = 0;
	Result->Val = (Std$Object_t *)List;
	UNLOCK(List);
	return SUCCESS;
};

METHOD("sort", TYP, T, TYP, Std$Function$T) {
//@list
//@comp
//:T
// Sorts <var>list</var> in place with the ordering given by <code>a &lt; b</code> if <code>comp(a, b)</code> succeeds.
// Algorithm taken from <a href="http://www.chiark.greenend.org.uk/~sgtatham/algorithms/">Simon Tatham's Algorithms Page</a>
	_list *List = (_list *)Args[0].Val;
	WRLOCK(List);
	if (List->Length <= 1) {
		UNLOCK(List);
		Result->Val = (Std$Object_t *)List;
		return SUCCESS;
	};
	Std$Object_t *Compare = Args[1].Val;
	_node *Head = List->Head;
	int InSize = 1;
	for (;;) {
		_node *P = Head;
		_node *Tail = Head = 0;

		int NMerges = 0;

		while (P) {
			NMerges++;
			_node *Q = P;
			int PSize = 0;
			for (int I = 0; I < InSize; I++) {
				PSize++;
				Q = Q->Next;
				if (!Q) break;
			};
			int QSize = InSize;
			_node *E;
			while (PSize > 0 || (QSize > 0 && Q)) {
				if (PSize == 0) {
					E = Q; Q = Q->Next; QSize--;
				} else if (QSize == 0 || !Q) {
					E = P; P = P->Next; PSize--;
				} else {
					Std$Function_result Result0;
					switch (Std$Function$call(Compare, 2, &Result0, P->Value, 0, Q->Value, 0)) {
					case SUSPEND: case SUCCESS:
						E = P; P = P->Next; PSize--;
	                    break;
            	    case FAILURE:
						E = Q; Q = Q->Next; QSize--;
	                    break;
					case MESSAGE:
						UNLOCK(List);
						Result->Val = Result0.Val;
						return MESSAGE;
					};
				};
				if (Tail) {
					Tail->Next = E;
				} else {
					Head = E;
				};
				E->Prev = Tail;
				Tail = E;
			};
			P = Q;
		};
		Tail->Next = 0;
		if (NMerges <= 1) {
			List->Head = Head;
			List->Tail = Tail;
			List->Index = 1;
			List->Cache = Head;
			List->Access = 4;
			List->Array = 0;
			Result->Val = (Std$Object_t *)List;
			UNLOCK(List);
			return SUCCESS;
		};
		InSize *= 2;
	};
};

#ifdef DOCUMENTING
METHOD("collect", TYP, Std$Function$T, ANY) {
#else
METHOD("collect", TYP, Std$Function$T) {
#endif
//@func
//@args...
//:T
// Returns a list of all values produced by <code>func(args)</code>.
	Std$Function_result Result0;
	_list *List = new(_list);
	List->Type = T;
	List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	Std$Object_t *Function = Args[0].Val;
	_node *Node, *Prev;
	unsigned long NoOfElements;
	switch (Std$Function$invoke(Function, Count - 1, &Result0, Args + 1)) {
	case SUSPEND:
		Node = new(_node);
		NoOfElements = 1;
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (;;) {
			switch (Std$Function$resume(&Result0)) {
			case SUSPEND:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
				break;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			case SUCCESS:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
			case FAILURE:
				List->Tail = Node;
				List->Length = NoOfElements;
				return SUCCESS;
			};
		};
	case MESSAGE:
		Result->Val = Result0.Val;
		return MESSAGE;
	case SUCCESS:
		Node = new(_node);
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		List->Tail = Node;
		List->Length = 1;
	case FAILURE:
		return SUCCESS;
	};
};

GLOBAL_FUNCTION(Collect, 1) {
//@func:Std$Function$T
//@args...
//:T
// Returns a list of all values produced by <code>func(args)</code>.
	Std$Function_result Result0;
	_list *List = new(_list);
	List->Type = T;
	List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	Std$Object_t *Function = Args[0].Val;
	_node *Node, *Prev;
	unsigned long NoOfElements;
	switch (Std$Function$invoke(Function, Count - 1, &Result0, Args + 1)) {
	case SUSPEND:
		Node = new(_node);
		NoOfElements = 1;
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (;;) {
			switch (Std$Function$resume(&Result0)) {
			case SUSPEND:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
				break;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			case SUCCESS:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
			case FAILURE:
				List->Tail = Node;
				List->Length = NoOfElements;
				return SUCCESS;
			};
		};
	case MESSAGE:
		Result->Val = Result0.Val;
		return MESSAGE;
	case SUCCESS:
		Node = new(_node);
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		List->Tail = Node;
		List->Length = 1;
	case FAILURE:
		return SUCCESS;
	};
};

GLOBAL_FUNCTION(CollectN, 2) {
//@func:Std$Function$T
//@list&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;list&lt;sub&gt;k&lt;/sub&gt;
//:T
// Returns a list of all values produced by <code>func(</code><var>arg<sub>1</sub></var><code>, ..., </code><var>arg<sub>k</sub></var><code>)</code> where <var>arg<sub>1</sub></var><code>, ..., </code><var>arg<sub>k</sub></var> are elements taken from <var>list<sub>1</sub></var><code>, ..., </code><var>list<sub>k</sub></var>
	Std$Function_result Result0;
	unsigned long Max = Std$Integer$get_small(Args[0].Val);
	_list *List = new(_list);
	List->Type = T;
	List->Lower = List->Upper = 0;
	List->Access = 4;
	INITLOCK(List);
	Result->Val = (Std$Object_t *)List;
	if (Max == 0) return SUCCESS;
	Std$Object_t *Function = Args[1].Val;
	_node *Node, *Prev;
	unsigned long NoOfElements;
	switch (Std$Function$invoke(Function, Count - 2, &Result0, Args + 2)) {
	case SUSPEND:
		Node = new(_node);
		NoOfElements = 1;
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		for (; --Max;) {
			switch (Std$Function$resume(&Result0)) {
			case SUSPEND:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
				break;
			case MESSAGE:
				Result->Val = Result0.Val;
				return MESSAGE;
			case SUCCESS:
				++NoOfElements;
				Prev = Node;
				Node = new(_node);
				(Node->Prev = Prev)->Next = Node;
				Node->Value = Result0.Val;
			case FAILURE:
				List->Tail = Node;
				List->Length = NoOfElements;
				return SUCCESS;
			};
		};
		List->Tail = Node;
		List->Length = NoOfElements;
		return SUCCESS;
	case MESSAGE:
		Result->Val = Result0.Val;
		return MESSAGE;
	case SUCCESS:
		Node = new(_node);
		Node->Value = Result0.Val;
		List->Head = Node;
		List->Cache = Node;
		List->Index = 1;
		List->Tail = Node;
		List->Length = 1;
	case FAILURE:
		return SUCCESS;
	};
};
