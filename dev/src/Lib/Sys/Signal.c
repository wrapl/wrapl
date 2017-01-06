#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <Riva/System.h>
#include <Sys/Signal.h>
#include <signal.h>

typedef Sys$Signal$t signal_t;
typedef Sys$Signal$set_t riva_sigset_t;

TYPE(T, Std$Integer$SmallT, Std$Integer$T, Std$Number$T);

signal_t Signal_SIGHUP[] = {{T, SIGHUP}};
signal_t Signal_SIGINT[] = {{T, SIGINT}};
signal_t Signal_SIGQUIT[] = {{T, SIGQUIT}};
signal_t Signal_SIGILL[] = {{T, SIGILL}};
signal_t Signal_SIGABRT[] = {{T, SIGABRT}};
signal_t Signal_SIGFPE[] = {{T, SIGFPE}};
signal_t Signal_SIGKILL[] = {{T, SIGKILL}};
signal_t Signal_SIGSEGV[] = {{T, SIGSEGV}};
signal_t Signal_SIGPIPE[] = {{T, SIGPIPE}};
signal_t Signal_SIGALRM[] = {{T, SIGALRM}};
signal_t Signal_SIGTERM[] = {{T, SIGTERM}};
signal_t Signal_SIGUSR1[] = {{T, SIGUSR1}};
signal_t Signal_SIGUSR2[] = {{T, SIGUSR2}};
signal_t Signal_SIGCHLD[] = {{T, SIGCHLD}};
signal_t Signal_SIGCONT[] = {{T, SIGCONT}};
signal_t Signal_SIGSTOP[] = {{T, SIGSTOP}};
signal_t Signal_SIGTSTP[] = {{T, SIGTSTP}};

TYPE(SetT);

GLOBAL_FUNCTION(SetNew, 0) {
	riva_sigset_t *Sigset = new(riva_sigset_t);
	Sigset->Type = SetT;
	sigemptyset(Sigset->Value);
	Result->Val = (Std$Object$t *)Sigset;
	return SUCCESS;
};

METHOD("empty", TYP, SetT) {
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	sigemptyset(Sigset->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("fill", TYP, SetT) {
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	sigfillset(Sigset->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("add", TYP, SetT, TYP, Std$Integer$SmallT) {
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	sigaddset(Sigset->Value, Std$Integer$get_small(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("del", TYP, SetT, TYP, Std$Integer$SmallT) {
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	sigdelset(Sigset->Value, Std$Integer$get_small(Args[1].Val));
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("in", TYP, Std$Integer$SmallT, TYP, SetT) {
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[1].Val;
	Result->Arg = Args[0];
	return sigismember(Sigset->Value, Std$Integer$get_small(Args[0].Val));
};

GLOBAL_FUNCTION(Wait, 1) {
	CHECK_EXACT_ARG_TYPE(0, SetT);
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	int Signal;
	if (sigwait(Sigset->Value, &Signal)) {
		Result->Val = Std$String$new("Error waiting for signal");
		return MESSAGE;
	} else {
		Result->Val = Std$Integer$new_small(Signal);
		return SUCCESS;
	};
};

GLOBAL_FUNCTION(Block, 1) {
	CHECK_EXACT_ARG_TYPE(0, SetT);
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		riva_sigset_t *OldSet = new(riva_sigset_t);
		OldSet->Type = SetT;
		if (sigprocmask(SIG_BLOCK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (sigprocmask(SIG_BLOCK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Unblock, 1) {
	CHECK_EXACT_ARG_TYPE(0, SetT);
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		riva_sigset_t *OldSet = new(riva_sigset_t);
		OldSet->Type = SetT;
		if (sigprocmask(SIG_UNBLOCK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (sigprocmask(SIG_UNBLOCK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(SetMask, 1) {
	CHECK_EXACT_ARG_TYPE(0, SetT);
	riva_sigset_t *Sigset = (riva_sigset_t *)Args[0].Val;
	if (Count > 1 && Args[1].Ref) {
		riva_sigset_t *OldSet = new(riva_sigset_t);
		OldSet->Type = SetT;
		if (sigprocmask(SIG_SETMASK, Sigset->Value, OldSet->Value)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
		Args[1].Ref[0] = (Std$Object$t *)OldSet;
	} else {
		if (sigprocmask(SIG_SETMASK, Sigset->Value, 0)) {
			Result->Val = Std$String$new("Error blocking signals");
			return MESSAGE;
		};
	};
	return SUCCESS;
};
