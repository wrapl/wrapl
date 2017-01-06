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

RIVA_CFUN(Riva$Thread_t *, new, void *(*)(void *), void *);
RIVA_CFUN(Riva$Thread_t *, create, void *, void *(*)(void *), void *);
RIVA_CFUN(Riva$Thread_t *, self);
RIVA_CFUN(void, exit, void *);
RIVA_CFUN(int, join, Riva$Thread_t *, void **);
RIVA_CFUN(int, equal, Riva$Thread_t *, Riva$Thread_t *);
RIVA_CFUN(int, signal, Riva$Thread_t *, int);

RIVA_CFUN(Riva$Thread_key *, key_new, void (*)(void *));
RIVA_CFUN(void, key_free, Riva$Thread_key *);
RIVA_CFUN(void *, key_get, Riva$Thread_key *);
RIVA_CFUN(void, key_set, Riva$Thread_key *, void *);

RIVA_CFUN(Riva$Thread_mutex *, mutex_new, void);
RIVA_CFUN(void, mutex_free, Riva$Thread_mutex *);
RIVA_CFUN(int, mutex_lock, Riva$Thread_mutex *);
RIVA_CFUN(int, mutex_trylock, Riva$Thread_mutex *);
RIVA_CFUN(int, mutex_unlock, Riva$Thread_mutex *);

RIVA_CFUN(Riva$Thread_cond *, cond_new, void);
RIVA_CFUN(int, cond_signal, Riva$Thread_cond *);
RIVA_CFUN(int, cond_broadcast, Riva$Thread_cond *);
RIVA_CFUN(int, cond_wait, Riva$Thread_cond *, Riva$Thread_mutex *);
RIVA_CFUN(int, cond_timedwait, Riva$Thread_cond *, Riva$Thread_mutex *, Riva$Thread_timeval *);
RIVA_CFUN(void, cond_free, Riva$Thread_cond *);

RIVA_CFUN(Riva$Thread_rwlock *, rwlock_new, void);
RIVA_CFUN(int, rwlock_rdlock, Riva$Thread_rwlock *);
RIVA_CFUN(int, rwlock_tryrdlock, Riva$Thread_rwlock *);
RIVA_CFUN(int, rwlock_wrlock, Riva$Thread_rwlock *);
RIVA_CFUN(int, rwlock_trywrlock, Riva$Thread_rwlock *);
RIVA_CFUN(int, rwlock_unlock, Riva$Thread_rwlock *);
RIVA_CFUN(int, rwlock_free, Riva$Thread_rwlock *);

#undef RIVA_MODULE

#endif
