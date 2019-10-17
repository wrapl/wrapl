#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Std.h>
#include <Riva/Memory.h>
#include <Riva/Thread.h>
#include <Riva/System.h>
#include <Sys/Module.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

MODULE(IO, Stream);
// Defines the base stream types and provides default implementations of stream methods for their subtypes.

#ifdef WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

SYMBOL($read, "read");
SYMBOL($readx, "readx");
SYMBOL($readi, "readi");
SYMBOL($write, "write");
SYMBOL($flush, "flush");
SYMBOL($close, "close");
SYMBOL($eoi, "eoi");
SYMBOL($seek, "seek");
SYMBOL($tell, "tell");
SYMBOL($remaining, "remaining");
SYMBOL($block, "block");

typedef IO$Stream$t stream_t;

TYPE(T);
// A stream of bytes. Streams may be read-only, write-only or read-write and may support seeking. New stream types should inherit from one of the subtypes defined below.
TYPE(ReaderT, T);
// A readable stream. Subtypes of <var>ReaderT</var> should define at least the <code>:read(@ReaderT, @Std.Address.T, @Std.Integer.SmallT)</code> method, default implementations for the other read methods are provided in this module.
TYPE(WriterT, T);
// A writable stream. Subtypes of <var>WriterT</var> should define at least the <code>:write(@WriterT, @Std.Address.T, @Std.Integer.SmallT)</code> method, default implementations for the other write methods are provided in this module.
TYPE(SeekerT, T);
// A seekable stream. Subtypes of <var>SeekerT</var> should define at least the <code>:seek(@SeekerT, @Std.Integer.SmallT)</code> method, default implementations for the other seek methods are provided in this module.
TYPE(TextReaderT, ReaderT, T);
// A readable stream with added methods for reading text.
TYPE(TextWriterT, WriterT, T);
// A writable stream with added methods for writing text.

#ifndef DOCUMENTING
TYPE(MessageT, Sys$Program$ErrorT);
#endif

AMETHOD(Std$String$Of, TYP, MessageT) {
//@msg
//:Std$String$T
	IO$Stream$messaget *Msg = (IO$Stream$messaget *)Args[0].Val;
	Result->Val = Std$String$new(Msg->Message);
	return SUCCESS;
};

#ifndef DOCUMENTING
TYPE(ConvertMessageT, MessageT, Sys$Program$ErrorT);
TYPE(GenericMessageT, MessageT, Sys$Program$ErrorT);
TYPE(OpenMessageT, MessageT, Sys$Program$ErrorT);
TYPE(ReadMessageT, MessageT, Sys$Program$ErrorT);
TYPE(WriteMessageT, MessageT, Sys$Program$ErrorT);
TYPE(FlushMessageT, MessageT, Sys$Program$ErrorT);
TYPE(SeekMessageT, MessageT, Sys$Program$ErrorT);
TYPE(CloseMessageT, MessageT, Sys$Program$ErrorT);
TYPE(PollMessageT, MessageT, Sys$Program$ErrorT);

Std$Object$t *_message_new(const Std$Type$t *Type, const char *Description) {
	IO$Stream$messaget *Message = new(IO$Stream$messaget);
	Message->Type = Type;
	Message->Message = Description;
	Message->StackTrace = Sys$Program$stack_trace(32);
	return (Std$Object$t *)Message;
};

Std$Object$t *_message_new_format(const Std$Type$t *Type, const char *Format, ...) {
	IO$Stream$messaget *Message = new(IO$Stream$messaget);
	Message->Type = Type;
	va_list Args;
	va_start(Args, Format);
	int Length = vasprintf(&Message->Message, Format, Args);
	va_end(Args);
	Message->StackTrace = Sys$Program$stack_trace(32);
	return (Std$Object$t *)Message;
};

Std$Object$t *_message_from_errno(const Std$Type$t *Type) {
	IO$Stream$messaget *Message = new(IO$Stream$messaget);
	Message->Type = Type;
	char Buffer[256];
	Message->Message = Riva$Memory$strdup(strerror_r(Riva$System$get_errno(), Buffer, 256));
	Message->StackTrace = Sys$Program$stack_trace(32);
	return (Std$Object$t *)Message;
};

