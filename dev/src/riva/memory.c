#include "libriva.h"
#include <stdio.h>
#include <stdarg.h>
#include "gc/gc_typed.h"

//#define SUPER_GC_DEBUG

#ifdef LINUX
#include <malloc.h>
#endif

static void *memory_calloc(size_t Count, size_t Size) {
#ifdef SUPER_GC_DEBUG
	return GC_debug_malloc(Count * Size, "Memory._calloc", __builtin_return_address(0));
#else
	return GC_MALLOC(Count *Size);
#endif
};

#ifdef SUPER_GC_DEBUG

static void *debug_strdup(const char *String) {
	return GC_debug_strdup(String, "Memory._strdup", __builtin_return_address(0));
};

static void *debug_alloc(size_t Size) {
	return GC_debug_malloc(Size, "Memory._alloc", __builtin_return_address(0));
};

static void *debug_alloc_atomic(size_t Size) {
	return GC_debug_malloc_atomic(Size, "Memory._alloc_atomic", __builtin_return_address(0));
};

static void *debug_alloc_uncollectable(size_t Size) {
	return GC_debug_malloc_uncollectable(Size, "Memory._alloc_uncollectable", __builtin_return_address(0));
};

static void *debug_realloc(void *Ptr, size_t Size) {
	return GC_debug_realloc(Ptr, Size, "Memory._realloc", __builtin_return_address(0));
};

#endif

#ifdef LINUX

static FILE *MemLog = 0;

static void *debug_alloc(size_t Size) {
	void *Result = GC_malloc(Size);
	debug_hdr *Hdr = debug_get_hdr(__builtin_return_address(0));
	if (Hdr) {
		fprintf(MemLog, "[%x] %s:%d alloc %d bytes\n", Result, Hdr->StrInfo, Hdr->IntInfo, Size);
	} else {
		fprintf(MemLog, "[%x] 0x%x alloc %d bytes\n", Result, __builtin_return_address(0), Size);
	};
	return Result;
};

static void *debug_alloc_atomic(size_t Size) {
	void *Result = GC_malloc_atomic(Size);
	debug_hdr *Hdr = debug_get_hdr(__builtin_return_address(0));
	if (Hdr) {
		fprintf(MemLog, "[%x] %s:%d alloc_atomic %d bytes\n", Result, Hdr->StrInfo, Hdr->IntInfo, Size);
	} else {
		fprintf(MemLog, "[%x] 0x%x alloc_atomic %d bytes\n", Result, __builtin_return_address(0), Size);
	};
	return Result;
};

static void *debug_alloc_uncollectable(size_t Size) {
	void *Result = GC_malloc_uncollectable(Size);
	debug_hdr *Hdr = debug_get_hdr(__builtin_return_address(0));
	if (Hdr) {
		fprintf(MemLog, "[%x] %s:%d alloc_uncollectable %d bytes\n", Result, Hdr->StrInfo, Hdr->IntInfo, Size);
	} else {
		fprintf(MemLog, "[%x] 0x%x alloc_uncollectable %d bytes\n", Result, __builtin_return_address(0), Size);
	};
	return Result;
};

static void *debug_realloc(void *Ptr, size_t Size) {
	void *Result = GC_realloc(Ptr, Size);
	debug_hdr *Hdr = debug_get_hdr(__builtin_return_address(0));
	if (Hdr) {
		fprintf(MemLog, "[%x] %s:%d realloc %x -> %d bytes\n", Result, Hdr->StrInfo, Hdr->IntInfo, Ptr, Size);
	} else {
		fprintf(MemLog, "[%x] 0x%x realloc %x -> %d bytes\n", Result, __builtin_return_address(0), Ptr, Size);
	};
	return Result;
};

static void *debug_malloc_hook(size_t Size, void *Caller) {
	void *Result = GC_malloc_uncollectable(Size);
	fprintf(MemLog, "[%x] 0x%x malloc_hook %d bytes\n", Result, Caller, Size);
	return Result;
};

static void *debug_realloc_hook(void *Ptr, size_t Size, void *Caller) {
	void *Result = GC_realloc(Ptr, Size);
	debug_hdr *Hdr = debug_get_hdr(Caller);
	fprintf(MemLog, "[%x] 0x%x realloc_hook %x -> %d bytes\n", Result, Caller, Ptr, Size);
	return Result;
};

