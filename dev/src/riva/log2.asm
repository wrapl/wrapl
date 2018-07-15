section .text

global log_nolog
log_nolog:
	ret

global log_writef
extern __log_Writef
log_writef:
%ifdef X86
	jmp [__log_Writef]
%elifdef X64
	mov rax, __log_Writef
	jmp [rax]
%endif

global log_writes
extern __log_Writes
log_writes:
%ifdef X86
	jmp [__log_Writes]
%elifdef X64
	mov rax, __log_Writes
	jmp [rax]
%endif

global log_writen
extern __log_Writen
log_writen:
%ifdef X86
	jmp [__log_Writen]
%elifdef X64
	mov rax, __log_Writen
	jmp [rax]
%endif

global log_errorf
extern __log_Errorf
log_errorf:
%ifdef X86
	jmp [__log_Errorf]
%elifdef X64
	mov rax, __log_Errorf
	jmp [rax]
%endif

global log_errors
extern __log_Errors
log_errors:
%ifdef X86
	jmp [__log_Errors]
%elifdef X64
	mov rax, __log_Errors
	jmp [rax]
%endif

global log_errorn
extern __log_Errorn
log_errorn:
%ifdef X86
	jmp [__log_Errorn]
%elifdef X64
	mov rax, __log_Errorn
	jmp [rax]
%endif
