#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <Std.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <nettle/md5.h>

typedef struct node_t {
	struct node_t *Next;
	uint32_t Length;
	const unsigned char *Chars;
} node_t;

typedef struct buffer_t {
	const Std$Type$t *Type;
	node_t *Head, *Tail;
	uint32_t Position;
} buffer_t;

TYPE(T, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);
// A simple non-seekable string buffer.

TYPED_INSTANCE(int, IO$Stream$eoi, T, buffer_t *Stream) {
	return (Stream->Head == 0);
};

TYPED_INSTANCE(int, IO$Stream$close, T, buffer_t *Stream, int Mode) {
};

static Std$Integer$smallt Zero[] = {{Std$Integer$SmallT, 0}};

/*#ifdef WINDOWS

static inline void *mempcpy(void *Dst, const void *Src, int Len) {
	memcpy(Dst, Src, Len);
	return (char *)Dst + Len;
};

#endif*/

TYPED_INSTANCE(int, IO$Stream$read, T, buffer_t *Stream, char *Buffer, int Count, int Block) {
	node_t *Node = Stream->Head;
	if (Node == 0) return 0;
	uint32_t Total = 0;
	const char *Src = Node->Chars;
	char *Dst = Buffer;
	uint32_t Rem0 = Node->Length;
	uint32_t Rem1 = Count;
	while (Rem0 <= Rem1) {
		Dst = mempcpy(Dst, (void *)Src, Rem0);
		Rem1 -= Rem0;
		Total += Rem0;
		if (Node = Node->Next) {
			Rem0 = Node->Length;
			Src = Node->Chars;
		} else {
			Stream->Head = Stream->Tail = 0;
			return Total;
		};
	};
	memcpy(Dst, (void *)Src, Rem1);
	Total += Rem1;
	Node->Length -= Rem1;
	Node->Chars += Rem1;
	Stream->Head = Node;
	Stream->Position += Total;
	return Total;
};

TYPED_INSTANCE(char, IO$Stream$readc, T, buffer_t *Stream) {
	node_t *Node = Stream->Head;
	if (Node == 0) return EOF;
	char Char = Node->Chars[0];
	if (Node->Length == 1) {
		if (Stream->Head = Node->Next) {
		} else {
			Stream->Tail = 0;
		};
	} else {
		Node->Chars++;
		Node->Length--;
	};
	Stream->Position += 1;
	return Char;
};

static char *extract_chars_rest(buffer_t *Stream) {
	int Length = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Length += Node->Length;
	char *Buffer = Riva$Memory$alloc_atomic(Length + 1);
	char *Ptr = Buffer;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Ptr = mempcpy(Ptr, Node->Chars, Node->Length);
	Buffer[Length] = 0;
	Stream->Head = Stream->Tail = 0;
	Stream->Position += Length;
	return Buffer;
};

static char *extract_chars(buffer_t *Stream, node_t *EndNode, int EndOffset) {
	int Length = EndOffset;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) Length += Node->Length;
	char *Result = Riva$Memory$alloc_atomic(Length + 1);
	Result[Length] = 0;
	char *Ptr = Result;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) {
		memcpy(Ptr, Node->Chars, Node->Length);
		Ptr += Node->Length;
	};
	if (EndOffset) {
		memcpy(Ptr, EndNode->Chars, EndOffset);
		if ((EndNode->Length -= EndOffset) == 0) {
			EndNode = EndNode->Next;
		} else {
			EndNode->Chars += EndOffset;
		};
	};
	Stream->Head = EndNode;
	if (EndNode == 0) Stream->Tail = 0;
	Stream->Position += Length;
	return Result;
};

static char *read_chars_length(buffer_t *Stream, int Max) {
	node_t *Node = Stream->Head;
	int Remaining = Max;
	while (Node) {
		if (Remaining <= Node->Length) {
			return extract_chars(Stream, Node, Remaining);
		} else {
			Remaining -= Node->Length;
			Node = Node->Next;
		};
	};
	return extract_chars_rest(Stream);
};

