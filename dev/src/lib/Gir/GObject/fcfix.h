#include <glob.h>

extern void *fcfix_malloc(size_t size);
extern void fcfix_free(void *ptr);
extern void *fcfix_calloc(size_t nmemb, size_t size);
extern void *fcfix_realloc(void *ptr, size_t size);