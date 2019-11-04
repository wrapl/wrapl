#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>
#include <Std.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <string.h>

SYMBOL($block, "block");

typedef struct node_t {
	struct node_t *Next;
	uint32_t Length;
	unsigned const char *Chars;
} node_t;

typedef struct buffer_t {
	const Std$Type$t *Type;
	node_t *Head, *Tail;
	pthread_cond_t Ready[1];
	pthread_mutex_t Lock[1];
	int Closed;
} buffer_t;

TYPE(T, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

#ifdef WINDOWS

static inline void *mempcpy(void *Dst, const void *Src, int Len) {
	memcpy(Dst, Src, Len);
	return (char *)Dst + Len;
};

#endif

static inline void lock(buffer_t *Stream) {
	pthread_mutex_lock(Stream->Lock);
};

static inline void unlock(buffer_t *Stream) {
	pthread_mutex_unlock(Stream->Lock);
};

static inline void wait(buffer_t *Stream) {
	pthread_cond_wait(Stream->Ready, Stream->Lock);
};

static inline void broadcast(buffer_t *Stream) {
	pthread_cond_broadcast(Stream->Ready);
};

static void buffer_finalize(buffer_t *Stream, void *Data) {
	pthread_mutex_destroy(Stream->Lock);
	pthread_cond_destroy(Stream->Ready);
};

static int buffer_eoi(buffer_t *Stream) {
	return (Stream->Head == 0);
};

static int buffer_close(buffer_t *Stream, int Mode) {
	lock(Stream);
	Stream->Closed = 1;
	broadcast(Stream);
	unlock(Stream);
};

static Std$Integer$smallt Zero[] = {{Std$Integer$SmallT, 0}};

static inline int read(buffer_t *Stream, char *Buffer, int Count) {
	lock(Stream);
	node_t *Node = Stream->Head;
	if (Node == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return 0;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			return -1;
		};
		wait(Stream);
		Node = Stream->Head;
		if (Node == 0) {
			unlock(Stream);
			return 0;
		};
	};
	uint32_t Total = 0;
	const char *Src = Node->Chars;
	const char *Dst = Buffer;
	uint32_t Rem0 = Node->Length;
	uint32_t Rem1 = Count;
	while (Rem0 <= Rem1) {
		Dst = mempcpy(Dst, Src, Rem0);
		Rem1 -= Rem0;
		Total += Rem0;
		if (Node = Node->Next) {
			Rem0 = Node->Length;
			Src = Node->Chars;
		} else {
			Stream->Head = Stream->Tail = 0;
			unlock(Stream);
			return Total;
		};
	};
	memcpy(Dst, Src, Rem1);
	Total += Rem1;
	Node->Chars += Rem1;
	Stream->Head = Node;
	unlock(Stream);
	return Total;
};

static int buffer_read(buffer_t *Stream, char *Buffer, int Count, int Blocks) {
	if (Blocks) {
		int Total = 0;
		while (Count) {
			int Bytes = read(Stream, Buffer, Count);
			if (Bytes == -1) return -1;
			if (Bytes == 0) return Total;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return read(Stream, Buffer, Count);
	};
};

static char buffer_readc(buffer_t *Stream) {
	lock(Stream);
	node_t *Node = Stream->Head;
	if (Node == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return EOF;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			return -1;
		};
		wait(Stream);
		Node = Stream->Head;
		if (Node == 0) {
			unlock(Stream);
			return EOF;
		};
	};
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
	unlock(Stream);
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
	unlock(Stream);
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
	unlock(Stream);
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

static char *buffer_readx(buffer_t *Stream, int Max, const char *Term, int TermSize) {
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return 0;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			return 0;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return 0;
		};
	};
	node_t *Node = Stream->Head;
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
			int Remaining = Max;
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, Term[0], Node->Length);
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

static char *buffer_readi(buffer_t *Stream, int Max, const char *Term, int TermSize) {
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return 0;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			return 0;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return 0;
		};
	};
	node_t *Node = Stream->Head;
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
			int Remaining = Max;
			while (Node) {
				unsigned char *Find = memchr(Node->Chars, Term, Node->Length);
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

static char *buffer_reads(buffer_t *Stream, int Count) {
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return "";
		};
		if (Stream->Closed == 2) {
			unlock(Stream);
			return 0;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return "";
		};
	};
	return read_chars_length(Stream, Count);
};

static char *buffer_readl(buffer_t *Stream) {
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return 0;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			return 0;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return 0;
		};
	};
	node_t *Node = Stream->Head;
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

static int buffer_write(buffer_t *Stream, const char *Buffer, int Count, int Block) {
	char *Chars = Riva$Memory$alloc_atomic(Count);
	memcpy(Chars, Buffer, Count);
	node_t *Node = new(node_t);
	Node->Length = Count;
	Node->Chars = Chars;
	lock(Stream);
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	broadcast(Stream);
	unlock(Stream);
	return Count;
};