static inline const char *findcset(const char *Chars, uint8_t *Mask, int Length) {
    while (Length) {
    	--Length;
        char Char = *Chars;
        if (Mask[Char >> 3] & (1 << (Char & 7))) return Chars;
        ++Chars;
    };
    return 0;
};

TYPED_INSTANCE(char *, IO$Stream$readx, T, buffer_t *Stream, int Max, const char *Term, int TermSize) {
	node_t *Node = Stream->Head;
	if (Node == 0) return 0;
	if (TermSize == 0) {
		if (Max == 0) {
			return extract_chars_rest(Stream);
		} else {
			return read_chars_length(Stream, Max);
		};
	} else if (TermSize == 1) {
		if (Max == 0) {
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, Term[0], Node->Length);
				if (Find) {
					char *Result = extract_chars(Stream, Node, Find - Node->Chars);
					if (--Node->Length == 0) {
						Stream->Head = Node->Next;
					} else {
						++Node->Chars;
					};
					return Result;
				} else {
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		} else {
			uint8_t Mask[32];
			memset(Mask, 0, 32);
		    for (int I = 0; I < TermSize; ++I) {
		        char Char = Term[I];
		        Mask[Char >> 3] |= 1 << (Char & 7);
		    };
			int Remaining = Max;
			while (Node) {
				const unsigned char *Find = findcset(Node->Chars, Mask, Node->Length);
				if (Find) {
					int Offset = Find - Node->Chars;
					if (Remaining < Offset) {
						return extract_chars(Stream, Node, Remaining);
					} else {
						char *Result = extract_chars(Stream, Node, Offset);
						if (--Node->Length == 0) {
							Stream->Head = Node->Next;
						} else {
							++Node->Chars;
						};
						return Result;
					};
				} else if (Remaining <= Node->Length) {
					return extract_chars(Stream, Node, Remaining);
				} else {
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		};
	} else {
		char IsTerm[256] = {0,};
		for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
		if (Max == 0) {
			while (Node) {
				for (int Offset = 0; Offset < Node->Length; ++Offset) {
					if (IsTerm[Node->Chars[Offset]]) {
						char *Result = extract_chars(Stream, Node, Offset);
						if (--Node->Length == 0) {
							Stream->Head = Node->Next;
						} else {
							++Node->Chars;
						};
						return Result;
					};
				};
				Node = Node->Next;
			};
			return extract_chars_rest(Stream);
		} else {
			int Remaining = Max;
			while (Node) {
				if (Remaining <= Node->Length) {
					for (int Offset = 0; Offset <= Remaining; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							char *Result = extract_chars(Stream, Node, Offset);
							if (--Node->Length == 0) {
								Stream->Head = Node->Next;
							} else {
								++Node->Chars;
							};
							return Result;
						};
					};
					return extract_chars(Stream, Node, Remaining);
				} else {
					for (int Offset = 0; Offset < Node->Length; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							char *Result = extract_chars(Stream, Node, Offset);
							if (--Node->Length == 0) {
								Stream->Head = Node->Next;
							} else {
								++Node->Chars;
							};
							return Result;
						};
					};
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		};
	};
};

TYPED_INSTANCE(char *, IO$Stream$readi, T, buffer_t *Stream, int Max, const char *Term, int TermSize) {
	node_t *Node = Stream->Head;
	if (Node == 0) return 0;
	if (TermSize == 0) {
		if (Max == 0) {
			return extract_chars_rest(Stream);
		} else {
			return read_chars_length(Stream, Max);
		};
	} else if (TermSize == 1) {
		if (Max == 0) {
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, Term[0], Node->Length);
				if (Find) {
					return extract_chars(Stream, Node, (Find + 1) - Node->Chars);
				} else {
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		} else {
			uint8_t Mask[32];
			memset(Mask, 0, 32);
		    for (int I = 0; I < TermSize; ++I) {
		        char Char = Term[I];
		        Mask[Char >> 3] |= 1 << (Char & 7);
		    };
			int Remaining = Max;
			while (Node) {
				const unsigned char *Find = findcset(Node->Chars, Mask, Node->Length);
				if (Find) {
					int Offset = Find - Node->Chars;
					if (Remaining < Offset) {
						return extract_chars(Stream, Node, Remaining);
					} else {
						char *Result = extract_chars(Stream, Node, Offset);
						if (--Node->Length == 0) {
							Stream->Head = Node->Next;
						} else {
							++Node->Chars;
						};
						return Result;
					};
				} else if (Remaining <= Node->Length) {
					return extract_chars(Stream, Node, Remaining);
				} else {
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		};
	} else {
		char IsTerm[256] = {0,};
		for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
		if (Max == 0) {
			while (Node) {
				for (int Offset = 0; Offset < Node->Length; ++Offset) {
					if (IsTerm[Node->Chars[Offset]]) {
						return extract_chars(Stream, Node, Offset + 1);
					};
				};
				Node = Node->Next;
			};
			return extract_chars_rest(Stream);
		} else {
			int Remaining = Max;
			while (Node) {
				if (Remaining <= Node->Length) {
					for (int Offset = 0; Offset <= Remaining; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							return extract_chars(Stream, Node, Offset + 1);
						};
					};
					return extract_chars(Stream, Node, Remaining);
				} else {
					for (int Offset = 0; Offset < Node->Length; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							return extract_chars(Stream, Node, Offset + 1);
						};
					};
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			return extract_chars_rest(Stream);
		};
	};
};

TYPED_INSTANCE(char *, IO$Stream$readl, T, buffer_t *Stream) {
	node_t *Node = Stream->Head;
	if (Node == 0) return 0;
	int Length = 0;
	while (Node) {
		unsigned char *Find = memchr(Node->Chars, '\n', Node->Length);
		if (Find) {
			char *Result = extract_chars(Stream, Node, Find - Node->Chars);
			if (--Node->Length == 0) {
				Stream->Head = Node->Next;
			} else {
				++Node->Chars;
			};
			return Result;
		} else {
			Node = Node->Next;
		};
	};
	return extract_chars_rest(Stream);
};

TYPED_INSTANCE(int, IO$Stream$write, T, buffer_t *Stream, const char *Buffer, int Count, int Blocks) {
	char *Chars = Riva$Memory$alloc_atomic(Count);
	memcpy(Chars, Buffer, Count);
	node_t *Node = new(node_t);
	Node->Length = Count;
	Node->Chars = Chars;
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
	} else {
		Stream->Head = Node;
	};
	Stream->Tail = Node;
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$writec, T, buffer_t *Stream, char Char) {
	char *Chars = Riva$Memory$alloc_atomic(1);
	Chars[0] = Char;
	node_t *Node = new(node_t);
	Node->Length = 1;
	Node->Chars = Chars;
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
	} else {
		Stream->Head = Node;
	};
	Stream->Tail = Node;
	return 1;
};

TYPED_INSTANCE(int, IO$Stream$writes, T, buffer_t *Stream, const char *Text) {
	int Length = strlen(Text);
	char *Chars = Riva$Memory$alloc_atomic(Length);
	memcpy(Chars, Text, Length);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
	} else {
		Stream->Head = Node;
	};
	Stream->Tail = Node;
	return Length;
};

TYPED_INSTANCE(int, IO$Stream$writef, T, buffer_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	if (Stream->Tail) {
		Stream->Tail->Next = Node;		
	} else {
		Stream->Head = Node;
	};
	Stream->Tail = Node;
	return Length;
};

GLOBAL_FUNCTION(New, 0) {
//:T
// Creates and returns an empty buffer
	buffer_t *Stream = new(buffer_t);
	Stream->Type = T;
	Result->Val = (Std$Object$t *)Stream;
	return SUCCESS;
};

METHOD("close", TYP, T) {
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$Address$T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	node_t *Node = Stream->Head;
	if (Node == 0) {
		Result->Val = (Std$Object$t *)Zero;
		return SUCCESS;
	};
	uint32_t Total = 0;
	const char *Src = Node->Chars;
	char *Dst = Std$Address$get_value(Args[1].Val);
	uint32_t Rem0 = Node->Length;
	uint32_t Rem1 = Std$Address$get_length(Args[1].Val);
	while (Rem0 <= Rem1) {
		Dst = mempcpy(Dst, Src, Rem0);
		Rem1 -= Rem0;
		Total += Rem0;
		if (Node = Node->Next) {
			Rem0 = Node->Length;
			Src = Node->Chars;
		} else {
			Stream->Head = Stream->Tail = 0;
			Stream->Position += Total;
			Result->Val = Std$Integer$new_small(Total);
			return SUCCESS;
		};
	};
	memcpy(Dst, Src, Rem1);
	Total += Rem1;
	Node->Length -= Rem1;
	Node->Chars += Rem1;
	Stream->Head = Node;
	Stream->Position += Total;
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

static Std$Object$t *extract_string_rest(buffer_t *Stream) {
	int NoOfBlocks = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	Std$Address$t *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) {
		String->Length.Value += (Block->Length.Value = Node->Length);
		Block->Value = Node->Chars;
		Block++;
	};
	Stream->Head = Stream->Tail = 0;
	Stream->Position += String->Length.Value;
	return Std$String$freeze(String);
};

METHOD("rest", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	int NoOfBlocks = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	Std$Address$t *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) {
		String->Length.Value += (Block->Length.Value = Node->Length);
		Block->Value = Node->Chars;
		Block++;
	};
	Stream->Head = Stream->Tail = 0;
	Stream->Position += String->Length.Value;
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
};

