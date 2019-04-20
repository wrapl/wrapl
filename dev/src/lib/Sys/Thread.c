#include <Std.h>
#include <Riva/System.h>
#include <Riva/Memory.h>
#include <Sys/Signal.h>
#include <Agg/Table.h>
#include <stdio.h>
#include <unistd.h>

TYPE(T);
TYPE(KeyT);
TYPE(MutexT);
TYPE(SemaphoreT);
TYPE(CondT);
TYPE(RWLockT);
TYPE(QueueT, MutexT);
TYPE(GateT, MutexT);

/*#ifdef WINDOWS

typedef struct thread_t {
	const Std$Type$t *Type;
} thread_t;

typedef struct mutex_t {
	const Std$Type$t *Type;
} mutex_t;

typedef struct cond_t {
	const Std$Type$t *Type;
} cond_t;

typedef struct rwlock_t {
	const Std$Type$t *Type;
} rwlock_t;

typedef struct key_t {
	const Std$Type$t *Type;
} key_t;

GLOBAL_FUNCTION(New, 1) {
};

GLOBAL_FUNCTION(Self, 0) {
};

GLOBAL_FUNCTION(MutexNew, 0) {
};

GLOBAL_METHOD(MutexLock, 1, "lock", TYP, MutexT) {
};

GLOBAL_METHOD(MutexTryLock, 1, "trylock", TYP, MutexT) {
};

GLOBAL_METHOD(MutexUnlock, 1, "unlock", TYP, MutexT) {
};

GLOBAL_FUNCTION(CondNew, 0) {
};

GLOBAL_METHOD(CondWait, 2, "wait", TYP, CondT, TYP, MutexT) {
};

GLOBAL_METHOD(CondSignal, 1, "signal", TYP, CondT) {
};

GLOBAL_METHOD(CondBroadcast, 1, "broadcast", TYP, CondT) {
};

GLOBAL_FUNCTION(RWLockNew, 0) {
};

GLOBAL_METHOD(RWLockRdLock, 1, "rdlock", TYP, RWLockT) {
};

GLOBAL_METHOD(RWLockWrLock, 1, "wrlock", TYP, RWLockT) {
};

GLOBAL_METHOD(RWLockUnlock, 1, "unlock", TYP, RWLockT) {
};

GLOBAL_FUNCTION(KeyNew, 0) {
};

GLOBAL_METHOD(KeyGet, 1, "get", TYP, KeyT) {
};

GLOBAL_METHOD(KeySet, 2, "set", TYP, KeyT, SKP) {
};

typedef struct queue_node {
} queue_node;

typedef struct queue_t {
} queue_t;

GLOBAL_FUNCTION(QueueNew, 0) {
};

GLOBAL_METHOD(QueuePut, 2, "put", TYP, QueueT, ANY) {
};

GLOBAL_METHOD(QueuePush, 2, "push", TYP, QueueT, ANY) {
};

GLOBAL_METHOD(QueuePop, 1, "pop", TYP, QueueT) {
};

GLOBAL_METHOD(QueuePeek, 1, "peek", TYP, QueueT) {
};

GLOBAL_METHOD(QueueLength, 1, "length", TYP, QueueT) {
};

GLOBAL_FUNCTION(Sleep, 1) {
    sleep(((Std$Integer_smallt *)Args[0].Val)->Value);
    return SUCCESS;
};

INITIAL() {
};

#else*/

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

static pthread_key_t ThreadKey;

typedef struct thread_t {
	const Std$Type$t *Type;
	pthread_t Handle;
	union {
		struct {
			Std$Object$t *Data;
			Std$Function$result Result;
			Std$Function$status Status;
		};
		struct {
			Std$Object$t *Function;
			Std$Function$argument *Args;
			unsigned long Count;
		};
	};
} thread_t;

typedef struct mutex_t {
	const Std$Type$t *Type;
	pthread_mutex_t Handle;
} mutex_t;

typedef struct semaphore_t {
	const Std$Type$t *Type;
	sem_t Handle;
} semaphore_t;

typedef struct cond_t {
	const Std$Type$t *Type;
	pthread_cond_t Handle;
} cond_t;

typedef struct rwlock_t {
	const Std$Type$t *Type;
	pthread_rwlock_t Handle;
} rwlock_t;

typedef struct Gate_t {
	const Std$Type$t *Type;
	pthread_mutex_t Mutex;
	pthread_cond_t Condition;
	Std$Function$argument Result;
	Std$Function$status Status;
} Gate_t;

