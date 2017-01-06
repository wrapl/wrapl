#include "bitset.h"
#include "limits.h"

#include <stdio.h>
#include <algorithm>

struct bitset_internal_t : bitset_t {
	uint32_t Lo, Hi;
	bitset_internal_t() {};
	bitset_internal_t(uint32_t Lo, uint32_t Hi) : bitset_t(0) {
		this->Lo = Lo;
		this->Hi = Hi;
	};
};

bitset_t::bitset_t() {
	Next = new bitset_internal_t(0, LONG_MAX);
};

bitset_t::bitset_t(bitset_t *Copy) {
	bitset_t *Prev = this;
	bitset_internal_t *Node = Copy->Next;
	do {
		bitset_internal_t *Copy = new bitset_internal_t(Node->Lo, Node->Hi);
		Prev->Next = Copy;
		Prev = Copy;
		Node = Node->Next;
	} while (Node);
};

bitset_t::bitset_t(bitset_t *A, bitset_t *B) {
	bitset_internal_t *A0 = A->Next;
	bitset_internal_t *B0 = B->Next;
	bitset_internal_t *Node = Next = new bitset_internal_t;
	for (;;) {
		if (A0->Lo > B0->Hi) {
			B0 = B0->Next;
		} else if (B0->Lo > A0->Hi) {
			A0 = A0->Next;
		} else {
			Node->Lo = std::max(A0->Lo, B0->Lo);
			if (A0->Hi > B0->Hi) {
				Node->Hi = B0->Hi;
				B0 = B0->Next;
			} else if (A0->Hi < B0->Hi) {
				Node->Hi = A0->Hi;
				A0 = A0->Next;
			} else {
				Node->Hi = A0->Hi;
				if ((A0 = A0->Next) == 0) return;
				if ((B0 = B0->Next) == 0) return;
			};
			Node = (Node->Next = new bitset_internal_t);
		};
	};
};

void bitset_t::reserve(const bitset_t *B) {
	bitset_internal_t *A0 = Next;
	bitset_internal_t *B0 = B->Next;
	bitset_internal_t *Node = Next = new bitset_internal_t;
	for (;;) {
		if (A0->Lo > B0->Hi) {
			B0 = B0->Next;
		} else if (B0->Lo > A0->Hi) {
			A0 = A0->Next;
		} else {
			Node->Lo = std::max(A0->Lo, B0->Lo);
			if (A0->Hi > B0->Hi) {
				Node->Hi = B0->Hi;
				B0 = B0->Next;
			} else if (A0->Hi < B0->Hi) {
				Node->Hi = A0->Hi;
				A0 = A0->Next;
			} else {
				Node->Hi = A0->Hi;
				if ((A0 = A0->Next) == 0) return;
				if ((B0 = B0->Next) == 0) return;
			};
			Node = (Node->Next = new bitset_internal_t);
		};
	};
};

void bitset_t::reserve(int N) {
	bitset_t *Prev = this;
	for (;;) {
		bitset_internal_t *Node = Prev->Next;
		if (Node->Lo == N) {
			if (Node->Hi == N) {
				Prev->Next = Node->Next; return;
			} else {
				++Node->Lo; return;
			};
		} else if (Node->Hi > N) {
			bitset_internal_t *Split = new bitset_internal_t(Node->Lo, N - 1);
			Node->Lo = N + 1;
			(Prev->Next = Split)->Next = Node;
			return;
		} else if (Node->Hi == N) {
			Node->Hi = N - 1; return;
		};
		Prev = Node;
	};
};