METHOD("bytes", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	int Length = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Length += Node->Length;
	void *Bytes = Riva$Memory$alloc_atomic(Length);
	Std$Address$t *Address = Std$Address$new(Bytes, Length);
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) {
		memcpy(Bytes, Node->Chars, Node->Length);
		Bytes += Node->Length;
	}
	RETURN(Address);
}

METHOD("md5", TYP, T) {
	struct md5_ctx Context[1];
	md5_init(Context);
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) md5_update(Context, Node->Length, Node->Chars);
	char *Buffer = Riva$Memory$alloc_atomic(16);
	md5_digest(Context, 16, Buffer);
	Result->Val = Std$String$new_length(Buffer, 16);
	return SUCCESS;
};

static Std$Object$t *extract_string(buffer_t *Stream, node_t *EndNode, int EndOffset) {
	int NoOfBlocks = EndOffset ? 1 : 0;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	int Length = 0;
	Std$Address$t *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) {
		Length += (Block->Length.Value = Node->Length);
		Block->Value = Node->Chars;
		++Block;
	};
	if (EndOffset) {
		Length += (Block->Length.Value = EndOffset);
		Block->Value = EndNode->Chars;
		if ((EndNode->Length -= EndOffset) == 0) {
			EndNode = EndNode->Next;
		} else {
			EndNode->Chars += EndOffset;
		};
	};
	String->Length.Value = Length;
	Stream->Head = EndNode;
	if (EndNode == 0) Stream->Tail = 0;
	Stream->Position += Length;
	return Std$String$freeze(String);
};

