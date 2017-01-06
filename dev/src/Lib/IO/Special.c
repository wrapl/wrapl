#include <Std.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>

#include <string.h>

static Std$Integer_smallt ZERO[1] = {{Std$Integer$SmallT, 0}};

typedef struct special_t {
	const Std$Type_t *Type;
} special_t;	

/*
TYPE(ZeroT, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(ZeroT, special_t, Zero)[1] = {{ZeroT}};

TYPED_INSTANCE(void, IO$Stream$close, ZeroT, IO$Stream_t *Stream, int Mode) {
};

TYPED_INSTANCE(int, IO$Stream$eoi, ZeroT, IO$Stream_t *Stream) {
	return 0;
};

TYPED_INSTANCE(int, IO$Stream$read, ZeroT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	memset(Buffer, 0, Count);
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$seek, ZeroT, IO$Stream_t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, ZeroT) {
	Result->Val = IO$Stream$CloseMessage;
	return MESSAGE;
};

METHOD("eoi", TYP, ZeroT) {
	return FAILURE;
};

METHOD("read", TYP, ZeroT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	memset(((Std$Address_t *)Args[1].Val)->Value, 0, ((Std$Integer_smallt *)Args[2].Val)->Value);
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("seek", TYP, ZeroT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = ZERO;
	return SUCCESS;
};

TYPE(NullT, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(NullT, special_t, Null)[1] = {{NullT}};

TYPED_INSTANCE(void, IO$Stream$close, NullT, IO$Stream_t *Stream, int Mode) {
};

TYPED_INSTANCE(void, IO$Stream$flush, NullT, IO$Stream_t *Stream) {
};

TYPED_INSTANCE(int, IO$Stream$write, NullT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$seek, NullT, IO$Stream_t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, NullT) {
	Result->Val = IO$Stream$CloseMessage;
	return MESSAGE;
};

METHOD("flush", TYP, NullT) {
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, NullT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("seek", TYP, NullT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = ZERO;
	return SUCCESS;
};
*/

TYPE(EmptyT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(EmptyT, special_t, Empty)[1] = {{EmptyT}};

TYPED_INSTANCE(void, IO$Stream$close, EmptyT, IO$Stream_t *Stream, int Mode) {
};

TYPED_INSTANCE(int, IO$Stream$eoi, EmptyT, IO$Stream_t *Stream) {
	return 1;
};

TYPED_INSTANCE(int, IO$Stream$read, EmptyT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	return 0;
};

TYPED_INSTANCE(void, IO$Stream$flush, EmptyT, IO$Stream_t *Stream) {
};

TYPED_INSTANCE(int, IO$Stream$write, EmptyT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	return Count;
};

TYPED_INSTANCE(int, IO$Stream$seek, EmptyT, IO$Stream_t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, EmptyT) {
	Result->Val = IO$Stream$CloseMessage;
	return MESSAGE;
};

METHOD("eoi", TYP, EmptyT) {
	return SUCCESS;
};

METHOD("read", TYP, EmptyT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	return FAILURE;
};

METHOD("flush", TYP, EmptyT) {
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, EmptyT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("seek", TYP, EmptyT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = (Std$Object_t *)ZERO;
	return SUCCESS;
};

TYPE(FullT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
GLOBAL(FullT, special_t, Full)[1] = {{FullT}};

TYPED_INSTANCE(void, IO$Stream$close, FullT, IO$Stream_t *Stream, int Mode) {
};

TYPED_INSTANCE(int, IO$Stream$eoi, FullT, IO$Stream_t *Stream) {
	return 0;
};

TYPED_INSTANCE(int, IO$Stream$read, FullT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	memset(Buffer, 0, Count);
	return Count;
};

TYPED_INSTANCE(void, IO$Stream$flush, FullT, IO$Stream_t *Stream) {
};

TYPED_INSTANCE(int, IO$Stream$write, FullT, IO$Stream_t *Stream, char *Buffer, int Count, int Block) {
	return -1;
};

TYPED_INSTANCE(int, IO$Stream$seek, FullT, IO$Stream_t *Stream, int Position, int Mode) {
	return 0;
};

METHOD("close", TYP, FullT) {
	Result->Val = IO$Stream$CloseMessage;
	return MESSAGE;
};

METHOD("eoi", TYP, FullT) {
	return FAILURE;
};

METHOD("read", TYP, FullT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	memset(((Std$Address_t *)Args[1].Val)->Value, 0, ((Std$Integer_smallt *)Args[2].Val)->Value);
	Result->Val = Args[2].Val;
	return SUCCESS;
};

METHOD("flush", TYP, FullT) {
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("write", TYP, FullT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	Result->Val = IO$Stream$WriteMessage;
	return MESSAGE;
};

METHOD("seek", TYP, FullT, TYP, Std$Integer$SmallT, TYP, IO$Stream$SeekModeT) {
	Result->Val = (Std$Object_t *)ZERO;
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

