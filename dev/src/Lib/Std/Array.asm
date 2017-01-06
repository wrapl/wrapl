%include "Std.inc"
%include "Riva/Memory.inc"

ctype T
; A fixed length array of objects
.invoke: equ 0

function Alloc, 1
; creates a new array with length elements, all initialized to <id>NIL</id>
;@length : Std$Integer$SmallT
;:T
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(Std$Array_t)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [Std$Array_t(eax).Values]
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Array_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$Array_t(eax).Length).Value], ecx
	push eax
	mov edi, [Std$Array_t(eax).Values]
	mov eax, Std$Object$Nil
	rep stosd
.empty:
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

function New, 0
; creates a new array with <var>values</var>
;@values... : Std$Object$T
;:T
	lea eax, [4 * esi + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(Std$Array_t)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [Std$Array_t(eax).Values]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Array_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$Array_t(eax).Length).Value], esi
	push eax
	mov ecx, [Std$Array_t(eax).Values]
	cmp esi, byte 0
	je .empty
.loop:
	mov edx, [Std$Function_argument(edi).Val]
	mov [ecx], edx
	add edi, byte sizeof(Std$Function_argument)
	add ecx, byte 4
	dec esi
	jnz .loop
.empty:
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

cfunction _new
	mov eax, [esp + 4]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push edi
	mov edi, eax
	mov ecx, [esp + 8]
	mov eax, Std$Object$Nil
	rep stosd
	pop edi
	push byte sizeof(Std$Array_t)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Array_t(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Array_t(eax).Values]
	pop dword [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	ret

%ifdef DOCUMENTING

%define Std$Array$T T

%define array_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
