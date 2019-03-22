#ifndef RIVA_MEMORY_H
#define RIVA_MEMORY_H

#define RIVA_MODULE Riva$Memory
#include <Riva-Header.h>

RIVA_CFUN(void, collect, void);

#ifdef __cplusplus
#undef malloc
#endif

RIVA_CFUN(const char *, strdup, const char *);
RIVA_CFUN(char *, alloc, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_uncollectable, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_large, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_atomic, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_atomic_uncollectable, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_atomic_large, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_code, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, alloc_aligned, unsigned int, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, calloc, unsigned int, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(char *, realloc, void *, unsigned int);
RIVA_CFUN(void, free, char *);
RIVA_CFUN(unsigned int, size, char *);

RIVA_CFUN(char *, alloc_stubborn, unsigned int) __attribute__ ((malloc));
RIVA_CFUN(void, freeze_stubborn, char *);
RIVA_CFUN(void, change_stubborn, char *);

#ifndef __cplusplus
#ifndef new
#define new(T) ((T *)Riva$Memory$alloc(sizeof(T)))
#endif
#else

#include <new>

inline void* operator new(std::size_t Size) throw () {return Riva$Memory$alloc(Size);}
inline void* operator new[](std::size_t Size) throw () {return Riva$Memory$alloc(Size);}
inline void* operator new(std::size_t Size, const std::nothrow_t&) throw() {return Riva$Memory$alloc(Size);}
inline void* operator new[](std::size_t Size, const std::nothrow_t&) throw() {return Riva$Memory$alloc(Size);}

inline void operator delete(void*) throw() {}
inline void operator delete[](void*) throw() {}
inline void operator delete(void*, std::size_t) throw() {}
inline void operator delete[](void*, std::size_t) throw() {}
inline void operator delete(void*, const std::nothrow_t&) throw() {}
inline void operator delete[](void*, const std::nothrow_t&) throw() {}

#endif

RIVA_CFUN(char *, base, char *);

RIVA_CFUN(char *, is_visible, char *);
RIVA_CFUN(int, gc_disabled);

typedef void (*Riva$Memory_finalizer)(char *, char *);

RIVA_CFUN(void, register_finalizer, char *, Riva$Memory_finalizer, char *, Riva$Memory_finalizer *, char **);
RIVA_CFUN(void, register_finalizer_ignore_self, char *, Riva$Memory_finalizer, char *, Riva$Memory_finalizer *, char **);
RIVA_CFUN(void, register_disappearing_link, char **, char *);

typedef void (*Riva$Memory$function)(void *);

RIVA_CFUN(void *, call_with_alloc_lock, Riva$Memory$function, void *);

#undef RIVA_MODULE

#endif
