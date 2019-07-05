#ifndef AGG_TABLE_H
#define AGG_TABLE_H

#include <Std.h>

#define RIVA_MODULE Agg$Table
#include <Riva-Header.h>

RIVA_TYPE(T);

RIVA_CFUN(Std$Object_t *, new, Std$Object_t *, Std$Object_t *)  __attribute__ ((malloc));
RIVA_CFUN(void, insert, Std$Object_t *, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(int, missing, Std$Object_t *, Std$Object_t ***);
RIVA_CFUN(int, delete, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(Std$Object_t *, index, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(Std$Object_t **, probe, Std$Object_t *, Std$Object_t *);
RIVA_CFUN(size_t, size, Std$Object_t *);
RIVA_CFUN(size_t, generation, Std$Object_t *);
RIVA_CFUN(int, foreach, Std$Object$t *, int (*)(Std$Object$t *, Std$Object$t *, void *), void *);

RIVA_OBJECT(New);
RIVA_OBJECT(Make);

RIVA_STRUCT(trav);

RIVA_CFUN(Agg$Table_trav *, trav_new);
RIVA_CFUN(Std$Object_t *, trav_first, Agg$Table_trav *, Std$Object_t *);
RIVA_CFUN(Std$Object_t *, trav_next, Agg$Table_trav *);

RIVA_CFUN(Std$Object_t *, node_key, Std$Object_t *)  __attribute__ ((const));
RIVA_CFUN(Std$Object_t *, node_value, Std$Object_t *)  __attribute__ ((pure));
RIVA_CFUN(Std$Object_t **, node_value_ref, Std$Object_t *)  __attribute__ ((pure));
RIVA_CFUN(unsigned long, node_hash, Std$Object_t *)  __attribute__ ((const));

#undef RIVA_MODULE

#endif
