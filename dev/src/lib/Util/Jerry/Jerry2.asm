%include "Std.inc"

extern jerry_invoke

ctype ValueT
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
	call jerry_invoke
	add esp, byte 12
	pop esp
	pop ecx
	pop edx
	pop ebx
	ret