static Std$Object$t *read_string_length(buffer_t *Stream, int Max) {
	node_t *Node = Stream->Head;
	int Remaining = Max;
	while (Node) {
		if (Remaining <= Node->Length) {
			return extract_string(Stream, Node, Remaining);
		} else {
			Remaining -= Node->Length;
			Node = Node->Next;
		};
	};
	return extract_string_rest(Stream);
};

METHOD("readx", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	node_t *Node = Stream->Head;
	if (Node == 0) return FAILURE;
	if (Term->Length.Value == 0) {
		if (Max == 0) {
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			Result->Val = read_string_length(Stream, Max);
			return SUCCESS;
		};
	} else if (Term->Length.Value == 1) {
		if (Max == 0) {
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Value), Node->Length);
				if (Find) {
					Result->Val = extract_string(Stream, Node, Find - Node->Chars);
					if (--Node->Length == 0) {
						Stream->Head = Node->Next;
					} else {
						++Node->Chars;
					};
					return SUCCESS;
				} else {
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			int Remaining = Max;
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, *((unsigned char *)Term->Blocks->Value), Node->Length);
				if (Find) {
					int Offset = Find - Node->Chars;
					if (Remaining < Offset) {
						Result->Val = extract_string(Stream, Node, Remaining);
						return SUCCESS;
					} else {
						Result->Val = extract_string(Stream, Node, Offset);
						if (--Node->Length == 0) {
							Stream->Head = Node->Next;
						} else {
							++Node->Chars;
						};
						return SUCCESS;
					};
				} else if (Remaining <= Node->Length) {
					Result->Val = extract_string(Stream, Node, Remaining);
					return SUCCESS;
				} else {
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		};
	} else {
		char IsTerm[256] = {0,};
		for (const Std$Address$t *Block = Term->Blocks; Block->Length.Value; Block++) {
			const unsigned char *Chars = Block->Value;
			for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
		};
		if (Max == 0) {
			while (Node) {
				for (int Offset = 0; Offset < Node->Length; ++Offset) {
					if (IsTerm[Node->Chars[Offset]]) {
						Result->Val = extract_string(Stream, Node, Offset);
						if (--Node->Length == 0) {
							Stream->Head = Node->Next;
						} else {
							++Node->Chars;
						};
						return SUCCESS;
					};
				};
				Node = Node->Next;
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			int Remaining = Max;
			while (Node) {
				if (Remaining <= Node->Length) {
					for (int Offset = 0; Offset <= Remaining; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							Result->Val = extract_string(Stream, Node, Offset);
							if (--Node->Length == 0) {
								Stream->Head = Node->Next;
							} else {
								++Node->Chars;
							};
							return SUCCESS;
						};
					};
					Result->Val = extract_string(Stream, Node, Remaining);
					return SUCCESS;
				} else {
					for (int Offset = 0; Offset < Node->Length; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							Result->Val = extract_string(Stream, Node, Offset);
							if (--Node->Length == 0) {
								Stream->Head = Node->Next;
							} else {
								++Node->Chars;
							};
							return SUCCESS;
						};
					};
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		};
	};
};