IO$Stream$messaget ConvertMessage[] = {{ConvertMessageT, "Conversion Error"}};
IO$Stream$messaget GenericMessage[] = {{GenericMessageT, "Generic Error"}};
IO$Stream$messaget OpenMessage[] = {{OpenMessageT, "Open Error"}};
IO$Stream$messaget ReadMessage[] = {{ReadMessageT, "Read Error"}};
IO$Stream$messaget WriteMessage[] = {{WriteMessageT, "Write Error"}};
IO$Stream$messaget FlushMessage[] = {{FlushMessageT, "Flush Error"}};
IO$Stream$messaget SeekMessage[] = {{SeekMessageT, "Seek Error"}};
IO$Stream$messaget CloseMessage[] = {{CloseMessageT, "Close Error"}};
IO$Stream$messaget PollMessage[] = {{PollMessageT, "Poll Error"}};
#endif

SUBMODULE(Message);
//@Error : Std$Type$T
// Base type for error messages sent by stream methods.
//@ConvertError : Message.Error
// Conversion error occurred.
//@GenericError : Message.Error
// Generic error occurred.
//@ReadError : Message.Error
// Read error occurred.
//@WriteError : Message.Error
// Write error occurred.
//@FlushError : Message.Error
// Flush error occurred.
//@SeekError : Message.Error
// Seek error occurred.
//@CloseError : Message.Error
// Close error occurred.
//@PollError : Message.Error
// Poll error occurred.

#ifdef DOCUMENTING
#define CloseModeT ?CloseMode.T
#else
TYPE(CloseModeT);
#endif

Std$Integer$smallt _CLOSE_BOTH[] = {{CloseModeT, IO$Stream$CLOSE_BOTH}};
Std$Integer$smallt _CLOSE_READ[] = {{CloseModeT, IO$Stream$CLOSE_READ}};
Std$Integer$smallt _CLOSE_WRITE[] = {{CloseModeT, IO$Stream$CLOSE_WRITE}};

SUBMODULE(CloseMode);
//@T : Std$Type$T
// Close mode to pass to <code>:close</code>.
//@Read : CloseMode.T
// Close the stream for further reading.
//@Write : CloseMode.T
// Close the stream for further writing.
//@Both : CloseMode.T
// Close the stream for further reading or writing.

static Std$Integer$smallt *_CLOSE[] = {_CLOSE_BOTH, _CLOSE_READ, _CLOSE_WRITE};

#ifdef DOCUMENTING
#define SeekModeT ?SeekMode.T
#else
TYPE(SeekModeT);
#endif

Std$Integer$smallt _SEEK_SET[] = {{SeekModeT, IO$Stream$SEEK_SET}};
Std$Integer$smallt _SEEK_CUR[] = {{SeekModeT, IO$Stream$SEEK_CUR}};
Std$Integer$smallt _SEEK_END[] = {{SeekModeT, IO$Stream$SEEK_END}};

SUBMODULE(SeekMode);
//@T : Std$Type$T
// Seek mode to pass to <code>:seek</code>.
//@Set : SeekMode.T
// Seek to absolute relative to beginning of stream.
//@Cur : SeekMode.T
// Seek to position relative to current position.
//@End : SeekMode.T
// Seek to position relative to end of stream.

static Std$Integer$smallt *_SEEK[] = {_SEEK_SET, _SEEK_CUR, _SEEK_END};

static IO$Stream$buffer * restrict Buffers;

#ifdef WINDOWS
static CRITICAL_SECTION BufferMutex;
#else
static pthread_mutex_t BufferMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

IO$Stream$buffer * restrict _alloc_buffer(void) {
#ifdef WINDOWS
    EnterCriticalSection(&BufferMutex);
#else
	pthread_mutex_lock(&BufferMutex);
#endif
	if (Buffers) {
		IO$Stream$buffer * restrict Buffer = Buffers;
		Buffers = Buffer->Next;
#ifdef WINDOWS
		LeaveCriticalSection(&BufferMutex);
#else
		pthread_mutex_unlock(&BufferMutex);
#endif
		Buffer->Next = 0;
		return Buffer;
	} else {
#ifdef WINDOWS
    	LeaveCriticalSection(&BufferMutex);
#else
		pthread_mutex_unlock(&BufferMutex);
#endif
		IO$Stream$buffer * restrict Buffer = (IO$Stream$buffer *)Riva$Memory$alloc_uncollectable(sizeof(IO$Stream$buffer));
		Buffer->Next = 0;
		return Buffer;
	};
};

