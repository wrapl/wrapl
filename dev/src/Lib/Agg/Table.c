#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
//#include <stdlib.h>

/*
This file was modified from avl.c in GNU libavl
*/

SYMBOL($AT, "@");
SYMBOL($COMP, "?");
SYMBOL($HASH, "#");

TYPE(T);
// A table of Key/Value pairs

TYPE(NodeType);
// A single Key/Value pair

/* Maximum AVL height. */
#ifndef AVL_MAX_HEIGHT
#define AVL_MAX_HEIGHT 32
#endif

/* Tree data structure. */

#ifdef THREADED

#include <pthread.h>

#define RDLOCK(T) pthread_rwlock_rdlock(((table_t *)T)->Lock)
#define WRLOCK(T) pthread_rwlock_wrlock(((table_t *)T)->Lock)
#define UNLOCK(T) pthread_rwlock_unlock(((table_t *)T)->Lock)
#define INITLOCK(T) pthread_rwlock_init(T->Lock, 0)

#else

#define RDLOCK(T) 0
#define WRLOCK(T) 0
#define UNLOCK(T) 0
#define INITLOCK(T) 0

#endif

typedef struct table_t table_t;
typedef struct node_t node_t;
typedef struct traverser_t traverser_t;

struct table_t {
	const Std$Type_t *Type;
	node_t *Root;          /* Tree's root. */
	unsigned long Count;                   /* Number of items in Table. */
	unsigned long Generation;       /* Generation number. */
	Std$Object_t *Compare;
	Std$Object_t *Hash;
	Std$Object_t *Default;
#ifdef THREADED
	pthread_rwlock_t Lock[1];
#endif
};

#ifdef THREADED
	
