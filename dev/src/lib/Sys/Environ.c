#include <Std.h>
#include <Riva/Memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
	#include <windows.h>
#endif

GLOBAL_FUNCTION(Get, 1) {
//@name:Std$String$T
//:Std$String$T
// Gets the value of the environment variable <var>name</var> if it is defined, fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
#ifdef WINDOWS
	char *Name = Std$String$flatten(Args[0].Val);
	int Length = GetEnvironmentVariable(Name, 0, 0);
	if (Length == 0) return FAILURE;
	char *Value = Riva$Memory$alloc_atomic(Length);
	GetEnvironmentVariable(Name, Value, Length);
	Result->Val = Std$String$new(Value);
#else
	char *Value = getenv(Std$String$flatten(Args[0].Val));
	if (Value == 0) return FAILURE;
	Result->Val = Std$String$copy(Value);
#endif
	return SUCCESS;
};

GLOBAL_FUNCTION(Set, 2) {
//@name:Std$String$T
//@value:Std$String$T
// Sets <code>name = value</code> in the current environment.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
#ifdef WINDOWS
	if (SetEnvironmentVariable(Std$String$flatten(Args[0].Val), Std$String$flatten(Args[1].Val)) == 0) {
		Result->Val = Std$String$new("Error setting environment variable");
		return MESSAGE;
	};
#else
	if (setenv(Std$String$flatten(Args[0].Val), Std$String$flatten(Args[1].Val), 1)) {
		Result->Val = Std$String$new("Error setting environment variable");
		return MESSAGE;
	};
#endif
	return SUCCESS;
};

GLOBAL_FUNCTION(GetCwd, 0) {
//:Std$String$T
// Returns the current working directory.
#ifdef WINDOWS
	int Length = GetCurrentDirectory(0, 0);
	char *Buffer = Riva$Memory$alloc_atomic(Length);
	GetCurrentDirectory(Length, Buffer);
	Result->Val = Std$String$new_length(Buffer, Length);
#else
	Result->Val = Std$String$new(getcwd(0, 0));
#endif
	return SUCCESS;
};

GLOBAL_FUNCTION(SetCwd, 1) {
//@dir:Std$String$T
// Changes the current directory to <var>dir</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
#ifdef WINDOWS
	if (SetCurrentDirectory(Std$String$flatten(Args[0].Val))) {
#else
	char DirName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, DirName);
	if (chdir(DirName)) {
#endif
		return SUCCESS;
	} else {
		Result->Val = "Error changing directory";
		return FAILURE;
	};
};
