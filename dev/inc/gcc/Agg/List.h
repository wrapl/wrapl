#ifndef AGG_LIST_H
#define AGG_LIST_H

#include <Std.h>

#define RIVA_MODULE Agg$List
#include <Riva-Header.h>

RIVA_STRUCT(node) {
	Std$Object$t *Value;
	Agg$List$node *Next, *Prev;
};

RIVA_STRUCT(t) {
	Std$Type$t const *Type;
	Agg$List$node *Head, *Tail, *Cache, **Array;
	unsigned long Length;
	unsigned long Index, Lower, Upper, Access;
};

RIVA_TYPE(T);
RIVA_OBJECT(New);
RIVA_OBJECT(Make);

RIVA_CFUN(Std$Object$t *, new, long, ...)  __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, newv, long, Std$Object$t **)  __attribute__ ((malloc));
RIVA_CFUN(Std$Object$t *, new0)  __attribute__ ((malloc));
RIVA_CFUN(void, empty, Std$Object$t *);
RIVA_CFUN(void, push, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(void, put, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(Std$Object$t *, pop, Std$Object$t *);
RIVA_CFUN(Std$Object$t *, pull, Std$Object$t *);
RIVA_CFUN(Agg$List$node *, find_node, Agg$List$t *, long);

#define Agg$List$length(L) ((Agg$List$t *)L)->Length
#define Agg$List$head(L) ((Agg$List$t *)L)->Head
#define Agg$List$tail(L) ((Agg$List$t *)L)->Tail

#undef RIVA_MODULE

#endif
