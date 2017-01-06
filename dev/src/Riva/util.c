#include "libriva.h"
#include <string.h>

char *concat2(const char *A, const char *B) {
	int Length = strlen(A) + strlen(B);
	char *R = GC_MALLOC_ATOMIC(Length + 1);
	stpcpy(stpcpy(R, A), B);
	R[Length] = 0;
	return R;
};

char *concat3(const char *A, const char *B, const char *C) {
	int Length = strlen(A) + strlen(B) + strlen(C);
	char *R = GC_MALLOC_ATOMIC(Length + 1);
	stpcpy(stpcpy(stpcpy(R, A), B), C);
	R[Length] = 0;
	return R;
};
