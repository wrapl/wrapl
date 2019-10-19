#include <Std.h>
#include <IO/Terminal.h>
#include <IO/Native.h>
#include <Riva/Memory.h>

TYPE(T, IO$Native$(T), IO$Stream$T);

TYPE(ReaderT, T, IO$Native$(TextReaderT), IO$Native$(ReaderT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, IO$Native$(TextWriterT), IO$Native$(WriterT), IO$Native$(T), IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

#if defined(WINDOWS) && !defined(CYGWIN)

NATIVE(t) In[] = {{ReaderT, 0}};
NATIVE(t) Out[] = {{WriterT, 0}};
NATIVE(t) Err[] = {{WriterT, 0}};

INITIAL() {
    In->Handle = GetStdHandle(STD_INPUT_HANDLE);
    Out->Handle = GetStdHandle(STD_OUTPUT_HANDLE);
    Err->Handle = GetStdHandle(STD_ERROR_HANDLE);
};

#else

#include <unistd.h>

GLOBAL(ReaderT, IO$Native$(t), In)[] = {{ReaderT, STDIN_FILENO}};
// Reads from the standard input channel

GLOBAL(WriterT, IO$Native$(t), Out)[] = {{WriterT, STDOUT_FILENO}};
// Writes to the standard output channel

GLOBAL(WriterT, IO$Native$(t), Err)[] = {{WriterT, STDERR_FILENO}};
// Writes to the standard error channel

#endif
