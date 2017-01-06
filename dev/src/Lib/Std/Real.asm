%include "Std.inc"
%include "Riva/Memory.inc"

ctype T, Std$Number$T
.invoke: equ 0

cglobal Zero, T
	dd T
	dq 0.0

cglobal One, T
	dd T
	dq 1.0

cfunction _alloc
	push byte sizeof(Std$Real_t)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	ret

extern atof
cfunction _new_string ;(char *Digits)
	push byte sizeof(Std$Real_t)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	push eax
	push dword [esp + 8]
	call atof
	add esp, byte 4
	pop eax
	fstp qword [Std$Real_t(eax).Value]
	ret

cfunction _new ;(double  Value)
	push byte sizeof(Std$Real_t)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	fld qword [esp + 4]
	fstp qword [Std$Real_t(eax).Value]
	ret

_function Compare
;@a:T
;@b:T
;:Std$Object$T
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi + 8).Val]
	fcomp qword [Std$Real_t(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret
.equal:
	mov ecx, Std$Object$Equal
	xor edx, edx
	xor eax, eax
	ret
.less:
	mov ecx, Std$Object$Less
	xor edx, edx
	xor eax, eax
	ret

_function Hash
;@a:T
;:Std$Integer$SmallT
	call Std$Integer$_alloc_small
	mov ecx, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(ecx).Value]
	fstp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

%ifdef DOCUMENTING

%define Std$Real$T T

%define real_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
