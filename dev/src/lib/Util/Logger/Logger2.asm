%include "Std.inc"

struct callback, Std$Object$t
	.Level: resd 1
	.Method: resd 1
endstruct

extern LogLevel

ctype CallbackT
.invoke:
; INPUT
; ecx = invoked function
; esi = number of arguments
; edi = pointer to array of arguments
; OUTPUT
; ecx = Std$Object$Nil
; edx = 0
; eax = 0
	mov eax, [callback(ecx).Level]
	cmp eax, [LogLevel]
	jb .disabled
	mov ecx, [callback(ecx).Method]
	mov eax, [Std$Object$t(ecx).Type]
	call [Std$Type$t(eax).invoke]
.disabled:
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

