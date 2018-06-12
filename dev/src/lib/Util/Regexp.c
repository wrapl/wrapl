#include <Std.h>
#include <Riva.h>

#include <tre/regex.h>

typedef struct regexp_t {
	Std$Type_t *Type;
	regex_t Handle[1];
} regexp_t;

TYPE(T);
// A regular expression.

static void regexp_finalize(regexp_t *R, void *Data) {
	regfree(R->Handle);
};

GLOBAL_FUNCTION(New, 1) {
//@regexp:Std$String$T
//:T
// Compiles <var>regexp</var> into a regular expression and returns it.
	Std$String_t *Expr = Args[0].Val;
	regexp_t *R = new(regexp_t);
	int Flags = REG_EXTENDED;
	if (regncomp(R->Handle, Std$String$flatten(Expr), Expr->Length.Value, Flags)) {
		Result->Val = "Invalid regular expression";
		return MESSAGE;
	} else {
		Riva$Memory$register_finalizer(R, regexp_finalize, 0, 0, 0);
		R->Type = T;
		Result->Val = R;
		return SUCCESS;
	};
};

typedef struct context_t {
	Std$String_block *Head;
	Std$String_block *Block;
	char *Chars;
	int Rem;
	int Offset;
} context_t;

static int get_next_char(tre_char_t *Char, unsigned int *PosAdd, context_t *Context) {
	if (Context->Rem-- == 0) {
		Context->Block++;
		Context->Rem = Context->Block->Length.Value;
		if (Context->Rem == 0) return 1;
		Char = Context->Block->Chars.Value;
	};
	Char[0] = *(Context->Chars++);
	PosAdd[0] = 1;
	return 0;
};

static void rewind(size_t Pos, context_t *Context) {
	Std$String_block *Block = Context->Head;
	Pos += Context->Offset;
	while (Pos >= Block->Length.Value) {
		Pos -= Block->Length.Value;
		Block++;
	};
	Context->Block = Block;
	Context->Rem = Block->Length.Value - Pos;
	Context->Chars = Block->Chars.Value + Pos;
};

static int compare(size_t Pos1, size_t Pos2, size_t Len, context_t *Context) {
	Pos1 += Context->Offset;
	Pos2 += Context->Offset;
	Std$String_block *Block1 = Context->Head;
	while (Pos1 >= Block1->Length.Value) {
		Pos1 -= Block1->Length.Value;
		Block1++;
	};
	int Rem1 = Block1->Length.Value - Pos1;
	char *Chars1 = Block1->Chars.Value + Pos1;
	Std$String_block *Block2 = Context->Head;
	while (Pos2 >= Block2->Length.Value) {
		Pos2 -= Block2->Length.Value;
		Block2++;
	};
	int Rem2 = Block2->Length.Value - Pos2;
	char *Chars2 = Block2->Chars.Value + Pos2;
	while (Len--) {
		if (Rem1-- == 0) {
			Block1++;
			Rem1 = Block1->Length.Value;
			if (Rem1 == 0) return 1;
			Chars1 = Block1->Chars.Value;
		};
		if (Rem2-- == 0) {
			Block2++;
			Rem2 = Block2->Length.Value;
			if (Rem2 == 0) return 1;
			Chars2 = Block2->Chars.Value;
		};
		if (*(Chars1++) != *(Chars2++)) return 1;
	};
	return 0;
};

typedef struct result_t {
	Std$Type_t *Type;
	Std$String_t *String;
	Std$Integer_smallt *Start, *End;
} result_t;

TYPE(ResultT);
// The result of a regular expression match.

METHOD("@", TYP, ResultT, VAL, Std$String$T) {
	result_t *Result0 = Args[0].Val;
	Result->Val = Result0->String;
	return SUCCESS;
};

METHOD("string", TYP, ResultT) {
//@result
//:Std$String$T
// Returns the matching string.
	result_t *Result0 = Args[0].Val;
	Result->Val = Result0->String;
	return SUCCESS;
};

METHOD("start", TYP, ResultT) {
//@result
//:Std$Integer$SmallT
// Returns the start position of the matching string.
	result_t *Result0 = Args[0].Val;
	Result->Val = Result0->Start;
	return SUCCESS;
};

METHOD("end", TYP, ResultT) {
//@result
//:Std$Integer$SmallT
// Returns the end position of the matching string.
	result_t *Result0 = Args[0].Val;
	Result->Val = Result0->End;
	return SUCCESS;
};

