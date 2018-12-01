#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <Riva/Debug.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef DOCUMENTING

#define ADDRESS_METHOD METHOD
#define ARRAY_METHOD METHOD
#define FUNCTION_METHOD METHOD
#define INTEGER_METHOD METHOD
#define NUMBER_METHOD METHOD
#define OBJECT_METHOD METHOD
#define RATIONAL_METHOD METHOD
#define REAL_METHOD METHOD
#define STRING_METHOD METHOD
#define SYMBOL_METHOD METHOD
#define TYPE_METHOD METHOD
#define NORMAL_METHOD METHOD

#endif

ADDRESS_METHOD("gets", TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	int Length = Std$Integer$get_small(Args[1].Val);
	int NumBlocks = Length / Std$String$MaxBlockSize + 1;
	Std$String$t *String = Std$String$alloc(NumBlocks);
	String->Length.Value = Length;
	void *Source = Std$Address$get_value(Args[0].Val);
	Std$String$block *Block = String->Blocks;
	while (Length > Std$String$MaxBlockSize) {
		void *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Std$String$MaxBlockSize);
		Block->Length.Value = Std$String$MaxBlockSize;
		memcpy(Dest, Source, Std$String$MaxBlockSize);
		Source += Std$String$MaxBlockSize;
		Length -= Std$String$MaxBlockSize;
		++Block;
	}
	void *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Length);
	Block->Length.Value = Length;
	memcpy(Dest, Source, Length);
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
}

ADDRESS_METHOD("gets", TYP, Std$Address$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	int Length = Std$Integer$get_small(Args[1].Val);
	int NumBlocks = Length / Std$String$MaxBlockSize + 1;
	Std$String$t *String = Std$String$alloc(NumBlocks);
	String->Length.Value = Length;
	void *Source = Std$Address$get_value(Args[0].Val) + Std$Integer$get_small(Args[2].Val);
	Std$String$block *Block = String->Blocks;
	while (Length > Std$String$MaxBlockSize) {
		void *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Std$String$MaxBlockSize);
		Block->Length.Value = Std$String$MaxBlockSize;
		memcpy(Dest, Source, Std$String$MaxBlockSize);
		Source += Std$String$MaxBlockSize;
		Length -= Std$String$MaxBlockSize;
		++Block;
	}
	void *Dest = Block->Chars.Value = Riva$Memory$alloc_atomic(Length);
	Block->Length.Value = Length;
	memcpy(Dest, Source, Length);
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
}

REAL_METHOD("@", TYP, Std$Real$T, VAL, Std$String$T, TYP, Std$String$T) {
	double Value = ((Std$Real_t *)Args[0].Val)->Value;
	const unsigned char *Format = Std$String$flatten(Args[2].Val);
	unsigned char *String;
	int Length = asprintf(&String, Format, Value);
	Result->Val = Std$String$new_length(String, Length);
	return SUCCESS;
};

typedef struct chars_generator {
	Std$Function_cstate State;
	const Std$String_block *Subject;
	long Left;
	const unsigned char *Next;
} chars_generator;

static long resume_chars_string(Std$Function$result *Result) {
	chars_generator *Gen = Result->State;
	if (--Gen->Left < 0) {
		if ((Gen->Left = (++Gen->Subject)->Length.Value - 1) < 0) return FAILURE;
		Gen->Next = Gen->Subject->Chars.Value;
	};
	Result->Val = Std$String$new_char(*Gen->Next++);
	return SUSPEND;
};

STRING_METHOD("chars", TYP, Std$String$T) {
//@str
//:T
// Generates the characters in <var>str</var> as strings of length <code>1</code>.
	const Std$String_t *Arg = (Std$String_t *)Args[0].Val;
	if (Arg->Length.Value == 0) return FAILURE;
	chars_generator *Gen = new(chars_generator);
	Gen->Subject = Arg->Blocks;
	Gen->Left = Gen->Subject->Length.Value - 1;
	Gen->Next = Gen->Subject->Chars.Value;
	Gen->State.Run = Std$Function$resume_c;
	Gen->State.Invoke = (Std$Function_cresumefn)resume_chars_string;
	Result->Val = Std$String$new_char(*Gen->Next++);
	Result->State = Gen;
	return SUSPEND;
};

STRING_METHOD("centre", TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	const Std$String_t *Str = (Std$String_t *)Args[0].Val;
	int Target = ((Std$Integer_smallt *)Args[1].Val)->Value;
	int Padding = Target - Str->Length.Value;
	if (Padding <= 0) {
		Result->Val = (Std$Object_t *)Str;
		return SUCCESS;
	};
	Std$String_t *New = Std$String$alloc(Str->Count + 2);
	New->Length.Value = Target;
	int LeftLength = Padding / 2;
	unsigned char *LeftChars = Riva$Memory$alloc_atomic(LeftLength + 1);
	memset(LeftChars, ' ', LeftLength);
	LeftChars[LeftLength] = 0;
	New->Blocks[0].Length.Value = LeftLength;
	New->Blocks[0].Chars.Value = LeftChars;
	Std$String_block *Last = mempcpy(New->Blocks + 1, Str->Blocks, Str->Count * sizeof(Std$String_block));
	int RightLength = Padding - LeftLength;
	unsigned char *RightChars = Riva$Memory$alloc_atomic(RightLength + 1);
	memset(RightChars, ' ', RightLength);
	RightChars[RightLength] = 0;
	Last->Length.Value = RightLength;
	Last->Chars.Value = RightChars;
	Result->Val = Std$String$freeze(New);
	return SUCCESS;
};

STRING_METHOD("centre", TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	const Std$String_t *Str = (Std$String_t *)Args[0].Val;
	const Std$String_t *Pad = (Std$String_t *)Args[2].Val;
	if (Pad->Length.Value == 0) {
		Result->Val = (Std$Object_t *)Str;
		return SUCCESS;
	};
	unsigned char PadChar = *(unsigned char *)Pad->Blocks[0].Chars.Value;
	int Target = ((Std$Integer_smallt *)Args[1].Val)->Value;
	int Padding = Target - Str->Length.Value;
	if (Padding <= 0) {
		Result->Val = (Std$Object_t *)Str;
		return SUCCESS;
	};
	Std$String_t *New = Std$String$alloc(Str->Count + 2);
	New->Length.Value = Target;
	int LeftLength = Padding / 2;
	unsigned char *LeftChars = Riva$Memory$alloc_atomic(LeftLength + 1);
	memset(LeftChars, PadChar, LeftLength);
	LeftChars[LeftLength] = 0;
	New->Blocks[0].Length.Value = LeftLength;
	New->Blocks[0].Chars.Value = LeftChars;
	Std$String_block *Last = mempcpy(New->Blocks + 1, Str->Blocks, Str->Count * sizeof(Std$String_block));
	int RightLength = Padding - LeftLength;
	unsigned char *RightChars = Riva$Memory$alloc_atomic(RightLength + 1);
	memset(RightChars, PadChar, RightLength);
	RightChars[RightLength] = 0;
	Last->Length.Value = RightLength;
	Last->Chars.Value = RightChars;
	Result->Val = Std$String$freeze(New);
	return SUCCESS;
};

