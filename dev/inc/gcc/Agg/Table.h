#ifndef AGG_TABLE_H
#define AGG_TABLE_H

#include <Std.h>

#define RIVA_MODULE Agg$Table
#include <Riva-Header.h>

RIVA_TYPE(T);

RIVA_CFUN(Std$Object$t *, new, Std$Object$t *, Std$Object$t *)  __attribute__ ((malloc));
RIVA_CFUN(void, insert, Std$Object$t *, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(Std$Object$t **, slot, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(int, missing, Std$Object$t *, Std$Object$t ***);
RIVA_CFUN(int, delete, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(Std$Object$t *, index, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(Std$Object$t **, probe, Std$Object$t *, Std$Object$t *);
RIVA_CFUN(size_t, size, Std$Object$t *);
RIVA_CFUN(size_t, generation, Std$Object$t *);
RIVA_CFUN(int, foreach, Std$Object$t *, int (*)(Std$Object$t *, Std$Object$t *, void *), void *);

RIVA_OBJECT(New);
RIVA_OBJECT(Make);

RIVA_STRUCT(trav);

RIVA_CFUN(Agg$Table$trav *, trav_new);
RIVA_CFUN(Std$Object$t *, trav_first, Agg$Table$trav *, Std$Object$t *);
RIVA_CFUN(Std$Object$t *, trav_next, Agg$Table$trav *);

RIVA_CFUN(Std$Object$t *, node_key, Std$Object$t *)  __attribute__ ((const));
RIVA_CFUN(Std$Object$t *, node_value, Std$Object$t *)  __attribute__ ((pure));
RIVA_CFUN(Std$Object$t **, node_value_ref, Std$Object$t *)  __attribute__ ((pure));
RIVA_CFUN(unsigned long, node_hash, Std$Object$t *)  __attribute__ ((const));

#undef RIVA_MODULE

#endif
