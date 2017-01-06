#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <Std/Object.h>
#include <Riva/Module.h>

#define RIVA_MODULE Sys$Thread
#include <Riva-Header.h>

RIVA_STRUCT(t);

RIVA_TYPE(T);
RIVA_TYPE(KeyT);
RIVA_TYPE(MutexT);
RIVA_TYPE(SemaphoreT);
RIVA_TYPE(CondT);
RIVA_TYPE(RWLockT);
RIVA_TYPE(QueueT);

RIVA_OBJECT(New);
RIVA_OBJECT(Self);
RIVA_OBJECT(Result);
RIVA_OBJECT(Return);
RIVA_OBJECT(Fail);
RIVA_OBJECT(Send);
RIVA_OBJECT(Kill);
RIVA_OBJECT(MutexNew);
RIVA_OBJECT(MutexLock);
RIVA_OBJECT(MutexTryLock);
RIVA_OBJECT(MutexUnlock);
RIVA_OBJECT(CondNew);
RIVA_OBJECT(CondWait);
RIVA_OBJECT(CondSignal);
RIVA_OBJECT(CondBroadcast);
RIVA_OBJECT(RWLockNew);
RIVA_OBJECT(RWLockRdLock);
RIVA_OBJECT(RWLockWrLock);
RIVA_OBJECT(RWLockUnlock);
RIVA_OBJECT(KeyNew);
RIVA_OBJECT(KeyGet);
RIVA_OBJECT(KeySet);
RIVA_OBJECT(Sleep);
RIVA_OBJECT(QueueNew);
RIVA_OBJECT(QueuePut);
RIVA_OBJECT(QueuePush);
RIVA_OBJECT(QueuePop);
RIVA_OBJECT(QueuePeek);
RIVA_OBJECT(QueueLength);

RIVA_CFUN(Sys$Thread$t *, self);

#undef RIVA_MODULE

#endif