STRING_METHOD("in", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:T
// Returns <var>a</var> if it is a substring of <var>b</var>, fails otherwise.
	const Std$String_t *Arg0 = (Std$String_t *)Args[0].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[1].Val;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Start = 0;
	for (;;) {
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = (unsigned char *)S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				Result->Arg = Args[0];
				return SUCCESS;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

STRING_METHOD("begins", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:T
// Returns <var>a</var> if it begins with <var>b</var>, fails otherwise.
	const Std$String_t *Subject = (Std$String_t *)Args[0].Val;
	const Std$String_t *Pattern = (Std$String_t *)Args[1].Val;
	
	const Std$String_block *S1 = Subject->Blocks;
	unsigned long SL = S1->Length.Value;
	const unsigned char *SC = S1->Chars.Value;
	const Std$String_block *P1 = Pattern->Blocks;
	unsigned long PL = P1->Length.Value;
	const unsigned char *PC = P1->Chars.Value;
	for (;;) {
		if (PL == 0) {
			Result->Arg = Args[0];
			return SUCCESS;
		};
		if (SL == 0) return FAILURE;
		if (*(SC++) != *(PC++)) break;
		if (--SL == 0) {
			S1 += 1;
			SL = S1->Length.Value;
			SC = S1->Chars.Value;
		};
		if (--PL == 0) {
			P1 += 1;
			PL = P1->Length.Value;
			PC = P1->Chars.Value;
		};
	};
	return FAILURE;
};

STRING_METHOD("ends", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:T
// Returns <var>a</var> if it ends with <var>b</var>, fails otherwise.
	const Std$String_t *Subject = (Std$String_t *)Args[0].Val;
	const Std$String_t *Pattern = (Std$String_t *)Args[1].Val;

	if (Subject->Length.Value < Pattern->Length.Value) return FAILURE;

	const Std$String_block *S1 = Subject->Blocks;
	unsigned long SL = S1->Length.Value;
	unsigned long Skip = Subject->Length.Value - Pattern->Length.Value;
	while (Skip >= SL) {
		Skip -= SL;
		S1 += 1;
		SL = S1->Length.Value;
	};
	SL -= Skip;
	const unsigned char *SC = (unsigned char *)S1->Chars.Value + Skip;
	const Std$String_block *P1 = Pattern->Blocks;
	unsigned long PL = P1->Length.Value;
	const unsigned char *PC = P1->Chars.Value;
	for (;;) {
		if (PL == 0) {
			Result->Arg = Args[0];
			return SUCCESS;
		};
		if (SL == 0) return FAILURE;
		if (*(SC++) != *(PC++)) break;
		if (--SL == 0) {
			S1 += 1;
			SL = S1->Length.Value;
			SC = S1->Chars.Value;
		};
		if (--PL == 0) {
			P1 += 1;
			PL = P1->Length.Value;
			PC = P1->Chars.Value;
		};
	};
	return FAILURE;
};

STRING_METHOD("~", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:Std.Integer.T
// Returns the Levenshtein distance between <var>a</var> and <var>b</var>.
	const Std$String_t *A = (Std$String_t *)Args[0].Val;
	const Std$String_t *B = (Std$String_t *)Args[1].Val;
	int LengthA = A->Length.Value;
	int LengthB = B->Length.Value;
	int Cache[LengthA];
	if (A == B) {
		Result->Val = Std$Integer$Zero;
		return SUCCESS;
	}
	if (LengthA == 0) {
		Result->Val = Std$Integer$new_small(LengthB);
		return SUCCESS;
	}
	if (LengthB == 0) {
		Result->Val = Std$Integer$new_small(LengthA);
		return SUCCESS;
	}
	for (int Index = 0; Index < LengthA; ++Index) Cache[Index] = Index + 1;
	int PosB = 0, R;
	for (Std$String$block *BlockB = B->Blocks; BlockB->Length.Value; ++BlockB) {
		int BlockLengthB = BlockB->Length.Value;
		char *BlockCharsB = BlockB->Chars.Value;
		for (int IndexB = 0; IndexB < BlockLengthB; ++IndexB) {
			char C = BlockCharsB[IndexB];
			int DistanceA = PosB;
			R = PosB;
			++PosB;
			int PosA = 0;
			for (Std$String$block *BlockA = A->Blocks; BlockA->Length.Value; ++BlockA) {
				int BlockLengthA = BlockA->Length.Value;
				char *BlockCharsA = BlockA->Chars.Value;
				for (int IndexA = 0; IndexA < BlockLengthA; ++IndexA) {
					int DistanceB = C == BlockCharsA[IndexA] ? DistanceA : DistanceA + 1;
					DistanceA = Cache[PosA];
					Cache[PosA] = R = DistanceA > R
						? DistanceB > R
							? R + 1
							: DistanceB
						: DistanceB > DistanceA
							? DistanceA + 1
							: DistanceB;
					++PosA;
				}
			}

		}
	}
	Result->Val = Std$Integer$new_small(R);
	return SUCCESS;
}

STRING_METHOD("distance", TYP, Std$String$T, TYP, Std$String$T) {
	Std$String$t *A = (Std$String$t *)Args[0].Val;
	Std$String$t *B = (Std$String$t *)Args[1].Val;
	int LenA = A->Length.Value;
	int LenB = B->Length.Value;
	if (LenA < LenB) {
		Std$String$t *C = A;
		A = B;
		B = C;
		int LenC = LenA;
		LenA = LenB;
		LenB = LenC;
	}
	int *Row0 = alloca((LenB + 1) * sizeof(int));
	int *Row1 = alloca((LenB + 1) * sizeof(int));
	int *Row2 = alloca((LenB + 1) * sizeof(int));
	int Insert = 1;
	int Replace = 1;
	int Swap = 1;
	int Delete = 1;
	for (int J = 0; J <= LenB; ++J) Row1[J] = J * Insert;
	int I = 0;
	char PrevA, PrevB;
	for (Std$String$block *BlockA = A->Blocks; BlockA->Length.Value; ++BlockA) {
		for (char *PtrA = BlockA->Chars.Value, *EndA = PtrA + BlockA->Length.Value; PtrA < EndA; ++PtrA, ++I) {
			Row2[0] = (I + 1) * Delete;
			int J = 0;
			for (Std$String$block *BlockB = B->Blocks; BlockB->Length.Value; ++BlockB) {
				for (char *PtrB = BlockB->Chars.Value, *EndB = PtrB + BlockB->Length.Value; PtrB < EndB; ++PtrB, ++J) {
					int Min = Row1[J] + Replace * (*PtrA != *PtrB);
					if (I > 0 && J > 0 && PrevA == *PtrB && *PtrA == PrevB && Min > Row0[J - 1] + Swap) {
						Min = Row0[J - 1] + Swap;
					}
					if (Min > Row1[J + 1] + Delete) Min = Row1[J + 1] + Delete;
					if (Min > Row2[J] + Insert) Min = Row2[J] + Insert;
					Row2[J + 1] = Min;
					PrevB = *PtrB;
				}
			}
			int *Dummy = Row0;
			Row0 = Row1;
			Row1 = Row2;
			Row2 = Dummy;
			PrevA = *PtrA;
		}
	}
	Result->Val = Std$Integer$new_small(Row1[LenB]);
	return SUCCESS;
}

/*
STRING_METHOD("in", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:Std$Integer$SmallT
// Generate each position where <var>a</var> occurs in <var>b</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[0].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[1].Val;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Position = 0, Start = 0;
	for (;;) {
		++Position;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				find_generator *Generator = new(find_generator);
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Pattern = Pattern;
				Generator->Subject = Subject;
				Generator->Limit = 0xFFFFFFFF;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
				Result->Val = Std$Integer$new_small(Position);
				Result->State = Generator;
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

STRING_METHOD("in", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
//@a
//@b
//@m
//:Std$Integer$SmallT
// Generate each position after <var>m</var> where <var>a</var> occurs in <var>b</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[0].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[1].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Position = Start;
	while (Start >= Subject->Length.Value) {
		Start -= Subject->Length.Value;
		++Subject;
		if (Subject->Length.Value == 0) return FAILURE;
	};
	for (;;) {
		++Position;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				find_generator *Generator = new(find_generator);
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Pattern = Pattern;
				Generator->Subject = Subject;
				Generator->Limit = 0xFFFFFFFF;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
				Result->Val = Std$Integer$new_small(Position);
				Result->State = Generator;
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

STRING_METHOD("in", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@a
//@b
//@m
//@n
//:Std$Integer$SmallT
// Generate each position between <var>m</var> and <var>n</var> where <var>a</var> occurs in <var>b</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[0].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[1].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	unsigned long Limit = ((Std$Integer_smallt *)Args[3].Val)->Value;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Position = Start;
	while (Start >= Subject->Length.Value) {
		Start -= Subject->Length.Value;
		++Subject;
		if (Subject->Length.Value == 0) return FAILURE;
	};
	for (;;) {
		++Position;
		if (Position > Limit) return FAILURE;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				find_generator *Generator = new(find_generator);
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Pattern = Pattern;
				Generator->Subject = Subject;
				Generator->Limit = Limit;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
				Result->Val = Std$Integer$new_small(Position);
				Result->State = Generator;
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};
*/

typedef struct find_generator {
	Std$Function_cstate State;
	const Std$String_block *Subject, *Pattern;
	unsigned long Start, Position, Limit;
} find_generator;

static long resume_find_string_string(Std$Function$result *Result) {
	// Search for occurences of Pattern in Subject starting at Index
	find_generator *Generator = Result->State;
	const Std$String_block *Subject = Generator->Subject;
	const Std$String_block *Pattern = Generator->Pattern;
	unsigned long Position = Generator->Position, Start = Generator->Start;
	unsigned long Limit = Generator->Limit;
	for (;;) {
		++Position;
		if (Position > Limit) return FAILURE;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = (unsigned char *)S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Subject = Subject;
				Result->Val = Std$Integer$new_small(Position);
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

typedef struct find_char_generator {
	Std$Function_cstate State;
	unsigned char Char;
	const Std$String_block *Subject;
	unsigned long Start, Index, Limit;
} find_char_generator;

static long resume_find_char_string(Std$Function$result *Result) {
	find_char_generator *Generator = Result->State;
	const Std$String_block *Subject = Generator->Subject;
	unsigned char Char = Generator->Char;
	unsigned long Index = Generator->Index;
	const unsigned char *SC = (unsigned char *)Subject->Chars.Value + Generator->Start;
	unsigned long SL = Subject->Length.Value - Generator->Start;
	while (SC) {
		const unsigned char *Position = memchr(SC, Char, SL);
		if (Position) {
			unsigned int Last = Position - (unsigned char *)Subject->Chars.Value + 1;
			Generator->Index = Index;
			Generator->Start = Last;
			Generator->Subject = Subject;
			Result->Val = Std$Integer$new_small(Index + Last);
			return SUSPEND;
		};
		Index += Subject->Length.Value;
		++Subject;
		SL = Subject->Length.Value;
		SC = Subject->Chars.Value;
	};
	return FAILURE;
};

SYMBOL($to, "to");

STRING_METHOD("find", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:Std$Integer$T
// Generate each position where <var>b</var> occurs in <var>a</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	if (Arg0->Length.Value == 0) {
		return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &Arg1->Length, 0);
	} else if (Arg0->Length.Value == 1) {
		unsigned char Char = ((unsigned char *)Arg0->Blocks[0].Chars.Value)[0];
		unsigned long Index = 0;
		for (const Std$String_block *Subject = Arg1->Blocks; Subject->Length.Value; ++Subject) {
			const unsigned char *Position = memchr(Subject->Chars.Value, Char, Subject->Length.Value);
			if (Position) {
				find_char_generator *Generator = new(find_char_generator);
				unsigned int Last = Position - (unsigned char *)Subject->Chars.Value + 1;
				Generator->Start = Last;
				Generator->Index = Index;
				Generator->Char = Char;
				Generator->Subject = Subject;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_char_string;
				Result->Val = Std$Integer$new_small(Index + Last);
				Result->State = Generator;
				return SUSPEND;
			};
			Index += Subject->Length.Value;
		};
		return FAILURE;
	} else {
		const Std$String_block *Subject = Arg1->Blocks;
		const Std$String_block *Pattern = Arg0->Blocks;
		unsigned long Position = 0, Start = 0;
		for (;;) {
			++Position;
			const Std$String_block *S1 = Subject;
			unsigned long SL = S1->Length.Value - Start;
			if (SL == 0) {
				S1 = ++Subject;
				SL = S1->Length.Value;
				if (SL == 0) return FAILURE;
				Start = 0;
			};
			const unsigned char *SC = (unsigned char *)S1->Chars.Value + Start;
			++Start;
			const Std$String_block *P1 = Pattern;
			const unsigned char *PC = P1->Chars.Value;
			unsigned long PL = P1->Length.Value;
			for (;;) {
				if (PL == 0) {
					find_generator *Generator = new(find_generator);
					Generator->Start = Start;
					Generator->Position = Position;
					Generator->Pattern = Pattern;
					Generator->Subject = Subject;
					Generator->Limit = 0xFFFFFFFF;
					Generator->State.Run = Std$Function$resume_c;
					Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
					Result->Val = Std$Integer$new_small(Position);
					Result->State = Generator;
					return SUSPEND;
				};
				if (SL == 0) return FAILURE;
				if (*(SC++) != *(PC++)) break;
				if (--SL == 0) {
					S1 += 1;
					SL = S1->Length.Value;
					SC = S1->Chars.Value;
				};
				if (--PL == 0) {
					P1 += 1;
					PL = P1->Length.Value;
					PC = P1->Chars.Value;
				};
			};
		};
		return FAILURE;
	};
};

STRING_METHOD("find", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
//@a
//@b
//@m
//:Std$Integer$SmallT
// Generate each position after <var>m</var> where <var>b</var> occurs in <var>a</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Position = Start;
	while (Start >= Subject->Length.Value) {
		Start -= Subject->Length.Value;
		++Subject;
		if (Subject->Length.Value == 0) return FAILURE;
	};
	for (;;) {
		++Position;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = (unsigned char *)S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				find_generator *Generator = new(find_generator);
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Pattern = Pattern;
				Generator->Subject = Subject;
				Generator->Limit = 0xFFFFFFFF;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
				Result->Val = Std$Integer$new_small(Position);
				Result->State = Generator;
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

STRING_METHOD("find", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
//@a
//@b
//@m
//@n
//:Std$Integer$SmallT
// Generate each position between <var>m</var> and <var>n</var> where <var>b</var> occurs in <var>a</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	unsigned long Limit = ((Std$Integer_smallt *)Args[3].Val)->Value;
	const Std$String_block *Subject = Arg1->Blocks;
	const Std$String_block *Pattern = Arg0->Blocks;
	unsigned long Position = Start;
	while (Start >= Subject->Length.Value) {
		Start -= Subject->Length.Value;
		++Subject;
		if (Subject->Length.Value == 0) return FAILURE;
	};
	for (;;) {
		++Position;
		if (Position > Limit) return FAILURE;
		const Std$String_block *S1 = Subject;
		unsigned long SL = S1->Length.Value - Start;
		if (SL == 0) {
			S1 = ++Subject;
			SL = S1->Length.Value;
			if (SL == 0) return FAILURE;
			Start = 0;
		};
		const unsigned char *SC = (unsigned char *)S1->Chars.Value + Start;
		++Start;
		const Std$String_block *P1 = Pattern;
		const unsigned char *PC = P1->Chars.Value;
		unsigned long PL = P1->Length.Value;
		for (;;) {
			if (PL == 0) {
				find_generator *Generator = new(find_generator);
				Generator->Start = Start;
				Generator->Position = Position;
				Generator->Pattern = Pattern;
				Generator->Subject = Subject;
				Generator->Limit = Limit;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_find_string_string;
				Result->Val = Std$Integer$new_small(Position);
				Result->State = Generator;
				return SUSPEND;
			};
			if (SL == 0) return FAILURE;
			if (*(SC++) != *(PC++)) break;
			if (--SL == 0) {
				S1 += 1;
				SL = S1->Length.Value;
				SC = S1->Chars.Value;
			};
			if (--PL == 0) {
				P1 += 1;
				PL = P1->Length.Value;
				PC = P1->Chars.Value;
			};
		};
	};
	return FAILURE;
};

STRING_METHOD("reverse", TYP, Std$String$T) {
//@s
//:Std$String$T
// returns the a string consisting of the same bytes as <var>s</var> in reverse order.
	Std$String_t *Str = (Std$String_t *)Args[0].Val;
	Std$String_t *New = Std$String$alloc(Str->Count);
	New->Length.Value = Str->Length.Value;
	int BlockCount = New->Count = Str->Count;
	for (int I = 0; I < BlockCount; ++I) {
		Std$String_block *OldBlock = Str->Blocks + (BlockCount - 1 - I);
		int Length = OldBlock->Length.Value;
		const unsigned char *OldChars = OldBlock->Chars.Value;
		unsigned char *NewChars = Riva$Memory$alloc_atomic(Length + 1);
		for (int J = 0; J < Length; ++J) NewChars[J] = OldChars[Length - 1 - J];
		NewChars[Length] = 0;
		New->Blocks[I].Length.Value = Length;
		New->Blocks[I].Chars.Value = NewChars;
	};
	Result->Val = Std$String$freeze(New);
	return SUCCESS;
};

typedef struct to_real_state {
	Std$Function_cstate C;
	double Current, Limit, Increment;
} to_real_state;

static long resume_to_real_inc(Std$Function$result * restrict Result) {
	to_real_state *State = Result->State;
	double Current = State->Current + State->Increment;
	if (Current > State->Limit) return FAILURE;
	if (Current == State->Limit) {
		Result->Val = Std$Real$new(Current);
		return SUCCESS;
	};
	State->Current = Current;
	Result->Val = Std$Real$new(Current);
	return SUSPEND;
};

static long resume_to_real_dec(Std$Function$result *Result) {
	to_real_state *State = Result->State;
	double Current = State->Current + State->Increment;
	if (Current < State->Limit) return FAILURE;
	if (Current == State->Limit) {
		Result->Val = Std$Real$new(Current);
		return SUCCESS;
	};
	State->Current = Current;
	Result->Val = Std$Real$new(Current);
	return SUSPEND;
};

REAL_METHOD("to", TYP, Std$Real$T, TYP, Std$Real$T) {
//@a
//@b
//:T
// Generates real numbers from <var>a</var> to <var>b</var> in increments of <code>1.0</code>.
	double From = ((Std$Real_t *)Args[0].Val)->Value;
	double To = ((Std$Real_t *)Args[1].Val)->Value;
	if (From > To) return FAILURE;
	if (From == To) {
		Result->Val = Args[0].Val;
		return SUCCESS;
	};
	to_real_state *State = new(to_real_state);
	State->Current = From;
	State->Limit = To;
	State->Increment = 1.0;
	State->C.Run = Std$Function$resume_c;
	State->C.Invoke = (Std$Function_cresumefn)resume_to_real_inc;
	Result->Val = Args[0].Val;
	Result->State = State;
	return SUSPEND;
};

extern void resume_repeat_value(void);

typedef struct repeat_value_state {
	void *Run, *Resume, *Chain;
	Std$Object_t *Value;
} repeat_value_state;

REAL_METHOD("to", TYP, Std$Real$T, TYP, Std$Real$T, TYP, Std$Real$T) {
//@a
//@b
//@c
//:T
// Generates real numbers from <var>a</var> to <var>b</var> in increments of <code>c</code>.
	double From = ((Std$Real_t *)Args[0].Val)->Value;
	double To = ((Std$Real_t *)Args[1].Val)->Value;
	double Increment = ((Std$Real_t *)Args[2].Val)->Value;
	if (Increment > 0.0) {
		if (From > To) return FAILURE;
		if (From == To) {
			Result->Val = Args[0].Val;
			return SUCCESS;
		};
		to_real_state *State = new(to_real_state);
		State->Current = From;
		State->Limit = To;
		State->Increment = Increment;
		State->C.Run = Std$Function$resume_c;
		State->C.Invoke = (Std$Function_cresumefn)resume_to_real_inc;
		Result->Val = Args[0].Val;
		Result->State = State;
		return SUSPEND;
	} else if (Increment < 0.0) {
		if (From < To) return FAILURE;
		if (From == To) {
			Result->Val = Args[0].Val;
			return SUCCESS;
		};
		to_real_state *State = new(to_real_state);
		State->Current = From;
		State->Limit = To;
		State->Increment = Increment;
		State->C.Run = Std$Function$resume_c;
		State->C.Invoke = (Std$Function_cresumefn)resume_to_real_dec;
		Result->Val = Args[0].Val;
		Result->State = State;
		return SUSPEND;
	} else {
		repeat_value_state *State = new(repeat_value_state);
		Result->Val = State->Value = Args[0].Val;
		State->Run = resume_repeat_value;
		Result->State = State;
		return SUSPEND;
	};
};

STRING_METHOD("map", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$String$T) {
//@str
//@old
//@new
//:T
// Returns a copy of <var>str</var> with each character in <var>old</var> replaced with the corresponding character in <var>new</var>.
	const Std$String_t *Source = (Std$String_t *)Args[0].Val;
	if (Source->Length.Value == 0) {
		Result->Val = (Std$Object_t *)Source;
		return SUCCESS;
	};
	char Map[256];
	for (int I = 0; I < 255; ++I) Map[I] = I;
	if (((Std$String_t *)Args[1].Val)->Length.Value != ((Std$String_t *)Args[2].Val)->Length.Value) {
		Result->Val = Std$String$new("Operands to :map must have same length");
		return MESSAGE;
	};
	const Std$String_block *ToBlock = ((Std$String_t *)Args[2].Val)->Blocks;
	const unsigned char *ToChars = ToBlock->Chars.Value;
	int K = 0;
	for (const Std$String_block *FromBlock = ((Std$String_t *)Args[1].Val)->Blocks; FromBlock->Length.Value; ++FromBlock) {
		const unsigned char *FromChars = FromBlock->Chars.Value;
		for (int J = 0; J < FromBlock->Length.Value; ++J) {
			Map[FromChars[J]] = ToChars[K];
			if (++K >= ToBlock->Length.Value) {
				++ToBlock;
				ToChars = ToBlock->Chars.Value;
				K = 0;
			};
		};
	};
	int Size = sizeof(Std$String_t) + (Source->Count + 1) * sizeof(Std$String_block);
	Std$String_t *Dest = (Std$String_t *)Riva$Memory$alloc_stubborn(Size);
	memcpy(Dest, Source, Size);
	for (int I = 0; I < Source->Count; ++I) {
		int Length = Source->Blocks[I].Length.Value;
		const unsigned char *SourceChars = Source->Blocks[I].Chars.Value;
		unsigned char *DestChars = Riva$Memory$alloc_atomic(Length);
		for (int J = 0; J < Length; ++J) DestChars[J] = Map[SourceChars[J]];
		Dest->Blocks[I].Chars.Value = DestChars;
	};
	Result->Val = Std$String$freeze(Dest);
	return SUCCESS;
};

extern int match_substring(const Std$String_block *StartBlock, int StartOffset, const Std$String_t *String, const Std$String_block **EndBlock, int *EndOffset);

static Std$String_t *map_next(int L, const Std$String_block *StartBlock, int StartOffset, const Agg$List_t *FromList, const Agg$List_t *ToList) {
	for (const Agg$List_node *FromNode = FromList->Head, *ToNode = ToList->Head; FromNode; FromNode = FromNode->Next, ToNode = ToNode->Next) {
		const Std$String_block *EndBlock;
		int EndOffset;
		if (match_substring(StartBlock, StartOffset, (Std$String_t *)FromNode->Value, &EndBlock, &EndOffset)) {
			const Std$String_t *To = (Std$String_t *)ToNode->Value;
			Std$String_t *New = map_next(L + To->Count, EndBlock, EndOffset, FromList, ToList);
			memcpy(New->Blocks + L, To->Blocks, To->Count * sizeof(Std$String_block));
			return New;
		};
	};
	const Std$String_block *MiddleBlock = StartBlock;
	int MiddleOffset = StartOffset;
	while (MiddleBlock->Length.Value) {
		for (const Agg$List_node *FromNode = FromList->Head, *ToNode = ToList->Head; FromNode; FromNode = FromNode->Next, ToNode = ToNode->Next) {
			const Std$String_block *EndBlock;
			int EndOffset;
			if (match_substring(MiddleBlock, MiddleOffset, (Std$String_t *)FromNode->Value, &EndBlock, &EndOffset)) {
				const Std$String_t *To = (Std$String_t *)ToNode->Value;
				if (MiddleOffset == 0) {
					MiddleBlock--;
					MiddleOffset = MiddleBlock->Length.Value;
				};
				int M = MiddleBlock - StartBlock + 1;
				Std$String_t *New = map_next(L + M + To->Count, EndBlock, EndOffset, FromList, ToList);
				memcpy(New->Blocks + L, StartBlock, M * sizeof(Std$String_block));
				New->Blocks[L].Chars.Value += StartOffset;
				New->Blocks[L + M - 1].Length.Value = MiddleOffset;
				New->Blocks[L].Length.Value -= StartOffset;
				memcpy(New->Blocks + L + M, To->Blocks, To->Count * sizeof(Std$String_block));
				return New;
			};
		};
		if (++MiddleOffset >= MiddleBlock->Length.Value) {
			MiddleBlock++;
			MiddleOffset = 0;
		};
	};
	int M = MiddleBlock - StartBlock + 1;
	Std$String_t *New = (Std$String_t *)Riva$Memory$alloc_stubborn(sizeof(Std$String_t) + (L + M + 1) * sizeof(Std$String_block));
	New->Type = Std$String$T;
	New->Length.Type = Std$Integer$SmallT;
	memcpy(New->Blocks + L, StartBlock, M * sizeof(Std$String_block));
	New->Blocks[L].Chars.Value += StartOffset;
	New->Blocks[L].Length.Value -= StartOffset;
	return New;
};

STRING_METHOD("map", TYP, Std$String$T, TYP, Agg$List$T, TYP, Agg$List$T) {
//@string
//@from
//@to
//:T
// Returns a copy of <var>string</var> with each occurance of a string in <var>from</var> with the corresponding string in <var>to</var>.
// The strings in <var>from</var> are tried in the order they occur in <var>from</var>.
	const Std$String_t *Subject = (Std$String_t *)Args[0].Val;
	const Agg$List_t *FromList = (Agg$List_t *)Args[1].Val;
	const Agg$List_t *ToList = (Agg$List_t *)Args[2].Val;
	if (FromList->Length != ToList->Length) {
		Result->Val = Std$String$new("List lengths do not match");
		return MESSAGE;
	};
	for (const Agg$List_node *Node = FromList->Head; Node; Node = Node->Next) {
		if (Node->Value->Type != Std$String$T) {
			Result->Val = Std$String$new("Search value is not a string");
			return MESSAGE;
		};
	};
	for (const Agg$List_node *Node = ToList->Head; Node; Node = Node->Next) {
		if (Node->Value->Type != Std$String$T) {
			Result->Val = Std$String$new("Replacement value is not a string");
			return MESSAGE;
		};
	};
	Std$String_t *New = map_next(0, Subject->Blocks, 0, (Agg$List_t *)Args[1].Val, (Agg$List_t *)Args[2].Val);
	for (const Std$String_block *Block = New->Blocks; Block->Length.Value; ++Block) {
		New->Length.Value += Block->Length.Value;
		New->Count += 1;
	};
	Result->Val = Std$String$freeze(New);
	return SUCCESS;
};

STRING_METHOD("lower", TYP, Std$String$T) {
//@str
//:T
// Equivalent to <code>str:map(</code><id>Upper</id><code>, </code><id>Lower</id><code>)</code>.
        static unsigned char Map[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
            25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
            48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99, 100, 101,
            102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
            108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
            126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
            144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
            162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
            180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
            198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
            216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
            234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251,
            252, 253, 254, 255
        };
	const Std$String_t *Source = (Std$String_t *)Args[0].Val;
	if (Source->Length.Value == 0) {
		Result->Val = (Std$Object_t *)Source;
		return SUCCESS;
	};
	int Size = sizeof(Std$String_t) + (Source->Count + 1) * sizeof(Std$String_block);
	Std$String_t *Dest = (Std$String_t *)Riva$Memory$alloc_stubborn(Size);
	memcpy(Dest, Source, Size);
	for (int I = 0; I < Source->Count; ++I) {
		int Length = Source->Blocks[I].Length.Value;
		const unsigned char *SourceChars = Source->Blocks[I].Chars.Value;
		unsigned char *DestChars = Riva$Memory$alloc_atomic(Length);
		for (int J = 0; J < Length; ++J) DestChars[J] = Map[SourceChars[J]];
		Dest->Blocks[I].Chars.Value = DestChars;
	};
	Result->Val = Std$String$freeze(Dest);
	return SUCCESS;
};

STRING_METHOD("upper", TYP, Std$String$T) {
//@str
//:T
// Equivalent to <code>str:map(</code><id>Lower</id><code>, </code><id>Upper</id><code>)</code>.
        static unsigned char Map[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
            25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
            48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
            71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93,
            94, 95, 96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
            85, 86, 87, 88, 89, 90, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
            136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
            154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
            172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
            190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
            208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
            226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
            244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
        };
	const Std$String_t *Source = (Std$String_t *)Args[0].Val;
	if (Source->Length.Value == 0) {
		Result->Val = (Std$Object_t *)Source;
		return SUCCESS;
	};
	int Size = sizeof(Std$String_t) + (Source->Count + 1) * sizeof(Std$String_block);
	Std$String_t *Dest = (Std$String_t *)Riva$Memory$alloc_stubborn(Size);
	memcpy(Dest, Source, Size);
	for (int I = 0; I < Source->Count; ++I) {
		int Length = Source->Blocks[I].Length.Value;
		unsigned const char *SourceChars = Source->Blocks[I].Chars.Value;
		unsigned char *DestChars = Riva$Memory$alloc_atomic(Length);
		for (int J = 0; J < Length; ++J) DestChars[J] = Map[SourceChars[J]];
		Dest->Blocks[I].Chars.Value = DestChars;
	};
	Result->Val = Std$String$freeze(Dest);
	return SUCCESS;
};

typedef struct any_char_generator {
	Std$Function_cstate State;
	uint8_t Mask[32];
	const Std$String_block *Subject;
	unsigned long Start, Index, Limit;
} any_char_generator;

static inline const unsigned char *findcset(const unsigned char *Chars, uint8_t *Mask, int Length) {
    while (Length) {
    	--Length;
        unsigned char Char = *Chars;
        if (Mask[Char >> 3] & (1 << (Char & 7))) return Chars;
        ++Chars;
    };
    return 0;
};

static long resume_any_char_string(Std$Function$result * restrict Result) {
	any_char_generator *Generator = Result->State;
	const Std$String_block *Subject = Generator->Subject;
	unsigned long Index = Generator->Index;
	const unsigned char *SC = (unsigned char *)Subject->Chars.Value + Generator->Start;
	unsigned long SL = Subject->Length.Value - Generator->Start;
	while (SC) {
		const unsigned char *Position = findcset(SC, Generator->Mask, SL);
		if (Position) {
			unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
			Generator->Index = Index;
			Generator->Start = Last;
			Generator->Subject = Subject;
			Result->Val = Std$Integer$new_small(Index + Last);
			return SUSPEND;
		};
		Index += Subject->Length.Value;
		++Subject;
		SL = Subject->Length.Value;
		SC = Subject->Chars.Value;
	};
	return FAILURE;
};

STRING_METHOD("any", TYP, Std$String$T, TYP, Std$String$T) {
//@str
//@chars
//:Std$Integer$SmallT
// Generates all positions in <var>str</var> with a character in <var>chars</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	if (Arg1->Length.Value == 0) return FAILURE;
	if (Arg0->Length.Value == 0) {
		return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &Arg1->Length, 0);
	} else {
	    uint8_t Mask[32];
	    memset(Mask, 0, 32);
	    for (const Std$String_block *Block = Arg0->Blocks; Block->Length.Value; ++Block) {
	        const unsigned char *Chars = Block->Chars.Value;
	        for (int I = 0; I < Block->Length.Value; ++I) {
	            unsigned char Char = Chars[I];
	            Mask[Char >> 3] |= 1 << (Char & 7);
	        };
	    };
		unsigned long Index = 0;
		for (const Std$String_block *Subject = Arg1->Blocks; Subject->Length.Value; ++Subject) {
			const unsigned char *Position = findcset(Subject->Chars.Value, Mask, Subject->Length.Value);
			if (Position) {
				any_char_generator *Generator = new(any_char_generator);
				unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
				Generator->Start = Last;
				Generator->Index = Index;
				memcpy(Generator->Mask, Mask, 32);
				Generator->Subject = Subject;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_any_char_string;
				Result->Val = Std$Integer$new_small(Index + Last);
				Result->State = Generator;
				return SUSPEND;
			};
			Index += Subject->Length.Value;
		};
		return FAILURE;
	};
};

STRING_METHOD("any", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
//@str
//@chars
//@m
//:Std$Integer$SmallT
// Generates all positions after <var>m</var> in <var>str</var> with a character in <var>chars</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	if (Arg1->Length.Value <= Start) return FAILURE;
	if (Arg0->Length.Value == 0) {
		return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &Arg1->Length, 0);
	} else {
	    uint8_t Mask[32];
	    memset(Mask, 0, 32);
	    for (const Std$String_block *Block = Arg0->Blocks; Block->Length.Value; ++Block) {
	        const unsigned char *Chars = Block->Chars.Value;
	        for (int I = 0; I < Block->Length.Value; ++I) {
	            unsigned char Char = Chars[I];
	            Mask[Char >> 3] |= 1 << (Char & 7);
	        };
	    };
		unsigned long Index = 0;
		const Std$String_block *Subject = Arg1->Blocks;
		while (Start >= Subject->Length.Value) {
			Index += Subject->Length.Value;
			Start -= Subject->Length.Value;
			++Subject;
		};
		const unsigned char *SC = Subject->Chars.Value + Start;
		unsigned long SL = Subject->Length.Value - Start;
		while (Subject->Length.Value) {
			const unsigned char *Position = findcset(SC, Mask, SL);
			if (Position) {
				any_char_generator *Generator = new(any_char_generator);
				unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
				Generator->Start = Last;
				Generator->Index = Index;
				memcpy(Generator->Mask, Mask, 32);
				Generator->Subject = Subject;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_any_char_string;
				Result->Val = Std$Integer$new_small(Index + Last);
				Result->State = Generator;
				return SUSPEND;
			};
			Index += Subject->Length.Value;
			++Subject;
			SC = Subject->Chars.Value;
			SL = Subject->Length.Value;
		};
		return FAILURE;
	};
};

typedef struct split_char_generator {
	Std$Function_cstate State;
	uint8_t Mask[32];
	const Std$String_block *SB;
	const unsigned char *SC;
	unsigned long SL, SI;
} split_char_generator;

static inline int charcset(unsigned char Char, uint8_t *CSet) {
    return CSet[Char >> 3] & (1 << (Char & 7));
};

static long resume_split_char_string(Std$Function$result *Result) {
	split_char_generator *Generator = Result->State;
	uint8_t *Mask = Generator->Mask;
	unsigned long SI = Generator->SI;
	const Std$String_block *SB = Generator->SB;
	const unsigned char *SC = Generator->SC;
	unsigned long SL = Generator->SL;
	while (charcset(*SC, Mask) != 0) {
		++SI;
		if (--SL == 0) {
			++SB;
			SC = SB->Chars.Value;
			SL = SB->Length.Value;
			if (SC == 0) return FAILURE;
		} else {
			++SC;
		};
	};
	unsigned long SI0 = SI;
	const Std$String_block *SB0 = SB;
	const unsigned char *SC0 = SC;
	unsigned long SL0 = SL;
	while (charcset(*SC, Mask) == 0) {
		++SI;
		if (--SL == 0) {
			++SB;
			SC = SB->Chars.Value;
			SL = SB->Length.Value;
			if (SC == 0) {
				int NoOfBlocks = SB - SB0;
				Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
				Slice->Length.Value = SI - SI0;
				Slice->Blocks[0].Length.Value = SL0;
				Slice->Blocks[0].Chars.Value = SC0;
				if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
				Result->Val = Std$String$freeze(Slice);
				return SUCCESS;
			};
		} else {
			++SC;
		};
	};
	if (SL == SB->Length.Value) {
		int NoOfBlocks = SB - SB0;
		Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
		Slice->Length.Value = SI - SI0;
		Slice->Blocks[0].Length.Value = SL0;
		Slice->Blocks[0].Chars.Value = SC0;
		if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
		Result->Val = Std$String$freeze(Slice);
	} else {
		int NoOfBlocks = (SB - SB0) + 1;
		Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
		Slice->Length.Value = SI - SI0;
		if (--NoOfBlocks) {
			Slice->Blocks[0].Length.Value = SL0;
			Slice->Blocks[0].Chars.Value = SC0;
			Slice->Blocks[NoOfBlocks].Length.Value = SB->Length.Value - SL;
			Slice->Blocks[NoOfBlocks].Chars.Value = SB->Chars.Value;
			if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
		} else {
			Slice->Blocks[0].Length.Value = SL0 - SL;
			Slice->Blocks[0].Chars.Value = SC0;
		};
		Result->Val = Std$String$freeze(Slice);
	};
	Generator->SC = SC;
	Generator->SL = SL;
	Generator->SI = SI;
	Generator->SB = SB;
	return SUSPEND;
};

STRING_METHOD("split", TYP, Std$String$T, TYP, Std$String$T) {
//@str
//@sep
//:T
// Generates substrings of <var>str</var> separated by characters in <var>sep</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	if (Arg1->Length.Value == 0) return FAILURE;
	if (Arg0->Length.Value == 0) {
		if (Arg1->Length.Value == 0) return FAILURE;
		chars_generator *Gen = new(chars_generator);
		Gen->Subject = Arg1->Blocks;
		Gen->Left = Gen->Subject->Length.Value - 1;
		Gen->Next = Gen->Subject->Chars.Value;
		Gen->State.Run = Std$Function$resume_c;
		Gen->State.Invoke = (Std$Function_cresumefn)resume_chars_string;
		Result->Val = Std$String$new_char(*Gen->Next++);
		Result->State = Gen;
		return SUSPEND;
	} else {
	    uint8_t Mask[32];
	    memset(Mask, 0, 32);
	    for (const Std$String_block *Block = Arg0->Blocks; Block->Length.Value; ++Block) {
	        const unsigned char *Chars = Block->Chars.Value;
	        for (int I = 0; I < Block->Length.Value; ++I) {
	            unsigned char Char = Chars[I];
	            Mask[Char >> 3] |= 1 << (Char & 7);
	        };
	    };
		unsigned long SI = 1;
		const Std$String_block *SB = Arg1->Blocks;
		const unsigned char *SC = SB->Chars.Value;
		unsigned long SL = SB->Length.Value;
		while (charcset(*SC, Mask) != 0) {
			++SI;
			if (--SL == 0) {
				++SB;
				SC = SB->Chars.Value;
				SL = SB->Length.Value;
				if (SC == 0) return FAILURE;
			} else {
				++SC;
			};
		};
		unsigned long SI0 = SI;
        const Std$String_block *SB0 = SB;
        const unsigned char *SC0 = SC;
        unsigned long SL0 = SL;
        while (charcset(*SC, Mask) == 0) {
        	++SI;
        	if (--SL == 0) {
        		++SB;
        		SC = SB->Chars.Value;
        		SL = SB->Length.Value;
        		if (SC == 0) {
        			int NoOfBlocks = SB - SB0;
        			Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
        			Slice->Length.Value = SI - SI0;
        			Slice->Blocks[0].Length.Value = SL0;
        			Slice->Blocks[0].Chars.Value = SC0;
        			if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
        			Result->Val = Std$String$freeze(Slice);
        			return SUCCESS;
        		};
        	} else {
        		++SC;
        	};
        };
        if (SL == SB->Length.Value) {
			int NoOfBlocks = SB - SB0;
			Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
			Slice->Length.Value = SI - SI0;
			Slice->Blocks[0].Length.Value = SL0;
			Slice->Blocks[0].Chars.Value = SC0;
			if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
			Result->Val = Std$String$freeze(Slice);
        } else {
			int NoOfBlocks = (SB - SB0) + 1;
			Std$String_t *Slice = Std$String$alloc(NoOfBlocks);
			Slice->Length.Value = SI - SI0;
			if (--NoOfBlocks) {
				Slice->Blocks[0].Length.Value = SL0;
				Slice->Blocks[0].Chars.Value = SC0;
				Slice->Blocks[NoOfBlocks].Length.Value = SB->Length.Value - SL;
				Slice->Blocks[NoOfBlocks].Chars.Value = SB->Chars.Value;
				if (--NoOfBlocks) memcpy(Slice->Blocks + 1, SB0 + 1, NoOfBlocks * sizeof(Std$String_block));
			} else {
				Slice->Blocks[0].Length.Value = SL0 - SL;
				Slice->Blocks[0].Chars.Value = SC0;
			};
			Result->Val = Std$String$freeze(Slice);
        };
        split_char_generator *Generator = new(split_char_generator);
        Generator->SC = SC;
        Generator->SL = SL;
        Generator->SI = SI;
        Generator->SB = SB;
        memcpy(Generator->Mask, Mask, 32);
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function_cresumefn)resume_split_char_string;
		Result->State = Generator;
		return SUSPEND;
	};
};

typedef struct skip_char_generator {
	Std$Function_cstate State;
	uint8_t Mask[32];
	const Std$String_block *Subject;
	unsigned long Start, Index, Limit;
} skip_char_generator;

static inline const unsigned char *skipcset(const unsigned char *Chars, uint8_t *Mask, int Length) {
    while (Length) {
    	--Length;
        unsigned char Char = *Chars;
        if (!(Mask[Char >> 3] & (1 << (Char & 7)))) return Chars;
        ++Chars;
    };
    return 0;
};

static long resume_skip_char_string(Std$Function$result *Result) {
	skip_char_generator *Generator = Result->State;
	const Std$String_block *Subject = Generator->Subject;
	unsigned long Index = Generator->Index;
	const unsigned char *SC = Subject->Chars.Value + Generator->Start;
	unsigned long SL = Subject->Length.Value - Generator->Start;
	while (SC) {
		const unsigned char *Position = skipcset(SC, Generator->Mask, SL);
		if (Position) {
			unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
			Generator->Index = Index;
			Generator->Start = Last;
			Generator->Subject = Subject;
			Result->Val = Std$Integer$new_small(Index + Last);
			return SUSPEND;
		};
		Index += Subject->Length.Value;
		++Subject;
		SL = Subject->Length.Value;
		SC = Subject->Chars.Value;
	};
	return FAILURE;
};

STRING_METHOD("skip", TYP, Std$String$T, TYP, Std$String$T) {
//@str
//@chars
//:Std$Integer$SmallT
// Generates all positions in <var>str</var> with a character not in <var>chars</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	if (Arg0->Length.Value == 0) {
		return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &Arg1->Length, 0);
	} else {
	    uint8_t Mask[32];
	    memset(Mask, 0, 32);
	    for (const Std$String_block *Block = Arg0->Blocks; Block->Length.Value; ++Block) {
	        const unsigned char *Chars = Block->Chars.Value;
	        for (int I = 0; I < Block->Length.Value; ++I) {
	            unsigned char Char = Chars[I];
	            Mask[Char >> 3] |= 1 << (Char & 7);
	        };
	    };
		unsigned long Index = 0;
		for (const Std$String_block *Subject = Arg1->Blocks; Subject->Length.Value; ++Subject) {
			const unsigned char *Position = skipcset(Subject->Chars.Value, Mask, Subject->Length.Value);
			if (Position) {
				skip_char_generator *Generator = new(skip_char_generator);
				unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
				Generator->Start = Last;
				Generator->Index = Index;
				memcpy(Generator->Mask, Mask, 32);
				Generator->Subject = Subject;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn)resume_skip_char_string;
				Result->Val = Std$Integer$new_small(Index + Last);
				Result->State = Generator;
				return SUSPEND;
			};
			Index += Subject->Length.Value;
		};
		return FAILURE;
	};
};

