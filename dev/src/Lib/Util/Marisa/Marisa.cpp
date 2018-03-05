#include <Riva.h>
#include <Std.h>
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
	Agent.set_query(Std$Integer$get_small(Args[1].Val));
	try {
		Trie->Handle->reverse_lookup(Agent);
		Result->Val = Std$String$copy_length(Agent.key().ptr(), Agent.key().length());
		return SUCCESS;
	} catch (marisa::Exception &error) {
		return FAILURE;
	}
}

