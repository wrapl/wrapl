#include <Riva/Memory.h>
#include <Std.h>
#include <stdio.h>

#include <histedit.h>

typedef struct editline_t {
	const Std$Type$t *Type;
	EditLine *Handle;
	History *HistoryHandle;
	HistEvent HistoryEvent[1];
	int Continuation;
	Std$Object$t *PromptFn;
} editline_t;

TYPE(T);

GLOBAL_FUNCTION(New, 1) {
	editline_t *Edit = new(editline_t);
	Edit->Type = T;
	Edit->Handle = el_init(Std$String$flatten(Args[0].Val), stdin, stdout, stderr);
	Edit->HistoryHandle = history_init();
	history(Edit->HistoryHandle, Edit->HistoryEvent, H_SETSIZE, 800);
	el_set(Edit->Handle, EL_CLIENTDATA, Edit);
	el_set(Edit->Handle, EL_EDITOR, "emacs");
	el_set(Edit->Handle, EL_HIST, history, Edit->HistoryHandle);
	/*el_set(Edit->Handle, EL_BIND, "-a", "a", "ed-prev-line", NULL);
	el_set(Edit->Handle, EL_BIND, "-a", "z", "ed-next-line", NULL);
	el_set(Edit->Handle, EL_BIND, NULL);*/
	Edit->Continuation = 0;
	Result->Val = Edit;
	return SUCCESS;
};

static char *call_prompt(EditLine *Handle) {
	editline_t *Edit;
	el_get(Handle, EL_CLIENTDATA, &Edit);
	Std$Function$result Result;
	Std$Function$call(Edit->PromptFn, 1, &Result, Edit, 0);
	return Std$String$flatten(Result.Val);
};

METHOD("prompt", TYP, T, TYP, Std$Function$T) {
	editline_t *Edit = Args[0].Val;
	Edit->PromptFn = Args[1].Val;
	el_set(Edit->Handle, EL_PROMPT, call_prompt, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("continue", TYP, T, TYP, Std$Symbol$T) {
	editline_t *Edit = Args[0].Val;
	Edit->Continuation = (Args[1].Val == $true);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("read", TYP, T) {
	editline_t *Edit = Args[0].Val;
	int Length;
	const char *Chars = el_gets(Edit->Handle, &Length);
	if (Length > 0) history(Edit->HistoryHandle, Edit->HistoryEvent, Edit->Continuation ? H_APPEND : H_ENTER, Chars);
	Result->Val = Std$String$copy_length(Chars, Length);
	return SUCCESS;
};

METHOD("reset", TYP, T) {
	editline_t *Edit = Args[0].Val;
	el_reset(Edit->Handle);
	return SUCCESS;
};

METHOD("end", TYP, T) {
	editline_t *Edit = Args[0].Val;
	el_end(Edit->Handle);
	return SUCCESS;
};
