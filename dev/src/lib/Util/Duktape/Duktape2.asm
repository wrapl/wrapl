%include "Std.inc"

extern duktape_invoke

ctype ObjectT
.invoke:
	;int3
	push byte 0
	push byte 0
	push dword Std$Object$Nil
	mov ebx, esp
	and esp, byte 0xF0
	push ebx
	push edi
	push esi
	push ecx
	call duktape_invoke
	add esp, byte 12
	pop esp
	pop ecx
	pop edx
	pop ebx
	ret