METHOD("rdlock", TYP, T) {
	table_t *Table = (table_t *)Args[0].Val;
	RDLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("wrlock", TYP, T) {
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("unlock", TYP, T) {
	table_t *Table = (table_t *)Args[0].Val;
	UNLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

#endif

/* An AVL Table Node. */
struct node_t {
	const Std$Type_t *NodeType;
	node_t *Link[2];  /* Subtrees. */
	Std$Object_t *Key, *Value;      /* Key/Value of Node. */
	unsigned long Hash;            /* Hash Value for Value. */
	signed char Balance;       /* Balance factor. */
};

#define TRAVERSER_FIELDS\
	Std$Function_cstate State;														\
	table_t *Table;				/* Tree being traversed. */				\
	node_t *Node;					/* Current Node in Table. */				\
	node_t *Stack[AVL_MAX_HEIGHT];	/* All the nodes above |Node|. */	\
	unsigned long Height;					/* Number of nodes in |avl_parent|. */	\
	unsigned long Generation;				/* Generation number. */

/* AVL traverser structure. */
struct traverser_t {
	TRAVERSER_FIELDS
};

/* Creates and returns a new table
   with comparison function |compare| using parameter |param|
   and memory allocator |allocator|.
   Returns |0| if memory allocation failed. */
table_t *avl_create (Std$Object_t *Compare, Std$Object_t *Hash) {
	table_t *Table = (table_t *)Riva$Memory$alloc(sizeof *Table);
	Table->Type = T;
	Table->Root = 0;
	Table->Count = 0;
	Table->Generation = 0;
	Table->Compare = Compare;
	Table->Hash = Hash;
	INITLOCK(Table);
	return Table;
}

static inline int compare(const table_t *Table, Std$Object_t *A, Std$Object_t *B) {
	Std$Function_result Result;
	Std$Function$call(Table->Compare, 2, &Result, A, 0, B, 0);
	return ((Std$Integer_smallt *)Result.Val)->Value;
};

/* Search |Table| for an item matching |item|, and return it if found.
   Otherwise return |0|. */
static inline Std$Object_t **avl_find (const table_t *Table, Std$Object_t *Key, unsigned long Hash) {
	node_t *p;
	for (p = Table->Root; p != 0;) {
		int cmp;
		if (Hash < p->Hash) {
			cmp = -1;
		} else if (Hash > p->Hash) {
			cmp = 1;
		} else {
			cmp = compare(Table, Key, p->Key);
		};
		if (cmp < 0) p = p->Link[0];
		else if (cmp > 0) p = p->Link[1];
		else return &p->Value;
	}
	return 0;
}

extern __attribute__ ((regparm(3))) int avl_compare_asm(const table_t *t, Std$Object_t *a, Std$Object_t *b);
extern __attribute__ ((regparm(3))) Std$Object_t **avl_find_asm(const table_t *Tree, Std$Object_t *Key, unsigned long Hash);
extern __attribute__ ((regparm(3))) Std$Object_t **avl_probe_asm(const table_t *Tree, Std$Object_t *Key, unsigned long Hash);

/* Inserts |item| into |Table| and returns a pointer to |item|'s address.
   If a duplicate item is found in the Table,
   returns a pointer to the duplicate without inserting |item|.
   Returns |0| in case of memory allocation failure. */
static Std$Object_t **avl_probe (table_t *Table, Std$Object_t *Key, unsigned long Hash) {
	node_t *y, *z; /* Top Node to update balance factor, and parent. */
	node_t *p, *q; /* Iterator, and parent. */
	node_t *n;     /* Newly inserted Node. */
	node_t *w;     /* New root of rebalanced subtree. */
	int dir;                /* Direction to descend. */

	unsigned char da[AVL_MAX_HEIGHT]; /* Cached comparison results. */
	unsigned char *k = da;              /* Number of cached results. */

	z = (node_t *) Table;
	y = Table->Root;
	node_t **s = &Table->Root;
	for (q = z, p = y; p != 0;) {
		if (p->Balance != 0) z = q, y = p, k = da;
		q = p;
		if (Hash < p->Hash) {
			s = &p->Link[0];
			p = *s;
			*(k++) = 0;
		} else if (Hash > p->Hash) {
			s = &p->Link[1];
			p = *s;
			*(k++) = 1;
		} else {
			int cmp = compare(Table, Key, p->Key);
			if (cmp < 0) {
				s = &p->Link[0];
				p = *s;
				*(k++) = 0;
			} else if (cmp > 0) {
				s = &p->Link[1];
				p = *s;
				*(k++) = 1;
			} else {
				return &p->Value;
			};
		};
	}

	n = *s = (node_t *)Riva$Memory$alloc(sizeof *n);
	n->NodeType = NodeType;

	Table->Count++;
	n->Key = Key;
	n->Hash = Hash;
	/*
	n->Value = 0;
	n->Link[0] = n->Link[1] = 0;
	n->Balance = 0;
	*/
	if (y == 0) return &n->Value;

	for (p = y, k = da; p != n;) {
		if (*(k++) == 0) {
			p->Balance--;
			p = p->Link[0];
		} else {
			p->Balance++;
			p = p->Link[1];
		};
	};

	static signed char balances[][2] = {{0, 1}, {0, 0}, {-1, 0}, {1, 0}, {0, 0}, {0, -1}};

	if (y->Balance == -2) {
		node_t *x = y->Link[0];
		if (x->Balance == -1) {
			w = x;
			y->Link[0] = x->Link[1];
			x->Link[1] = y;
			y->Balance = 0;
		} else {
			w = x->Link[1];
			x->Link[1] = w->Link[0];
			w->Link[0] = x;
			y->Link[0] = w->Link[1];
			w->Link[1] = y;
			x->Balance = balances[w->Balance + 1][0];
			y->Balance = balances[w->Balance + 1][1];
		}
	} else if (y->Balance == +2) {
		node_t *x = y->Link[1];
		if (x->Balance == +1) {
			w = x;
			y->Link[1] = x->Link[0];
			x->Link[0] = y;
			y->Balance = 0;
		} else {
			w = x->Link[0];
			x->Link[0] = w->Link[1];
			w->Link[1] = x;
			y->Link[1] = w->Link[0];
			w->Link[0] = y;
			x->Balance = balances[w->Balance + 4][0];
			y->Balance = balances[w->Balance + 4][1];
		}
	} else return &n->Value;
	w->Balance = 0;
	z->Link[y != z->Link[0]] = w;

	Table->Generation++;
	return &n->Value;
}

/* Deletes from |Table| and returns an item matching |item|.
   Returns a null pointer if no matching item found. */
static Std$Object_t *avl_delete (table_t *Table, Std$Object_t *Key, long Hash) {
	/* Stack of nodes. */
	node_t *pa[AVL_MAX_HEIGHT]; /* Nodes. */
	unsigned char da[AVL_MAX_HEIGHT];    /* |Link[]| indexes. */
	int k;                               /* Stack pointer. */

	node_t *p;   /* Traverses Table to find Node to delete. */
	int cmp;              /* Result of comparison between |item| and |p|. */
	Std$Object_t *data;

	k = 0;
	p = (node_t *) Table;
	for (cmp = -1; cmp != 0;) {
		int dir = cmp > 0;
		pa[k] = p;
		da[k++] = dir;
		p = p->Link[dir];
		if (p == 0) return 0;
		if (Hash < p->Hash) {
			cmp = -1;
		} else if (Hash > p->Hash) {
			cmp = 1;
		} else {
			cmp = avl_compare_asm(Table, Key, p->Key);
		};
	}
	data = p->Value;

	if (p->Link[1] == 0) {
		pa[k - 1]->Link[da[k - 1]] = p->Link[0];
	} else {
		node_t *r = p->Link[1];
		if (r->Link[0] == 0) {
			r->Link[0] = p->Link[0];
			r->Balance = p->Balance;
			pa[k - 1]->Link[da[k - 1]] = r;
			da[k] = 1;
			pa[k++] = r;
		} else {
			node_t *s;
			int j = k++;
			for (;;) {
				da[k] = 0;
				pa[k++] = r;
				s = r->Link[0];
				if (s->Link[0] == 0) break;
				r = s;
			}

			s->Link[0] = p->Link[0];
			r->Link[0] = s->Link[1];
			s->Link[1] = p->Link[1];
			s->Balance = p->Balance;

			pa[j - 1]->Link[da[j - 1]] = s;
			da[j] = 1;
			pa[j] = s;
		}
	}

	while (--k > 0) {
		node_t *y = pa[k];
		if (da[k] == 0) {
			y->Balance++;
			if (y->Balance == +1) {
				break;
			} else if (y->Balance == +2) {
				node_t *x = y->Link[1];
				if (x->Balance == -1) {
					node_t *w;
					w = x->Link[0];
					x->Link[0] = w->Link[1];
					w->Link[1] = x;
					y->Link[1] = w->Link[0];
					w->Link[0] = y;
					if (w->Balance == +1)
						x->Balance = 0, y->Balance = -1;
					else if (w->Balance == 0)
						x->Balance = y->Balance = 0;
					else /* |w->Balance == -1| */
						x->Balance = +1, y->Balance = 0;
					w->Balance = 0;
					pa[k - 1]->Link[da[k - 1]] = w;
				} else {
					y->Link[1] = x->Link[0];
					x->Link[0] = y;
					pa[k - 1]->Link[da[k - 1]] = x;
					if (x->Balance == 0) {
						x->Balance = -1;
						y->Balance = +1;
						break;
					} else x->Balance = y->Balance = 0;
				}
			}
		} else {
			y->Balance--;
			if (y->Balance == -1) {
				break;
			} else if (y->Balance == -2) {
				node_t *x = y->Link[0];
				if (x->Balance == +1) {
					node_t *w;
					w = x->Link[1];
					x->Link[1] = w->Link[0];
					w->Link[0] = x;
					y->Link[0] = w->Link[1];
					w->Link[1] = y;
					if (w->Balance == -1)
						x->Balance = 0, y->Balance = +1;
					else if (w->Balance == 0)
						x->Balance = y->Balance = 0;
					else /* |w->Balance == +1| */
						x->Balance = -1, y->Balance = 0;
					w->Balance = 0;
					pa[k - 1]->Link[da[k - 1]] = w;
				} else {
					y->Link[0] = x->Link[1];
					x->Link[1] = y;
					pa[k - 1]->Link[da[k - 1]] = x;
					if (x->Balance == 0) {
						x->Balance = +1;
						y->Balance = -1;
						break;
					} else x->Balance = y->Balance = 0;
				}
			}
		}
	}

	Table->Count--;
	Table->Generation++;
	return data;
}

static void trav_refresh (traverser_t *Trav) {
	Trav->Generation = Trav->Table->Generation;
	node_t *root = Trav->Table->Root;
	if (root == 0) {
		Trav->Node = 0;
		return;
	}
	if (Trav->Node != 0) {
		node_t *Node = Trav->Node;
		node_t *i, *j = 0;
		Trav->Height = 0;
		for (i = root; i != Node; ) {
			int cmp;
			if (Node->Hash < i->Hash) {
				cmp = -1;
			} else if (Node->Hash > i->Hash) {
				cmp = 1;
			} else {
				cmp = avl_compare_asm(Trav->Table, Node->Key, i->Key);
			};
			if (cmp > 0) j = i;
			Trav->Stack[Trav->Height++] = i;
			i = i->Link[cmp > 0];
			if (i == 0) {
				if (cmp > 0) {
					Trav->Node = Trav->Stack[--Trav->Height];
					return;
				} else {
					while (Trav->Stack[--Trav->Height] != j);
					Trav->Node = j;
					return;
				}
			}
		}
	}
}

/* Initializes |Trav| for |Table|
   and selects and returns a pointer to its least-valued item.
   Returns |0| if |Table| contains no nodes. */
static inline node_t *avl_t_first (traverser_t *Trav, table_t *Table) {
	node_t *x;
	Trav->Table = Table;
	Trav->Height = 0;
	Trav->Generation = Table->Generation;
	x = Table->Root;
	if (x != 0) while (x->Link[0] != 0) {
		Trav->Stack[Trav->Height++] = x;
		x = x->Link[0];
	}
	Trav->Node = x;
	return x != 0 ? x : 0;
}

/* Returns the next data item in inorder
   within the Table being traversed with |Trav|,
   or if there are no more data items returns |0|. */
static inline node_t *avl_t_next (traverser_t *Trav) {
	node_t *x;
	if (Trav->Generation != Trav->Table->Generation) trav_refresh (Trav);
	x = Trav->Node;
	if (x == 0) {
		return avl_t_first (Trav, Trav->Table);
	} else if (x->Link[1] != 0) {
		Trav->Stack[Trav->Height++] = x;
		x = x->Link[1];
		while (x->Link[0] != 0) {
			Trav->Stack[Trav->Height++] = x;
			x = x->Link[0];
		}
	} else {
		node_t *y;
		do {
			if (Trav->Height == 0) {
				Trav->Node = 0;
				return 0;
			}
			y = x;
			x = Trav->Stack[--Trav->Height];
		} while (y == x->Link[1]);
	}
	Trav->Node = x;
	return x;
}

Std$Object_t *_new(Std$Object_t *Comp, Std$Object_t *Hash) {
	return (Std$Object_t *)avl_create(Comp ?: $COMP, Hash ?: $HASH);
};

GLOBAL_FUNCTION(New, 0) {
//@comp:Std$Function$T=&#58;&quot;?&quot;
//@Hash:Std$Function$T=&#58;&quot;#&quot;
//:T
// Returns a new table with comparison function <var>comp</var> and Hash function <var>Hash</var>.
// <code>comp(a, b)</code> should return <code>-1</code>, <code>0</code> or <code>-1</code> if <code>a &lt; b</code>, <code>a = b</code> or <code>a &gt; b</code> respectively.
// <code>Hash(a)</code> should return a <id>Std/Integer/SmallT</id>.
	if (Count == 0) {
		Result->Val = (Std$Object_t *)avl_create($COMP, $HASH);
	} else if (Count == 1) {
		if (Std$Object$in(Args[0].Val, Std$Type$T)) {
			Std$Type$t *Type = (Std$Type$t *)Args[0].Val;
			Std$Object$t *Compare, *Hash;
			// TODO: look up compare and hash functions from :"?" and :"#" and Type
			Result->Val = (Std$Object_t *)avl_create(Args[0].Val, $HASH);
		} else {
			Result->Val = (Std$Object_t *)avl_create(Args[0].Val, $HASH);
		};
	} else if (Count == 2) {
		Result->Val = (Std$Object_t *)avl_create(Args[0].Val, Args[1].Val);
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(NewIdentity, 0) {
//:T
// Returns a new table in which two keys are the same only if they are the same object.
    Result->Val = (Std$Object_t *)avl_create(Std$Object$Compare, Std$Object$Hash);
    return SUCCESS;
};

METHOD("empty", TYP, T) {
//@t
//:T
// Removes all entries from <var>t</var> and returns it.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Table->Root = 0;
	Table->Count = 0;
	Table->Generation = 0;
	UNLOCK(Table);
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

GLOBAL_FUNCTION(Make, 0) {
//:T
// Returns a new table with the arguments taken as alternating Key/Value pairs.
	table_t *Table = avl_create($COMP, $HASH);
	int I;
	for (I = 0; I < Count; I += 2) {
		Std$Function_result Result1;
		Std$Object_t **Slot;
		Std$Function$call($HASH, 1, &Result1, Args[I].Val, 0);
		Slot = avl_probe_asm(Table, Args[I].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		*Slot = Args[I + 1].Val;
	};
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

GLOBAL_FUNCTION(Collect, 1) {
//@func:Std$Function$T
//@args...
//:T
// Returns a table consisting of the values returned by <code>func(args)</code> as keys (and <id>NIL</id> as each Value).
	Std$Function_result Result0;
	Std$Object_t *Function = Args[0].Val;
	table_t *Table = avl_create($COMP, $HASH);
	long Return = Std$Function$invoke(Function, Count - 1, &Result0, Args + 1);
	if (Return == SUCCESS) {
		Std$Function_result Result1;
		Std$Object_t **Slot;
		Std$Function$call($HASH, 1, &Result1, Result0.Val, 0);
		Slot = avl_probe_asm(Table, Result0.Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		*Slot = Std$Object$Nil;
	} else if (Return == SUSPEND) {
		Std$Function_result Result1;
		Std$Object_t **Slot;
		Std$Function$call($HASH, 1, &Result1, Result0.Val, 0);
		Slot = avl_probe_asm(Table, Result0.Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		*Slot = Std$Object$Nil;
		Return = Std$Function$resume(&Result0);
		while (Return == SUSPEND) {
			Std$Function$call($HASH, 1, &Result1, Result0.Val, 0);
			Slot = avl_probe_asm(Table, Result0.Val, ((Std$Integer_smallt *)Result1.Val)->Value);
			*Slot = Std$Object$Nil;
			Return = Std$Function$resume(&Result0);
		};
		if (Return == SUCCESS) {
			Std$Function$call($HASH, 1, &Result1, Result0.Val, 0);
			Slot = avl_probe_asm(Table, Result0.Val, ((Std$Integer_smallt *)Result1.Val)->Value);
			*Slot = Std$Object$Nil;
		} else if (Return == MESSAGE) {
			*Result = Result0;
			return MESSAGE;
		};
	} else if (Return == MESSAGE) {
		*Result = Result0;
		return MESSAGE;
	};
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

METHOD("copy", TYP, T) {
//@t
//:T
// Returns a shallow copy of <var>t</var>
	table_t *Source = (table_t *)Args[0].Val;
	table_t *Table = avl_create(Source->Compare, Source->Hash);
	traverser_t Traverser;
	RDLOCK(Source);
	for (node_t *Node = avl_t_first(&Traverser, Source); Node; Node = avl_t_next(&Traverser)) {
		avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Node->Value;
	};
	Table->Default = Source->Default;
	UNLOCK(Source);
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

METHOD("map", TYP, T, TYP, Std$Function$T) {
//@t
//@func
//:T
// Returns a copy of <var>t</var> with the same keys, but with values returned by calling <code>func(Key, Value)</code>
	table_t *Source = (table_t *)Args[0].Val;
	table_t *Table = avl_create(Source->Compare, Source->Hash);
	traverser_t Traverser;
	RDLOCK(Source);
	for (node_t *Node = avl_t_first(&Traverser, Source); Node; Node = avl_t_next(&Traverser)) {
		switch (Std$Function$call(Args[1].Val, 2, Result, Node->Key, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Result->Val;
			break;
		case FAILURE:
			break;
		case MESSAGE:
			return MESSAGE;
		};
	};
	Table->Default = Source->Default;
	UNLOCK(Source);
	Result->Val = (Std$Object_t *)Table;
	Result->Ref = 0;
	return SUCCESS;
};

METHOD("list", TYP, T, TYP, Std$Function$T) {
//@t
//@func
//:T
// Returns the list <code>func(Key, Value)</code> running over all pairs in <var>t</var>.
	table_t *Source = (table_t *)Args[0].Val;
	Agg$List$t *List = Agg$List$new0();
	traverser_t Traverser;
	RDLOCK(Source);
	for (node_t *Node = avl_t_first(&Traverser, Source); Node; Node = avl_t_next(&Traverser)) {
		switch (Std$Function$call(Args[1].Val, 2, Result, Node->Key, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			Agg$List$put(List, Result->Val);
			break;
		case FAILURE:
			break;
		case MESSAGE:
			return MESSAGE;
		};
	};
	UNLOCK(Source);
	Result->Val = (Std$Object_t *)List;
	Result->Ref = 0;
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, T) {
//@a
//@b
//:T
// Returns the union of <var>a</var> and <var>b</var>. If a Key exists in both <var>a</var> and <var>b</var> then the Value is taken from <var>b</var>.
	table_t *Table = avl_create($COMP, $HASH);
	traverser_t Traverser;
	RDLOCK(Args[0].Val);
	for (node_t *Node = avl_t_first(&Traverser, (table_t *)Args[0].Val); Node; Node = avl_t_next(&Traverser)) {
		avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Node->Value;
	};
	UNLOCK(Args[0].Val);
	RDLOCK(Args[1].Val);
	for (node_t *Node = avl_t_first(&Traverser, (table_t *)Args[1].Val); Node; Node = avl_t_next(&Traverser)) {
		avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Node->Value;
	};
	UNLOCK(Args[1].Val);
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, T) {
//@a
//@b
//:T
// Returns the difference of <var>a</var> and <var>b</var>, i.e. those <code>(Key, Value)</code> pairs of <var>a</var> where <var>Key</var> is not in <var>b</var>.
	table_t *Table = avl_create($COMP, $HASH);
	traverser_t Traverser;
	RDLOCK(Args[0].Val);
	RDLOCK(Args[1].Val);
	for (node_t *Node = avl_t_first(&Traverser, (table_t *)Args[0].Val); Node; Node = avl_t_next(&Traverser)) {
		if (avl_find_asm((table_t *)Args[1].Val, Node->Key, Node->Hash) == 0) {
			avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Node->Value;
		};
	};
	UNLOCK(Args[1].Val);
	UNLOCK(Args[0].Val);
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

METHOD("*", TYP, T, TYP, T) {
//@a
//@b
//:T
// Returns the intersection of <var>a</var> and <var>b</var>, i.e. those <code>(Key, Value)</code> pairs of <var>a</var> where <var>Key</var> is in <var>b</var>.
	table_t *Table = avl_create($COMP, $HASH);
	traverser_t Traverser;
	table_t *A = (table_t *)Args[0].Val;
	table_t *B = (table_t *)Args[1].Val;
	RDLOCK(A);
	RDLOCK(B);
	if (A->Count < B->Count) {
		for (node_t *Node = avl_t_first(&Traverser, A); Node; Node = avl_t_next(&Traverser)) {
			if (avl_find_asm(B, Node->Key, Node->Hash)) {
				avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Node->Value;
			};
		};
	} else {
		for (node_t *Node = avl_t_first(&Traverser, B); Node; Node = avl_t_next(&Traverser)) {
			Std$Object_t **Slot = avl_find_asm(A, Node->Key, Node->Hash);
			if (Slot) avl_probe_asm(Table, Node->Key, Node->Hash)[0] = Slot[0];
		};
	};
	RDLOCK(B);
	RDLOCK(A);
	Result->Val = (Std$Object_t *)Table;
	return SUCCESS;
};

void _insert(table_t *Table, Std$Object_t *Key, Std$Object_t *Value) {
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Key, 0);
	Slot = avl_probe_asm(Table, Key, ((Std$Integer_smallt *)Result1.Val)->Value);
	*Slot = Value ? Value : Std$Object$Nil;
	UNLOCK(Table);
};

#ifdef DOCUMENTING
METHOD("insert", TYP, T, ANY, ANY) {
#else
METHOD("insert", TYP, T, SKP) {
#endif
//@t
//@Key
//@Value=NIL
//:T
// Inserts the pair <code>(Key, Value)</code> into <var>t</var>.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	*Slot = Count > 2 ? Args[2].Val : Std$Object$Nil;
	UNLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("replace", TYP, T, ANY, ANY) {
//@t
//@Key
//@Value=NIL
//:T
// Inserts the pair <code>(Key, Value)</code> into <var>t</var>, returning the existing value if present, or failing.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	Result->Val = *Slot;
	*Slot = Args[2].Val;
	UNLOCK(Table);
	return Result->Val ? SUCCESS : FAILURE;
};

METHOD("modify", TYP, T, ANY, TYP, Std$Function$T) {
//@t
//@Key
//@mod
//:T
// Modifies the Value associated with <var>Key</var>, <code>t[Key] &lt;- mod($)</code>.
// Uses the default Value of <var>t</var> if <var>Key</var> is not present and a default has been set.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function$call(Table->Hash, 1, Result, Args[1].Val, 0);
	Std$Object_t **Ref;
	if (Table->Default) {
		Ref = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result->Val)->Value);
		if (*Ref == 0) {
			Std$Function$status Status = Std$Function$call(Table->Default, 1, Result, Args[1].Val, 0);
			if (Status > SUCCESS) {
				UNLOCK(Table);
				return Status;
			};
			*Ref = Result->Val;
		};
	} else {
		Ref = avl_find_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result->Val)->Value);
		if (Ref == 0) {
			UNLOCK(Table);
			return FAILURE;
		};
	};
	switch (Std$Function$call(Args[2].Val, 1, Result, *Ref, 0)) {
	case SUSPEND: case SUCCESS:
		*Ref = Result->Val; UNLOCK(Table); return SUCCESS;
	case FAILURE: UNLOCK(Table); return FAILURE;
	case MESSAGE: UNLOCK(Table); return MESSAGE;
	};
};

int _delete(table_t *Table, Std$Object_t *Key) {
	Std$Function_result Result1;
	WRLOCK(Table);
	Std$Function$call(Table->Hash, 1, &Result1, Key, 0);
	if (avl_delete(Table, Key, ((Std$Integer_smallt *)Result1.Val)->Value)) {
		UNLOCK(Table);
		return 0;
	} else {
		UNLOCK(Table);
		return 1;
	};
};

METHOD("delete", TYP, T, SKP) {
//@t
//@Key
//:T
// Removes the <var>Key</var> from <var>t</var>.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	Std$Object_t *Value = avl_delete(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	UNLOCK(Table);
	if (Value) {
		Result->Val = Value;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

SYMBOL($EQUAL, "=");

METHOD("remove", TYP, T, SKP) {
//@t
//@Value
//:T
// Removes the first <var>Key</var> found with Value <var>Value</var>.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	traverser_t Traverser;
	for (node_t *Node = avl_t_first(&Traverser, Table); Node; Node = avl_t_next(&Traverser)) {
		Std$Function_result Result0;
		if (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0) < FAILURE) {
			avl_delete(Table, Node->Key, Node->Hash);
			UNLOCK(Table);
			Result->Val = Node->Value;
			return SUCCESS;
		};
	};
	UNLOCK(Table);
	return FAILURE;
};

METHOD("remove_if", TYP, T, TYP, Std$Function$T) {
//@t
//@func
//:T
// Removes the first <code>(Key, Value)</code> pair for which <code>func(Key, Value)</code> succeeds.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	traverser_t Traverser;
	for (node_t *Node = avl_t_first(&Traverser, Table); Node; Node = avl_t_next(&Traverser)) {
		Std$Function_result Result0;
		if (Std$Function$call(Args[1].Val, 2, &Result0, Node->Key, 0, Node->Value, 0) < FAILURE) {
			Result->Val = avl_delete(Table, Node->Key, Node->Hash);
			UNLOCK(Table);
			return SUCCESS;
		};
	};
	UNLOCK(Table);
	return FAILURE;
};

METHOD("remove_all", TYP, T, TYP, Std$Function$T) {
//@t
//@func
//:T
// Removes the first <code>(Key, Value)</code> pair for which <code>func(Key, Value)</code> succeeds.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	size_t Removed = 0;
	traverser_t Traverser;
	for (node_t *Node = avl_t_first(&Traverser, Table); Node; Node = avl_t_next(&Traverser)) {
		switch (Std$Function$call(Args[1].Val, 2, Result, Node->Key, 0, Node->Value, 0)) {
		case SUSPEND:
		case SUCCESS:
			avl_delete(Table, Node->Key, Node->Hash);
			++Removed;
			break;
		case FAILURE:
			break;
		case MESSAGE:
			UNLOCK(Table);
			return MESSAGE;
		};
	};
	UNLOCK(Table);
	Result->Val = Std$Integer$new_small(Removed);
	return SUCCESS;
};

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
	const table_t *A = (table_t *)Args[0].Val;
	const table_t *B = (table_t *)Args[1].Val;
	Agg$ObjectTable$put(Cache, A, B);
	if (A->Count != B->Count) return FAILURE;
	if (A->Count == 0) {
		Agg$ObjectTable$put(Cache, A, B);
		Result->Arg = Args[1];
		return SUCCESS;
	};
	traverser_t Traverser;
	for (node_t *Node = avl_t_first(&Traverser, A); Node; Node = avl_t_next(&Traverser)) {
		Std$Object$t **Other = avl_find_asm(B, Node->Key, Node->Hash);
		if (!Other) return FAILURE;
		switch (Std$Function$call($EQUAL, 3, Result, Node->Value, 0, *Other, 0, Cache, 0)) {
		case SUSPEND:
		case SUCCESS:
			break;
		case FAILURE:
			return FAILURE;
		case MESSAGE:
			return MESSAGE;
		};
	};
	Result->Arg = Args[1];
	return SUCCESS;
};

Std$Object_t *_index(table_t *Table, Std$Object_t *Key) {
	Std$Function_result Result1;
	RDLOCK(Table);
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Key, 0);
	Slot = avl_find_asm(Table, Key, ((Std$Integer_smallt *)Result1.Val)->Value);
	if (Slot != 0) {
		UNLOCK(Table);
		return *Slot;
	} else {
		UNLOCK(Table);
		return 0;
	};
};

METHOD("defval", TYP, T, ANY) {
//@t
//@default
// Sets the default Value to return when a Key is not present
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Table->Default = Std$Function$constant_new(Args[1].Val);
	UNLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("deffun", TYP, T, ANY) {
//@t
//@default
// Sets the default function to create a Value when a Key is not present
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Table->Default = Args[1].Val;
	UNLOCK(Table);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("check", TYP, T, SKP, ANY) {
//@t
//@Key
//@default
// Returns the Value associated with <var>Key</var> in <var>t</var>.
// If <var>Key</var> is not present, a new pair <code>(Key, default)</code> is created and the call fails.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	if (*Slot == 0) {
		*Slot = Args[2].Val;
		UNLOCK(Table);
		return FAILURE;
	} else {
		Result->Val = *(Result->Ref = Slot);
		UNLOCK(Table);
		return SUCCESS;
	};
};

METHOD("missing", TYP, T, ANY) {
//@t
//@Key
//@var=NIL
// If <var>Key</var> is present in <var>t</var> then fail, after assigning the current Value to <var>var</var> if provided.
// Otherwise, inserts the pair <code>(Key, NIL)</code> into <var>t</var> and returns an assignable reference.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	int Size = Table->Count;
	Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	if (Table->Count == Size) {
		if (Count > 2 && Args[2].Ref) *Args[2].Ref = *Slot;
		UNLOCK(Table);
		return FAILURE;
	} else {
		*Slot = Std$Object$Nil;
		Result->Val = *(Result->Ref = Slot);
		UNLOCK(Table);
		return SUCCESS;
	};
};

METHOD("[]", TYP, T, SKP) {
//@t
//@Key
// Returns the Value associated with <var>Key</var> in <var>t</var>.
	table_t *Table = (table_t *)Args[0].Val;
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	if (Table->Default) {
		WRLOCK(Table);
		Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		if (Slot[0] == 0) {
			Std$Function$status Status = Std$Function$call(Table->Default, 2, Result, Args[1].Val, 0, Args[0].Val, 0);
			if (Status > SUCCESS) {
				UNLOCK(Table);
				return Status;
			};
			Slot[0] = Result->Val;
		};
		Result->Val = *(Result->Ref = Slot);
		UNLOCK(Table);
		return SUCCESS;
	} else {
		RDLOCK(Table);
		Slot = avl_find_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		if (Slot != 0) {
			Result->Val = *(Result->Ref = Slot);
			UNLOCK(Table);
			return SUCCESS;
		} else {
			UNLOCK(Table);
			return FAILURE;
		};
	};
};

METHOD("[]", TYP, T, SKP, ANY) {
//@t
//@Key
//@default
// Returns the Value associated with <var>Key</var> in <var>t</var>.
// If <var>Key</var> is not present, a new pair <code>(Key, default)</code> is created and <var>default</var> is returned.
	table_t *Table = (table_t *)Args[0].Val;
	WRLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
	if (*Slot == 0) *Slot = Args[2].Val;
	Result->Val = *(Result->Ref = Slot);
	UNLOCK(Table);
	return SUCCESS;
};

METHOD(".", TYP, T, SKP) {
	table_t *Table = (table_t *)Args[0].Val;
	RDLOCK(Table);
	Std$Function_result Result1;
	Std$Object_t **Slot;
	Std$Function$call(Table->Hash, 1, &Result1, Args[1].Val, 0);
	if (Table->Default) {
		Slot = avl_probe_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		if (Slot[0] == 0) {
			Std$Function$status Status = Std$Function$call(Table->Default, 2, Result, Args[1].Val, 0, Args[0].Val, 0);
			if (Status > SUCCESS) {
				UNLOCK(Table);
				return Status;
			};
			Slot[0] = Result->Val;
		};
		Result->Val = *(Result->Ref = Slot);
		UNLOCK(Table);
		return SUCCESS;
	} else {
		Slot = avl_find_asm(Table, Args[1].Val, ((Std$Integer_smallt *)Result1.Val)->Value);
		if (Slot != 0) {
			Result->Val = *(Result->Ref = Slot);
			UNLOCK(Table);
			return SUCCESS;
		} else {
			UNLOCK(Table);
			return FAILURE;
		};
	};
};

METHOD("in", SKP, TYP, T) {
//@Key
//@table
// Returns <var>Key</var> if it is a Key in <var>table</var>, fails otherwise.
	table_t *Table = (table_t *)Args[1].Val;
	RDLOCK(Table);
	Std$Function_result Result1;
	Std$Function$call(Table->Hash, 1, &Result1, Args[0].Val, 0);
	if (avl_find_asm(Table, Args[0].Val, ((Std$Integer_smallt *)Result1.Val)->Value)) {
		Result->Arg = Args[0];
		UNLOCK(Table);
		return SUCCESS;
	} else {
		UNLOCK(Table);
		return FAILURE;
	};
};

STRING(LeftBrace, "{");
STRING(RightBrace, "}");
STRING(SpaceIsSpace, " is ");
STRING(CommaSpace, ", ");

METHOD("@", TYP, T, VAL, Std$String$T) {
	Std$Function_result Buffer;
	Std$Object_t *Final = LeftBrace;
	traverser_t Traverser;
	RDLOCK(Args[0].Val);
	node_t *Node = avl_t_first(&Traverser, (table_t *)Args[0].Val);
	if (Node != 0) {
		Std$Function$call($AT, 2, &Buffer, Node->Key, 0, Std$String$T, 0);
		Final = Std$String$add(Final, Buffer.Val);
		if (Node->Value != Std$Object$Nil) {
			Final = Std$String$add(Final, SpaceIsSpace);
			Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0);
			Final = Std$String$add(Final, Buffer.Val);
		};
	};
	for (Node = avl_t_next(&Traverser); Node; Node = avl_t_next(&Traverser)) {
		Final = Std$String$add(Final, CommaSpace);
		Std$Function$call($AT, 2, &Buffer, Node->Key, 0, Std$String$T, 0);
		Final = Std$String$add(Final, Buffer.Val);
		if (Node->Value != Std$Object$Nil) {
			Final = Std$String$add(Final, SpaceIsSpace);
			Std$Function$call($AT, 2, &Buffer, Node->Value, 0, Std$String$T, 0);
			Final = Std$String$add(Final, Buffer.Val);
		};
	};
	UNLOCK(Args[0].Val);
	Result->Val = Std$String$add(Final, RightBrace);
	return SUCCESS;
};

METHOD("key", TYP, NodeType) {
//@Node
//:ANY
// Returns <var>Key</var> from a <code>(Key, Value)</code> pair.
	Result->Val = ((node_t *)Args[0].Val)->Key;
	return SUCCESS;
};

METHOD("value", TYP, NodeType) {
//@Node
//:ANY
// Returns <var>Value</var> from a <code>(Key, Value)</code> pair.
	Result->Val = *(Result->Ref = &((node_t *)Args[0].Val)->Value);
	return SUCCESS;
};

typedef struct avl_resume_data {
	traverser_t *Traverser;
	Std$Function_argument Result;
} avl_resume_data;

static long resume_items_table(avl_resume_data *Data) {
	node_t *Node = avl_t_next(Data->Traverser);
	if (Node != 0) {
		Data->Result.Val = (Std$Object_t *)Node;
		Data->Result.Ref = 0;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("items", TYP, T) {
//@t
//:NodeT
// Generates the <code>(Key, Value)</code> pairs in <var>t</var>.
	traverser_t *Traverser = new(traverser_t);
	node_t *Node = avl_t_first(Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_items_table;
	if (Node != 0) {
		Result->Val = (Std$Object_t *)Node;
		Result->State = Traverser;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

typedef struct loop_state {
	TRAVERSER_FIELDS
	Std$Object_t **Key, **Value;
} loop_state;

typedef struct avl_resume_loop_data {
	loop_state *Traverser;
	Std$Function_argument Result;
} avl_resume_loop_data;

static long resume_loop_table(avl_resume_loop_data *Data) {
	node_t *Node = avl_t_next((traverser_t *)Data->Traverser);
	if (Node != 0) {
		*(Data->Traverser->Key) = Node->Key;
		*(Data->Traverser->Value) = Node->Value;
		Data->Result.Val = Node;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("loop", TYP, T, ANY, ANY) {
//@t
//@Key+
//@Value+
// For each <code>(Key, Value)</code> in <var>t</var>, generates <id>NIL</id> after assigning <var>Key</var> and <var>Value</var>.
	loop_state *Traverser = new(loop_state);
	node_t *Node = avl_t_first((traverser_t *)Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_loop_table;
	Traverser->Key = Args[1].Ref;
	Traverser->Value = Args[2].Ref;
	if (Node != 0) {
		*(Traverser->Key) = Node->Key;
		*(Traverser->Value) = Node->Value;
		Result->State = Traverser;
		Result->Val = Node;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

static long resume_keys_table(avl_resume_data *Data) {
	node_t *Node = avl_t_next(Data->Traverser);
	if (Node != 0) {
		Data->Result.Val = Node->Key;
		Data->Result.Ref = 0;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("keys", TYP, T) {
//@t
//:ANY
// Generates the keys in <var>t</var>.
	traverser_t *Traverser = new(traverser_t);
	node_t *Node = avl_t_first(Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_keys_table;
	if (Node != 0) {
		Result->Val = Node->Key;
		Result->State = Traverser;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

static long resume_values_table(avl_resume_data *Data) {
	node_t *Node = avl_t_next(Data->Traverser);
	if (Node != 0) {
		Data->Result.Val = *(Data->Result.Ref = &Node->Value);
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("values", TYP, T) {
//@t
//:ANY
// Generates the values in <var>t</var>.
	traverser_t *Traverser = new(traverser_t);
	node_t *Node = avl_t_first(Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_values_table;
	if (Node != 0) {
		Result->Val = *(Result->Ref = &Node->Value);
		Result->State = Traverser;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

typedef struct pair_state {
	TRAVERSER_FIELDS
	Std$Object_t **Pair;
} pair_state;

typedef struct avl_resume_pair_data {
	pair_state *Traverser;
	Std$Function_argument Result;
} avl_resume_pair_data;

static long resume_keys2_table(avl_resume_pair_data *Data) {
	node_t *Node = avl_t_next(Data->Traverser);
	if (Node != 0) {
		Data->Result.Val = Node->Key;
		Data->Result.Ref = 0;
		*(Data->Traverser->Pair) = Node->Value;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("keys", TYP, T, ANY) {
//@t
//value+
//:ANY
// Generates the keys in <var>t</var> while assigning the corresponding value to <var>value</var>.
	pair_state *Traverser = new(pair_state);
	node_t *Node = avl_t_first(Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_keys2_table;
	Traverser->Pair = Args[1].Ref;
	if (Node != 0) {
		Result->Val = Node->Key;
		Args[1].Ref[0] = Node->Value;
		Result->State = Traverser;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

static long resume_values2_table(avl_resume_pair_data *Data) {
	node_t *Node = avl_t_next(Data->Traverser);
	if (Node != 0) {
		Data->Result.Val = *(Data->Result.Ref = &Node->Value);
		*(Data->Traverser->Pair) = Node->Key;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

METHOD("values", TYP, T, ANY) {
//@t
//@key+
//:ANY
// Generates the values in <var>t</var> while assigning the corresponding key to <var>key</var>.
	pair_state *Traverser = new(pair_state);
	node_t *Node = avl_t_first(Traverser, (table_t *)Args[0].Val);
	Traverser->State.Run = Std$Function$resume_c;
	Traverser->State.Invoke = (Std$Function_cresumefn)resume_values_table;
	Traverser->Pair = Args[1].Ref;
	if (Node != 0) {
		Result->Val = *(Result->Ref = &Node->Value);
		Args[1].Ref[0] = Node->Key;
		Result->State = Traverser;
		return SUSPEND;
	} else {
		return FAILURE;
	};
};

typedef struct find_state {
	TRAVERSER_FIELDS
	Std$Object_t *Value;
} find_state;

typedef struct find_resume_data {
	find_state *State;
	Std$Function_argument Result;
} find_resume_data;

static long resume_find_table(find_resume_data *Data) {
	node_t *Node;
	while (Node = avl_t_next((traverser_t *)Data->State)) {
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Node->Value, 0, Data->State->Value, 0)) {
		case SUSPEND: case SUCCESS:
			Data->Result.Val = Node->Key;
			return SUSPEND;
		case MESSAGE:
			Data->Result.Val = Result0.Val;
			return MESSAGE;
		};
	};
	return FAILURE;
};

METHOD("find", TYP, T, SKP) {
//@table
//@Value
//:ANY
// Generates the all keys in <var>table</var> which has Value <var>Value</var>.
	find_state *State = new(find_state);
	for (node_t *Node = avl_t_first((traverser_t *)State, (table_t *)Args[0].Val); Node; Node = avl_t_next((traverser_t *)State)) {
		Std$Function_result Result0;
		switch (Std$Function$call($EQUAL, 2, &Result0, Args[1].Val, 0, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			State->State.Run = Std$Function$resume_c;
			State->State.Invoke = (Std$Function_cresumefn)resume_find_table;
			State->Value = Args[1].Val;
			Result->Val = Node->Key;
			Result->State = State;
			return SUSPEND;
		case MESSAGE:
			Result->Val = Result0.Val;
			return MESSAGE;
		};
	};
	return FAILURE;
};

size_t _size(Std$Object_t *Table) {
	return ((table_t *)Table)->Count;
};

size_t _generation(Std$Object_t *Table) {
	return ((table_t *)Table)->Generation;
};

METHOD("size", TYP, T) {
//@t
//:Std$Integer$SmallT
// Returns the number of entries in <var>t</var>.
	Result->Val = Std$Integer$new_small(((table_t *)Args[0].Val)->Count);
	return SUCCESS;
};

METHOD("Hash", TYP, T) {
//@t
//:Std$Function$T
// Returns the Hash function used in <var>t</var>.
	Result->Val = ((table_t *)Args[0].Val)->Hash;
	return SUCCESS;
};

METHOD("compare", TYP, T) {
//@t
//:Std$Function$T
// Returns the comparison function used in <var>t</var>.
	Result->Val = ((table_t *)Args[0].Val)->Compare;
	return SUCCESS;
};

METHOD("newv", TYP, Std$Type$T, TYP, T) {
//@type
//@fields
//:ANY
// Creates a new instance <var>instance</var> of <var>type</var> and for each <code>(Key, Value)</code> pair in <var>fields</var>, sets <code>Key(instance) &lt;- Value</code>. Returns <var>instance</var>.
// Each <var>Key</var> should be a <id>Std/Symbol/T</id>.
	Std$Function$call(Args[0].Val, 0, Result);
	Std$Object_t *Object = Result->Val;
	traverser_t Traverser;
	Std$Function_result Field;
	for (node_t *Node = avl_t_first(&Traverser, (table_t *)Args[1].Val); Node; Node = avl_t_next(&Traverser)) {
		if (Std$Function$call(Node->Key, 1, &Field, Object, 0) == MESSAGE) {
			Result->Val = Field.Val;
			return MESSAGE;
		};
		Field.Ref[0] = Node->Value;
	};
	return SUCCESS;
};

void _foreach(table_t *Table, int (*callback)(Std$Object$t *, Std$Object$t *, void *), void *Data) {
	traverser_t Trav[1];
	for (node_t *Node = avl_t_first(Trav, Table); Node; Node = avl_t_next(Trav)) {
		if (callback(Node->Key, Node->Value, Data)) return;
	};
};

traverser_t *_trav_new(void) {
	return new(traverser_t);
};

node_t *_trav_first(traverser_t *Trav, table_t *Table) {
	return avl_t_first(Trav, Table);
};

node_t *_trav_next(traverser_t *Trav) {
	return avl_t_next(Trav);
};

Std$Object_t *_node_key(node_t *Node) {
	return Node->Key;
};

Std$Object_t *_node_value(node_t *Node) {
	return Node->Value;
};

Std$Object_t **_node_value_ref(node_t *Node) {
	return &Node->Value;
};

unsigned long _node_hash(node_t *Node) {
	return Node->Hash;
};