void _free_buffer(IO$Stream$buffer * restrict Buffer) {
#ifdef WINDOWS
    EnterCriticalSection(&BufferMutex);
#else
	pthread_mutex_lock(&BufferMutex);
#endif
	Buffer->Next = Buffers;
	Buffers = Buffer;
#ifdef WINDOWS
	LeaveCriticalSection(&BufferMutex);
#else
	pthread_mutex_unlock(&BufferMutex);
#endif
};

void _free_buffers(IO$Stream$buffer * restrict Head, IO$Stream$buffer * restrict Tail) {
#ifdef WINDOWS
	EnterCriticalSection(&BufferMutex);
#else
	pthread_mutex_lock(&BufferMutex);
#endif
	Tail->Next = Buffers;
	Buffers = Head;
#ifdef WINDOWS
	LeaveCriticalSection(&BufferMutex);
#else
	pthread_mutex_unlock(&BufferMutex);
#endif
};

typedef struct fast_buffer {
	struct fast_buffer *restrict Next;
	unsigned char * restrict Chars;
} fast_buffer;

#define FastBufferSize 100
static fast_buffer *restrict alloc_fast_buffer(void) {
	fast_buffer *Buffer = new(fast_buffer);
	Buffer->Chars = Riva$Memory$alloc_atomic(FastBufferSize);
	return Buffer;
};

TYPED_FUNCTION(int, _t_close, IO$Stream$t *Stream, IO$Stream$closemode Mode) {
	printf("No method defined for \"close\"\n");
};

TYPED_FUNCTION(int, _t_read, IO$Stream$t *Stream, char *Buffer, int Count, int Blocking) {
	printf("No method defined for \"read\"\n");
};

TYPED_FUNCTION(char, _t_readc, IO$Stream$t *Stream) {
	printf("No method defined for \"readc\"\n");
};

TYPED_FUNCTION(char *, _t_readx, IO$Stream$t *Stream, int Count, const char *Term, int Blocking) {
	printf("No method defined for \"readx\"\n");
};

TYPED_FUNCTION(char *, _t_readi, IO$Stream$t *Stream, int Count, const char *Buffer, int Blocking) {
	printf("No method defined for \"readi\"\n");
};

TYPED_FUNCTION(char *, _t_readl, IO$Stream$t *Stream) {
	printf("No method defined for \"readl\"\n");
};

TYPED_FUNCTION(char, _t_eoi, IO$Stream$t *Stream) {
	printf("No method defined for \"eoi\"\n");
};

TYPED_FUNCTION(int, _t_write, IO$Stream$t *Stream, const char *Buffer, int Count, int Blocking) {
	printf("No method defined for \"write\"\n");
};

TYPED_FUNCTION(void, _t_writec, IO$Stream$t *Stream, char Char) {
	printf("No method defined for \"writec\"\n");
};

TYPED_FUNCTION(void, _t_writes, IO$Stream$t *Stream, char *String) {
	printf("No method defined for \"writes\"\n");
};

TYPED_FUNCTION(void, _t_writef, IO$Stream$t *Stream, char *Format, ...) {
	printf("No method defined for \"writef\"\n");
};

TYPED_FUNCTION(void, _t_flush, IO$Stream$t *Stream) {
	printf("No method defined for \"flush\"\n");
};

TYPED_FUNCTION(int, _t_seek, IO$Stream$t *Stream, int Position, IO$Stream$seekmode Mode) {
	printf("No method defined for \"seek\"\n");
};

TYPED_FUNCTION(int, _t_tell, IO$Stream$t *Stream) {
	printf("No method defined for \"tell\"\n");
};

TYPED_FUNCTION(size_t, _t_remaining, IO$Stream$t *Stream) {
	printf("No method defined for \"remaining\"\n");
};

