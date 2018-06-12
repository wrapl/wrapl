#include "libriva.h"

#include <stdint.h>
#include <signal.h>
#include <string.h>

#ifdef LINUX

#include <pthread.h>

static void *thread_new(void *(*Func)(void *), void *Arg) {
	pthread_t Thread;
	GC_pthread_create(&Thread, 0, Func, Arg);
	return (void *)Thread;
};

static void *thread_create(void *Attrs, void *(*Func)(void *), void *Arg) {
	pthread_t Thread;
	GC_pthread_create(&Thread, Attrs, Func, Arg);
	return (void *)Thread;
};

static void *thread_key_new(void (*Func)(void *)) {
	pthread_key_t Key;
	pthread_key_create(&Key, Func);
	return (void *)Key;
};

static void *thread_mutex_new(void) {
	pthread_mutex_t *Mutex = new(pthread_mutex_t);
	pthread_mutex_init(Mutex, 0);
	return (void *)Mutex;
};

static void *thread_cond_new(void) {
	pthread_cond_t *Cond = new(pthread_cond_t);
	pthread_cond_init(Cond, 0);
	return (void *)Cond;
};

static void *thread_rwlock_new(void) {
	pthread_rwlock_t *RwLock = new(pthread_rwlock_t);
	pthread_rwlock_init(RwLock, 0);
	return (void *)RwLock;
};

void thread_init(void) {
	module_t *Module = module_new("", "Riva/Thread");
	module_add_alias(Module, "library:/Riva/Thread");
	module_export(Module, "_new", 0, thread_new);
	module_export(Module, "_create", 0, thread_create);
	module_export(Module, "_self", 0, pthread_self);
	module_export(Module, "_equal", 0, pthread_equal);
	module_export(Module, "_join", 0, pthread_join);
	module_export(Module, "_exit", 0, pthread_exit);
	module_export(Module, "_signal", 0, pthread_kill);
	module_export(Module, "_key_new", 0, thread_key_new);
	module_export(Module, "_key_set", 0, pthread_setspecific);
	module_export(Module, "_key_get", 0, pthread_getspecific);
	module_export(Module, "_key_del", 0, pthread_key_delete);
	module_export(Module, "_mutex_new", 0, thread_mutex_new);
	module_export(Module, "_mutex_lock", 0, pthread_mutex_lock);
	module_export(Module, "_mutex_trylock", 0, pthread_mutex_trylock);
	module_export(Module, "_mutex_unlock", 0, pthread_mutex_unlock);
	module_export(Module, "_mutex_free", 0, pthread_mutex_destroy);
	module_export(Module, "_cond_new", 0, thread_cond_new);
	module_export(Module, "_cond_signal", 0, pthread_cond_signal);
	module_export(Module, "_cond_broadcast", 0, pthread_cond_broadcast);
	module_export(Module, "_cond_wait", 0, pthread_cond_wait);
	module_export(Module, "_cond_timedwait", 0, pthread_cond_timedwait);
	module_export(Module, "_cond_free", 0, pthread_cond_destroy);
	module_export(Module, "_rwlock_new", 0, thread_rwlock_new);
	module_export(Module, "_rwlock_rdlock", 0, pthread_rwlock_rdlock);
	module_export(Module, "_rwlock_tryrdlock", 0, pthread_rwlock_tryrdlock);
	module_export(Module, "_rwlock_wrlock", 0, pthread_rwlock_wrlock);
	module_export(Module, "_rwlock_trywrlock", 0, pthread_rwlock_trywrlock);
	module_export(Module, "_rwlock_unlock", 0, pthread_rwlock_unlock);
	module_export(Module, "_rwlock_free", 0, pthread_rwlock_destroy);
};

#else

#include <windows.h>
#include <gc/gc.h>

static HANDLE ThreadMutex;

typedef struct thread_init_t {
    void *(*StartFunc)(void *);
    void *Arg;
} thread_init_t;

static unsigned long __stdcall ThreadFunc(thread_init_t *Init) {
    Init->StartFunc(Init->Arg);
};

static void *thread_new(void *(*StartFunc)(void *), void *Arg) {
    thread_init_t Init = {StartFunc, Arg};
    unsigned long ThreadID;
	void *Thread = CreateThread(0, 0, ThreadFunc, &Init, 0, &ThreadID);
	return (void *)Thread;
};

static void *thread_self() {
};

static void *thread_join(void *Thread) {
};

static void *thread_key_new(void (*Func)(void *)) {
	return (void *)TlsAlloc();
};

static void thread_key_del(void *Key) {
};

static void *thread_key_get(void *Key) {
};

static void thread_key_set(void *Key, void *Value) {
};

static void *thread_mutex_new(void) {
	return CreateMutex(0, 0, 0);
};

static void thread_mutex_del(void *Mutex) {
};

static int thread_mutex_lock(void *Mutex) {
};

static int thread_mutex_trylock(void *Mutex) {
};

static int thread_mutex_unlock(void *Mutex) {
};

void thread_init(void) {
    //ThreadMutex = CreateMutex(0, 0, 0);
	module_t *Module = module_alias("", "Riva/Thread");
	module_export(Module, "_new", 0, thread_new);
	module_export(Module, "_self", 0, thread_self);
	module_export(Module, "_join", 0, thread_join);
	module_export(Module, "_key_new", 0, thread_key_new);
	module_export(Module, "_key_set", 0, thread_key_set);
	module_export(Module, "_key_get", 0, thread_key_get);
	module_export(Module, "_key_del", 0, thread_key_del);
	module_export(Module, "_mutex_new", 0, thread_mutex_new);
	module_export(Module, "_mutex_lock", 0, thread_mutex_lock);
	module_export(Module, "_mutex_trylock", 0, thread_mutex_trylock);
	module_export(Module, "_mutex_unlock", 0, thread_mutex_unlock);
	module_export(Module, "_mutex_del", 0, thread_mutex_del);
};

#endif
