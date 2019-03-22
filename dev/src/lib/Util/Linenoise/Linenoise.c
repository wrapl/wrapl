#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include "linenoise.h"

GLOBAL_FUNCTION(Read, 1) {
	CHECK_ARG_TYPE(0, Std$String$T);
	const char *Line = linenoise(Std$String$flatten(Args[0].Val));
	RETURN(Std$String$new(Line));
}

GLOBAL_FUNCTION(SetMultiLine, 1) {
	linenoiseSetMultiLine(Args[0].Val == $true);
	return SUCCESS;
}

GLOBAL_FUNCTION(HistoryAdd, 1) {
	CHECK_ARG_TYPE(0, Std$String$T);
	const char *Line = Std$String$flatten(Args[0].Val);
	if (!Line) FAIL;
	if (linenoiseHistoryAdd(Line) < 0) {
		Result->Val = Std$String$new("Error in HistoryAdd");
		return MESSAGE;
	} else {
		return SUCCESS;
	}
}

GLOBAL_FUNCTION(HistorySetMaxLen, 1) {
	CHECK_ARG_TYPE(0, Std$Integer$SmallT);
	if (linenoiseHistorySetMaxLen(Std$Integer$get_small(Args[0].Val)) < 0) {
		Result->Val = Std$String$new("Error in HistorySetMaxLen");
		return MESSAGE;
	} else {
		return SUCCESS;
	}
}

GLOBAL_FUNCTION(HistorySave, 1) {
	CHECK_ARG_TYPE(0, Std$String$T);
	if (linenoiseHistorySave(Std$String$flatten(Args[0].Val)) < 0) {
		Result->Val = Std$String$new("Error in HistorySave");
		return MESSAGE;
	} else {
		return SUCCESS;
	}
}

GLOBAL_FUNCTION(HistoryLoad, 1) {
	CHECK_ARG_TYPE(0, Std$String$T);
	if (linenoiseHistoryLoad(Std$String$flatten(Args[0].Val)) < 0) {
		Result->Val = Std$String$new("Error in HistoryLoad");
		return MESSAGE;
	} else {
		return SUCCESS;
	}
}

Std$Object$t *CompletionCallback = Std$Object$Nil;

static void riva_completion(const char *Buffer, linenoiseCompletions *Completions) {
	Std$Function$result Result[1];
	switch (Std$Function$call(CompletionCallback, 1, Result, Std$String$new(Buffer), 0)) {
	case SUSPEND: case SUCCESS: {
		if (Result->Val->Type == Agg$List$T) {
			for (Agg$List$node *Node = ((Agg$List$t *)Result->Val)->Head; Node; Node = Node->Next) {
				if (Node->Value->Type == Std$String$T) {
					linenoiseAddCompletion(Completions, Std$String$flatten(Node->Value));
				}
			}
		}
		break;
	}
	case FAILURE: break;
	case MESSAGE: break;
	}
}

GLOBAL_FUNCTION(CompletionSet, 1) {
	if (Args[0].Val != Std$Object$Nil) {
		CompletionCallback = Args[0].Val;
		linenoiseSetCompletionCallback(riva_completion);
	} else {
		linenoiseSetCompletionCallback(0);
	}
	return SUCCESS;
}
