#if defined(WINDOWS) && !defined(CYGWIN)
    #include <IO/Windows.h>
    #define IO$Native$(x) IO$Windows$ ## x
#else
    #include <IO/Posix.h>
    #define IO$Native$(x) IO$Posix$ ## x
#endif