STRING_METHOD("skip", TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
//@str
//@chars
//@m
//:Std$Integer$SmallT
// Generates all positions after <var>m</var> in <var>str</var> with a character not in <var>chars</var>.
	const Std$String_t *Arg0 = (Std$String_t *)Args[1].Val;
	const Std$String_t *Arg1 = (Std$String_t *)Args[0].Val;
	int Start = ((Std$Integer_smallt *)Args[2].Val)->Value - 1;
	if (Start < 0) Start += Arg1->Length.Value + 1;
	if (Start < 0) return FAILURE;
	if (Arg1->Length.Value <= Start) return FAILURE;
	if (Arg0->Length.Value == 0) {
		return Std$Function$call(Std$Integer$ToSmallSmall, 2, Result, Std$Integer$new_small(1), 0, &Arg1->Length, 0);
	} else {
	    uint8_t Mask[32];
	    memset(Mask, 0, 32);
	    for (const Std$String_block *Block = Arg0->Blocks; Block->Length.Value; ++Block) {
	        const unsigned char *Chars = Block->Chars.Value;
	        for (int I = 0; I < Block->Length.Value; ++I) {
	            unsigned char Char = Chars[I];
	            Mask[Char >> 3] |= 1 << (Char & 7);
	        };
	    };
		unsigned long Index = 0;
		const Std$String_block *Subject = Arg1->Blocks;
		while (Start >= Subject->Length.Value) {
			Index += Subject->Length.Value;
			Start -= Subject->Length.Value;
			++Subject;
		};
		const unsigned char *SC = Subject->Chars.Value + Start;
		unsigned long SL = Subject->Length.Value - Start;
		while (Subject->Length.Value) {
			const unsigned char *Position = skipcset(SC, Mask, SL);
			if (Position) {
				skip_char_generator *Generator = new(skip_char_generator);
				unsigned int Last = Position - (const unsigned char *)Subject->Chars.Value + 1;
				Generator->Start = Last;
				Generator->Index = Index;
				memcpy(Generator->Mask, Mask, 32);
				Generator->Subject = Subject;
				Generator->State.Run = Std$Function$resume_c;
				Generator->State.Invoke = (Std$Function_cresumefn) resume_skip_char_string;
				Result->Val = Std$Integer$new_small(Index + Last);
				Result->State = Generator;
				return SUSPEND;
			};
			Index += Subject->Length.Value;
			++Subject;
			SC = Subject->Chars.Value;
			SL = Subject->Length.Value;
		};
		return FAILURE;
	};
};