static int buffer_writec(buffer_t *Stream, char Char) {
	char *Chars = Riva$Memory$alloc_atomic(1);
	Chars[0] = Char;
	node_t *Node = new(node_t);
	Node->Length = 1;
	Node->Chars = Chars;
	lock(Stream);
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	broadcast(Stream);
	unlock(Stream);
	return 1;
};

static int buffer_writes(buffer_t *Stream, const char *Text) {
	int Length = strlen(Text);
	char *Chars = Riva$Memory$alloc_atomic(Length);
	memcpy(Chars, Text, Length);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	lock(Stream);
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	broadcast(Stream);
	unlock(Stream);
	return Length;
};

static int buffer_writef(buffer_t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Chars;
	int Length = vasprintf(&Chars, Format, Args);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	lock(Stream);
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	broadcast(Stream);
	unlock(Stream);
	return Length;
};

INITIAL() {
	Util$TypedFunction$set(IO$Stream$eoi, T, buffer_eoi);
	Util$TypedFunction$set(IO$Stream$close, T, buffer_close);
	Util$TypedFunction$set(IO$Stream$read, T, buffer_read);
	Util$TypedFunction$set(IO$Stream$readc, T, buffer_readc);
	Util$TypedFunction$set(IO$Stream$readx, T, buffer_readx);
	Util$TypedFunction$set(IO$Stream$readi, T, buffer_readi);
	Util$TypedFunction$set(IO$Stream$readl, T, buffer_readl);
	Util$TypedFunction$set(IO$Stream$write, T, buffer_write);;
	Util$TypedFunction$set(IO$Stream$writec, T, buffer_writec);
	Util$TypedFunction$set(IO$Stream$writes, T, buffer_writes);
	Util$TypedFunction$set(IO$Stream$writef, T, buffer_writef);
};
GLOBAL_FUNCTION(New, 0) {
	buffer_t *Stream = new(buffer_t);
	Stream->Type = T;
	pthread_cond_init(Stream->Ready, 0);
	pthread_mutex_init(Stream->Lock, 0);
	Result->Val = Stream;
	return SUCCESS;
};

METHOD("close", TYP, T) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	Stream->Closed = 1;
	broadcast(Stream);
	unlock(Stream);
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	node_t *Node = Stream->Head;
	if (Node == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			Result->Val = Zero;
			return SUCCESS;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		Node = Stream->Head;
		if (Node == 0) {
			unlock(Stream);
			Result->Val = Zero;
			return SUCCESS;
		};
	};
	uint32_t Total = 0;
	const char *Src = Node->Chars;
	const char *Dst = ((Std$Address$t *)Args[1].Val)->Value;
	uint32_t Rem0 = Node->Length;
	uint32_t Rem1 = ((Std$Integer$smallt *)Args[2].Val)->Value;
	while (Rem0 <= Rem1) {
		Dst = mempcpy(Dst, Src, Rem0);
		Rem1 -= Rem0;
		Total += Rem0;
		if (Node = Node->Next) {
			Rem0 = Node->Length;
			Src = Node->Chars;
		} else {
			Stream->Head = Stream->Tail = 0;
			unlock(Stream);
			Result->Val = Std$Integer$new_small(Total);
			return SUCCESS;
		};
	};
	memcpy(Dst, Src, Rem1);
	Total += Rem1;
	Node->Chars += Rem1;
	Stream->Head = Node;
	unlock(Stream);
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	node_t *Node = Stream->Head;
	if (Node == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			Result->Val = Zero;
			return SUCCESS;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		Node = Stream->Head;
		if (Node == 0) {
			unlock(Stream);
			Result->Val = Zero;
			return SUCCESS;
		};
	};
	uint32_t Total = 0;
	const char *Src = Node->Chars;
	const char *Dst = ((Std$Address$t *)Args[1].Val)->Value;
	uint32_t Rem0 = Node->Length;
	uint32_t Rem1 = ((Std$Integer$smallt *)Args[2].Val)->Value;
	while (Rem0 <= Rem1) {
		Dst = mempcpy(Dst, Src, Rem0);
		Rem1 -= Rem0;
		Total += Rem0;
		if (Node = Node->Next) {
			Rem0 = Node->Length;
			Src = Node->Chars;
		} else {
			Stream->Head = Stream->Tail = 0;
			wait(Stream);
			Node = Stream->Head;
			if (Node == 0) goto done;
			Rem0 = Node->Length;
			Src = Node->Chars;
		};
	};
	memcpy(Dst, Src, Rem1);
	Total += Rem1;
	Node->Chars += Rem1;
	Stream->Head = Node;
done:
	unlock(Stream);
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

static Std$String$t *extract_string_rest(buffer_t *Stream) {
	int NoOfBlocks = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	String->Count = NoOfBlocks;
	Std$String$block *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) {
		String->Length.Value += (Block->Length.Value = Node->Length);
		Block->Chars.Value = Node->Chars;
		Block++;
	};
	Std$String$freeze(String);
	Stream->Head = Stream->Tail = 0;
	unlock(Stream);
	return String;
};