#ifdef WINDOWS
#define key_t _key_t
#endif

typedef struct thread_key_t {
	const Std$Type$t *Type;
	pthread_key_t Handle;
} thread_key_t;

static void *thread_func(thread_t *Thread) {
	pthread_setspecific(ThreadKey, Thread);
	Std$Function_argument Args[Thread->Count];
	Std$Object$t *Function = Thread->Function;
	unsigned long Count = Thread->Count;
	memcpy(Args, Thread->Args, Count * sizeof(Std$Function_argument));
	Thread->Data = Std$Object$Nil;
	Thread->Status = FAILURE;
	Thread->Status = Std$Function$invoke(Function, Count, &Thread->Result, Args);
};

thread_t *thread_self(void) {
	thread_t *Thread = pthread_getspecific(ThreadKey);
	//printf("Thread = %x\n", Thread);
	if (Thread == 0) {
		Thread = new(thread_t);
		Thread->Type = T;
		Thread->Handle = pthread_self();
		Thread->Data = Std$Object$Nil;
		Thread->Status = FAILURE;
		pthread_setspecific(ThreadKey, Thread);
	};
	return Thread;
};

AMETHOD(Std$String$Of, TYP, T) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	char *Buffer;
	Result->Val = Std$String$new_length(Buffer, asprintf(&Buffer, "<thread @ %x>", Thread->Handle));
	return SUCCESS;
};

GLOBAL_FUNCTION(New, 1) {
	thread_t *Thread = new(thread_t);
	Thread->Type = T;
	Thread->Function = Args[Count - 1].Val;
	Thread->Count = Count - 1;
	Thread->Args = (Std$Function_argument *)Riva$Memory$alloc(Thread->Count * sizeof(Std$Function_argument));
	memcpy(Thread->Args, Args, Thread->Count * sizeof(Std$Function_argument));
	pthread_create(&Thread->Handle, 0, thread_func, (void *)Thread);
	//printf("\e[32mCreating thread <thread @ %x>\e[0m\n", Thread);
	Result->Val = (Std$Object$t *)Thread;
	return SUCCESS;
};