void bitset_t::reserve(uint32_t Lo, uint32_t Hi) {
	bitset_t *Prev = this;
	for (;;) {
		bitset_internal_t *Node = Prev->Next;
		if (Node->Lo == Lo) {
			if (Node->Hi == Hi) {
				Prev->Next = Node->Next; return;
			} else if (Node->Hi > Hi) {
				Node->Lo = Hi + 1; return;
			} else {
				Prev->Next = Node->Next;
				Lo = Node->Hi + 1;
			};
		} else if (Node->Hi > Lo) {
			if (Node->Hi == Hi) {
				Node->Hi = Lo - 1; return;
			} else if (Node->Hi > Hi) {
				bitset_internal_t *Split = new bitset_internal_t(Node->Lo, Lo - 1);
				Node->Lo = Hi + 1;
				(Prev->Next = Split)->Next = Node;
				return;
			} else {
				uint32_t Temp = Node->Hi;
				Node->Hi = Lo - 1;
				Lo = Temp + 1;
			};
		} else if (Node->Hi == Lo) {
			Node->Hi = Lo - 1;
			Lo = Lo + 1;
		};
		Prev = Node;
	};
};

uint32_t bitset_t::allocate() {
	bitset_internal_t *Node = Next;
	uint32_t Free = Node->Lo;
	if (Free == Node->Hi) Next = Node->Next; else ++Node->Lo;
	return Free;
};

uint32_t bitset_t::below(uint32_t Max) {
	bitset_internal_t *Node = Next;
	uint32_t Free = Node->Lo;
	if (Free >= Max) return 0xFFFFFFFF;
	if (Free == Node->Hi) Next = Node->Next; else ++Node->Lo;
	return Free;
};

uint32_t bitset_t::size() {
    return Next->Lo;
};

uint32_t bitset_t::allocate(const bitset_t *With0) {
	bitset_t *Prev = this;
	bitset_internal_t *Node = Next;
	bitset_internal_t *With = With0->Next;
	for (;;) {
		if (Node->Lo > With->Hi) {
			With = With->Next;
		} else if (With->Lo > Node->Hi) {
			Node = (Prev = Node)->Next;
		} else if (Node->Lo >= With->Lo) {
			uint32_t Free = Node->Lo;
			if (Free < Node->Hi) {
				Node->Lo = Free + 1; return Free;
			} else {
				Prev->Next = Node->Next; return Free;
			};
		} else {
			uint32_t Free = With->Lo;
			if (Free < Node->Hi) {
				bitset_internal_t *Split = new bitset_internal_t(Node->Lo, Free - 1);
				Node->Lo = Free + 1;
				(Prev->Next = Split)->Next = Node;
				return Free;
			} else {
				Node->Hi = Free - 1; return Free;
			};
		};
	};
};

uint32_t bitset_t::allocate(uint32_t Count) {
	if (Count == 0) return 0;
	bitset_t *Prev = this;
	for (;;) {
		bitset_internal_t *Node = Prev->Next;
		uint32_t Free = Node->Lo;
		uint32_t End = Free + Count - 1;
		if (End < Node->Hi) {
			Node->Lo = End + 1; return Free;
		} else if (End == Node->Hi) {
			Prev->Next = Node->Next; return Free;
		};
		Prev = Node;
	};
};

uint32_t bitset_t::allocate(uint32_t Count, const bitset_t *With0) {
	if (Count == 0) return 0;
	bitset_t *Prev = this;
	bitset_internal_t *Node = Next;
	bitset_internal_t *With = With0->Next;
	for (;;) {
		if (Node->Lo > With->Hi) {
			With = With->Next;
		} else if (With->Lo > Node->Hi) {
			Node = (Prev = Node)->Next;
		} else if (Node->Lo >= With->Lo) {
			uint32_t Free = Node->Lo;
			uint32_t End = Free + Count - 1;
			if (End <= With->Hi) {
				if (End < Node->Hi) {
					Node->Lo = End + 1; return Free;
				} else if (End == Node->Hi) {
					Prev->Next = Node->Next; return Free;
				};
			};
		} else {
			uint32_t Free = With->Lo;
			uint32_t End = Free + Count - 1;
			if (End <= With->Hi) {
				if (End < Node->Hi) {
					bitset_internal_t *Split = new bitset_internal_t(Node->Lo, Free - 1);
					Node->Lo = End + 1;
					(Prev->Next = Split)->Next = Node;
					return Free;
				} else if (End == Node->Hi) {
					Node->Hi = Free - 1; return Free;
				};
			};
		};
	};
};

void bitset_t::update(bitset_t *With) {
	Next = (new bitset_t(this, With))->Next;
};
