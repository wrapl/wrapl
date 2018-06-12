section .text

global log_nolog
log_nolog:
	ret

global log_writef
extern __log_Writef
log_writef:
	jmp [__log_Writef]

global log_writes
extern __log_Writes
log_writes:
	jmp [__log_Writes]

global log_writen
extern __log_Writen
log_writen:
	jmp [__log_Writen]

global log_errorf
extern __log_Errorf
log_errorf:
	jmp [__log_Errorf]

global log_errors
extern __log_Errors
log_errors:
	jmp [__log_Errors]

global log_errorn
extern __log_Errorn
log_errorn:
	jmp [__log_Errorn]