SYMBOL($any, "any");
SYMBOL($skip, "skip");
SYMBOL($INDEX, "[]");

STRING_METHOD("before", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:T
// Returns the largest initial substring of <var>a</var> that does not contain any characters in <var>b</var>.
	static Std$Integer_smallt One[] = {{Std$Integer$SmallT, 1}};
	Std$Function_result Result0;
	Std$Function_argument Args0[3] = {{Args[0].Val, 0}, {Args[1].Val, 0}, {0, 0}};
	switch (Std$Function$invoke($any, 2, &Result0, Args0)) {
	case FAILURE: {
		Result->Val = Args[0].Val;
		return SUCCESS;
	};
	case MESSAGE: {
		Result->Val = Result0.Val;
		return MESSAGE;
	};
	default: break;
	};
	Args0[1].Val = (Std$Object_t *)One;
	Args0[2].Val = Result0.Val;
	return Std$Function$invoke($INDEX, 3, Result, Args0);
};

STRING(EmptyString, "");

STRING_METHOD("after", TYP, Std$String$T, TYP, Std$String$T) {
//@a
//@b
//:$T
// Returns the substring of <var>a</var> after the first occurance of a character in <var>b</var>. Returns all of <var>a</var> if no character in <var>b</var> occurrs in <var>a</var>.
	static Std$Integer_smallt Zero[] = {{Std$Integer$SmallT, 0}};
	Std$Function_result Result0;
	Std$Function_argument Args0[3] = {{Args[0].Val, 0}, {Args[1].Val, 0}, {0, 0}};
	switch (Std$Function$invoke($any, 2, &Result0, Args0)) {
	case FAILURE: {
		Result->Val = Args[0].Val;
		return SUCCESS;
	};
	case MESSAGE: {
		Result->Val = Result0.Val;
		return MESSAGE;
	};
	default: break;
	};
	Args0[2].Val = Result0.Val;
	switch (Std$Function$invoke($skip, 3, &Result0, Args0)) {
	case FAILURE: {
		Result->Val = (Std$Object_t *)EmptyString;
		return SUCCESS;
	};
	case MESSAGE: {
		Result->Val = Result0.Val;
		return MESSAGE;
	};
	default: break;
	};
	Args0[1].Val = Result0.Val;
	Args0[2].Val = (Std$Object_t *)Zero;
	return Std$Function$invoke($INDEX, 3, Result, Args0);
};