METHOD("readi", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	node_t *Node = Stream->Head;
	if (Node == 0) return FAILURE;
	if (Term->Length.Value == 0) {
		if (Max == 0) {
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			Result->Val = read_string_length(Stream, Max);
			return SUCCESS;
		};
	} else if (Term->Length.Value == 1) {
		if (Max == 0) {
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Value), Node->Length);
				if (Find) {
					Result->Val = extract_string(Stream, Node, (Find + 1) - Node->Chars);
					return SUCCESS;
				} else {
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			int Remaining = Max;
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Value), Node->Length);
				if (Find) {
					int Offset = Find - Node->Chars;
					if (Remaining < Offset) {
						Result->Val = extract_string(Stream, Node, Remaining);
						return SUCCESS;
					} else {
						Result->Val = extract_string(Stream, Node, Offset + 1);
						return SUCCESS;
					};
				} else if (Remaining <= Node->Length) {
					Result->Val = extract_string(Stream, Node, Remaining);
					return SUCCESS;
				} else {
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		};
	} else {
		char IsTerm[256] = {0,};
		for (const Std$Address$t *Block = Term->Blocks; Block->Length.Value; Block++) {
			const unsigned char *Chars = Block->Value;
			for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
		};
		if (Max == 0) {
			while (Node) {
				for (int Offset = 0; Offset < Node->Length; ++Offset) {
					if (IsTerm[Node->Chars[Offset]]) {
						Result->Val = extract_string(Stream, Node, Offset + 1);
						return SUCCESS;
					};
				};
				Node = Node->Next;
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		} else {
			int Remaining = Max;
			while (Node) {
				if (Remaining <= Node->Length) {
					for (int Offset = 0; Offset <= Remaining; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							Result->Val = extract_string(Stream, Node, Offset + 1);
							return SUCCESS;
						};
					};
					Result->Val = extract_string(Stream, Node, Remaining);
					return SUCCESS;
				} else {
					for (int Offset = 0; Offset < Node->Length; ++Offset) {
						if (IsTerm[Node->Chars[Offset]]) {
							Result->Val = extract_string(Stream, Node, Offset + 1);
							return SUCCESS;
						};
					};
					Remaining -= Node->Length;
					Node = Node->Next;
				};
			};
			Result->Val = extract_string_rest(Stream);
			return SUCCESS;
		};
	};
};