TYPED_INSTANCE(void, _t_flush, T, IO$Stream$t *Stream) {
	Std$Function$result Result;
	Std$Function$call($flush, 1, &Result, Stream, 0);
};

TYPED_INSTANCE(int, _t_close, T, IO$Stream$t *Stream, IO$Stream$closemode Mode) {
	Std$Function$result Result;
	return Std$Function$call($close, 2, &Result, Stream, 0, _CLOSE[Mode], 0) > SUCCESS;
};

TYPED_INSTANCE(int, _t_eoi, ReaderT, IO$Stream$t *Stream) {
	Std$Function$result Result;
	if (Std$Function$call($eoi, 1, &Result, Stream, 0) < FAILURE) {
		return 1;
	} else {
		return 0;
	};
};

TYPED_INSTANCE(int, _t_read, ReaderT, IO$Stream$t *Stream, char * restrict Buffer, int Count, int Block) {
	if (Block) {
		int Read = 0;
		Std$Function$result Result;
		while (Count) {
			switch (Std$Function$call($read, 3, &Result, Stream, 0, Std$Address$new(Buffer), 0, Std$Integer$new_small(Count), 0, $block, 0)) {
			case SUSPEND: case SUCCESS: {
				int Bytes = Std$Integer$get_small(Result.Val);
				Buffer += Bytes;
				Read += Bytes;
				Count -= Bytes;
				break;
			};
			case FAILURE:
				return Read;
			case MESSAGE:
				return -1;
			};
		};
		return Read;
	} else {
		Std$Function$result Result;
		switch (Std$Function$call($read, 2, &Result, Stream, 0, Std$Address$new(Buffer), 0, Std$Integer$new_small(Count), 0)) {
		case SUSPEND: case SUCCESS:
			return Std$Integer$get_small(Result.Val);
		case FAILURE:
			return 0;
		case MESSAGE:
			return -1;
		};
	};
};