static inline Std$Object_t *finish_integer(mpz_t Z) {
	if (mpz_fits_slong_p(Z)) {
		return Std$Integer$new_small(mpz_get_si(Z));
	} else {
		return Std$Integer$new_big(Z);
	};
};

static inline Std$Object_t *finish_rational(mpq_t R) {
	if (mpz_cmp_si(mpq_denref(R), 1)) {
		return Std$Rational$new(R);
	} else if (mpz_fits_slong_p(mpq_numref(R))) {
		return Std$Integer$new_small(mpz_get_si(mpq_numref(R)));
	} else {
		return Std$Integer$new_big(mpq_numref(R));
	};
};

STRING_METHOD("@", TYP, Std$String$T, VAL, Std$Number$T) {
	const unsigned char *Buffer = Std$String$flatten(Args[0].Val);
	mpq_t R;
	mpq_init(R);
	if (mpq_set_str(R, Buffer, 10) == 0) {
		mpq_canonicalize(R);
		Result->Val = finish_rational(R);
		return SUCCESS;
	} else {
		unsigned char *Tail;
		double Val = strtod(Buffer, &Tail);
		if (Tail > Buffer) {
			Result->Val = Std$Real$new(atof(Buffer));
			return SUCCESS;
		} else {
			return FAILURE;
		};
	};
};

