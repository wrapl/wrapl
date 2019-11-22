#include <Std.h>
#include <IO/Stream.h>
#include <IO/Native.h>
#include <Riva/Memory.h>
#include <Sys/Time.h>
#include <Util/Event/Buffer.h>
#include <Util/TypedFunction.h>
#include <event2/buffer.h>

typedef Util$Event$Buffer$t buffer_t;

TYPE(T, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

GLOBAL_FUNCTION(New, 0) {
	buffer_t *Buffer = new(buffer_t);
	Buffer->Type = T;
	Buffer->Handle = evbuffer_new();
	RETURN(Buffer);
}

TYPED_INSTANCE(int, IO$Stream$read, T, buffer_t *Buffer, char *Data, int Count) {
	return evbuffer_remove(Buffer->Handle, Data, Count);
}

TYPED_INSTANCE(char, IO$Stream$readc, T, buffer_t *Buffer) {
	char Char;
	if (evbuffer_remove(Buffer->Handle, &Char, 1) == 1) return Char;
	return -1;
}

TYPED_INSTANCE(char *, IO$Stream$readl, T, buffer_t *Buffer) {
	return evbuffer_readln(Buffer->Handle, NULL, EVBUFFER_EOL_LF);
}

TYPED_INSTANCE(int, IO$Stream$write, T, buffer_t *Buffer, const char *Data, int Count) {
	return evbuffer_add(Buffer->Handle, Data, Count);
}

TYPED_INSTANCE(int, IO$Stream$writec, T, buffer_t *Buffer, char Char) {
	if (!evbuffer_add(Buffer->Handle, &Char, 1)) return 1;
	return -1;
}

TYPED_INSTANCE(int, IO$Stream$writes, T, buffer_t *Buffer, const char *String) {
	return evbuffer_add(Buffer->Handle, String, strlen(String));
}

TYPED_INSTANCE(int, IO$Stream$writef, T, buffer_t *Buffer, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	int Written = evbuffer_add_vprintf(Buffer->Handle, Format, Args);
	va_end(Args);
	return Written;
}

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffer_t *Buffer = (buffer_t *)Args[0].Val;
	const void *Data = Std$Address$get_value(Args[1].Val);
	size_t Length = Std$Integer$get_small(Args[2].Val);
	size_t Read = evbuffer_remove(Buffer->Handle, Data, Length);
	if (Read == -1) SEND(Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__));
	RETURN(Std$Integer$new_small(Read));
}

METHOD("read", TYP, T) {
	buffer_t *Buffer = (buffer_t *)Args[0].Val;
	size_t Length;
	char *Chars = evbuffer_readln(Buffer->Handle, &Length, EVBUFFER_EOL_LF);
	if (!Chars) SEND(Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__));
	RETURN(Std$String$new_length(Chars, Length));
}

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffer_t *Buffer = (buffer_t *)Args[0].Val;
	const void *Data = Std$Address$get_value(Args[1].Val);
	size_t Length = Std$Integer$get_small(Args[2].Val);
	evbuffer_add(Buffer->Handle, Data, Length);
	RETURN(Args[2].Val);
}

METHOD("copy", TYP, T, TYP, T) {
	buffer_t *Source = (buffer_t *)Args[0].Val;
	buffer_t *Dest = (buffer_t *)Args[1].Val;
	size_t Length = evbuffer_get_length(Source);
	evbuffer_add_buffer(Dest->Handle, Source->Handle);
	RETURN(Std$Integer$new_small(Length));
}

METHOD("copy", TYP, IO$Native$(T), TYP, T) {
	IO$Native$(t) *Source = (IO$Native$(t) *)Args[0].Val;
	buffer_t *Dest = (buffer_t *)Args[1].Val;
	size_t Old = evbuffer_get_length(Dest);
	evbuffer_add_file(Dest->Handle, Source->Handle, 0, -1);
	size_t New = evbuffer_get_length(Dest);
	RETURN(Std$Integer$new_small(New - Old));
}

METHOD("copy", TYP, IO$Native$(T), TYP, T, TYP, Std$Integer$SmallT) {
	IO$Native$(t) *Source = (IO$Native$(t) *)Args[0].Val;
	buffer_t *Dest = (buffer_t *)Args[1].Val;
	size_t Length = Std$Integer$get_small(Args[2].Val);
	int Read = evbuffer_read(Dest->Handle, Source->Handle, Length);
	if (Read == -1) SEND(Sys$Program$error_new_format(IO$Stream$ReadMessageT, "%s:%d", __FILE__, __LINE__));
	RETURN(Std$Integer$new_small(Read));
}

METHOD("copy", TYP, T, TYP, IO$Native$(T)) {
	buffer_t *Source = (buffer_t *)Args[0].Val;
	IO$Native$(t) *Dest = (IO$Native$(t) *)Args[1].Val;
	int Written = evbuffer_write(Source->Handle, Dest->Handle);
	if (Written == -1) SEND(Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__));
	RETURN(Std$Integer$new_small(Written));
}

METHOD("copy", TYP, T, TYP, IO$Native$(T), TYP, Std$Integer$SmallT) {
	buffer_t *Source = (buffer_t *)Args[0].Val;
	IO$Native$(t) *Dest = (IO$Native$(t) *)Args[1].Val;
	size_t Length = Std$Integer$get_small(Args[2].Val);
	int Written = evbuffer_write_atmost(Source->Handle, Dest->Handle, Length);
	if (Written == -1) SEND(Sys$Program$error_new_format(IO$Stream$WriteMessageT, "%s:%d", __FILE__, __LINE__));
	RETURN(Std$Integer$new_small(Written));
}

METHOD("length", TYP, T) {
	buffer_t *Buffer = (buffer_t *)Args[0].Val;
	RETURN(Std$Integer$new_small(evbuffer_get_length(Buffer->Handle)));
}

METHOD("prepend", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	buffer_t *Buffer = (buffer_t *)Args[0].Val;
	const void *Data = Std$Address$get_value(Args[1].Val);
	size_t Length = Std$Integer$get_small(Args[2].Val);
	evbuffer_prepend(Buffer->Handle, Data, Length);
	RETURN(Args[2].Val);
}

METHOD("prepend", TYP, T, TYP, T) {
	buffer_t *Source = (buffer_t *)Args[0].Val;
	buffer_t *Dest = (buffer_t *)Args[1].Val;
	size_t Length = evbuffer_get_length(Source);
	evbuffer_prepend_buffer(Dest->Handle, Source->Handle);
	RETURN(Std$Integer$new_small(Length));
}

