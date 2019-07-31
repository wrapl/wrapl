#if defined(WINDOWS) && !defined(CYGWIN)
    #include <IO/Windows.h>
    #define NATIVE(x) IO$Windows ## x
#else
    #include <IO/Posix.h>
    #define NATIVE(x) IO$Posix$ ## x
#endif