static void memory_log_writes(const char *String) {
	fputs(String, MemLog);
};

static void memory_log_writen(const char *String, unsigned long Length) {
	fwrite(String, 1, Length, MemLog);
};

static void memory_log_writef(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	vfprintf(MemLog, Format, Args);
	va_end(Args);
};

void memory_log_enable(void) {
	MemLog = fopen("memory.log", "w");
	module_t *Module = module_load(0, "Riva/Memory");
	module_export(Module, "_alloc", 0, debug_alloc);
	module_export(Module, "_alloc_atomic", 0, debug_alloc_atomic);
	module_export(Module, "_alloc_uncollectable", 0, debug_alloc_uncollectable);
	module_export(Module, "_realloc", 0, debug_realloc);
#ifdef LINUX
	__malloc_hook = (void *)debug_malloc_hook;
	__realloc_hook = (void *)debug_realloc_hook;
#endif
	__log_Writes = memory_log_writes;
	__log_Writen = memory_log_writen;
	__log_Writef = memory_log_writef;
};

void memory_log_close(void) {
	if (MemLog) fclose(MemLog);
};

#endif

static GC_descr CodeDescr;

void *memory_alloc_code(size_t Size) {
	const char **Result = GC_MALLOC_EXPLICITLY_TYPED(Size + 8, CodeDescr);
#ifdef LINUX
	Result[0] = DebugKey;
#endif
	return Result + 1;
};

void memory_init(void) {
	GC_word CodeBitmap[1] = {2};
	CodeDescr = GC_make_descriptor(CodeBitmap, 2);

	module_t *Module = module_new("Riva/Memory");
	module_add_alias(Module, "library:/Riva/Memory");
	module_export(Module, "_collect", 0, GC_gcollect);
#ifdef SUPER_GC_DEBUG
	module_export(Module, "_strdup", 0, debug_strdup);
	module_export(Module, "_alloc", 0, debug_alloc);
	module_export(Module, "_alloc_atomic", 0, debug_alloc_atomic);
	module_export(Module, "_alloc_uncollectable", 0, debug_alloc_uncollectable);
	module_export(Module, "_realloc", 0, debug_realloc);
	module_export(Module, "_free", 0, GC_debug_free);
#else
	module_export(Module, "_strdup", 0, GC_strdup);
	module_export(Module, "_alloc", 0, GC_malloc);
	module_export(Module, "_alloc_atomic", 0, GC_malloc_atomic);
	module_export(Module, "_alloc_uncollectable", 0, GC_malloc_uncollectable);
	module_export(Module, "_alloc_atomic_uncollectable", 0, GC_malloc_atomic_uncollectable);
	module_export(Module, "_alloc_large", 0, GC_malloc_ignore_off_page);
	module_export(Module, "_alloc_atomic_large", 0, GC_malloc_atomic_ignore_off_page);
	module_export(Module, "_realloc", 0, GC_realloc);
	module_export(Module, "_free", 0, GC_free);
#endif
	module_export(Module, "_alloc_code", 0, memory_alloc_code);
	module_export(Module, "_alloc_aligned", 0, GC_memalign);
	module_export(Module, "_calloc", 0, memory_calloc);
	module_export(Module, "_base", 0, GC_base);
	module_export(Module, "_register_finalizer", 0, GC_register_finalizer);
	module_export(Module, "_register_finalizer_ignore_self", 0, GC_register_finalizer_ignore_self);
	module_export(Module, "_register_disappearing_link", 0, GC_general_register_disappearing_link);
	module_export(Module, "_is_visible", 0, GC_is_visible);
	module_export(Module, "_gc_disabled", 0, GC_is_disabled);
	module_export(Module, "_size", 0, GC_size);
	module_export(Module, "_call_with_alloc_lock", 0, GC_call_with_alloc_lock);
	
	module_export(Module, "_alloc_stubborn", 0, GC_malloc_stubborn);
	module_export(Module, "_freeze_stubborn", 0, GC_end_stubborn_change);
	module_export(Module, "_change_stubborn", 0, GC_change_stubborn);
};
