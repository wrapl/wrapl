#include <Std.h>
#include <Riva.h>
#include <Sys/Service.h>
#include <Agg/StringTable.h>

#include <pthread.h>
#include <string.h>

struct Sys$Service$t {
	const Std$Type$t *Type;
	enum {UNKNOWN, STARTING, RUNNING} State;
	union {
		pthread_cond_t Cond;
		Std$Object$t *Value;
	};
};

TYPE(T);

static Agg$StringTable$t Services[1] = {Agg$StringTable$INIT};

static pthread_mutex_t ServicesMutex = PTHREAD_MUTEX_INITIALIZER;

Sys$Service$t *_new(const char *Name) {
	//printf("Looking for service %s\n", Name);
	pthread_mutex_lock(&ServicesMutex);
	Sys$Service$t *Service = Agg$StringTable$get(Services, Name, strlen(Name));
	if (Service == 0) {
		Service = new(Sys$Service$t);
		Service->Type = T;
		Service->State = UNKNOWN;
		pthread_cond_init(&Service->Cond, 0);
		Agg$StringTable$put(Services, Name, strlen(Name), Service);
	};
	switch (Service->State) {
	case UNKNOWN:
		Service->State = STARTING;
		pthread_mutex_unlock(&ServicesMutex);
		return Service;
	case STARTING:
		pthread_cond_wait(&Service->Cond, &ServicesMutex);
	case RUNNING:
		pthread_mutex_unlock(&ServicesMutex);
		return 0;
	};
	
};

void _start(Sys$Service$t *Service, Std$Object$t *Value) {
	//printf("Starting service \n");
	pthread_mutex_lock(&ServicesMutex);
	Service->State = RUNNING;
	pthread_cond_broadcast(&Service->Cond);
	Service->Value = Value;
	pthread_mutex_unlock(&ServicesMutex);
};

Std$Object$t *_get(const char *Name) {
	pthread_mutex_lock(&ServicesMutex);
	Sys$Service$t *Service = Agg$StringTable$get(Services, Name, strlen(Name));
	if (Service == 0) {
		Service = new(Sys$Service$t);
		Service->Type = T;
		Service->State = UNKNOWN;
		pthread_cond_init(&Service->Cond, 0);
		Agg$StringTable$put(Services, Name, strlen(Name), Service);
	};
	switch (Service->State) {
	case UNKNOWN:
	case STARTING:
		pthread_cond_wait(&Service->Cond, &ServicesMutex);
	case RUNNING:
		pthread_mutex_unlock(&ServicesMutex);
		return 0;
	};
};


GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Result->Val = _new(Std$String$flatten(Args[0].Val));
	return SUCCESS;
};


GLOBAL_METHOD(Start, 1, "start", TYP, T) {
	CHECK_EXACT_ARG_TYPE(0, T);
	_start(Args[0].Val, Args[1].Val);
	Result->Arg = Args[1];
	return SUCCESS;
};

GLOBAL_FUNCTION(Get, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Result->Val = _get(Std$String$flatten(Args[0].Val));
	return SUCCESS;
};
