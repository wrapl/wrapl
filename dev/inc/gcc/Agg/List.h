#ifndef AGG_LIST_H
#define AGG_LIST_H

#include <Std.h>

#define RIVA_MODULE Agg$List
#include <Riva-Header.h>

RIVA_STRUCT(node) {
	Std$Object_t *Value;
	Agg$List_node *Next, *Prev;
};

RIVA_STRUCT(t) {
	Std$Type_t const *Type;
	Agg$List_node *Head, *Tail, *Cache, **Array;
	unsigned long Length;
	unsigned long Index, Lower, Upper, Access;
};

RIVA_TYPE(T);
RIVA_OBJECT(New);
RIVA_OBJECT(Make);

RIVA_CFUN(Std$Object_t *, new, long, ...)  __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, newv, long, Std$Object_t **)  __attribute__ ((malloc));
RIVA_CFUN(Std$Object_t *, new0)  __attribute__ ((malloc));
RIVA_CFUN(void, empty, Std$Object_t *);
RIVA_CFUN(void, push, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(void, put, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(Std$Object_t *, pop, Std$Object_t *);
RIVA_CFUN(Std$Object_t *, pull, Std$Object_t *);
RIVA_CFUN(Agg$List_node *, find_node, Agg$List_t *, long);

#define Agg$List$length(L) ((Agg$List_t *)L)->Length
#define Agg$List$head(L) ((Agg$List_t *)L)->Head
#define Agg$List$tail(L) ((Agg$List_t *)L)->Tail

#undef RIVA_MODULE

#endif