GLOBAL_FUNCTION(Detach, 0) {
	thread_t *Thread;
	if (Count > 0) {
		CHECK_EXACT_ARG_TYPE(0, T);
		Thread = (thread_t *)Args[0].Val;
	} else {
		Thread = thread_self();
	};
	pthread_detach(Thread->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("detach", TYP, T) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	pthread_detach(Thread->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(Self, 0) {
	Result->Val = (Std$Object$t *)thread_self();
	return SUCCESS;
};

GLOBAL_FUNCTION(Errno, 0) {
	Result->Val = Std$Integer$new_small(Riva$System$get_errno());
	return SUCCESS;
};

GLOBAL_FUNCTION(Data, 0) {
	thread_t *Thread;
	if (Count > 0) {
		CHECK_EXACT_ARG_TYPE(0, T);
		Thread = (thread_t *)Args[0].Val;
	} else {
		Thread = thread_self();
	};
	Result->Val = *(Result->Ref = &Thread->Data);
	return SUCCESS;
};

METHOD("data", TYP, T) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	Result->Val = *(Result->Ref = &Thread->Data);
	return SUCCESS;
};

GLOBAL_FUNCTION(Result, 0) {
	thread_t *Thread;
	if (Count > 0) {
		CHECK_EXACT_ARG_TYPE(0, T);
		Thread = (thread_t *)Args[0].Val;
	} else {
		Thread = thread_self();
	};
	*Result = Thread->Result;
	return Thread->Status;
};

GLOBAL_METHOD(SetSuccess, 2, "success", TYP, T, ANY) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	Thread->Result.Arg = Args[1];
	Thread->Status = SUCCESS;
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(SetFailure, 1, "failure", TYP, T) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	Thread->Status = FAILURE;
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(SetMessage, 2, "message", TYP, T, ANY) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	Thread->Result.Arg = Args[1];
	Thread->Status = MESSAGE;
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(SetResult, 3, "result", TYP, T, TYP, Std$Integer$SmallT, ANY) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	Thread->Result.Arg = Args[2];
	Thread->Status = Std$Integer$get_small(Args[1].Val);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("result", TYP, T) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	*Result = Thread->Result;
	return Thread->Status;
};

GLOBAL_FUNCTION(Return, 1) {
	thread_t *Thread = thread_self();
	Thread->Result.Arg = Args[0];
	Thread->Status = SUCCESS;
	pthread_exit(0);
};

GLOBAL_FUNCTION(Fail, 0) {
	thread_t *Thread = thread_self();
	Thread->Status = FAILURE;
	pthread_exit(0);
};

GLOBAL_FUNCTION(Send, 1) {
	thread_t *Thread = thread_self();
	Thread->Result.Arg = Args[0];
	Thread->Status = MESSAGE;
	pthread_exit(0);
};

GLOBAL_METHOD(Signal, 2, "signal", TYP, T, TYP, Std$Integer$SmallT) {
	thread_t *Thread = (thread_t *)Args[0].Val;
	//printf("\e[32mThread.Signal(<thread @ %x>, %d)\e[0m\n", Thread, Std$Integer$get_small(Args[1].Val));
	pthread_kill(Thread->Handle, Std$Integer$get_small(Args[1].Val));
	return SUCCESS;
};

GLOBAL_FUNCTION(Block, 1) {
	CHECK_EXACT_ARG_TYPE(0, Sys$Signal$SetT);
	Sys$Signal$set_t *Sigset = (Sys$Signal$set_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		Sys$Signal$set_t *OldSet = new(Sys$Signal$set_t);
		OldSet->Type = Sys$Signal$SetT;
		if (pthread_sigmask(SIG_BLOCK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (pthread_sigmask(SIG_BLOCK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Unblock, 1) {
	CHECK_EXACT_ARG_TYPE(0, Sys$Signal$SetT);
	Sys$Signal$set_t *Sigset = (Sys$Signal$set_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		Sys$Signal$set_t *OldSet = new(Sys$Signal$set_t);
		OldSet->Type = Sys$Signal$SetT;
		if (pthread_sigmask(SIG_UNBLOCK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (pthread_sigmask(SIG_UNBLOCK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(SetMask, 1) {
	CHECK_EXACT_ARG_TYPE(0, Sys$Signal$SetT);
	Sys$Signal$set_t *Sigset = (Sys$Signal$set_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		Sys$Signal$set_t *OldSet = new(Sys$Signal$set_t);
		OldSet->Type = Sys$Signal$SetT;
		if (pthread_sigmask(SIG_SETMASK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (pthread_sigmask(SIG_SETMASK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};

#ifdef DOCUMENTING
#define MutexTypeT ?MutexType.T
#else
TYPE(MutexTypeT, Std$Integer$SmallT);
#endif

Std$Integer$smallt MutexTypeNormal[] = {{MutexTypeT, PTHREAD_MUTEX_NORMAL}};
Std$Integer$smallt MutexTypeErrorCheck[] = {{MutexTypeT, PTHREAD_MUTEX_ERRORCHECK}};
Std$Integer$smallt MutexTypeRecursive[] = {{MutexTypeT, PTHREAD_MUTEX_RECURSIVE}};
Std$Integer$smallt MutexTypeDefault[] = {{MutexTypeT, PTHREAD_MUTEX_DEFAULT}};

GLOBAL_FUNCTION(MutexNew, 0) {
	mutex_t *Mutex = new(mutex_t);
	Mutex->Type = MutexT;
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	for (size_t I = 0; I < Count; ++I) {
		if (Args[I].Val->Type == MutexTypeT) {
			pthread_mutexattr_settype(&Attr, Std$Integer$get_small(Args[I].Val));
		};
	};
	pthread_mutex_init(&Mutex->Handle, &Attr);
	pthread_mutexattr_destroy(&Attr);
	Result->Val = (Std$Object$t *)Mutex;
	return SUCCESS;
};

GLOBAL_METHOD(MutexLock, 1, "lock", TYP, MutexT) {
	mutex_t *Mutex = (mutex_t *)Args[0].Val;
	if (pthread_mutex_lock(&Mutex->Handle)) {
		Result->Val = Std$String$new("Error locking mutex");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(MutexTryLock, 1, "trylock", TYP, MutexT) {
	mutex_t *Mutex = (mutex_t *)Args[0].Val;
	return pthread_mutex_trylock(&Mutex->Handle) ? FAILURE : SUCCESS;
};

GLOBAL_METHOD(MutexWait, 2, "wait", TYP, MutexT, TYP, Std$Integer$SmallT) {
	mutex_t *Mutex = (mutex_t *)Args[0].Val;
	struct timespec Time;
	clock_gettime(CLOCK_REALTIME, &Time);
	Time.tv_sec += Std$Integer$get_small(Args[1].Val);
	switch (pthread_mutex_timedlock(&Mutex->Handle, &Time)) {
	case 0: break;
	case ETIMEDOUT:
		return FAILURE;
	default:
		Result->Val = Std$String$new("Error locking mutex");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(MutexUnlock, 1, "unlock", TYP, MutexT) {
	mutex_t *Mutex = (mutex_t *)Args[0].Val;
	if (pthread_mutex_unlock(&Mutex->Handle)) {
		Result->Val = Std$String$new("Error locking mutex");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(SemaphoreNew, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	semaphore_t *Semaphore = new(semaphore_t);
	Semaphore->Type = SemaphoreT;
	sem_init(&Semaphore->Handle, 0, Std$Integer$get_small(Args[0].Val));
	Result->Val = (Std$Object$t *)Semaphore;
	return SUCCESS;
};

GLOBAL_METHOD(SemaphorePost, 1, "post", TYP, SemaphoreT) {
	semaphore_t *Semaphore = (semaphore_t *)Args[0].Val;
	if (sem_post(&Semaphore->Handle)) {
		Result->Val = Std$String$new("Error posting on semaphore");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(SemaphoreWait, 1, "wait", TYP, SemaphoreT) {
	semaphore_t *Semaphore = (semaphore_t *)Args[0].Val;
	if (sem_wait(&Semaphore->Handle)) {
		Result->Val = Std$String$new("Error posting on semaphore");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(SemaphoreTryWait, 1, "trywait", TYP, SemaphoreT) {
	semaphore_t *Semaphore = (semaphore_t *)Args[0].Val;
	if (sem_trywait(&Semaphore->Handle)) {
		if (Riva$System$get_errno() == EAGAIN) return FAILURE;
		Result->Val = Std$String$new("Error posting on semaphore");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(CondNew, 0) {
	cond_t *Cond = new(cond_t);
	Cond->Type = CondT;
	pthread_cond_init(&Cond->Handle, 0);
	Result->Val = (Std$Object$t *)Cond;
	return SUCCESS;
};

GLOBAL_METHOD(CondWait, 2, "wait", TYP, CondT, TYP, MutexT) {
	cond_t *Cond = (cond_t *)Args[0].Val;
	mutex_t *Mutex = (mutex_t *)Args[1].Val;
	if (pthread_cond_wait(&Cond->Handle, &Mutex->Handle)) {
		Result->Val = Std$String$new("Error waiting on condition");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(CondSignal, 1, "signal", TYP, CondT) {
	cond_t *Cond = (cond_t *)Args[0].Val;
	if (pthread_cond_signal(&Cond->Handle)) {
		Result->Val = Std$String$new("Error signalling condition");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(CondBroadcast, 1, "broadcast", TYP, CondT) {
	cond_t *Cond = (cond_t *)Args[0].Val;
	if (pthread_cond_broadcast(&Cond->Handle)) {
		Result->Val = Std$String$new("Error signalling condition");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(RWLockNew, 0) {
	rwlock_t *RWLock = new(rwlock_t);
	RWLock->Type = RWLockT;
	pthread_rwlock_init(&RWLock->Handle, 0);
	Result->Val = (Std$Object$t *)RWLock;
	return SUCCESS;
};

GLOBAL_METHOD(RWLockRdLock, 1, "rdlock", TYP, RWLockT) {
	rwlock_t *RWLock = (rwlock_t *)Args[0].Val;
	if (pthread_rwlock_rdlock(&RWLock->Handle)) {
		Result->Val = Std$String$new("Error locking rwlock");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(RWLockWrLock, 1, "wrlock", TYP, RWLockT) {
	rwlock_t *RWLock = (rwlock_t *)Args[0].Val;
	if (pthread_rwlock_wrlock(&RWLock->Handle)) {
		Result->Val = Std$String$new("Error locking rwlock");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(RWLockUnlock, 1, "unlock", TYP, RWLockT) {
	rwlock_t *RWLock = (rwlock_t *)Args[0].Val;
	if (pthread_rwlock_unlock(&RWLock->Handle)) {
		Result->Val = Std$String$new("Error unlocking rwlock");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(KeyNew, 0) {
	thread_key_t *Key = new(thread_key_t);
	Key->Type = KeyT;
	pthread_key_create(&Key->Handle, 0);
	Result->Val = (Std$Object$t *)Key;
	return SUCCESS;
};

GLOBAL_METHOD(KeyGet, 1, "get", TYP, KeyT) {
	thread_key_t *Key = (thread_key_t *)Args[0].Val;
	void *Value = pthread_getspecific(Key->Handle);
	if (Value) {
		Result->Val = Value;
		return SUCCESS;
	} else if (Count > 1) {
		switch (Std$Function$call(Args[1].Val, 0, Result)) {
		case SUSPEND: case SUCCESS:
			pthread_setspecific(Key->Handle, Result->Val);
			return SUCCESS;
		case FAILURE: return FAILURE;
		case MESSAGE: return MESSAGE;
		};
	} else {
		return FAILURE;
	};
};

GLOBAL_METHOD(KeySet, 2, "set", TYP, KeyT, ANY) {
	thread_key_t *Key = (thread_key_t *)Args[0].Val;
	pthread_setspecific(Key->Handle, Args[1].Val);
	Result->Arg = Args[1];
	return SUCCESS;
};

GLOBAL_FUNCTION(Sleep, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
    sleep(((Std$Integer_smallt *)Args[0].Val)->Value);
    return SUCCESS;
};

GLOBAL_FUNCTION(USleep, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
    usleep(((Std$Integer_smallt *)Args[0].Val)->Value);
    return SUCCESS;
};

typedef struct queue_node {
	struct queue_node *Next;
	Std$Function_argument Arg;
} queue_node;

typedef struct queue_t {
	const Std$Type$t *Type;
	pthread_mutex_t Mutex;
	pthread_cond_t Cond;
	int Length;
	queue_node *Head, *Tail;
	queue_node Quick[1];
} queue_t;

GLOBAL_FUNCTION(QueueNew, 0) {
	queue_t *Queue = new(queue_t);
	Queue->Type = QueueT;
	pthread_mutex_init(&Queue->Mutex, 0);
	pthread_cond_init(&Queue->Cond, 0);
	Result->Val = (Std$Object$t *)Queue;
	return SUCCESS;
};

GLOBAL_METHOD(QueuePut, 2, "put", TYP, QueueT, ANY) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	if (Queue->Tail) {
		queue_node *Node = new(queue_node);
		Node->Arg = Args[1];
		Queue->Tail->Next = Node;
		Queue->Tail = Node;
	} else {
		Queue->Head = Queue->Tail = Queue->Quick;
		Queue->Quick->Arg = Args[1];
		Queue->Quick->Next = 0;
	};
	for (int I = 2; I < Count; ++I) {
		queue_node *Node = new(queue_node);
		Node->Arg = Args[I];
		Queue->Tail->Next = Node;
		Queue->Tail = Node;
	};
	Queue->Length += Count - 1;
	pthread_cond_broadcast(&Queue->Cond);
	pthread_mutex_unlock(&Queue->Mutex);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(QueuePush, 2, "push", TYP, QueueT, ANY) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	if (Queue->Tail) {
		queue_node *Last = Queue->Head;
		for (int I = 1; I < Count; ++I) {
			queue_node *Node = new(queue_node);
			Node->Arg = Args[I];
			Node->Next = Last;
			Last = Node;
		};
		Queue->Head = Last;
	} else if (Count > 2) {
		queue_node *Last = new(queue_node);
		Last->Arg = Args[1];
		Queue->Tail = Last;
		for (int I = 2; I < Count - 1; ++I) {
			queue_node *Node = new(queue_node);
			Node->Arg = Args[I];
			Node->Next = Last;
			Last = Node;
		};
		Queue->Head = Queue->Quick;
		Queue->Quick->Arg = Args[Count - 1];
		Queue->Quick->Next = Last;
	} else {
		Queue->Head = Queue->Tail = Queue->Quick;
		Queue->Quick->Arg = Args[1];
		Queue->Quick->Next = 0;
	};
	Queue->Length += Count - 1;
	pthread_cond_broadcast(&Queue->Cond);
	pthread_mutex_unlock(&Queue->Mutex);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(QueuePop, 1, "pop", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	while (!Queue->Head) if (pthread_cond_wait(&Queue->Cond, &Queue->Mutex) != 0) {
		Result->Val = Std$String$new("Error waiting for queue");
		return MESSAGE;
	};
	Result->Arg = Queue->Head->Arg;
	if (!(Queue->Head = Queue->Head->Next)) Queue->Tail = 0;
	Queue->Length--;
	pthread_mutex_unlock(&Queue->Mutex);
	return SUCCESS;
};

GLOBAL_METHOD(QueueWait, 2, "wait", TYP, QueueT, TYP, Std$Integer$SmallT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	struct timespec Time;
	clock_gettime(CLOCK_REALTIME, &Time);
	Time.tv_sec += Std$Integer$get_small(Args[1].Val);
	pthread_mutex_lock(&Queue->Mutex);
	while (!Queue->Head) switch (pthread_cond_timedwait(&Queue->Cond, &Queue->Mutex, &Time)) {
	case 0: break;
	case ETIMEDOUT:
		pthread_mutex_unlock(&Queue->Mutex);
		return FAILURE;
	default:
		Result->Val = Std$String$new("Error waiting for queue");
		return MESSAGE;
	};
	Result->Arg = Queue->Head->Arg;
	if (!(Queue->Head = Queue->Head->Next)) Queue->Tail = 0;
	Queue->Length--;
	pthread_mutex_unlock(&Queue->Mutex);
	return SUCCESS;
};

GLOBAL_METHOD(QueuePeek, 1, "peek", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	if (!Queue->Head) {
		pthread_mutex_unlock(&Queue->Mutex);
		return FAILURE;
	};
	Result->Arg = Queue->Head->Arg;
	if (!(Queue->Head = Queue->Head->Next)) Queue->Tail = 0;
	Queue->Length--;
	pthread_mutex_unlock(&Queue->Mutex);
	return SUCCESS;
};

GLOBAL_METHOD(QueueLength, 1, "length", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	Result->Val = Std$Integer$new_small(Queue->Length);
	pthread_mutex_unlock(&Queue->Mutex);
	return SUCCESS;
};

GLOBAL_METHOD(QueueLock, 1, "lock", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(QueueUnlock, 1, "unlock", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_unlock(&Queue->Mutex);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(QueueHead, 1, "head", TYP, QueueT) {
	queue_t *Queue = (queue_t *)Args[0].Val;
	pthread_mutex_lock(&Queue->Mutex);
	if (!Queue->Head) {
		pthread_mutex_unlock(&Queue->Mutex);
		return FAILURE;
	}
	Result->Arg = Queue->Head->Arg;
	pthread_mutex_unlock(&Queue->Mutex);
	return SUCCESS;
};

void thread_exit(int Signal) {
	pthread_exit(0);
};

void signal_ignore(int Signal) {
};

GLOBAL_FUNCTION(GateNew, 0) {
	Gate_t *Gate = new(Gate_t);
	Gate->Type = GateT;
	Gate->Status = MESSAGE + 1;
	pthread_mutex_init(&Gate->Mutex, 0);
	pthread_cond_init(&Gate->Condition, 0);
	Result->Val = (Std$Object$t *)Gate;
	return SUCCESS;
};

GLOBAL_METHOD(GateWait, 1, "wait", TYP, GateT) {
	Gate_t *Gate = (Gate_t *)Args[0].Val;
	pthread_mutex_lock(&Gate->Mutex);
	while (Gate->Status > MESSAGE) if (pthread_cond_wait(&Gate->Condition, &Gate->Mutex) != 0) {
		Result->Val = Std$String$new("Error waiting for Gate");
		return MESSAGE;
	};
	Result->Arg = Gate->Result;
	Std$Function$status Status = Gate->Status;
	Gate->Status = MESSAGE + 1;
	pthread_mutex_unlock(&Gate->Mutex);
	return Status;
};

GLOBAL_METHOD(GateResume, 2, "resume", TYP, GateT, TYP, Std$Integer$SmallT, ANY) {
	Gate_t *Gate = (Gate_t *)Args[0].Val;
	pthread_mutex_lock(&Gate->Mutex);
	Gate->Status = Std$Integer$get_small(Args[1].Val);
	Gate->Result = Args[2];
	pthread_cond_signal(&Gate->Condition);
	pthread_mutex_unlock(&Gate->Mutex);
	return SUCCESS;
};

INITIAL() {
	pthread_key_create(&ThreadKey, 0);
 	struct sigaction Action;
 	Action.sa_handler = signal_ignore;
 	Action.sa_flags = SA_RESTART;
 	sigaction(SIGUSR1, &Action, 0);
	sigaction(SIGUSR2, &Action, 0);
// 	thread_t *Thread = new(thread_t);
// 	Thread->Type = T;
// 	Thread->Handle = pthread_self();
// 	Thread->Status = FAILURE;
// 	pthread_setspecific(ThreadKey, Thread);
};

//#endif
