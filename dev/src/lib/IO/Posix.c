#include <IO/Posix.h>
#include <Util/TypedFunction.h>
#include <Sys/Module.h>
#include <Std.h>
#include <Riva.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/mman.h>

SYMBOL($block, "block");

TYPE(T, IO$Stream$T);

TYPE(ReaderT, T, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, IO$Stream$WriterT, IO$Stream$T);
TYPE(SeekerT, T, IO$Stream$SeekerT, IO$Stream$T);

TYPE(TextReaderT, T, ReaderT, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
TYPE(TextWriterT, T, WriterT, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

Std$Integer$smallt __POLLIN[] = {{Std$Integer$SmallT, POLLIN}};
Std$Integer$smallt __POLLOUT[] = {{Std$Integer$SmallT, POLLOUT}};
Std$Integer$smallt __POLLHUP[] = {{Std$Integer$SmallT, POLLHUP}};
Std$Integer$smallt __POLLPRI[] = {{Std$Integer$SmallT, POLLPRI}};

static int _SEEK[] = {
	[IO$Stream$SEEK_SET] = SEEK_SET,
	[IO$Stream$SEEK_CUR] = SEEK_CUR,
	[IO$Stream$SEEK_END] = SEEK_END
};

typedef struct fast_buffer {
	struct fast_buffer * restrict Next;
	unsigned char *Chars;
} fast_buffer;

#define FastBufferSize 1000
static fast_buffer * restrict alloc_fast_buffer(void) {
	fast_buffer *Buffer = new(fast_buffer);
	Buffer->Chars = Riva$Memory$alloc_atomic(FastBufferSize);
	return Buffer;
};

static void posix_finalize(IO$Posix$t *Stream, void *Data) {
	close(Stream->Handle);
};

void _posix_register_finalizer(IO$Posix$t *Stream) {
	Riva$Memory$register_finalizer((void *)Stream, (void *)posix_finalize, 0, 0, 0);
};

void _posix_unregister_finalizer(IO$Posix$t *Stream) {
	Riva$Memory$register_finalizer((void *)Stream, 0, 0, 0, 0);
};

IO$Posix$t *_posix_new(const Std$Type$t *Type, int Handle) {
	IO$Posix$t *Stream = (IO$Posix$t *)Riva$Memory$alloc_atomic(sizeof(IO$Posix$t));
	Stream->Type = Type;
	Stream->Handle = Handle;
	Riva$Memory$register_finalizer((void *)Stream, (void *)posix_finalize, 0, 0, 0);
	fcntl(Handle, F_SETFD, fcntl(Handle, F_GETFD, 0) | FD_CLOEXEC);
	return Stream;
};

static inline int write_all(int Handle, const char *Chars, int Count) {
	while (Count) {
		int Bytes = write(Handle, Chars, Count);
		if (Bytes == -1) return -1;
		Count -= Bytes;
		Chars += Bytes;
	};
	return 0;
};

TYPED_INSTANCE(int, IO$Stream$read, ReaderT, IO$Posix$t *Stream, char * restrict Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = read(Stream->Handle, Buffer, Count);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return read(Stream->Handle, Buffer, Count);
	};
};

TYPED_INSTANCE(const char *, IO$Stream$readx, ReaderT, IO$Posix$t *Stream, int Max, const char *Term, int TermSize) {
	int Handle = Stream->Handle;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: return (char *)-1;
	case 0: return 0;
	};
	unsigned char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	if (IsTerm[Char]) return "";
	if (Max == 1) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		return Chars;
	};
	IO$Stream$buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return (char *)-1;
		default: {
			if (IsTerm[Char]) {
			} else {
				if (Space == 0) {
					IO$Stream$buffer *Buffer = IO$Stream$alloc_buffer();
					Tail = (Tail->Next = Buffer);
					Space = IO$Stream$BufferSize;
					Ptr = Tail->Chars;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				if (Length == Max) {
				} else {
					break;
				};
			};
		};
		case 0: {
			unsigned char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream$buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Length -= IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, Length);
			Ptr[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

TYPED_INSTANCE(const char *, IO$Stream$readi, ReaderT, IO$Posix$t *Stream, int Max, const char *Term, int TermSize) {
	int Handle = Stream->Handle;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1: return (char *)-1;
	case 0: return 0;
	};
	unsigned char IsTerm[256] = {0,};
	for (int I = 0; I < TermSize; ++I) IsTerm[Term[I]] = 1;
	if (IsTerm[Char] || (Max == 1)) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		return Chars;
	};
	IO$Stream$buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return (char *)-1;
		default: {
			if (Space == 0) {
				IO$Stream$buffer *Buffer = IO$Stream$alloc_buffer();
				Tail = (Tail->Next = Buffer);
				Space = IO$Stream$BufferSize;
				Ptr = Tail->Chars;
			};
			*(Ptr++) = Char;
			--Space;
			++Length;
			if (IsTerm[Char] || (Length == Max)) {
			} else {
				break;
			};
		};
		case 0: {
			unsigned char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream$buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Length -= IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, Length);
			Ptr[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

TYPED_INSTANCE(void, IO$Stream$flush, WriterT, IO$Posix$t *Stream) {
	fsync(Stream->Handle);
};

TYPED_INSTANCE(int, IO$Stream$close, T, IO$Posix$t *Stream, int Mode) {
	if (close(Stream->Handle) == 0) {
		Riva$Memory$register_finalizer((void *)Stream, 0, 0, 0, 0);
		return 0;
	} else {
		return 1;
	}
};

TYPED_INSTANCE(int, IO$Stream$eoi, ReaderT, IO$Posix$t *Stream) {
	off_t Current = lseek(Stream->Handle, 0, SEEK_CUR);
	if (Current == -1) return -1;
	return 0;
};

TYPED_INSTANCE(char, IO$Stream$readc, ReaderT, IO$Posix$t *Stream) {
	char Char;
	int Status = read(Stream->Handle, &Char, 1);
	if (Status < 0) return 0;
	if (Status == 0) return EOF;
	return Char;
};

TYPED_INSTANCE(const char *, IO$Stream$readl, ReaderT, IO$Posix$t *Stream) {
	int Handle = Stream->Handle;
	char Char;
	do {
		switch (read(Handle, &Char, 1)) {
		case -1: return (char *)-1;
		case 0: return 0;
		};
	} while (Char == '\r');
	if (Char == '\n') return "";
	IO$Stream$buffer *Head, *Tail;
	Head = Tail = IO$Stream$alloc_buffer();
	int Length = 0;
	int Space = IO$Stream$BufferSize;
	char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: IO$Stream$free_buffers(Head, Tail); return (char *)-1;
		default: {
			if (Char == '\n') {
			} else if (Char == '\r') {
				break;
			} else {
				if (Space == 0) {
					IO$Stream$buffer *Buffer = IO$Stream$alloc_buffer();
					Tail = (Tail->Next = Buffer);
					Space = IO$Stream$BufferSize;
					Ptr = Tail->Chars;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				break;
			};
		};
		case 0: {
			char *Chars = Riva$Memory$alloc_atomic(Length + 1);
			Ptr = Chars;
			IO$Stream$buffer *Buffer = Head;
			while (Buffer->Next) {
				memcpy(Ptr, Buffer->Chars, IO$Stream$BufferSize);
				Ptr += IO$Stream$BufferSize;
				Buffer = Buffer->Next;
			};
			memcpy(Ptr, Buffer->Chars, (Length - 1) % IO$Stream$BufferSize + 1);
			Chars[Length] = 0;
			IO$Stream$free_buffers(Head, Tail);
			return Chars;
		};
		};
	};
};

TYPED_INSTANCE(int, IO$Stream$write, WriterT, IO$Posix$t *Stream, const char *Buffer, int Count, int Blocks) {
	if (Blocks) {
		int Total = 0;
		while (Count) {
			int Bytes = write(Stream->Handle, Buffer, Count);
			if (Bytes == -1) return -1;
			if (Bytes == 0) return Total;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		};
		return Total;
	} else {
		return write(Stream->Handle, Buffer, Count);
	};
};

TYPED_INSTANCE(void, IO$Stream$writec, WriterT, IO$Posix$t *Stream, char Char) {
	int Result = write(Stream->Handle, &Char, 1);
};

TYPED_INSTANCE(void, IO$Stream$writes, WriterT, IO$Posix$t *Stream, const char *Text) {
	write_all(Stream->Handle, Text, strlen(Text));
};

TYPED_INSTANCE(void, IO$Stream$writef, WriterT, IO$Posix$t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	write_all(Stream->Handle, Buffer, Length);
};

TYPED_INSTANCE(int, IO$Stream$seek, SeekerT, IO$Posix$t *Stream, int Position, int Mode) {
	return lseek(Stream->Handle, Position, _SEEK[Mode]);
};

TYPED_INSTANCE(int, IO$Stream$tell, SeekerT, IO$Posix$t *Stream) {
	return lseek(Stream->Handle, 0, SEEK_CUR);
};

METHOD("flush", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	if (fsync(Stream->Handle) < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$FlushMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("close", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	if (close(Stream->Handle)) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$CloseMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	} else {
		Riva$Memory$register_finalizer((void *)Stream, 0, 0, 0, 0);
		return SUCCESS;
	};
};

METHOD("closed", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	// TO BE FIXED
	return FAILURE;
};

METHOD("eoi", TYP, ReaderT) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	off_t Current = lseek(Stream->Handle, 0, SEEK_CUR);
	if (Current == -1) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$GenericMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
	};
	return FAILURE;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesRead = read(Stream->Handle, Buffer, Size);
	if (BytesRead < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, VAL, $block) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesRead = 0;
	while (Size) {
		int Bytes = read(Stream->Handle, Buffer, Size);
		if (Bytes < 0) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
		if (Bytes == 0) break;
		BytesRead += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	};
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

METHOD("readx", TYP, TextReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1:
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	case 0:
		return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (const Std$Address$t *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	if (IsTerm[Char]) {Result->Val = Std$String$Empty; return SUCCESS;};
	if (Max == 1) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		Result->Val = Std$String$new_length(Chars, 1);
		return SUCCESS;
	};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1:
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		default: {
			if (IsTerm[Char]) {
			} else {
				if (Space == 0) {
					Tail = (Tail->Next = alloc_fast_buffer());
					Space = FastBufferSize;
					Ptr = Tail->Chars;
					++NoOfBlocks;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				if (Length == Max) {
				} else {
					break;
				};
			};
		};
		case 0: {
			Std$String$t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$Address$t *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("readi", TYP, TextReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	unsigned char Char;
	switch (read(Handle, &Char, 1)) {
	case -1:
		Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno())); return MESSAGE;
	case 0: return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (const Std$Address$t *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Value;
		for (int I = 0; I < Block->Length.Value; ++I) IsTerm[Chars[I]] = 1;
	};
	if (IsTerm[Char] || (Max == 1)) {
		unsigned char *Chars = Riva$Memory$alloc_atomic(2);
		Chars[0] = Char;
		Chars[1] = 0;
		Result->Val = Std$String$new_length(Chars, 1);
		return SUCCESS;
	};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1:
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		default: {
			if (Space == 0) {
				Tail = (Tail->Next = alloc_fast_buffer());
				Space = FastBufferSize;
				Ptr = Tail->Chars;
				++NoOfBlocks;
			};
			*(Ptr++) = Char;
			--Space;
			++Length;
			if (IsTerm[Char] || (Length == Max)) {
			} else {
				break;
			};
		};
		case 0: {
			Std$String$t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$Address$t *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("rest", TYP, ReaderT) {
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	for (;;) {
		int Bytes = read(Handle, Ptr, Space);
		if (Bytes == -1) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
		if (Bytes == 0) break;
		Length += Bytes;
		Ptr += Bytes;
		Space -= Bytes;
		if (Space == 0) {
			Tail = (Tail->Next = alloc_fast_buffer());
			Space = FastBufferSize;
			Ptr = Tail->Chars;
			++NoOfBlocks;
		};
	};
	if (Ptr == Tail->Chars) NoOfBlocks--;
	Std$String$t *String = Std$String$alloc(NoOfBlocks);
	String->Length.Value = Length;
	Std$Address$t *Block = String->Blocks;
	for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
		Block->Length.Value = FastBufferSize;
		Block->Value = Buffer->Chars;
		++Block;
	};
	if (Ptr != Tail->Chars) {
		Block->Length.Value = Ptr - Tail->Chars;
		Block->Value = Tail->Chars;
	};
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
};

METHOD("read", TYP, ReaderT, TYP, Std$Integer$SmallT) {
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	if (Max == 0) {
		Result->Val = Std$String$Empty;
		return SUCCESS;
	};
	if (Max <= FastBufferSize) {
		char *String = Riva$Memory$alloc_atomic(Max + 1);
		int Length = 0;
		char *Buffer = String;
		while (Max) {
			int Bytes = read(Handle, Buffer, Max);
			if (Bytes == 0) break;
			if (Bytes == -1) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
			Buffer += Bytes;
			Length += Bytes;
			Max -= Bytes;
		};
		Buffer[0] = 0;
		if (Length) {
			Result->Val = Std$String$new_length(String, Length);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	} else {
		fast_buffer *Head, *Tail;
		Head = Tail = alloc_fast_buffer();
		int Length = 0, NoOfBlocks = 1;
		unsigned char *Ptr = Tail->Chars;
		while (Max > FastBufferSize) {
			int Space = FastBufferSize;
			while (Space > 0) {
				int Bytes = read(Handle, Ptr, Space);
				if (Bytes == -1) {
					Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
					return MESSAGE;
				};
				if (Bytes == 0) goto finished;
				Length += Bytes;
				Ptr += Bytes;
				Space -= Bytes;
			};
			Tail = (Tail->Next = alloc_fast_buffer());
			++NoOfBlocks;
			Ptr = Tail->Chars;
			Max -= FastBufferSize;
		};
		while (Max > 0) {
			int Bytes = read(Handle, Ptr, Max);
			if (Bytes == -1) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
			if (Bytes == 0) goto finished;
			Length += Bytes;
			Ptr += Bytes;
			Max -= Bytes;
		};
	finished:
		if (Length == 0) return FAILURE;
		if (Ptr == Tail->Chars) NoOfBlocks--;
		Std$String$t *String = Std$String$alloc(NoOfBlocks);
		String->Length.Value = Length;
		Std$Address$t *Block = String->Blocks;
		for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
			Block->Length.Value = FastBufferSize;
			Block->Value = Buffer->Chars;
			++Block;
		};
		if (Ptr != Tail->Chars) {
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Value = Tail->Chars;
		};
		Result->Val = Std$String$freeze(String);
		return SUCCESS;
		
		
		/*int Space = FastBufferSize;
		unsigned char *Ptr = Tail->Chars;
		
		while (Max) {
			int Bytes = read(Handle, Ptr, Space);
			if (Bytes == -1) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
			if (Bytes == 0) break;
			Length += Bytes;
			Ptr += Bytes;
			Space -= Bytes;
			if (Space == 0) {
				Tail = (Tail->Next = alloc_fast_buffer());
				Space = FastBufferSize;
				Ptr = Tail->Chars;
				++NoOfBlocks;
			};
			Max -= Bytes;
		};
		if (Length == 0) return FAILURE;
		if (Ptr == Tail->Chars) NoOfBlocks--;
		Std$String$t *String = Std$String$alloc(NoOfBlocks);
		String->Length.Value = Length;
		Std$Address$t *Block = String->Blocks;
		for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
			Block->Length.Value = FastBufferSize;
			Block->Value = Buffer->Chars;
			++Block;
		};
		if (Ptr != Tail->Chars) {
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Value = Tail->Chars;
		};
		Result->Val = Std$String$freeze(String);
		return SUCCESS;*/
	};
};

/*METHOD("read", TYP, ReaderT, TYP, Std$Integer$SmallT, VAL, $block) {
	printf("Using new read\n");
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	int Max = ((Std$Integer$smallt *)Args[1].Val)->Value;
	if (Max <= FastBufferSize) {
		char *String = Riva$Memory$alloc_atomic(Max + 1);
		int Length = 0;
		char *Buffer = String;
		while (Max) {
			int Bytes = read(Handle, Buffer, Max);
			if (Bytes == 0) continue;
			if (Bytes == -1) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
			Buffer += Bytes;
			Length += Bytes;
			Max -= Bytes;
		};
		Buffer[0] = 0;
		if (Length) {
			Result->Val = Std$String$new_length(String, Length);
			return SUCCESS;
		} else {
			return FAILURE;
		};
	} else {
		fast_buffer *Head, *Tail;
		Head = Tail = alloc_fast_buffer();
		int Length = 0, NoOfBlocks = 1;
		int Space = FastBufferSize;
		unsigned char *Ptr = Tail->Chars;
		while (Max) {
			int Bytes = read(Handle, Ptr, Space);
			if (Bytes == -1) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
			if (Bytes == 0) continue;
			Length += Bytes;
			Ptr += Bytes;
			Space -= Bytes;
			if (Space == 0) {
				Tail = (Tail->Next = alloc_fast_buffer());
				Space = FastBufferSize;
				Ptr = Tail->Chars;
				++NoOfBlocks;
			};
			Max -= Bytes;
		};
		if (Length == 0) return FAILURE;
		if (Ptr == Tail->Chars) NoOfBlocks--;
		Std$String$t *String = Std$String$alloc(NoOfBlocks);
		String->Length.Value = Length;
		Std$Address$t *Block = String->Blocks;
		for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
			Block->Length.Value = FastBufferSize;
			Block->Value = Buffer->Chars;
			++Block;
		};
		if (Ptr != Tail->Chars) {
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Value = Tail->Chars;
		};
		Result->Val = Std$String$freeze(String);
		return SUCCESS;
	};
};*/

METHOD("read", TYP, TextReaderT) {
	int Handle = ((IO$Posix$t *)Args[0].Val)->Handle;
	char Char;
	do {
		switch (read(Handle, &Char, 1)) {
		case -1:
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		case 0:
			return FAILURE;
		};
	} while (Char == '\r');
	if (Char == '\n') {Result->Val = Std$String$Empty; return SUCCESS;};
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	char *Ptr = Tail->Chars;
	*(Ptr++) = Char;
	--Space;
	++Length;
	for (;;) {
		switch (read(Handle, &Char, 1)) {
		case -1: {
			Result->Val = Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		}
		default: {
			if (Char == '\n') {
			} else if (Char == '\r') {
				break;
			} else {
				if (Space == 0) {
					Tail = (Tail->Next = alloc_fast_buffer());
					Space = FastBufferSize;
					Ptr = Tail->Chars;
					++NoOfBlocks;
				};
				*(Ptr++) = Char;
				--Space;
				++Length;
				break;
			};
		};
		case 0: {
			Std$String$t *String = Std$String$alloc(NoOfBlocks);
			String->Length.Value = Length;
			Std$Address$t *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesWritten = write(Stream->Handle, Buffer, Size);
	if (BytesWritten < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T, VAL, $block) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Address$get_length(Args[1].Val);
	int BytesWritten = 0;
	while (Size) {
		int Bytes = write(Stream->Handle, Buffer, Size);
		if (Bytes < 0) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
		if (Bytes == 0) break;
		BytesWritten += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	};
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
};

METHOD("write", TYP, WriterT, TYP, Std$String$T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	const Std$String$t *String = (Std$String$t *)Args[1].Val;
	for (int I = 0; I < String->Count; ++I) {
		if (write_all(Stream->Handle, String->Blocks[I].Value, String->Blocks[I].Length.Value) < 0) {
			Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
			return MESSAGE;
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, TextWriterT, ANY) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	Std$Function$result Result0;
	switch (Std$Function$call(Std$String$Of, 1, &Result0, Args[1].Val, Args[1].Ref)) {
	case SUSPEND:
	case SUCCESS: {
		const Std$String$t *String = (Std$String$t *)Result0.Val;
		for (int I = 0; I < String->Count; ++I) {
			if (write_all(Stream->Handle, String->Blocks[I].Value, String->Blocks[I].Length.Value) < 0) {
				Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
				return MESSAGE;
			};
		};
		Result->Arg = Args[0];
		return SUCCESS;
	};
	case FAILURE:
		Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	case MESSAGE:
		Result->Arg = Result0.Arg;
		return MESSAGE;
	};
};

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	int Position = ((Std$Integer$smallt *)Args[1].Val)->Value;
	Position = lseek(Stream->Handle, Position, _SEEK[((Std$Integer$smallt *)Args[2].Val)->Value]);
	if (Position < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$SeekMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Position);
	return SUCCESS;
};

METHOD("tell", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(lseek(Stream->Handle, 0, SEEK_CUR));
	return SUCCESS;
};

METHOD("poll", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	struct pollfd PollFd[] = {{Stream->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value, 0}};
	int Status = poll(PollFd, 1, ((Std$Integer$smallt *)Args[2].Val)->Value);
	if (Status < 0) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$PollMessageT, "%s:%d: %s", __FILE__, __LINE__, strerror(Riva$System$get_errno()));
		return MESSAGE;
	};
	if (Status == 0) return FAILURE;
	Result->Val = Std$Integer$new_small(PollFd->revents);
	return SUCCESS;
};

typedef struct pair_t {
	IO$Posix$t *Rd, *Wr;
} pair_t;

static void *_link_thread_func(pair_t *Pair) {
	char Buffer[256];
	int Rd = Pair->Rd->Handle;
	int Wr = Pair->Wr->Handle;
	Pair = 0;
	for (;;) {
		int BytesRead = read(Rd, Buffer, 256);
		if (BytesRead <= 0) {
			shutdown(Wr, 1);
			return 0;
		};
		char *Tmp = Buffer;
		while (BytesRead) {
			int BytesWritten = write(Wr, Tmp, BytesRead);
			if (BytesWritten < 0) return 0;
			Tmp += BytesWritten;
			BytesRead -= BytesWritten;
		};
	};
};

METHOD("link", TYP, T, TYP, T) {
	pthread_t Thread[1];
	pair_t *Pair = new(pair_t);
	Pair->Rd = (IO$Posix$t *)Args[0].Val;
	Pair->Wr = (IO$Posix$t *)Args[1].Val;
	pthread_create(Thread, 0, (void *)_link_thread_func, Pair);
	Result->Arg = Args[1];
	return SUCCESS;
};

METHOD("fd", TYP, T) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Stream->Handle);
	return SUCCESS;
};

METHOD("set_blocking", TYP, T, VAL, $true) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	int Flags = fcntl(Stream->Handle, F_GETFL, 0);
	fcntl(Stream->Handle, F_SETFL, Flags & ~O_NONBLOCK);
	return SUCCESS;
};

METHOD("set_blocking", TYP, T, VAL, $false) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	int Flags = fcntl(Stream->Handle, F_GETFL, 0);
	fcntl(Stream->Handle, F_SETFL, Flags | O_NONBLOCK);
	return SUCCESS;
};

Std$Integer$smallt MMAP_PROT_READ[] = {{Std$Integer$SmallT, PROT_READ}};
Std$Integer$smallt MMAP_PROT_WRITE[] = {{Std$Integer$SmallT, PROT_WRITE}};
Std$Integer$smallt MMAP_PROT_EXEC[] = {{Std$Integer$SmallT, PROT_EXEC}};

CONSTANT(Prot, Sys$Module$T) {
// Flags for selecting opening mode <dl class="submodule">
// <dt><code>.</code>Read : <id>Std/Integer/SmallT</id></dt><dd>Open a file for reading.</dd>
// <dt><code>.</code>Write : <id>Std/Integer/SmallT</id></dt><dd>Open a file for writing.</dd>
// <dt><code>.</code>Text : <id>Std/Integer/SmallT</id></dt><dd>Open a file in text mode (returned object is a <id>IO/Stream/TextReaderT</id> or <id>IO/Stream/TextWriterT</id>.</dd>
// <dt><code>.</code>Append : <id>Std/Integer/SmallT</id></dt><dd>For files opened in write mode, append new output to the end of the file.</dd>
// </dl>
	Sys$Module$t *Module = Sys$Module$new("Prot");
	Sys$Module$export(Module, "Read", 0, (void *)MMAP_PROT_READ);
	Sys$Module$export(Module, "Write", 0, (void *)MMAP_PROT_WRITE);
	Sys$Module$export(Module, "Exec", 0, (void *)MMAP_PROT_EXEC);
	return (Std$Object$t *)Module;
};

METHOD("mmap", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	IO$Posix$t *Stream = (IO$Posix$t *)Args[0].Val;
	size_t Length = Std$Integer$get_small(Args[1].Val);
	void *Address = mmap(0, Length,
		Std$Integer$get_small(Args[2].Val),
		MAP_SHARED,
		Stream->Handle,
		Std$Integer$get_small(Args[3].Val)
	);
	if (Address == MAP_FAILED) {
		Result->Val = Sys$Program$error_from_errno(IO$Stream$OpenMessageT);
		return MESSAGE;
	};
	Result->Val = Std$Address$new(Address, Length);
	return SUCCESS;
};

METHOD("munmap", TYP, Std$Address$T) {
	void *Address = Std$Address$get_value(Args[0].Val);
	size_t Length = Std$Address$get_length(Args[0].Val);
	if (munmap(Address, Length)) {
		Result->Val = Sys$Program$error_from_errno(IO$Stream$OpenMessageT);
		return MESSAGE;
	};
	return SUCCESS;
};

static ssize_t cfile_read(void *Stream, char *Buffer, size_t Size) {
	printf("cfile_read(0x%x, 0x%x, %d)\n", Stream, Buffer, Size);
	return IO$Stream$read(Stream, Buffer, Size, 1);
};

static ssize_t cfile_write(void *Stream, const char *Buffer, size_t Size) {
	printf("cfile_write(0x%x, 0x%x, %d)\n", Stream, Buffer, Size);
	return IO$Stream$write(Stream, Buffer, Size, 1);
};

static int cfile_seek(void *Stream, off64_t *Position, int Whence) {
	printf("cfile_seek(0x%x, 0x%x, %d)\n", Stream, Position, Whence);
	static IO$Stream$seekmode Modes[] = {
		[SEEK_SET] = IO$Stream$SEEK_SET,
		[SEEK_CUR] = IO$Stream$SEEK_CUR,
		[SEEK_END] = IO$Stream$SEEK_END
	};
	return IO$Stream$seek(Stream, *(int *)Position, Modes[Whence]);
};

static int cfile_close(void *Stream) {
	printf("cfile_close(0x%x)\n", Stream);
	IO$Stream$close(Stream, IO$Stream$CLOSE_BOTH);
	return 0;
};

FILE *_cfile(IO$Stream$t *Stream) {
	static cookie_io_functions_t Functions = {
		.read = cfile_read,
		.write = cfile_write,
		.seek = cfile_seek,
		.close = cfile_close
	};
	printf("_cfile(0x%x)\n", Stream);
	return fopencookie(Stream, "rw", Functions);
};
