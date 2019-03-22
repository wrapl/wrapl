#include <Std.h>
#include <Riva.h>
#include <Agg.h>
#include <Util/TypedFunction.h>
#include <Sys/Module.h>
#include <dlfcn.h>

#define USES_NO_ENGINE_HANDLE
#include <eclipse.h>
#include <sepia.h>

typedef struct dident_t {
	const Std$Type$t *Type;
	dident Value;
} dident_t;

extern Std$Type$t DidentT[1];

METHOD("@", TYP, DidentT, VAL, Std$String$T) {
	dident_t *Dident = (dident_t *)Args[0].Val;
	int Length = DidLength(Dident->Value);
	char *String = Riva$Memory$alloc_atomic(Length + 4);
	memcpy(String, DidName(Dident->Value), Length);
	String[Length++] = '/';
	Length += sprintf(String + Length, "%d", DidArity(Dident->Value));
	RETURN(Std$String$new_length(String, Length));
}

typedef struct pword_t {
	const Std$Type$t *Type;
	pword Value;
} pword_t;

TYPE(PWordT);

GLOBAL_FUNCTION(Var, 0) {
	pword_t *Var = new(pword_t);
	Var->Type = PWordT;
	Var->Value = ec_newvar();
	RETURN(Var);
}

typedef struct ref_t {
	const Std$Type$t *Type;
	ec_ref Value;
} ref_t;

TYPE(RefT);

GLOBAL_FUNCTION(Ref, 0) {
	ref_t *Ref = new(ref_t);
	Ref->Type = RefT;
	Ref->Value = ec_ref_create_newvar();
	RETURN(Ref);
}

static Std$Function$status from_eclipse(pword PWord, Std$Function$result *Result) {
	long Long;
	double Double;
	char *String;
	pword Tail;
	dident Dident;
	if (ec_get_long(PWord, &Long) == PSUCCEED) {
		RETURN(Std$Integer$new_small(Long));
	} else if (ec_get_double(PWord, &Double) == PSUCCEED) {
		RETURN(Std$Real$new(Double));
	} else if (ec_get_string_length(PWord, &String, &Long) == PSUCCEED) {
		RETURN(Std$String$copy_length(String, Long));
	} else if (ec_get_nil(PWord) == PSUCCEED) {
		RETURN(Std$Object$Nil);
	} else if (ec_get_list(PWord, &PWord, &Tail) == PSUCCEED) {
		Std$Function$status Status = from_eclipse(PWord, Result);
		if (Status >= FAILURE) return Status;
		Std$Object$t *List = Agg$List$new(1, Result->Val);
		while (ec_get_list(Tail, &PWord, &Tail) == PSUCCEED) {
			Std$Function$status Status = from_eclipse(PWord, Result);
			if (Status >= FAILURE) return Status;
			Agg$List$put(List, Result->Val);
		}
		RETURN(List);
	} else if (ec_get_atom(PWord, &Dident) == PSUCCEED) {
		dident_t *Atom = new(dident_t);
		Atom->Type = DidentT;
		Atom->Value = Dident;
		RETURN(Dident);
	} else if (ec_is_var(PWord) == PSUCCEED) {
		FAIL;
	} else {
		SEND(Std$String$new("Unsupported pword type"));
	}
}

METHOD("get", TYP, RefT) {
	ref_t *Ref = (ref_t *)Args[0].Val;
	return from_eclipse(ec_ref_get(Ref->Value), Result);
}

METHOD("cut", TYP, RefT) {
	ref_t *Ref = (ref_t *)Args[0].Val;
	ec_cut_to_chp(Ref->Value);
	RETURN(Std$Object$Nil);
}

typedef struct result_generator {
	Std$Function$cstate State;
} result_generator;

static void result_finalize(result_generator *Generator, void *Arg) {

}

static ec_ref ResumeRef = 0;

static Std$Function$status result_resume(Std$Function$result *Result) {
	ec_post_goal(ec_atom(ec_did("fail", 0)));
	switch (ec_resume1(ResumeRef)) {
	case PFAIL:
		ec_ref_destroy(ResumeRef);
		ResumeRef = 0;
		return FAILURE;
	case PSUCCEED:
		return SUSPEND;
	case PTHROW:
		Result->Val = Std$String$new("Error thrown ec_resume");
		return MESSAGE;
	default:
		Result->Val = Std$String$new("Unsupported return value from ec_resume");
		return MESSAGE;
	}
}

GLOBAL_FUNCTION(Resume, 0) {
	ResumeRef = ec_ref_create(ec_nil());
	switch (ec_resume1(ResumeRef)) {
	case PFAIL:
		ec_ref_destroy(ResumeRef);
		ResumeRef = 0;
		return FAILURE;
	case PSUCCEED: {
		result_generator *Generator = new(result_generator);
		Riva$Memory$register_finalizer(Generator, result_finalize, 0, 0, 0);
		if (Generator->State.Chain) Riva$Memory$register_disappearing_link(&Generator->State.Chain, Generator->State.Chain);
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = result_resume;
		Result->State = Generator;
		return SUSPEND;
	}
	case PTHROW:
		SEND(Std$String$new("Error thrown ec_resume"));
	default:
		SEND(Std$String$new("Unsupported return value from ec_resume"));
	}
}

