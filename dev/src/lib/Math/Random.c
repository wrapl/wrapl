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
	char *Chars = Riva$Memory$alloc_atomic(Length + 1);
	int Left = Length;
	char *Ptr = Chars;
	while (Left--) *(Ptr++) = gmp_urandomm_ui(Random->State, 8);
	*Ptr = 0;
	Result->Val = Std$String$new_length(Chars, Length);
	return SUCCESS;
};

METHOD("generate", TYP, T, VAL, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	random_t *Random = Args[0].Val;
	int Length = ((Std$Integer_smallt *)Args[2].Val)->Value;
	int NoOfChars = ((Std$String_t *)Args[3].Val)->Length.Value;
	char CharSet[NoOfChars + 1];
	Std$String$flatten_to(Args[3].Val, CharSet);
	char *Chars = Riva$Memory$alloc_atomic(Length + 1);
	int Left = Length;
	char *Ptr = Chars;
	while (Left--) *(Ptr++) = CharSet[gmp_urandomm_ui(Random->State, NoOfChars)];
	*Ptr = 0;
	Result->Val = Std$String$new_length(Chars, Length);
	return SUCCESS;
};

CONSTANT(Default, T) {
	random_t *Random = new(random_t);
	Random->Type = T;
	gmp_randinit_default(Random->State);
	return Random;
};
