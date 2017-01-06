#include <Std.h>
#include <IO/Terminal.h>
#include <Riva/Memory.h>

#if defined(WINDOWS) && !defined(CYGWIN)
    #include <IO/Windows.h>
    #define NATIVE(x) IO$Windows ## x
#else
    #include <IO/Posix.h>
    #define NATIVE(x) IO$Posix ## x
#endif

TYPE(T, NATIVE($T), IO$Stream$T);

TYPE(ReaderT, T, NATIVE($TextReaderT), NATIVE($ReaderT), NATIVE($T), IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);
TYPE(WriterT, T, NATIVE($TextWriterT), NATIVE($WriterT), NATIVE($T), IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$T);

#if defined(WINDOWS) && !defined(CYGWIN)

NATIVE(_t) In[] = {{ReaderT, 0}};
NATIVE(_t) Out[] = {{WriterT, 0}};
NATIVE(_t) Err[] = {{WriterT, 0}};

INITIAL() {
    In->Handle = GetStdHandle(STD_INPUT_HANDLE);
    Out->Handle = GetStdHandle(STD_OUTPUT_HANDLE);
    Err->Handle = GetStdHandle(STD_ERROR_HANDLE);
};

#else

#include <unistd.h>

GLOBAL(ReaderT, NATIVE(_t), In)[] = {{ReaderT, STDIN_FILENO}};
// Reads from the standard input channel

GLOBAL(WriterT, NATIVE(_t), Out)[] = {{WriterT, STDOUT_FILENO}};
// Writes to the standard output channel

GLOBAL(WriterT, NATIVE(_t), Err)[] = {{WriterT, STDERR_FILENO}};
// Writes to the standard error channel

#endif