GLOBAL_FUNCTION(Cut, 0) {
	if (ResumeRef) {
		ec_cut_to_chp(ResumeRef);
		ec_post_goal(ec_atom(ec_did("fail", 0)));
		ec_resume1(ResumeRef);
		ec_ref_destroy(ResumeRef);
		ResumeRef = 0;
	}
	return SUCCESS;
}

METHOD("post", TYP, PWordT) {
	pword_t *Goal = (pword_t *)Args[0].Val;
	ec_post_goal(Goal->Value);
	RETURN0;
}

TYPED_FUNCTION(int, to_eclipse, Std$Object$t *Value, pword *Dest) {
	//if (Value == Std$Object$Nil) return ec_nil();
	Dest[0] = ec_nil();
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, Std$Integer$SmallT, Std$Object$t *Value, pword *Dest) {
	Dest[0] = ec_long(Std$Integer$get_small(Value));
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, Std$Real$T, Std$Object$t *Value, pword *Dest) {
	Dest[0] = ec_double(Std$Real$get_value(Value));
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, Std$String$T, Std$Object$t *Value, pword *Dest) {
	Dest[0] = ec_length_string(Std$String$get_length(Value), Std$String$flatten(Value));
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, Agg$List$T, Std$Object$t *Value, pword *Dest) {
	pword List = ec_nil();
	for (Agg$List$node *Node = Agg$List$tail(Value); Node; Node = Node->Prev) {
		pword NodeWord;
		to_eclipse(Node->Value, &NodeWord);
		List = ec_list(NodeWord, List);
	}
	Dest[0] = List;
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, PWordT, pword_t *PWord, pword *Dest) {
	Dest[0] = PWord->Value;
	return 0;
}

TYPED_INSTANCE(int, to_eclipse, RefT, ref_t *Ref, pword *Dest) {
	Dest[0] = ec_ref_get(Ref->Value);
	return 0;
}

Std$Function$status dident_invoke(const dident_t *Dident, unsigned long Count, const Std$Function$argument *Args, Std$Function$result *Result) {
	printf("Calling dident %s/%d with %d arguments\n", DidName(Dident->Value), DidArity(Dident->Value), Count);
	pword Arguments[Count];
	for (int I = 0; I < Count; ++I) to_eclipse(Args[I].Val, Arguments + I);
	pword_t *Term = new(pword_t);
	Term->Type = PWordT;
	if (DidArity(Dident->Value) != Count) {
		dident D = ec_did(DidName(Dident->Value), Count);
		Term->Value = ec_term_array(D, Arguments);
	} else {
		Term->Value = ec_term_array(Dident->Value, Arguments);
	}
	RETURN(Term);
}

METHOD("[]", TYP, DidentT, TYP, Std$Integer$SmallT) {
	dident_t *Dident0 = (dident_t *)Args[0].Val;
	int N = Std$Integer$get_small(Args[1].Val);
	dident_t *DidentN = new(dident_t);
	DidentN->Type = DidentT;
	DidentN->Value = ec_did(DidName(Dident0->Value), N);
	RETURN(DidentN);
}

static int dident_import(void *Engine, const char *Name, int *IsRef, void **Data) {
	dident_t *Dident = new(dident_t);
	Dident->Type = DidentT;
	Dident->Value = ec_did(Name, 0);
	Data[0] = Dident;
	IsRef[0] = 0;
	return 1;
}

CONSTANT(Did, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Dident");
	Riva$Module$provider_t *Provider = Riva$Module$get_default_provider(Module);
	Riva$Module$set_import_func(Provider, 0, dident_import);
	return Module;
}

INITIAL(Riva$Module$provider_t *Provider) {
	const char *ModulePath = Sys$Module$get_path(Provider->Module);

	char *LibPath;
	const char *OldLibPath = getenv("LD_LIBRARY_PATH");
	if (OldLibPath) {
		int NewLength = strlen(OldLibPath) + 1 + strlen(ModulePath) + strlen("/eclipse/lib/i386_linux") + 1;
		LibPath = Riva$Memory$alloc_atomic(NewLength);
		stpcpy(stpcpy(stpcpy(stpcpy(LibPath, OldLibPath), ":"), ModulePath), "/eclipse/lib/i386_linux");
	} else {
		int NewLength = strlen(ModulePath) + strlen("/eclipse/lib/i386_linux") + 1;
		LibPath = Riva$Memory$alloc_atomic(NewLength);
		stpcpy(stpcpy(LibPath, ModulePath), "/eclipse/lib/i386_linux");
	}
	printf("Setting LD_LIBRARY_PATH=<%s>\n", LibPath);
	setenv("LD_LIBRARY_PATH", LibPath, 1);	const char *EclipsePath = Riva$Memory$alloc_atomic(strlen(ModulePath) + strlen("/eclipse") + 1);
	strcpy(stpcpy(EclipsePath, ModulePath), "/eclipse");
	printf("Setting Eclipse path = %s\n", EclipsePath);
	ec_set_option_ptr(EC_OPTION_ECLIPSEDIR, EclipsePath);
	ec_init();
}
