%include "Std.inc"
%include "Riva/Memory.inc"

%macro check_type 1
	mov eax, [Std$Object$t(ecx).Type]
	mov eax, [Std$Type(eax).Types]
%%loop:
	cmp [eax], dword 0
	je _invalid_argument
	cmp [eax], dword %1
	jne %%loop
%endmacro

cfunction _push_boolean
	mov ecx, [edi + 8 * ebx]
	mov eax, 0
	cmp ecx, Std$Symbol$false
	je .false
	cmp ecx, Std$Symbol$true
	jne _invalid_argument
	mov eax, 1
.false:
	push eax
	ret

cfunction _push_integer
	mov ecx, [edi + 8 * ebx]
	check_type Std$Integer$SmallT
	push dword [Std$Integer$smallt(ecx).Value]
	ret

cfunction _push_real
	mov ecx, [edi + 8 * ebx]
	check_type Std$Real$T
	fld qword [Std$Real$t(ecx).Value]
	sub esp, byte 8
	fstp qword [esp]
	ret

cfunction _push_string
	mov ecx, [edi + 8 * ebx]
	check_type Std$String$T
	push ecx
	call Std$String$_flatten
	mov [esp], eax
	ret



cfunction _invalid_argument
	ret
	