METHOD("rest", TYP, T) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			Result->Val = Std$String$new_length("", 0);
			return SUCCESS;
		};
		if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		if (Stream->Closed) {
			unlock(Stream);
			Result->Val = Std$String$new_length("", 0);
			return SUCCESS;
		};
	};
	int NoOfBlocks = 0;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	String->Count = NoOfBlocks;
	Std$String$block *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node; Node = Node->Next) {
		String->Length.Value += (Block->Length.Value = Node->Length);
		Block->Chars.Value = Node->Chars;
		Block++;
	};
	Std$String$freeze(String);
	Stream->Head = Stream->Tail = 0;
	unlock(Stream);
	Result->Val = String;
	return SUCCESS;
};

static Std$String$t *extract_string(buffer_t *Stream, node_t *EndNode, int EndOffset) {
	int NoOfBlocks = EndOffset ? 1 : 0;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) ++NoOfBlocks;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	String->Count = NoOfBlocks;
	int Length = 0;
	Std$String$block *Block = String->Blocks;
	for (node_t *Node = Stream->Head; Node != EndNode; Node = Node->Next) {
		Length += (Block->Length.Value = Node->Length);
		Block->Chars.Value = Node->Chars;
		++Block;
	};
	if (EndOffset) {
		Length += (Block->Length.Value = EndOffset);
		Block->Chars.Value = EndNode->Chars;
		if ((EndNode->Length -= EndOffset) == 0) {
			EndNode = EndNode->Next;
		} else {
			EndNode->Chars += EndOffset;
		};
	};
	String->Length.Value = Length;
	Std$String$freeze(String);
	Stream->Head = EndNode;
	if (EndNode == 0) Stream->Tail = 0;
	unlock(Stream);
	return String;
};

static Std$String$t *read_string_length(buffer_t *Stream, int Max) {
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
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return FAILURE;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return FAILURE;
		};
	};
	Std$String$t *Term = Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	node_t *Node = Stream->Head;
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
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Chars.Value), Node->Length);
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
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Chars.Value), Node->Length);
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
		for (Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
			unsigned char *Chars = Block->Chars.Value;
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
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return FAILURE;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return FAILURE;
		};
	};
	Std$String$t *Term = Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	node_t *Node = Stream->Head;
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
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Chars.Value), Node->Length);
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
				unsigned char *Find = memchr(Node->Chars, *((char *)Term->Blocks->Chars.Value), Node->Length);
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
		for (Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
			unsigned char *Chars = Block->Chars.Value;
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
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return FAILURE;
		} else if (Stream->Closed == 2) {
			unlock(Stream);
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return FAILURE;
		};
	};
	Result->Val = read_string_length(Stream, ((Std$Integer$smallt *)Args[1].Val)->Value);
	return SUCCESS;
};

METHOD("read", TYP, T) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	if (Stream->Head == 0) {
		if (Stream->Closed == 1) {
			Stream->Closed = 2;
			unlock(Stream);
			return FAILURE;
		} else if (Stream->Closed == 2) {
			Result->Val = IO$Stream$Message$new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__);
			return MESSAGE;
		};
		wait(Stream);
		if (Stream->Head == 0) {
			unlock(Stream);
			return FAILURE;
		};
	};
	node_t *Node = Stream->Head;
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

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffer_t *Stream = Args[0].Val;
	long Length = ((Std$Integer$smallt *)Args[2].Val)->Value;
	char *Chars = Riva$Memory$alloc_atomic(Length);
	memcpy(Chars, ((Std$Address$t *)Args[1].Val)->Value, Length);
	node_t *Node = new(node_t);
	Node->Length = Length;
	Node->Chars = Chars;
	lock(Stream);
	if (Stream->Tail) {
		Stream->Tail->Next = Node;
		Stream->Tail = Node;
	} else {
		Stream->Head = Node;
		Stream->Tail = Node;
	};
	broadcast(Stream);
	unlock(Stream);
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("write", TYP, T, TYP, Std$String$T) {
	buffer_t *Stream = Args[0].Val;
	int NoOfBlocks = ((Std$String$t *)Args[1].Val)->Count;
	Std$String$block *Block = ((Std$String$t *)Args[1].Val)->Blocks;
	if (Block->Length.Value) {
		node_t *Node = new(node_t);
		Node->Chars = Block->Chars.Value;
		Node->Length = Block->Length.Value;
		lock(Stream);
		if (Stream->Tail) {
			Stream->Tail->Next = Node;
		} else {
			Stream->Head = Node;
		};
		while ((++Block)->Length.Value) {
			node_t *Next = new(node_t);
			Node->Next = Next;
			Node = Next;
			Node->Chars = Block->Chars.Value;
			Node->Length = Block->Length.Value;
		};
		Stream->Tail = Node;
		broadcast(Stream);
		unlock(Stream);
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("empty", TYP, T) {
	buffer_t *Stream = Args[0].Val;
	lock(Stream);
	Stream->Head = Stream->Tail = 0;
	unlock(Stream);
	Result->Arg = Args[0];
	return SUCCESS;
};

