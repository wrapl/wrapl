#include <Riva.h>
#include <Std.h>
#include <Agg/List.h>
#include <marisa.h>

typedef struct trie_t {
	const Std$Type$t *Type;
	marisa::Trie Handle[1];
} trie_t;

typedef struct keyset_t {
	const Std$Type$t *Type;
	marisa::Keyset Handle[1];
} keyset_t;

TYPE(T);
TYPE(KeysetT);

GLOBAL_FUNCTION(KeysetNew, 0) {
	keyset_t *Keyset = new(keyset_t);
	Keyset->Type = KeysetT;
	new (Keyset->Handle) marisa::Keyset();
	Result->Val = (Std$Object$t *)Keyset;
	return SUCCESS;
}

METHOD("add", TYP, KeysetT, TYP, Std$String$T) {
	keyset_t *Keyset = (keyset_t *)Args[0].Val;
	const char *Key = Std$String$flatten(Args[1].Val);
	int Length = Std$String$get_length(Args[1].Val);
	Keyset->Handle->push_back(Key, Length);
	Result->Arg = Args[0];
	return SUCCESS;
}

GLOBAL_FUNCTION(New, 0) {
	trie_t *Trie = new(trie_t);
	Trie->Type = T;
	new (Trie->Handle) marisa::Trie();
	Result->Val = (Std$Object$t *)Trie;
	return SUCCESS;
}

METHOD("build", TYP, T, TYP, KeysetT) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	keyset_t *Keyset = (keyset_t *)Args[1].Val;
	Trie->Handle->build(Keyset->Handle[0]);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("build", TYP, T, TYP, Agg$List$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	marisa::Keyset Keyset;
	for (Agg$List$node *Node = Agg$List$head(Args[1].Val); Node; Node = Node->Next) {
		const char *Key = Std$String$flatten(Node->Value);
		int Length = Std$String$get_length(Node->Value);
		Keyset.push_back(Key, Length);
	}
	Trie->Handle->build(Keyset);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("mmap", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	Trie->Handle->mmap(FileName);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("map", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	Trie->Handle->map(Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val));
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("load", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	Trie->Handle->load(FileName);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("save", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	Trie->Handle->save(FileName);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("[]", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	marisa::Agent Agent;
	Agent.set_query(Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val));
	if (Trie->Handle->lookup(Agent)) {
		Result->Val = Std$Integer$new_small(Agent.key().id());
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

METHOD("[]", TYP, T, TYP, Std$Integer$SmallT) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	marisa::Agent Agent;
	int Id = Std$Integer$get_small(Args[1].Val);
	if ((size_t)Id >= Trie->Handle->size()) return FAILURE;
	Agent.set_query(Id);
	Trie->Handle->reverse_lookup(Agent);
	Result->Val = Std$String$copy_length(Agent.key().ptr(), Agent.key().length());
	return SUCCESS;
}

typedef struct agent_generator {
	Std$Function$cstate State;
	marisa::Agent Agent[1];
	marisa::Trie *Trie;
} agent_generator;

typedef struct agent_resume_data {
	agent_generator *Generator;
	Std$Function$argument Result;
} agent_resume_data;

static Std$Function$status resume_common_prefix_search(agent_resume_data *Data) {
	agent_generator *Generator = Data->Generator;
	if (Generator->Trie->common_prefix_search(Generator->Agent[0])) {
		Data->Result.Val = Std$String$copy_length(Generator->Agent->key().ptr(), Generator->Agent->key().length());
		return SUSPEND;
	} else {
		return FAILURE;
	}
}

METHOD("common_prefix_search", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	agent_generator *Generator = new(agent_generator);
	new (Generator->Agent) marisa::Agent();
	Generator->Agent->set_query(Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val));
	if (Trie->Handle->common_prefix_search(Generator->Agent[0])) {
		Result->Val = Std$String$copy_length(Generator->Agent->key().ptr(), Generator->Agent->key().length());
		Generator->Trie = Trie->Handle;
		Generator->State.Run = (void *)Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function$cresumefn)resume_common_prefix_search;
		Result->State = Generator;
		return SUSPEND;
	} else {
		return FAILURE;
	}
}

static Std$Function$status resume_predictive_search(agent_resume_data *Data) {
	agent_generator *Generator = Data->Generator;
	if (Generator->Trie->predictive_search(Generator->Agent[0])) {
		Data->Result.Val = Std$String$copy_length(Generator->Agent->key().ptr(), Generator->Agent->key().length());
		return SUSPEND;
	} else {
		return FAILURE;
	}
}

METHOD("predictive_search", TYP, T, TYP, Std$String$T) {
	trie_t *Trie = (trie_t *)Args[0].Val;
	agent_generator *Generator = new(agent_generator);
	new (Generator->Agent) marisa::Agent();
	Generator->Agent->set_query(Std$String$flatten(Args[1].Val), Std$String$get_length(Args[1].Val));
	if (Trie->Handle->predictive_search(Generator->Agent[0])) {
		Result->Val = Std$String$copy_length(Generator->Agent->key().ptr(), Generator->Agent->key().length());
		Generator->Trie = Trie->Handle;
		Generator->State.Run = (void *)Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function$cresumefn)resume_predictive_search;
		Result->State = Generator;
		return SUSPEND;
	} else {
		return FAILURE;
	}
}