RATIONAL_METHOD("@", TYP, Std$String$T, VAL, Std$Rational$T) {
//@str
//:T
// <var>str</var> should be of the form <code>"num/den"</code>.
	Std$Rational_t *R = new(Std$Rational_t);
	R->Type = Std$Rational$T;
	mpq_init(R->Value);
	mpq_set_str(R->Value, Std$String$flatten(Args[0].Val), 10);
	mpq_canonicalize(R->Value);
	Result->Val = (Std$Object_t *)R;
	return SUCCESS;
};

INTEGER_METHOD("/", TYP, Std$Integer$BigT, TYP, Std$Integer$BigT) {
	Std$Integer_bigt *A = (Std$Integer_bigt *)Args[0].Val;
	Std$Integer_bigt *B = (Std$Integer_bigt *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpz_set(mpq_numref(C), A->Value);
	mpz_set(mpq_denref(C), B->Value);
	mpq_canonicalize(C);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

INTEGER_METHOD("/", TYP, Std$Integer$SmallT, TYP, Std$Integer$BigT) {
	Std$Integer_smallt *A = (Std$Integer_smallt *)Args[0].Val;
	Std$Integer_bigt *B = (Std$Integer_bigt *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpz_set_si(mpq_numref(C), A->Value);
	mpz_set(mpq_denref(C), B->Value);
	mpq_canonicalize(C);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

INTEGER_METHOD("/", TYP, Std$Integer$BigT, TYP, Std$Integer$SmallT) {
	Std$Integer_bigt *A = (Std$Integer_bigt *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpz_set(mpq_numref(C), A->Value);
	mpz_set_si(mpq_denref(C), B->Value);
	mpq_canonicalize(C);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("num", TYP, Std$Rational$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	Result->Val = finish_integer(mpq_numref(R->Value));
	return SUCCESS;
};

RATIONAL_METHOD("den", TYP, Std$Rational$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	Result->Val = finish_integer(mpq_denref(R->Value));
	return SUCCESS;
};

RATIONAL_METHOD("@", TYP, Std$Rational$T, VAL, Std$String$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	Result->Val = Std$String$new(mpq_get_str(0, 10, R->Value));
	return SUCCESS;
};

RATIONAL_METHOD("abs", TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *C = new(Std$Rational_t);
	C->Type = Std$Rational$T;
	mpq_init(C->Value);
	mpq_abs(C->Value, A->Value);
	Result->Val = (Std$Object_t *)C;
	return SUCCESS;
};

RATIONAL_METHOD("-", TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *C = new(Std$Rational_t);
	C->Type = Std$Rational$T;
	mpq_init(C->Value);
	mpq_neg(C->Value, A->Value);
	Result->Val = (Std$Object_t *)C;
	return SUCCESS;
};

RATIONAL_METHOD("+", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_add(C, A->Value, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};
	
RATIONAL_METHOD("-", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_sub(C, A->Value, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("*", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_mul(C, A->Value, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("/", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_div(C, A->Value, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("+", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_si(B, ((Std$Integer_smallt *)Args[1].Val)->Value, 1);
	mpq_t C;
	mpq_init(C);
	mpq_add(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("-", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_si(B, ((Std$Integer_smallt *)Args[1].Val)->Value, 1);
	mpq_t C;
	mpq_init(C);
	mpq_sub(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("*", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_si(B, ((Std$Integer_smallt *)Args[1].Val)->Value, 1);
	mpq_t C;
	mpq_init(C);
	mpq_mul(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("/", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_si(B, ((Std$Integer_smallt *)Args[1].Val)->Value, 1);
	mpq_t C;
	mpq_init(C);
	mpq_div(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("+", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_si(A, ((Std$Integer_smallt *)Args[0].Val)->Value, 1);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_add(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("-", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_si(A, ((Std$Integer_smallt *)Args[0].Val)->Value, 1);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_sub(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("*", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_si(A, ((Std$Integer_smallt *)Args[0].Val)->Value, 1);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_mul(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("/", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_si(A, ((Std$Integer_smallt *)Args[0].Val)->Value, 1);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_div(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("+", TYP, Std$Rational$T, TYP, Std$Integer$BigT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_z(B, ((Std$Integer_bigt *)Args[1].Val)->Value);
	mpq_t C;
	mpq_init(C);
	mpq_add(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("-", TYP, Std$Rational$T, TYP, Std$Integer$BigT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_z(B, ((Std$Integer_bigt *)Args[1].Val)->Value);
	mpq_t C;
	mpq_init(C);
	mpq_sub(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("*", TYP, Std$Rational$T, TYP, Std$Integer$BigT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_z(B, ((Std$Integer_bigt *)Args[1].Val)->Value);
	mpq_t C;
	mpq_init(C);
	mpq_mul(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("/", TYP, Std$Rational$T, TYP, Std$Integer$BigT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	mpq_t B;
	mpq_init(B);
	mpq_set_z(B, ((Std$Integer_bigt *)Args[1].Val)->Value);
	mpq_t C;
	mpq_init(C);
	mpq_div(C, A->Value, B);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("+", TYP, Std$Integer$BigT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_z(A, ((Std$Integer_bigt *)Args[0].Val)->Value);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_add(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("-", TYP, Std$Integer$BigT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_z(A, ((Std$Integer_bigt *)Args[0].Val)->Value);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_sub(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("*", TYP, Std$Integer$BigT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_z(A, ((Std$Integer_bigt *)Args[0].Val)->Value);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_mul(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

RATIONAL_METHOD("/", TYP, Std$Integer$BigT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_z(A, ((Std$Integer_bigt *)Args[0].Val)->Value);
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	mpq_t C;
	mpq_init(C);
	mpq_div(C, A, B->Value);
	Result->Val = finish_rational(C);
	return SUCCESS;
};

static Std$Integer_smallt Zero[] = {{Std$Integer$SmallT, 0}};
static Std$Integer_smallt One[] = {{Std$Integer$SmallT, 1}};

INTEGER_METHOD("^", TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	int A = ((Std$Integer_smallt *)Args[0].Val)->Value;
	int B = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (B > 0) {
		if (A == -1) {
			if (B % 2) {
				Result->Val = Args[0].Val;
			} else {
				Result->Val = (Std$Object_t *)One;
			};
		} else if (A == 1) {
			Result->Val = Args[0].Val;
		} else if (A == 0) {
			Result->Val = (Std$Object_t *)Zero;
		} else {
			mpz_t C;
			mpz_init_set_si(C, A);
			mpz_pow_ui(C, C, B);
			Result->Val = finish_integer(C);
		};
	} else if (B < 0) {
		B = -B;
		if (A == -1) {
			if (B % 2) {
				Result->Val = Args[0].Val;
			} else {
				Result->Val = (Std$Object_t *)One;
			};
		} else if (A == 1) {
			Result->Val = Args[0].Val;
		} else if (A == 0) {
			Result->Val = (Std$Object_t *)Zero;
		} else {
			mpq_t C;
			mpq_init(C);
			mpz_set_ui(mpq_numref(C), 1);
			mpz_set_si(mpq_denref(C), A);
			mpz_pow_ui(mpq_denref(C), mpq_denref(C), B);
			Result->Val = finish_rational(C);
		};
	} else {
		Result->Val = (Std$Object_t *)One;
	};
	return SUCCESS;
};

INTEGER_METHOD("^", TYP, Std$Integer$BigT, TYP, Std$Integer$SmallT) {
	Std$Integer_bigt *A = (Std$Integer_bigt *)Args[0].Val;
	int B = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (B > 0) {
		Std$Integer_bigt *C = new(Std$Integer_bigt);
		C->Type = Std$Integer$BigT;
		mpz_init(C->Value);
		mpz_pow_ui(C->Value, A->Value, B);
		Result->Val = (Std$Object_t *)C;
	} else if (B < 0) {
		B = -B;
		mpq_t C;
		mpq_init(C);
		mpz_set_ui(mpq_numref(C), 1);
		mpz_pow_ui(mpq_denref(C), A->Value, B);
		Result->Val = finish_rational(C);
	} else {
		Result->Val = (Std$Object_t *)One;
	};
	return SUCCESS;
};

RATIONAL_METHOD("^", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	int B = ((Std$Integer_smallt *)Args[1].Val)->Value;
	if (B > 0) {
		mpq_t C;
		mpq_init(C);
		mpz_pow_ui(mpq_numref(C), mpq_numref(A->Value), B);
		mpz_pow_ui(mpq_denref(C), mpq_denref(A->Value), B);
		Result->Val = Std$Rational$new(C);
	} else if (B < 0) {
		B = -B;
		mpq_t C;
		mpq_init(C);
		mpz_pow_ui(mpq_numref(C), mpq_denref(A->Value), B);
		mpz_pow_ui(mpq_denref(C), mpq_numref(A->Value), B);
		mpq_canonicalize(C);
		Result->Val = finish_rational(C);
	} else {
		Result->Val = (Std$Object_t *)One;
	};
	return SUCCESS;
};

static inline Std$Function_status rational_power(mpq_t A, mpq_t B, Std$Function_result *Result) {
	int Power, Root;
	if (!mpz_fits_slong_p(mpq_numref(B))) return FAILURE;
	if (!mpz_fits_slong_p(mpq_denref(B))) return FAILURE;
	Power = mpz_get_si(mpq_numref(B));
	Root = mpz_get_si(mpq_denref(B));
	mpq_t C;
	mpq_init(C);
	if (Power > 0) {
		if (!mpz_root(mpq_numref(C), mpq_numref(A), Root)) return FAILURE;
		if (!mpz_root(mpq_denref(C), mpq_denref(A), Root)) return FAILURE;
		mpz_pow_ui(mpq_numref(C), mpq_numref(C), Power);
		mpz_pow_ui(mpq_denref(C), mpq_denref(C), Power);
		mpq_canonicalize(C);
		Result->Val = finish_rational(C);
		return SUCCESS;
	} else if (Power < 0) {
		Power = -Power;
		if (!mpz_root(mpq_numref(C), mpq_denref(A), Root)) return FAILURE;
		if (!mpz_root(mpq_denref(C), mpq_numref(A), Root)) return FAILURE;
		mpz_pow_ui(mpq_numref(C), mpq_numref(C), Power);
		mpz_pow_ui(mpq_denref(C), mpq_denref(C), Power);
		mpq_canonicalize(C);
		Result->Val = finish_rational(C);
		return SUCCESS;
	} else {
		Result->Val = (Std$Object_t *)One;
		return SUCCESS;
	};
};

INTEGER_METHOD("^", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_si(A, ((Std$Integer_smallt *)Args[0].Val)->Value, 1);
	return rational_power(A, ((Std$Rational_t *)Args[1].Val)->Value, Result);
};

INTEGER_METHOD("^", TYP, Std$Integer$BigT, TYP, Std$Rational$T) {
	mpq_t A;
	mpq_init(A);
	mpq_set_z(A, ((Std$Integer_bigt *)Args[0].Val)->Value);
	return rational_power(A, ((Std$Rational_t *)Args[1].Val)->Value, Result);
};

RATIONAL_METHOD("^", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	return rational_power(((Std$Rational_t *)Args[0].Val)->Value, ((Std$Rational_t *)Args[1].Val)->Value, Result);
};

RATIONAL_METHOD("<", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_cmp(A->Value, B->Value) < 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_cmp(A->Value, B->Value) > 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("<=", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_cmp(A->Value, B->Value) <= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">=", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_cmp(A->Value, B->Value) >= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("=", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_equal(A->Value, B->Value)) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("~=", TYP, Std$Rational$T, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Rational_t *B = (Std$Rational_t *)Args[1].Val;
	if (mpq_equal(A->Value, B->Value)) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("<", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) < 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) > 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("<=", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) <= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">=", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) >= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("=", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) == 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("~=", TYP, Std$Rational$T, TYP, Std$Integer$SmallT) {
	Std$Rational_t *A = (Std$Rational_t *)Args[0].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[1].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) != 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("<", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) >= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) <= 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("<=", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) > 0) {Result->Val = Args[1].Val; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD(">=", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) < 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("=", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) == 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("~=", TYP, Std$Integer$SmallT, TYP, Std$Rational$T) {
	Std$Rational_t *A = (Std$Rational_t *)Args[1].Val;
	Std$Integer_smallt *B = (Std$Integer_smallt *)Args[0].Val;
	if (mpq_cmp_si(A->Value, B->Value, 1) != 0) {Result->Arg = Args[1]; return SUCCESS;} else {return FAILURE;};
};

RATIONAL_METHOD("is0", TYP, Std$Rational$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	if (mpq_sgn(R->Value)) return FAILURE;
	Result->Val = (Std$Object_t *)R;
	return SUCCESS;
};

RATIONAL_METHOD("@", TYP, Std$Rational$T, VAL, Std$Real$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	Result->Val = Std$Real$new(mpq_get_d(R->Value));
	return SUCCESS;
};

REAL_METHOD("@", TYP, Std$Real$T, VAL, Std$Rational$T) {
	double X = ((Std$Real_t *)Args[0].Val)->Value;
	int Negative = 0;
	if (X < 0.0) {
		Negative = 1;
		X = -X;
	};
	double Z = X;
	double Y = floor(Z);
	double A = 0.0, B = 1.0;
	double N;
	
	for (int I = 20; --I;) {
		Z = Z - Y;
		if (fabs(Z) == 0.0) break;
		Z = 1.0 / Z;
		Y = floor(Z);
		double C = B;
		B = B * Y + A;
		A = C;
		N = round(B * X);
		if (fabs(N / B - X) < 1e-15) break;
	};
	
	mpq_t R;
	mpz_init_set_d(mpq_numref(R), Negative ? -N : N);
	mpz_init_set_d(mpq_denref(R), B);
	mpq_canonicalize(R);
	Result->Val = finish_rational(R);
	return SUCCESS;
};

RATIONAL_METHOD("floor", TYP, Std$Rational$T) {
Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	mpz_t Quotient;
	mpz_init(Quotient);
	mpz_fdiv_q(Quotient, mpq_numref(R->Value), mpq_denref(R->Value));
	if (mpz_fits_slong_p(Quotient)) {
		Result->Val = Std$Integer$new_small(mpz_get_si(Quotient));
	} else {
		Result->Val = Std$Integer$new_big(Quotient);
	};
	return SUCCESS;
};

RATIONAL_METHOD("ceil", TYP, Std$Rational$T) {
	Std$Rational_t *R = (Std$Rational_t *)Args[0].Val;
	mpz_t Quotient;
	mpz_init(Quotient);
	mpz_cdiv_q(Quotient, mpq_numref(R->Value), mpq_denref(R->Value));
	if (mpz_fits_slong_p(Quotient)) {
		Result->Val = Std$Integer$new_small(mpz_get_si(Quotient));
	} else {
		Result->Val = Std$Integer$new_big(Quotient);
	};
	return SUCCESS;
};

STRING_METHOD("gets", TYP, Std$Address$T) {
	Result->Val = Std$String$copy(((Std$Address_t *)Args[0].Val)->Value);
	return SUCCESS;
};

NORMAL_METHOD("trace", TYP, Std$Symbol$NoMethodMessageT) {
	Std$Symbol$nomethodmessage *Message = (Std$Symbol$nomethodmessage *)Args[0].Val;
	printf("no method: %s\n", Std$String$flatten(Message->Symbol->Name));
	for (int I = 0; I < Message->Count; ++I) {
		printf("\t%s\n", Message->Stack[I]);
	};
	return SUCCESS;
};

/*
typedef struct scanner_position {
	Std$String_block *Block;
	uint32_t Offset;
	uint32_t Index;
} scanner_position;

typedef struct scanner_t {
	Std$Type_t *Type;
	const Std$String_t *Subject;
	scanner_position Old, Cur;
} scanner_t;

TYPE(ScannerT);

METHOD("scan", TYP, Std$String$T) {
	const Std$String_t *Subject = Args[0].Val;
	scanner_t *Scanner = new(scanner_t);
	Scanner->Type = ScannerT;
	Scanner->Subject = Subject;
	Scanner->Cur.Block = Subject->Blocks;
	Scanner->Cur.Offset = 0;
	Scanner->Cur.Index = 0;
	Result->Val = Scanner;
	return SUCCESS;
};

typedef struct scanner_restore_generator {
	Std$Function_cstate State;
	scanner_t *Scanner;
	scanner_position Position;
} scanner_restore_generator;

static long resume_scanner_restore(Std$Function$result *Result) {
	scanner_restore_generator *Generator = Result->State;
	Generator->Scanner->Cur = Generator->Position;
	return FAILURE;
};

METHOD("get", TYP, ScannerT) {
	scanner_t *Scanner = Args[0].Val;
	int N = Scanner->Cur.Block - Scanner->Old.Block + 1;
	Std$String_t *String = Std$String$alloc(N);
	memcpy(String->Blocks, Scanner->Old.Block, N * sizeof(Std$String_block));
	String->Blocks[0].Chars.Value += Scanner->Old.Offset;
	String->Blocks[N - 1].Length.Value = Scanner->Cur.Offset;
	String->Blocks[0].Length.Value -= Scanner->Old.Offset;
	Riva$Memory$freeze_stubborn(String);
};

METHOD("any", TYP, ScannerT, TYP, Std$String$T) {
	scanner_t *Scanner = Args[0].Val;
	if (Scanner->Cur.Block->Chars.Value == 0) return FAILURE;
	const Std$String_t *Chars = Args[1].Val;
	uint8_t Mask[32];
	memset(Mask, 0, 32);
	for (Std$String_block *Block = Chars->Blocks; Block->Length.Value; ++Block) {
		unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) {
			unsigned char Char = Chars[I];
			Mask[Char >> 3] |= 1 << (Char & 7);
		};
	};
	unsigned char Char = ((unsigned char *)Scanner->Cur.Block->Chars.Value)[Scanner->Cur.Offset];
	if (Mask[Char >> 3] & (1 << (Char & 7))) {
		scanner_restore_generator *Generator = new(scanner_restore_generator);
		Generator->Scanner = Scanner;
		Generator->Position = Scanner->Cur;
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = (Std$Function_cresumefn)resume_scanner_restore;
		++Scanner->Cur.Index;
		if (++Scanner->Cur.Offset >= Scanner->Cur.Block->Length.Value) {
			Scanner->Cur.Block++;
			Scanner->Cur.Offset = 0;
		};
		Result->Val = Scanner;
		Result->State = Generator;
		return SUSPEND;
	};
};

METHOD("upto", TYP, ScannerT, TYP, Std$String$T) {
	scanner_t *Scanner = Args[0].Val;
	if (Scanner->Cur.Block->Chars.Value == 0) return FAILURE;
	const Std$String_t *Chars = Args[1].Val;
	uint8_t Mask[32];
	memset(Mask, 0, 32);
	for (Std$String_block *Block = Chars->Blocks; Block->Length.Value; ++Block) {
		unsigned char *Chars = Block->Chars.Value;
		for (int I = 0; I < Block->Length.Value; ++I) {
			unsigned char Char = Chars[I];
			Mask[Char >> 3] |= 1 << (Char & 7);
		};
	};
};
*/