METHOD("read", TYP, T, TYP, Std$Integer$SmallT) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	if (Stream->Head) {
		Result->Val = read_string_length(Stream, ((Std$Integer$smallt *)Args[1].Val)->Value);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("read", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	node_t *Node = Stream->Head;
	if (Node == 0) return FAILURE;
	int Length = 0;
	while (Node) {
		unsigned char *Find = memchr(Node->Chars, '\n', Node->Length);
		if (Find) {
			Result->Val = extract_string(Stream, Node, Find - Node->Chars);
			if (--Node->Length == 0) {
				Stream->Head = Node->Next;
			} else {
				++Node->Chars;
			};
			return SUCCESS;
		} else {
			Node = Node->Next;
		};
	};
	Result->Val = extract_string_rest(Stream);
	return SUCCESS;
};

METHOD("write", TYP, T, TYP, Std$Address$T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	long Length = Std$Address$get_length(Args[1].Val);
	char *Chars = Riva$Memory$alloc_atomic(Length);
	memcpy(Chars, Std$Address$get_value(Args[1].Val), Length);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("write", TYP, T, TYP, Std$String$T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	int NoOfBlocks = ((Std$String$t *)Args[1].Val)->Count;
	Std$Address$t *Block = ((Std$String$t *)Args[1].Val)->Blocks;
	if (Block->Length.Value) {
		node_t *Node = new(node_t);
		Node->Chars = Block->Value;
		Node->Length = Block->Length.Value;
		if (Stream->Tail) {
			Stream->Tail->Next = Node;
		} else {
			Stream->Head = Node;
		};
		while ((++Block)->Length.Value) {
			node_t *Next = new(node_t);
			Node->Next = Next;
			Node = Next;
			Node->Chars = Block->Value;
			Node->Length = Block->Length.Value;
		};
		Stream->Tail = Node;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$tell, T, buffer_t *Stream) {
	return Stream->Position;
};

METHOD("tell", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Stream->Position);
	return SUCCESS;
};

TYPED_INSTANCE(size_t, IO$Stream$remaining, T, buffer_t *Stream) {
	size_t Length = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Length += Node->Length;
	return Length;
};

METHOD("empty", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	Stream->Head = Stream->Tail = 0;
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("length", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	int Length = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Length += Node->Length;
	Result->Val = Std$Integer$new_small(Length);
	return SUCCESS;
};

METHOD("remaining", TYP, T) {
	buffer_t *Stream = (buffer_t *)Args[0].Val;
	int Length = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) Length += Node->Length;
	Result->Val = Std$Integer$new_small(Length);
	return SUCCESS;
};

METHOD("copy", TYP, T, TYP, IO$Stream$WriterT) {
	buffer_t *Rd = (buffer_t *)Args[0].Val;
	IO$Stream$t *Wr = Args[1].Val;
	IO$Stream$writefn write = Util$TypedFunction$get(IO$Stream$write, Wr->Type);
	int Total = 0;
	for (node_t *Node = Rd->Head; Node; Node = Node->Next) {
		const char *Buffer = Node->Chars;
		int Length = Node->Length;
		while (Length) {
			int Bytes = write(Wr, Node->Chars, Node->Length, 0);
			if (Bytes == -1) {
				Node->Chars = Buffer;
				Node->Length = Length;
				Rd->Head = Node;
				Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
				return MESSAGE;
			};
			Total += Bytes;
			Buffer += Bytes;
			Length -= Bytes;
			Rd->Position += Bytes;
			if (Bytes < Node->Length) {
				Node->Chars = Buffer;
				Node->Length = Length;
				Rd->Head = Node;
				Result->Val = Std$Integer$new_small(Total);
				return SUCCESS;
			};
		};
	};
	Rd->Head = Rd->Tail = 0;
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("copy", TYP, T, TYP, IO$Stream$WriterT, TYP, Std$Integer$SmallT) {
	buffer_t *Rd = (buffer_t *)Args[0].Val;
	IO$Stream$t *Wr = Args[1].Val;
	IO$Stream$writefn write = Util$TypedFunction$get(IO$Stream$write, Wr->Type);
	int Required = Std$Integer$get_small(Args[2].Val);
	
	Result->Val = Std$String$new("Not implemented yet!");
	return MESSAGE;
	
	int Total = 0;
	for (node_t *Node = Rd->Head; Node; Node = Node->Next) {
		const char *Buffer = Node->Chars;
		int Length = Node->Length;
		while (Length) {
			int Bytes = write(Wr, Node->Chars, Node->Length, 0);
			if (Bytes == -1) {
				Node->Chars = Buffer;
				Node->Length = Length;
				Rd->Head = Node;
				Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
				return MESSAGE;
			};
			Total += Bytes;
			Buffer += Bytes;
			Length -= Bytes;
			Rd->Position += Bytes;
			if (Bytes < Node->Length) {
				Node->Chars = Buffer;
				Node->Length = Length;
				Rd->Head = Node;
				Result->Val = Std$Integer$new_small(Total);
				return SUCCESS;
			};
		};
	};
	Rd->Head = Rd->Tail = 0;
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("copy", TYP, IO$Stream$ReaderT, TYP, T, TYP, Std$Integer$SmallT) {
	IO$Stream$t *Rd = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(IO$Stream$read, Rd->Type);
	buffer_t *Wr = (buffer_t *)Args[1].Val;
	int Required = Std$Integer$get_small(Args[2].Val);
	int Total = 0;
	node_t Tail[1] = {0,};
	node_t *Node = Tail;
	while (Required > 256) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(256);
		int Length = read(Rd, Chars, 256, 0);
		if (Length == -1) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		Total += Length;
		Node = Node->Next = new(node_t);
		Node->Length = Length;
		Node->Chars = Chars;
		if (Length < 256) goto done;
		Required -= 256;
	};
	unsigned char *Chars = Riva$Memory$alloc_atomic(Required);
	int Length = read(Rd, Node->Chars, Required, 0);
	if (Length == -1) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
	Total += Length;
	Node = Node->Next = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
done:
	if (Wr->Tail) {
		Wr->Tail->Next = Tail->Next;
		Wr->Tail = Node;
	} else {
		Wr->Head = Tail->Next;
		Wr->Tail = Node;
	};
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("copy", TYP, T, TYP, T) {
	buffer_t *Rd = (buffer_t *)Args[0].Val;
	buffer_t *Wr = (buffer_t *)Args[1].Val;
	int Length = 0;
	if (Rd->Head) {
		for (node_t *Node = Rd->Head; Node; Node = Node->Next) Length += Node->Length;
		Rd->Position += Length;
		if (Wr->Tail) {
			Wr->Tail->Next = Rd->Head;
		} else {
			Wr->Head = Rd->Head;
		};
		Wr->Tail = Rd->Tail;
	};
	Rd->Head = Rd->Tail = 0;
	Result->Val = Std$Integer$new_small(Length);
	return SUCCESS;
};

METHOD("put", TYP, Std$Address$T, TYP, T) {
//@address
//@t
// Writes the contents of <var>t</var> at <var>address</var>.
	unsigned char *Address = Std$Address$get_value(Args[0].Val);
	buffer_t *Buffer = (buffer_t *)Args[1].Val;
	for (node_t *Node = Buffer->Head; Node; Node = Node->Next) {
		memcpy(Address, Node->Chars, Node->Length);
		Address += Node->Length;
		Buffer->Position += Node->Length;
	}
	Buffer->Head = Buffer->Tail = 0;
	return SUCCESS;
}
