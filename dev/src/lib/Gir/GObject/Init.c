#include <Riva.h>
#include <Std.h>
#include <Sys/Service.h>
#include <Sys/Module.h>
#include <dlfcn.h>

#include <glib-object.h>

#include <stdio.h>

#if defined(LINUX) || defined(CYGWIN)
#	include <pthread.h>
#	include <sched.h>
#	include <errno.h>
#endif

static void *__malloc(gsize N) {
	if (N > 65536) {
		return Riva$Memory$alloc_atomic(N);
	} else if (N > 4096) {
		return Riva$Memory$alloc_large(N);
	} else {
		return Riva$Memory$alloc(N);
	};
};
static void *__realloc(void *P, gsize N) {
	if (Riva$Memory$base(P) == 0) return Riva$Memory$realloc(P, N);
	size_t O = Riva$Memory$size(P);
	if (N <= 4096) return Riva$Memory$realloc(P, N);
	//if (N <= O) return P;
	void *R = Riva$Memory$alloc_large(N);
	if (R == 0) return 0;
	memcpy(R, P, O);
	Riva$Memory$free(P);
	return R;
};
static void __free(void *P) {};
static void *__calloc(gsize N, gsize M) {return __malloc(N * M);};

static GMemVTable MemVTable = {
	(void *)__malloc,
	(void *)__realloc,
	(void *)__free,
	(void *)__calloc,
	(void *)__malloc,
	(void *)__realloc
};

static gint g_thread_priority_map[G_THREAD_PRIORITY_URGENT + 1] = {
	[G_THREAD_PRIORITY_LOW] = 0,
	[G_THREAD_PRIORITY_NORMAL] = 0,
	[G_THREAD_PRIORITY_HIGH] = 0,
	[G_THREAD_PRIORITY_URGENT] = 0
};

static GMutex *mutex_new(void) {
	pthread_mutex_t *Mutex = new(pthread_mutex_t);
	pthread_mutex_init(Mutex, 0);
	return (GMutex *)Mutex;
};

static int mutex_trylock(GMutex *mutex) {
	int result = pthread_mutex_trylock(mutex);
	if (result == EBUSY) return FALSE;
	return TRUE;
};

static GCond *cond_new(void) {
	pthread_cond_t *Cond = new(pthread_cond_t);
	pthread_cond_init(Cond, 0);
	return (GCond *)Cond;
};

static gboolean cond_timed_wait(GCond *cond, GMutex *entered_mutex, GTimeVal *abs_time) {
	int result;
	struct timespec end_time;
	gboolean timed_out;
	if (!abs_time) {
		result = pthread_cond_wait(cond, entered_mutex);
		timed_out = FALSE;
	} else {
		end_time.tv_sec = abs_time->tv_sec;
		end_time.tv_nsec = abs_time->tv_usec * 1000;
		result = pthread_cond_timedwait(cond, entered_mutex, &end_time);
		timed_out = (result == ETIMEDOUT);
    };
	return !timed_out;
};

static GPrivate *private_new(GDestroyNotify destructor) {
	pthread_key_t Key;
	pthread_key_create(&Key, destructor);
	return (GPrivate *)Key;
};

static void thread_create(GThreadFunc func, gpointer data, gulong stack_size, gboolean joinable, gboolean bound, GThreadPriority priority, gpointer thread, GError **error) {
	//printf("Trying to create a thread...\n");
#if defined(LINUX) || defined(CYGWIN)
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (stack_size) pthread_attr_setstacksize(&attr, stack_size);
	if (bound) pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
	struct sched_param sched;
    pthread_attr_getschedparam(&attr, &sched);
    sched.sched_priority = g_thread_priority_map[priority];
    pthread_attr_setschedparam(&attr, &sched);
	pthread_create(&thread, &attr, func, data);
    pthread_attr_destroy(&attr);
#endif
};

static void thread_join(gpointer thread) {
	gpointer ignore;
	pthread_join(thread, &ignore);
};

static void thread_set_priority(gpointer thread, GThreadPriority priority) {
#if defined(LINUX) || defined(CYGWIN)
	struct sched_param sched;
    int policy;
    pthread_getschedparam(*(pthread_t*)thread, &policy, &sched);
    sched.sched_priority = g_thread_priority_map[priority];
    pthread_setschedparam(*(pthread_t*)thread, policy, &sched);
#endif
};

static void thread_self(pthread_t **thread) {
	*thread = pthread_self();
};

static int thread_equal(pthread_t ** a, pthread_t ** b) {
	return pthread_equal(*a, *b) != 0;
};

static GThreadFunctions ThreadVTable = {
	.mutex_new = mutex_new,
	.mutex_lock = pthread_mutex_lock,
	.mutex_trylock = mutex_trylock,
	.mutex_unlock = pthread_mutex_unlock,
	.mutex_free = pthread_mutex_destroy,
	.cond_new = cond_new,
	.cond_signal = pthread_cond_signal,
	.cond_broadcast = pthread_cond_broadcast,
	.cond_wait = pthread_cond_wait,
	.cond_timed_wait = cond_timed_wait,
	.cond_free = pthread_cond_destroy,
	.private_new = private_new,
	.private_get = pthread_getspecific,
	.private_set = pthread_setspecific,
	.thread_create = thread_create,
	.thread_yield = sched_yield,
	.thread_join = thread_join,
	.thread_exit = pthread_exit,
	.thread_set_priority = thread_set_priority,
	.thread_self = thread_self,
	.thread_equal = thread_equal
};

INITIAL(Riva$Module$provider_t *Provider) {
	Sys$Service_t *Service = Sys$Service$new("gobject");
	if (Service) {
		//printf("Initializing gobject system...\n");
		//g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, 1);
		//g_mem_set_vtable(&MemVTable);
		
		char Buffer[1024];
		strcpy(stpcpy(Buffer, Riva$Module$get_path(Provider->Module)), "/libfontconfig.so");
		//printf("Buffer = <%s>\n", Buffer);
		void *Handle = dlopen(Buffer, RTLD_LAZY | RTLD_GLOBAL);
		if (!Handle) {
			printf("Error = %s\n", dlerror());
			printf("fontconfig = %x\n", Handle);
		}
		
#if defined(LINUX) || defined(CYGWIN)
		struct sched_param sched;
		int policy;
		pthread_getschedparam(pthread_self(), &policy, &sched);
		g_thread_priority_map[G_THREAD_PRIORITY_LOW] = sched_get_priority_min(policy);
		g_thread_priority_map[G_THREAD_PRIORITY_URGENT] = sched_get_priority_max(policy);
		g_thread_priority_map[G_THREAD_PRIORITY_NORMAL] = sched.sched_priority;
		g_thread_priority_map[G_THREAD_PRIORITY_HIGH] = ((sched.sched_priority + sched_get_priority_max(policy)) * 2) / 3;
#endif
		//printf("Initializing glib threads...\n");
		if (!g_thread_supported()) g_thread_init(0);
		g_type_init();
		//printf("done.\n");
		Sys$Service$start(Service, Std$Object$Nil);
	};
};
