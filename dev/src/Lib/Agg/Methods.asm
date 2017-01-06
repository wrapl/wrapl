%include "Std.inc"
%include "Riva/Memory.inc"

symbol ?INDEX, "[]"
symbol ?EQUAL, "="

method "[]=", ANY, TYP, Std$Integer$SmallT, ANY
	push edi
	mov esi, 2
	mov ecx, ?INDEX
	call Std$Symbol$T.invoke
	pop edi
	jmp [.result + 4 * eax]
.failure:
.message:
	ret
.success:
.suspend:
	push edx
	push ecx
	push dword [Std$Function_argument(edi + 16).Ref]
	push dword [Std$Function_argument(edi + 16).Val]
	mov edi, esp
	mov esi, 2
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	add esp, byte 16
	ret
datasect
	dd .suspend
.result:
	dd .success
	dd .failure
	dd .message

method "[]=", ANY, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT, ANY
	push edi
	mov esi, 3
	mov ecx, ?INDEX
	call Std$Symbol$T.invoke
	pop edi
	jmp [.result + 4 * eax]
.failure:
.message:
	ret
.success:
.suspend:
	push edx
	push ecx
	push dword [Std$Function_argument(edi + 24).Ref]
	push dword [Std$Function_argument(edi + 24).Val]
	mov edi, esp
	mov esi, 2
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	add esp, byte 16
	ret
datasect
	dd .suspend
.result:
	dd .success
	dd .failure
	dd .message