TYPED_INSTANCE(const char *, _t_readx, ReaderT, IO$Stream$t *Stream, int Max, char *Term, int TermSize) {
	Std$Function$result Result;
	switch (Std$Function$call($readx, 3, &Result, Stream, 0, Std$Integer$new_small(Max), 0, TermSize ? Std$String$new_length(Term, TermSize) : Std$String$Empty, 0)) {
	case SUCCESS: case SUSPEND:
		return Std$String$flatten(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return (char *)-1;
	};
};

TYPED_INSTANCE(const char *, _t_readi, ReaderT, IO$Stream$t *Stream, int Max, const char *Term, int TermSize) {
	Std$Function$result Result;
	switch (Std$Function$call($readi, 3, &Result, Stream, 0, Std$Integer$new_small(Max), 0, TermSize ? Std$String$new_length(Term, TermSize) : Std$String$Empty, 0)) {
	case SUCCESS: case SUSPEND:
		return Std$String$flatten(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return (char *)-1;
	};
};

TYPED_INSTANCE(char, _t_readc, ReaderT, IO$Stream$t *Stream) {
	static Std$Integer$smallt One[1] = {{Std$Integer$SmallT, 1}};
	Std$Function$result Result;
	switch (Std$Function$call($read, 3, &Result, Stream, 0, One, 0, $block, 0)) {
	case SUCCESS: case SUSPEND:
		return Std$String$get_char(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return -1;
	};
};

TYPED_INSTANCE(const char *, _t_readl, ReaderT, IO$Stream$t *Stream) {
	Std$Function$result Result;
	switch (Std$Function$call($read, 1, &Result, Stream, 0)) {
	case SUCCESS: case SUSPEND:
		return Std$String$flatten(Result.Val);
	case FAILURE:
		return 0;
	case MESSAGE:
		return (char *)-1;
	};
};

TYPED_INSTANCE(int, _t_write, WriterT, IO$Stream$t *Stream, const char *Buffer, int Count, int Block) {
	if (Block) {
		Std$Function$result Result;
		int Written = 0;
		while (Count) {
			switch (Std$Function$call($write, 3, &Result, Stream, 0, Std$Address$new(Buffer), 0, Std$Integer$new_small(Count), 0, $block, 0)) {
			case SUSPEND: case SUCCESS: {
				int Bytes = Std$Integer$get_small(Result.Val);
				Buffer += Bytes;
				Written += Bytes;
				Count -= Bytes;
				break;
			};
			case FAILURE:
				return Written;
			case MESSAGE:
				return -1;
			};
		};
		return Written;
	} else {
		Std$Function$result Result;
		switch (Std$Function$call($write, 2, &Result, Stream, 0, Std$Address$new(Buffer), 0, Std$Integer$new_small(Count), 0)) {
		case SUSPEND: case SUCCESS:
			return Std$Integer$get_small(Result.Val);
		case FAILURE:
			return 0;
		case MESSAGE:
			return -1;
		};
	};
};

TYPED_INSTANCE(int, _t_writec, WriterT, IO$Stream$t *Stream, char Char) {
	Std$Object$t *String = Std$String$new_char(Char);
	Std$Function$result Result;
	return Std$Function$call($write, 2, &Result, Stream, 0, String, 0) > SUCCESS;
};

TYPED_INSTANCE(int, _t_writes, WriterT, IO$Stream$t *Stream, const char *Text) {
	Std$Object$t *String = Std$String$new(Text);
	Std$Function$result Result;
	return Std$Function$call($write, 2, &Result, Stream, 0, String, 0) > SUCCESS;
};

TYPED_INSTANCE(int, _t_writef, WriterT, IO$Stream$t *Stream, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Buffer;
	int Length = vasprintf(&Buffer, Format, Args);
	Std$Object$t *String = Std$String$new_length(Buffer, Length);
	Std$Function$result Result;
	return Std$Function$call($write, 2, &Result, Stream, 0, String, 0) > SUCCESS;
};

TYPED_INSTANCE(int, _t_seek, SeekerT, IO$Stream$t *Stream, int Position, IO$Stream$seekmode Mode) {
	if ((Mode < IO$Stream$SEEK_SET) || (Mode > IO$Stream$SEEK_END)) return -1;
	Std$Function$result Result;
	if (Std$Function$call($seek, 3, &Result, Stream, 0, Std$Integer$new_small(Position), 0, _SEEK[Mode], 0) < FAILURE) {
		return Std$Integer$get_small(Result.Val);
	} else {
		return -1;
	};
};

TYPED_INSTANCE(int, _t_tell, SeekerT, IO$Stream$t *Stream) {
	Std$Function$result Result;
	if (Std$Function$call($tell, 1, &Result, Stream, 0) < FAILURE) {
		return Std$Integer$get_small(Result.Val);
	} else {
		return -1;
	};
};

TYPED_INSTANCE(size_t, _t_remaining, SeekerT, IO$Stream$t *Stream) {
	Std$Function$result Result;
	if (Std$Function$call($remaining, 1, &Result, Stream, 0) < FAILURE) {
		return Std$Integer$get_small(Result.Val);
	} else {
		return 0;
	};
};

#ifdef WINDOWS
INITIAL() {
	InitializeCriticalSection(&BufferMutex);
};
#endif

METHOD("flush", TYP, T) {
//@t
// Completes any pending operations on <var>t</var>.
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("close", TYP, T) {
//@t
// Closes <var>t</var> for further reading or writing
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("closed", TYP, T) {
//@t
// Fails if <var>t</var> is still open for reading or writing.
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("eoi", TYP, ReaderT) {
//@t
// Succeeds if <var>t</var> has read an end of file marker.
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
//@rd
//@buffer
//@length
//:Std$Integer$SmallT
// Reads up to <var>length</var> bytes from <var>rd</var> into <var>buffer</var>
// Returns the number of bytes read
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("readx", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
//@rd
//@max
//@terminal
//:Std$String$T
// Reads bytes from <var>rd</var> until either <var>max</var> bytes have been read, or a byte in <var>terminal</var> has been read, or <var>rd</var> is empty.
// Returns the bytes read as a string <em>excluding</em> the final byte if it is in <var>terminal</var>.
// Passing <code>0</code> for <var>max</var> will ignore the number of bytes read.
// If <code>terminal = ""</code>, then only the number of bytes is checked
// If both <code>max = 0</code> and <code>terminal = ""</code> then all the remaining bytes from <var>rd</var> are returned.
	stream_t *Stream = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Stream->Type);
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = Std$Integer$get_small(Args[1].Val);
	unsigned char Char;
	switch (read(Stream, &Char, 1, 0)) {
	case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
	case 0: return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
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
		switch (read(Stream, &Char, 1, 0)) {
		case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
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
			Std$String$block *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Chars.Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("readi", TYP, ReaderT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
//@rd
//@max
//@terminal
//:Std$String$T
// Reads bytes from <var>rd</var> until either <var>max</var> bytes have been read, or a byte in <var>terminal</var> has been read, or <var>rd</var> is empty.
// Returns the bytes read as a string <em>including</em> the final byte if it is in <var>terminal</var>.
// Passing <code>0</code> for <var>max</var> will ignore the number of bytes read.
// If <code>terminal = ""</code>, then only the number of bytes is checked
// If both <code>max = 0</code> and <code>terminal = ""</code> then all the remaining bytes from <var>rd</var> are returned.
	stream_t *Stream = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Stream->Type);
	const Std$String$t *Term = (Std$String$t *)Args[2].Val;
	int Max = Std$Integer$get_small(Args[1].Val);
	unsigned char Char;
	switch (read(Stream, &Char, 1, 0)) {
	case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
	case 0: return FAILURE;
	};
	unsigned char IsTerm[256] = {0,};
	for (const Std$String$block *Block = Term->Blocks; Block->Length.Value; Block++) {
		const unsigned char *Chars = Block->Chars.Value;
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
		switch (read(Stream, &Char, 1, 0)) {
		case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
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
			Std$String$block *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Chars.Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("rest", TYP, ReaderT) {
//@rd
//:Std$String$T
// Returns the remaining contents of <var>rd</var>.
	stream_t *Stream = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Stream->Type);
	fast_buffer *Head, *Tail;
	Head = Tail = alloc_fast_buffer();
	int Length = 0, NoOfBlocks = 1;
	int Space = FastBufferSize;
	unsigned char *Ptr = Tail->Chars;
	for (;;) {
		int Bytes = read(Stream, Ptr, Space, 0);
		if (Bytes == -1) {
			Result->Val = (Std$Object$t *)ReadMessage;
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
	Std$String$block *Block = String->Blocks;
	for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
		Block->Length.Value = FastBufferSize;
		Block->Chars.Value = Buffer->Chars;
		++Block;
	};
	if (Ptr != Tail->Chars) {
		Block->Length.Value = Ptr - Tail->Chars;
		Block->Chars.Value = Tail->Chars;
	};
	Result->Val = Std$String$freeze(String);
	return SUCCESS;
};

METHOD("read", TYP, ReaderT, TYP, Std$Integer$SmallT) {
//@rd
//@count
//:Std$String$T
// Reads <var>count</var> bytes from <var>rd</var> and returns them as a string.
	stream_t *Stream = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Stream->Type);
	int Max = Std$Integer$get_small(Args[1].Val);
	if (Max <= FastBufferSize) {
		char *String = Riva$Memory$alloc_atomic(Max + 1);
		int Length = 0;
		char *Buffer = String;
		while (Max) {
			int Bytes = read(Stream, Buffer, Max, 1);
			if (Bytes == 0) break;
			if (Bytes == -1) {
				Result->Val = (Std$Object$t *)ReadMessage;
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
			int Bytes = read(Stream, Ptr, Space, 1);
			if (Bytes == -1) {
				Result->Val = (Std$Object$t *)ReadMessage;
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
		Std$String$block *Block = String->Blocks;
		for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
			Block->Length.Value = FastBufferSize;
			Block->Chars.Value = Buffer->Chars;
			++Block;
		};
		if (Ptr != Tail->Chars) {
			Block->Length.Value = Ptr - Tail->Chars;
			Block->Chars.Value = Tail->Chars;
		};
		Result->Val = Std$String$freeze(String);
		return SUCCESS;
	};
};

METHOD("read", TYP, TextReaderT) {
//@rd
//:Std$String$T
// Reads the next line of text from <var>rd</var> and returns it as a string without the carriage return.
// This methods differs from <code>t:readx("\n", 0)</code> in that it treats <code>"\r\n"</code> as a single carriage return.
	stream_t *Stream = Args[0].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Stream->Type);
	char Char;
	do {
		switch (read(Stream, &Char, 1, 0)) {
		case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
		case 0: return FAILURE;
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
		switch (read(Stream, &Char, 1, 0)) {
		case -1: Result->Val = (Std$Object$t *)ReadMessage; return MESSAGE;
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
			Std$String$block *Block = String->Blocks;
			for (fast_buffer *Buffer = Head; Buffer->Next; Buffer = Buffer->Next) {
				Block->Length.Value = FastBufferSize;
				Block->Chars.Value = Buffer->Chars;
				++Block;
			};
			Block->Length.Value = (Length - 1) % FastBufferSize + 1;
			Block->Chars.Value = Tail->Chars;
			Result->Val = Std$String$freeze(String);
			return SUCCESS;
		};
		};
	};
};

METHOD("copy", TYP, ReaderT, TYP, WriterT) {
//@rd
//@wr
//:Std$Integer$T
// Copies the contents of <var>rd</var> to <var>wr</var>.
	IO$Stream$t *Rd = Args[0].Val;
	IO$Stream$t *Wr = Args[1].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Rd->Type);
	IO$Stream$writefn write = Util$TypedFunction$get(_t_write, Wr->Type);
	int Rem = Std$Integer$get_small(Args[2].Val);
	int Total = 0;
	char Buffer[1024];
	for (;;) {
		int Read = read(Rd, Buffer, 1024, 0);
		if (Read == -1) {
			Result->Val = (Std$Object$t *)ReadMessage;
			return MESSAGE;
		};
		if (Read == 0) break;
		char *Ptr = Buffer;
		while (Read) {
			int Written = write(Wr, Ptr, Read, 1);
			if (Written == -1) {
				Result->Val = (Std$Object$t *)WriteMessage;
				return MESSAGE;
			};
			Total += Written;
			Read -= Written;
			Ptr += Written;
		};
	};
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("copy", TYP, ReaderT, TYP, WriterT, TYP, Std$Integer$SmallT) {
//@rd
//@wr
//@count
//:Std$Integer$SmallT
// Copies <var>count</var> bytes from <var>rd</var> to <var>wr</var>.
	IO$Stream$t *Rd = Args[0].Val;
	IO$Stream$t *Wr = Args[1].Val;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Rd->Type);
	IO$Stream$writefn write = Util$TypedFunction$get(_t_write, Wr->Type);
	int Rem = Std$Integer$get_small(Args[2].Val);
	int Total = 0;
	char Buffer[1024];
	while (Rem > 1024) {
		int Read = read(Rd, Buffer, 1024, 0);
		if (Read == -1) {
			Result->Val = (Std$Object$t *)ReadMessage;
			return MESSAGE;
		};
		Rem -= Read;
		char *Ptr = Buffer;
		while (Read) {
			int Written = write(Wr, Ptr, Read, 1);
			if (Written == -1) {
				Result->Val = (Std$Object$t *)WriteMessage;
				return MESSAGE;
			};
			Total += Written;
			Read -= Written;
			Ptr += Written;
		};
	};
	char *Ptr = Buffer;
	int Rem2 = Rem;
	while (Rem) {
		int Read = read(Rd, Ptr, Rem, 0);
		if (Read == -1) {
			Result->Val = (Std$Object$t *)ReadMessage;
			return MESSAGE;
		};
		Rem -= Read;
		Ptr += Read;
	};
	Ptr = Buffer;
	while (Rem2) {
		int Written = write(Wr, Ptr, Rem2, 1);
		if (Written == -1) {
			Result->Val = (Std$Object$t *)WriteMessage;
			return MESSAGE;
		};
		Total += Written;
		Rem2 -= Written;
		Ptr += Written;
	};
	Result->Val = Std$Integer$new_small(Total);
	return SUCCESS;
};

METHOD("write", TYP, WriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
//@wr
//@buffer
//@length
// Writes up to <var>length</var> bytes to <var>wr</var> from <var>buffer</var>
// Returns the number of bytes written
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("write", TYP, TextWriterT, ANY) {
//@wr
//@value
//:TextWriterT
// Writes <code>value@Std.String.T</code> to <var>wr</var>.
	Std$Function$result Result0;
	switch (Std$Function$call(Std$String$Of, 1, &Result0, Args[1].Val, Args[1].Ref)) {
	case SUSPEND:
	case SUCCESS:
		return Std$Function$call($write, 2, Result, Args[0].Val, Args[0].Ref, Result0.Val, Result0.Ref);
	case FAILURE:
		Result->Val = (Std$Object$t *)ConvertMessage;
		return MESSAGE;
	case MESSAGE:
		Result->Arg = Result0.Arg;
		return MESSAGE;
	};
};

METHOD("writes", TYP, TextWriterT) {
//@wr
//  Writes each argument after the first to <var>wr</var>.
	for (int I = 1; I < Count; ++I) {
		Std$Function$status Status = Std$Function$call($write, 2, Result, Args[0].Val, Args[0].Ref, Args[I].Val, Args[I].Ref);
		if (Status >= FAILURE) return Status;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, WriterT, TYP, Std$String$T) {
//@wr
//@string
//:WriterT
// Writes <var>string</var> to <var>wr</var>.
	stream_t *Stream = Args[0].Val;
	IO$Stream$writefn write = Util$TypedFunction$get(_t_write, Stream->Type);
	const const Std$String$t *String = (Std$String$t *)Args[1].Val;
	for (const Std$String$block *Block = String->Blocks; Block->Length.Value; Block++) {
		int Length = Block->Length.Value;
		const char *Chars = Block->Chars.Value;
		while (Length) {
			int Written = write(Stream, Chars, Length, 1);
			if (Written == -1) {
				Result->Val = (Std$Object$t *)WriteMessage;
				return MESSAGE;
			};
			Chars += Written;
			Length -= Written;
		};
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("seek", TYP, SeekerT, TYP, Std$Integer$SmallT, TYP, SeekModeT) {
//@t
//@position
// Seeks to the <var>position</var><sup>th</sup> byte in <var>t</var>
	Result->Val = (Std$Object$t *)GenericMessage;
	return MESSAGE;
};

METHOD("tell", TYP, T) {
//@t
//:Std$Integer$SmallT
// Returns the current position in <var>t</var>
	stream_t *Stream = Args[0].Val;
	int Seek = _t_seek(Stream, Std$Integer$get_small(Args[1].Val), IO$Stream$SEEK_CUR);
	if (Seek == -1) {
		Result->Val = (Std$Object$t *)SeekMessage;
		return MESSAGE;
	};
	Result->Val = Std$Integer$new_small(Seek);
	return SUCCESS;
};

#ifdef LINUX

#include <pthread.h>

typedef struct pair_t {
	IO$Stream$t *Rd, *Wr;
} pair_t;

static void *_link_thread_func(pair_t * restrict Pair) {
	char Buffer[256];
	IO$Stream$t *Rd = Pair->Rd;
	IO$Stream$t *Wr = Pair->Wr;
	IO$Stream$readfn read = Util$TypedFunction$get(_t_read, Rd->Type);
	IO$Stream$writefn write = Util$TypedFunction$get(_t_write, Wr->Type);
	Pair = 0;
	for (;;) {
		int BytesRead = read(Rd, Buffer, 256, 0);
		if (BytesRead == -1) {
			_t_close(Wr, 1);
			return 0;
		};
		char *Tmp = Buffer;
		while (BytesRead) {
			int BytesWritten = write(Wr, Tmp, BytesRead, 1);
			if (BytesWritten < 0) return 0;
			Tmp += BytesWritten;
			BytesRead -= BytesWritten;
		};
	};
};

METHOD("link", TYP, T, TYP, T) {
//@rd
//@wr
// Links <var>rd</var> and <var>wr</var> so that any input available on <var>rd</var> is written to <var>wr</var>.
// The default implementation creates a new thread and so <var>rd</var> should block until input is available.
	pthread_t Thread;
	pair_t *Pair = new(pair_t);
	Pair->Rd = Args[0].Val;
	Pair->Wr = Args[1].Val;
	pthread_create(&Thread, 0, (void *)_link_thread_func, Pair);
	return SUCCESS;
};

#endif
