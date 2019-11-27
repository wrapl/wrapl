#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>

#include <string.h>

static Std$Integer$smallt ZERO[1] = {{Std$Integer$SmallT, 0}};

typedef struct special_t {
	const Std$Type$t *Type;
} special_t;	

TYPE(EmptyT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(EmptyT, special_t, Empty)[1] = {{EmptyT}};

TYPED_INSTANCE(void, IO$Stream$close, EmptyT, IO$Stream$t *Stream, int Mode) {
};

TYPED_INSTANCE(int, IO$Stream$eoi, EmptyT, IO$Stream$t *Stream) {
	return 1;
};

TYPED_INSTANCE(int, IO$Stream$read, EmptyT, IO$Stream$t *Stream, char *Buffer, int Count, int Block) {
	return 0;
};

TYPED_INSTANCE(void, IO$Stream$flush, EmptyT, IO$Stream$t *Stream) {
};

TYPED_INSTANCE(int, IO$Stream$write, EmptyT, IO$Stream$t *Stream, char *Buffer, int Count, int Block) {
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$seek, EmptyT, IO$Stream$t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, EmptyT) {
	Result->Val = Sys$Program$error_new_format(IO$Stream$CloseMessageT, "%s:%d", __FILE__, __LINE__);
	return MESSAGE;
};

METHOD("eoi", TYP, EmptyT) {
	return SUCCESS;
};

METHOD("read", TYP, EmptyT, TYP, Std$Address$T) {
	return FAILURE;
};

METHOD("flush", TYP, EmptyT) {
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, EmptyT, TYP, Std$Address$T) {
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("seek", TYP, EmptyT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = (Std$Object$t *)ZERO;
	return SUCCESS;
};

TYPE(FullT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(FullT, special_t, Full)[1] = {{FullT}};

TYPED_INSTANCE(void, IO$Stream$close, FullT, IO$Stream$t *Stream, int Mode) {
};

TYPED_INSTANCE(int, IO$Stream$eoi, FullT, IO$Stream$t *Stream) {
	return 0;
};

TYPED_INSTANCE(int, IO$Stream$read, FullT, IO$Stream$t *Stream, char *Buffer, int Count, int Block) {
	memset(Buffer, 0, Count);
	return Count;
};

TYPED_INSTANCE(void, IO$Stream$flush, FullT, IO$Stream$t *Stream) {
};

TYPED_INSTANCE(int, IO$Stream$write, FullT, IO$Stream$t *Stream, char *Buffer, int Count, int Block) {
	return -1;
};

TYPED_INSTANCE(int, IO$Stream$seek, FullT, IO$Stream$t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, FullT) {
	Result->Val = Sys$Program$error_new_format(IO$Stream$CloseMessageT, "%s:%d", __FILE__, __LINE__);
	return MESSAGE;
};

METHOD("eoi", TYP, FullT) {
	return FAILURE;
};

METHOD("read", TYP, FullT, TYP, Std$Address$T) {
	memset(Std$Address$get_value(Args[1].Val), 0, Std$Address$get_length(Args[1].Val));
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("flush", TYP, FullT) {
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, FullT, TYP, Std$Address$T) {
	Result->Val = Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__);
	return MESSAGE;
};

METHOD("seek", TYP, FullT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = (Std$Object$t *)ZERO;
	return SUCCESS;
};

#ifdef LINUX
#include <IO/Posix.h>
#include <stdio.h>

TYPE(RandomT, IO$Posix$ReaderT, IO$Posix$SeekerT, IO$Posix$T, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);

CONSTANT(Random, IO$Stream$T) {
	return IO$Posix$new(RandomT, open("/dev/random", O_RDONLY, 0644));
};

CONSTANT(URandom, IO$Stream$T) {
	return IO$Posix$new(RandomT, open("/dev/urandom", O_RDONLY, 0644));
};
#endif

