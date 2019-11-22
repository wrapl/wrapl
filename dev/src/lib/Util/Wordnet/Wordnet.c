#include <Std.h>
#include <Riva.h>
#include <Sys/Module.h>
#include <Agg/List.h>
#include <wn.h>

typedef struct synset_t {
	const Std$Type$t *Type;
	SynsetPtr Handle;
} synset_t;

TYPE(SynsetT);

TYPE(PosT);

Std$Integer$smallt PosNoun[] = {{PosT, NOUN}};
Std$Integer$smallt PosVerb[] = {{PosT, VERB}};
Std$Integer$smallt PosAdj[] = {{PosT, ADJ}};
Std$Integer$smallt PosAdv[] = {{PosT, ADV}};
Std$Integer$smallt PosSatellite[] = {{PosT, SATELLITE}};

GLOBAL_FUNCTION(Find, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_ARG_TYPE(1, PosT);
	SynsetPtr ResultSet = findtheinfo_ds(
		Std$String$flatten(Args[0].Val),
		Std$Integer$get_small(Args[1].Val),
		0,
		Count > 2 ? Std$Integer$get_small(Args[2].Val) : 0
	);
	if (!ResultSet) FAIL;
	Std$Object$t *ResultList = Agg$List$new(0);
	while (ResultSet) {
		synset_t *Synset = new(synset_t);
		Synset->Type = SynsetT;
		Synset->Handle = ResultSet;
		Agg$List$put(ResultList, Synset);
		ResultSet = ResultSet->nextss;
	}
	RETURN(ResultList);
}

METHOD("words", TYP, SynsetT) {
	synset_t *Synset = (synset_t *)Args[0].Val;
	Std$Object$t *WordList = Agg$List$new(0);
	for (int I = 0; I < Synset->Handle->wcount; ++I) {
		Agg$List$put(WordList, Std$String$copy(Synset->Handle->words[I]));
	}
	RETURN(WordList);
}

AMETHOD(Std$String$Of, TYP, SynsetT) {
	synset_t *Synset = (synset_t *)Args[0].Val;
	RETURN(Std$String$copy(Synset->Handle->words[0]));
}

METHOD("definition", TYP, SynsetT) {
	synset_t *Synset = (synset_t *)Args[0].Val;
	RETURN(Std$String$copy(Synset->Handle->defn));
}

METHOD("=", TYP, SynsetT, TYP, SynsetT) {
	synset_t *Synset1 = (synset_t *)Args[1].Val;
	synset_t *Synset2 = (synset_t *)Args[2].Val;
	if (Synset1->Handle->wnsns == Synset2->Handle->wnsns) {
		RETURN1;
	} else {
		FAIL;
	}
}

#define SYNSET_METHOD(MethodName, PtrType) \
METHOD(MethodName, TYP, SynsetT) { \
	synset_t *Synset = (synset_t *)Args[0].Val; \
	SynsetPtr ResultSet = traceptrs_ds(Synset->Handle, PtrType, 0, 0); \
	if (!ResultSet) FAIL; \
	Std$Object$t *ResultList = Agg$List$new(0); \
	while (ResultSet) { \
		synset_t *Synset = new(synset_t); \
		Synset->Type = SynsetT; \
		Synset->Handle = ResultSet; \
		Agg$List$put(ResultList, Synset); \
		ResultSet = ResultSet->nextss; \
	} \
	RETURN(ResultList); \
}

SYNSET_METHOD("antonym", ANTPTR);
SYNSET_METHOD("hypernym", HYPERPTR);
SYNSET_METHOD("hyponym", HYPOPTR);
SYNSET_METHOD("entailment", ENTAILPTR);
SYNSET_METHOD("similar", SIMPTR);
SYNSET_METHOD("is_member", ISMEMBERPTR);
SYNSET_METHOD("is_stuff", ISSTUFFPTR);
SYNSET_METHOD("is_part", ISPARTPTR);
SYNSET_METHOD("has_member", HASMEMBERPTR);
SYNSET_METHOD("has_stuff", HASSTUFFPTR);
SYNSET_METHOD("has_part", HASPARTPTR);
SYNSET_METHOD("meronym", MERONYM);
SYNSET_METHOD("holonym", HOLONYM);
SYNSET_METHOD("cause_to", CAUSETO);
SYNSET_METHOD("participle", PPLPTR);
SYNSET_METHOD("see_also", SEEALSOPTR);
SYNSET_METHOD("pertain", PERTPTR);
SYNSET_METHOD("attribute", ATTRIBUTE);
SYNSET_METHOD("verb_group", VERBGROUP);
SYNSET_METHOD("derivation", DERIVATION);
SYNSET_METHOD("classification", CLASSIFICATION);
SYNSET_METHOD("class", CLASS);
SYNSET_METHOD("hmeronym", HMERONYM);
SYNSET_METHOD("hholonym", HHOLONYM);
SYNSET_METHOD("synonym", SYNS);
SYNSET_METHOD("coords", COORDS);
SYNSET_METHOD("relatives", RELATIVES);

METHOD("next", TYP, SynsetT) {
	synset_t *Synset = (synset_t *)Args[0].Val;
	SynsetPtr ResultSet = Synset->Handle->ptrlist;
	if (!ResultSet) FAIL;
	Std$Object$t *ResultList = Agg$List$new(0);
	while (ResultSet) {
		synset_t *Synset = new(synset_t);
		Synset->Type = SynsetT;
		Synset->Handle = ResultSet;
		Agg$List$put(ResultList, Synset);
		ResultSet = ResultSet->nextss;
	}
	RETURN(ResultList);
}

GLOBAL_FUNCTION(Morph, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_ARG_TYPE(1, PosT);
	int Pos = Std$Integer$get_small(Args[1].Val);
	char *ResultStr = morphstr(Std$String$flatten(Args[0].Val), Pos);
	if (ResultStr) {
		Std$Object$t *ResultList = Agg$List$new(0);
		Agg$List$put(ResultList, Std$String$copy(ResultStr));
		while ((ResultStr = morphstr(0, Pos))) {
			Agg$List$put(ResultList, Std$String$copy(ResultStr));
		}
		RETURN(ResultList);
	} else {
		FAIL;
	}
}

INITIAL(Riva$Module$provider_t *Provider) {
	char *DictionaryPath;
	asprintf(&DictionaryPath, "%s/Wordnet/dict", Sys$Module$get_path(Provider->Module));
	printf("Dictionary path = %s\n", DictionaryPath);
	setenv("WNSEARCHDIR", DictionaryPath, 1);
	wninit();
}
