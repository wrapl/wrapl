bits 32

%macro wrap 2
extern %2
global %1
%1:
	jmp %2
%endmacro

section .text

wrap dl_open, GC_dlopen
wrap pthread_cancel, GC_pthread_cancel
wrap pthread_create, GC_pthread_create
wrap pthread_detach, GC_pthread_detach
wrap pthread_exit, GC_pthread_exit
wrap pthread_join, GC_pthread_join
wrap pthread_sigmask, GC_pthread_sigmask
wrap fcfix_free, GC_free
wrap fcfix_realloc, GC_realloc

extern GC_malloc_atomic_uncollectable

global fcfix_malloc
fcfix_malloc:
	jmp GC_malloc_atomic_uncollectable

global fcfix_calloc
fcfix_calloc:
	mov eax, [esp + 4]
	imul eax, [esp + 8]
	mov [esp + 4], eax
	jmp GC_malloc_atomic_uncollectable

global fcfix_strdup
fcfix_strdup:
	push esi
	push edi
	mov edi, [esp + 12]
	mov esi, edi
	xor eax, eax
	xor ecx, ecx
	not ecx
	repne scasb
	not ecx
	push ecx
	call GC_malloc_atomic_uncollectable
	mov [esp], eax
	mov edi, eax
	repnz movsb
	pop eax
	pop edi
	pop esi
	ret


