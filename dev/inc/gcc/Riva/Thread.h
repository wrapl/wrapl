#ifndef RIVA_THREAD_H
#define RIVA_THREAD_H

#define RIVA_MODULE Riva$Thread
#include <Riva-Header.h>

RIVA_STRUCT(t);
RIVA_STRUCT(key);
RIVA_STRUCT(mutex);
RIVA_STRUCT(cond);
RIVA_STRUCT(rwlock);
RIVA_STRUCT(timeval);

RIVA_CFUN(Riva$Thread$t *, new, void *(*)(void *), void *);
RIVA_CFUN(Riva$Thread$t *, create, void *, void *(*)(void *), void *);
RIVA_CFUN(Riva$Thread$t *, self);
RIVA_CFUN(void, exit, void *);
RIVA_CFUN(int, join, Riva$Thread$t *, void **);
RIVA_CFUN(int, equal, Riva$Thread$t *, Riva$Thread$t *);
RIVA_CFUN(int, signal, Riva$Thread$t *, int);

RIVA_CFUN(Riva$Thread$key *, key_new, void (*)(void *));
RIVA_CFUN(void, key_free, Riva$Thread$key *);
RIVA_CFUN(void *, key_get, Riva$Thread$key *);
RIVA_CFUN(void, key_set, Riva$Thread$key *, void *);

RIVA_CFUN(Riva$Thread$mutex *, mutex_new, void);
RIVA_CFUN(void, mutex_free, Riva$Thread$mutex *);
RIVA_CFUN(int, mutex_lock, Riva$Thread$mutex *);
RIVA_CFUN(int, mutex_trylock, Riva$Thread$mutex *);
RIVA_CFUN(int, mutex_unlock, Riva$Thread$mutex *);

RIVA_CFUN(Riva$Thread$cond *, cond_new, void);
RIVA_CFUN(int, cond_signal, Riva$Thread$cond *);
RIVA_CFUN(int, cond_broadcast, Riva$Thread$cond *);
RIVA_CFUN(int, cond_wait, Riva$Thread$cond *, Riva$Thread$mutex *);
RIVA_CFUN(int, cond_timedwait, Riva$Thread$cond *, Riva$Thread$mutex *, Riva$Thread$timeval *);
RIVA_CFUN(void, cond_free, Riva$Thread$cond *);

RIVA_CFUN(Riva$Thread$rwlock *, rwlock_new, void);
RIVA_CFUN(int, rwlock_rdlock, Riva$Thread$rwlock *);
RIVA_CFUN(int, rwlock_tryrdlock, Riva$Thread$rwlock *);
RIVA_CFUN(int, rwlock_wrlock, Riva$Thread$rwlock *);
RIVA_CFUN(int, rwlock_trywrlock, Riva$Thread$rwlock *);
RIVA_CFUN(int, rwlock_unlock, Riva$Thread$rwlock *);
RIVA_CFUN(int, rwlock_free, Riva$Thread$rwlock *);

#undef RIVA_MODULE

#endif