#ifdef DOCUMENTING
METHOD("match", TYP, T, TYP, Std$String$T, ANY) {
//@regexp
//@string
//@var&lt;sub&gt;1&lt;/sub&gt;+,&#160;...,&#160;var&lt;sub&gt;k&lt;/sub&gt;+
//:ResultT
// Attempts to match <var>string</var> to <var>regexp</var>. Bracketed expressions are stored in the <var>var&lt;sub&gt;i&lt;/sub&gt;</var>.
#else
METHOD("match", TYP, T, TYP, Std$String$T) {
#endif
	regex_t *PReg = ((regexp_t *)Args[0].Val)->Handle;
	Std$String_t *String = Args[1].Val;
	context_t Context[1] = {{
		String->Blocks,
		String->Blocks,
		String->Blocks->Chars.Value,
		String->Blocks->Length.Value,
		0
	}};
	tre_str_source Source[1] = {{
		get_next_char,
		rewind,
		compare,
		Context
	}};
	regmatch_t PMatch[PReg->re_nsub + 1];
	if (reguexec(PReg, Source, PReg->re_nsub + 1, PMatch, 0)) return FAILURE;
	Std$Function_argument *Out = Args + 2;
	int Max = Count - 2;
	if (Max > PReg->re_nsub) Max = PReg->re_nsub + 1;
	for (int I = 1; I < Max; ++I) {
		if (PMatch[I].rm_so != -1) {
			//printf("(%d) = [%d, %d]\n", I, PMatch[I].rm_so, PMatch[I].rm_eo);
			if (Out->Ref) Out->Ref[0] = Std$String$slice(String, PMatch[I].rm_so, PMatch[I].rm_eo);
		};
		Out++;
	};
	result_t *Result0 = new(result_t);
	Result0->Type = ResultT;
	Result0->String = Std$String$slice(String, PMatch[0].rm_so, PMatch[0].rm_eo);
	Result0->Start = Std$Integer$new_small(PMatch[0].rm_so);
	Result0->End = Std$Integer$new_small(PMatch[0].rm_eo);
	Result->Val = Result0;
	return SUCCESS;
};

#ifdef DOCUMENTING
METHOD("match", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT, ANY) {
//@regexp
//@string
//@start
//@var&lt;sub&gt;1&lt;/sub&gt;+,&#160;...,&#160;var&lt;sub&gt;k&lt;/sub&gt;+
//:ResultT
// Attempts to match <code>string[start, 0]</code> to <var>regexp</var>. Bracketed expressions are stored in the <var>var&lt;sub&gt;i&lt;/sub&gt;</var>.
#else
METHOD("match", TYP, T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
#endif
	regex_t *PReg = ((regexp_t *)Args[0].Val)->Handle;
	Std$String_t *String = Args[1].Val;
	uint32_t Offset = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	uint32_t Start = Offset;
	Std$String_block *Subject = String->Blocks;
	while (Start >= Subject->Length.Value) {
		Start -= Subject->Length.Value;
		++Subject;
		if (Subject->Length.Value == 0) return FAILURE;
	};
	context_t Context[1] = {{
		String->Blocks,
		Subject,
		Subject->Chars.Value + Start,
		Subject->Length.Value - Start,
		Offset
	}};
	tre_str_source Source[1] = {{
		get_next_char,
		rewind,
		compare,
		Context
	}};
	regmatch_t PMatch[PReg->re_nsub + 1];
	if (reguexec(PReg, Source, PReg->re_nsub + 1, PMatch, 0)) return FAILURE;
	Std$Function_argument *Out = Args + 2;
	int Max = Count - 2;
	if (Max > PReg->re_nsub) Max = PReg->re_nsub + 1;
	for (int I = 1; I < Max; ++I) {
		if (PMatch[I].rm_so != -1) {
			//printf("(%d) = [%d, %d]\n", I, PMatch[I].rm_so, PMatch[I].rm_eo);
			if (Out->Ref) Out->Ref[0] = Std$String$slice(String, PMatch[I].rm_so + Offset, PMatch[I].rm_eo + Offset);
		};
		Out++;
	};
	result_t *Result0 = new(result_t);
	Result0->Type = ResultT;
	Result0->String = Std$String$slice(String, PMatch[0].rm_so + Offset, PMatch[0].rm_eo + Offset);
	Result0->Start = Std$Integer$new_small(PMatch[0].rm_so + Offset);
	Result0->End = Std$Integer$new_small(PMatch[0].rm_eo + Offset);
	Result->Val = Result0;
	return SUCCESS;
};
