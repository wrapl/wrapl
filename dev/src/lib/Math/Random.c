#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>

#include <gmp.h>

typedef struct random_t {
	Std$Type_t *Type;
	gmp_randstate_t State;
} random_t;

TYPE(T);

GLOBAL_FUNCTION(New, 0) {
	random_t *Random = new(random_t);
	Random->Type = T;
	gmp_randinit_default(Random->State);
	Result->Val = Random;
	return SUCCESS;
};

METHOD("seed", TYP, T, TYP, Std$Integer$SmallT) {
	random_t *Random = Args[0].Val;
	Std$Integer_smallt *Seed = Args[1].Val;
	gmp_randseed_ui(Random->State, Seed->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("seed", TYP, T, TYP, Std$Integer$BigT) {
	random_t *Random = Args[0].Val;
	Std$Integer_bigt *Seed = Args[1].Val;
	gmp_randseed(Random->State, Seed->Value);
	Result->Arg = Args[0];
	return SUCCESS;
};

#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>

METHOD("_seed", TYP, T) {
	random_t *Random = Args[0].Val;
	long Data[8];
	int Src = open("/dev/urandom", O_RDONLY, 0644);
	read(Src, Data, sizeof(Data));
	close(Src);
	mpz_t Seed;
	mpz_init(Seed);
	mpz_import(Seed, 8, 1, sizeof(Data[0]), 0, 0, Data);
	gmp_randseed(Random->State, Seed);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("__seed", TYP, T) {
	random_t *Random = Args[0].Val;
	long Data[8];
	int Src = open("/dev/random", O_RDONLY, 0644);
	read(Src, Data, sizeof(Data));
	close(Src);
	mpz_t Seed;
	mpz_init(Seed);
	mpz_import(Seed, 8, 1, sizeof(Data[0]), 0, 0, Data);
	gmp_randseed(Random->State, Seed);
	Result->Arg = Args[0];
	return SUCCESS;
};
#else
#include <windows.h>
#include <wincrypt.h>

METHOD("_seed", TYP, T) {
	random_t *Random = Args[0].Val;
	long Data[8];
	HCRYPTPROV CryptProv;
	CryptAcquireContext(&CryptProv, "BlahBlahBlah", NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	CryptGenRandom(CryptProv, 8, Data);
	CryptReleaseContext(CryptProv, 0);
//#error This needs to be done on other platforms
	mpz_t Seed;
	mpz_init(Seed);
	mpz_import(Seed, 8, 1, sizeof(Data[0]), 0, 0, Data);
	gmp_randseed(Random->State, Seed);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("__seed", TYP, T) {
	random_t *Random = Args[0].Val;
	long Data[8];
	HCRYPTPROV CryptProv;
	CryptAcquireContext(&CryptProv, "BlahBlahBlah", NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	CryptGenRandom(CryptProv, 8, Data);
	CryptReleaseContext(CryptProv, 0);
//#error This needs to be done on other platforms
	mpz_t Seed;
	mpz_init(Seed);
	mpz_import(Seed, 8, 1, sizeof(Data[0]), 0, 0, Data);
	gmp_randseed(Random->State, Seed);
	Result->Arg = Args[0];
	return SUCCESS;
};
#endif

METHOD("generate", TYP, T, VAL, Std$Integer$SmallT) {
	random_t *Random = Args[0].Val;
	Result->Val = Std$Integer$new_small(gmp_urandomb_ui(Random->State, 32));
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	random_t *Random = Args[0].Val;
	Result->Val = Std$Integer$new_small(gmp_urandomm_ui(Random->State, ((Std$Integer_smallt *)Args[2].Val)->Value));
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$Integer$T, TYP, Std$Integer$SmallT) {
	random_t *Random = Args[0].Val;
	Result->Val = Std$Integer$new_small(gmp_urandomm_ui(Random->State, ((Std$Integer_smallt *)Args[2].Val)->Value));
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$Integer$T, TYP, Std$Integer$BigT) {
	random_t *Random = Args[0].Val;
	mpz_t Z;
	mpz_urandomm(Z, Random->State, ((Std$Integer_bigt *)Args[2].Val)->Value);
	if (mpz_fits_slong_p(Z)) {
		Result->Val = Std$Integer$new_small(mpz_get_si(Z));
		return SUCCESS;
	} else {
		Result->Val = Std$Integer$new_big(Z);
		return SUCCESS;
	};
};

METHOD("generate", TYP, T, VAL, Std$Real$T) {
	random_t *Random = Args[0].Val;
	double R;
	unsigned long *I = &R;
	I[1] = 0x3ff00000 + gmp_urandomb_ui(Random->State, 20);
	I[0] = gmp_urandomb_ui(Random->State, 32);
	Result->Val = Std$Real$new(R - 1.0);
	return SUCCESS;
};

METHOD("generate", TYP, T, TYP, Agg$List$T) {
	random_t *Random = Args[0].Val;
	Agg$List$t *List = (Agg$List$t *)Args[1].Val;
	int Index = gmp_urandomm_ui(Random->State, List->Length);
	Result->Val = Agg$List$find_node(List, Index + 1)->Value;
	return SUCCESS;
};

METHOD("[]", TYP, Agg$List$T, TYP, T) {
	Agg$List$t *List = (Agg$List$t *)Args[0].Val;
	random_t *Random = Args[1].Val;
	int Index = gmp_urandomm_ui(Random->State, List->Length);
	Result->Val = Agg$List$find_node(List, Index + 1)->Value;
	return SUCCESS;
};

METHOD("shuffle", TYP, Agg$List$T, TYP, T) {
	Agg$List$t *List = (Agg$List$t *)Args[0].Val;
	random_t *Random = (random_t *)Args[1].Val;
	if (!List->Length) RETURN(List);
	Agg$List$node **Nodes = (Agg$List$node **)Riva$Memory$alloc(List->Length * sizeof(Agg$List$node *));
	Agg$List$node **P = Nodes;
	for (Agg$List$node *Node = List->Head; Node; Node = Node->Next) *P++ = Node;
	for (int I = List->Length; --I >= 0;) {
		int J = gmp_urandomm_ui(Random->State, I + 1);
		Agg$List$node *Temp = Nodes[J];
		Nodes[J] = Nodes[I];
		Nodes[I] = Temp;
	}
	Agg$List$node *Tail = List->Head = Nodes[List->Length - 1];
	Tail->Prev = 0;
	for (int I = List->Length - 1; --I >= 0;) {
		Agg$List$node *Node = Nodes[I];
		Tail->Next = Node;
		Node->Prev = Tail;
		Tail = Node;
	}
	Tail->Next = 0;
	List->Tail = Tail;
	List->Index = 1;
	List->Cache = List->Head;
	List->Access = 4;
	List->Array = 0;
	RETURN(List);
}

double _uniform01(random_t *Random) {
	double R;
	unsigned long *I = &R;
	I[1] = 0x3ff00000 + gmp_urandomb_ui(Random->State, 20);
	I[0] = gmp_urandomb_ui(Random->State, 32);
	return R - 1.0;
};

METHOD("generate", TYP, T, VAL, Std$Real$T, TYP, Std$Real$T) {
	random_t *Random = Args[0].Val;
	double R;
	unsigned long *I = &R;
	I[1] = 0x3ff00000 + gmp_urandomb_ui(Random->State, 20);
	I[0] = gmp_urandomb_ui(Random->State, 32);
	Result->Val = Std$Real$new((R - 1.0) * ((Std$Real_t *)Args[2].Val)->Value);
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$String$T, TYP, Std$Integer$SmallT) {
	random_t *Random = Args[0].Val;
	int Length = ((Std$Integer_smallt *)Args[2].Val)->Value;
	int NumBlocks = Length / Std$String$MaxBlockSize + 1;
	Std$String$t *String = Std$String$alloc(NumBlocks);
	String->Length.Value = Length;
	Std$String$block *Block = String->Blocks;
	while (Length > Std$String$MaxBlockSize) {
		uint32_t *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Std$String$MaxBlockSize);
		Block->Length.Value = Std$String$MaxBlockSize;
		for (int Left = Std$String$MaxBlockSize; --Left >= 0;) {
			*(Dest++) = gmp_urandomb_ui(Random->State, 32);
		}
		Length -= Std$String$MaxBlockSize;
		++Block;
	}
	uint8_t *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Length);
	Block->Length.Value = Length;
	for (int Left = Length; --Left >= 0;) {
		*(Dest++) = gmp_urandomb_ui(Random->State, 8);
	}
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	random_t *Random = Args[0].Val;
	int Length = ((Std$Integer_smallt *)Args[2].Val)->Value;
	int NoOfChars = ((Std$String_t *)Args[3].Val)->Length.Value;
	char CharSet[NoOfChars + 1];
	Std$String$flatten_to(Args[3].Val, CharSet);
	int NumBlocks = Length / Std$String$MaxBlockSize + 1;
	Std$String$t *String = Std$String$alloc(NumBlocks);
	String->Length.Value = Length;
	Std$String$block *Block = String->Blocks;
	while (Length > Std$String$MaxBlockSize) {
		char *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Std$String$MaxBlockSize);
		Block->Length.Value = Std$String$MaxBlockSize;
		for (int Left = Std$String$MaxBlockSize; --Left >= 0;) {
			*(Dest++) = CharSet[gmp_urandomm_ui(Random->State, NoOfChars)];
		}
		Length -= Std$String$MaxBlockSize;
		++Block;
	}
	char *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Length);
	Block->Length.Value = Length;
	for (int Left = Length; --Left >= 0;) {
		*(Dest++) = CharSet[gmp_urandomm_ui(Random->State, NoOfChars)];
	}
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
};

CONSTANT(Default, T) {
	random_t *Random = new(random_t);
	Random->Type = T;
	gmp_randinit_default(Random->State);
	return Random;
};
