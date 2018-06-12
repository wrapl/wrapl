%include "Std.inc"
%include "Riva/Memory.inc"

struct levels
	.Length:	resd 1
	.Levels:
endstruct

%define Std$Type$T T

ctype T
;  the type of all types
cfunction T.invoke
	push ecx
	mov eax, [Std$Type_t(ecx).Fields]
	mov eax, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	push eax
	cmp esi, eax
	cmova esi, eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax

	push edi
	lea edi, [eax + 4 + 4 * esi]
	mov ecx, [esp + 8]
	sub ecx, esi
	mov eax, Std$Object$Nil
	rep stosd
	pop edi

	dec esi
	js .nofields
	mov ecx, [esp]
.copyfields:
	add ecx, byte 4
	mov edx, [Std$Function_argument(edi).Val]
	add edi, byte sizeof(Std$Function_argument)
	dec esi
	mov [ecx], edx
	jns .copyfields
.nofields:
	pop ecx
	add esp, byte 4
	pop dword [Std$Object_t(ecx).Type]
	xor edx, edx
	xor eax, eax
	ret	

function Of, 1
;  returns the type of value
;@value
;:T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Object_t(eax).Type]
	xor edx, edx
	xor eax, eax
	ret

cfunction _is
	push ecx
	mov eax, [esp + 12]
	mov ecx, [esp + 8]
	mov ecx, [Std$Type_t(ecx).Types]
.loop:
	cmp [ecx], dword 0
	je .false
	cmp [ecx], eax
	je .true
	add ecx, byte 4
	jmp .loop
.false:
	xor eax, eax
	; cmp to set flags to ne
	cmp eax, ecx
.true:
	pop ecx
	ret

cfunction _new
;	push byte sizeof(fields)
;	call Riva$Memory$_alloc
;	mov [esp], eax
;	push byte sizeof(levels)
;	call Riva$Memory$_alloc
;	mov [esp], eax
;	push byte 8
;	call Riva$Memory$_alloc
;	mov [esp], eax
;	push byte sizeof(type)
;	call Riva$Memory$_alloc
;	add esp, byte 4
;	mov [Std$Object_t(eax).Type], dword T
;	pop ecx
;	pop dword [Std$Type_t(eax).Levels]
;	pop dword [Std$Type_t(eax).Fields]
;	mov [ecx], eax
;	mov [Std$Type_t(eax).Types], ecx
;	ret

symbol ?CALL, "()"

_function New
;  creates a new type derived from parents and adding fields
;@fields : Std$Array$T
;@parents... : T
;:T
	push ebp
	push edi
	push esi
	push byte sizeof(Std$Type_t)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Type_t(eax).Invoke], dword .invoke
	mov eax, [Std$Function_argument(edi).Val]
	push byte 1
	push dword [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	dec esi
	jz near .noparents
.sizeloop:
	mov eax, [Std$Function_argument(edi + 8 * esi).Val]
	mov ebx, [Std$Type_t(eax).Fields]
	mov ebx, [Std$Integer_smallt(Std$Array_t(ebx).Length).Value]
	add [esp], ebx
	mov ebx, [Std$Type_t(eax).Levels]
	mov ebx, [levels(ebx).Length]
	add [esp + 4], ebx
	dec esi
	jnz .sizeloop
	mov eax, [esp]
	push eax
	call Std$Array$_new
	mov [esp], eax
	mov eax, [esp + 8]
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov ebx, [esp + 16]
	mov [esp], eax
	mov [eax], ebx
	mov eax, [esp + 12]
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc_atomic
	mov ebx, [esp + 16]
	mov [esp], eax
	mov [levels(eax).Length], ebx
	; [esp] = Levels
	; [esp + 4] = Types
	; [esp + 8] = Fields
	; [esp + 12] = NoOfFields
	; [esp + 16] = NoOfTypes
	; [esp + 20] = Type
	; [esp + 24] = NoOfArgs
	; [esp + 28] = Args
	mov edi, [esp + 8]
	mov edi, [Std$Array_t(edi).Values]
	mov edx, [esp + 28]
	mov ebx, [esp + 24]
	dec ebx
.fieldcopyloop:
	add edx, byte 8
	mov esi, [Std$Function_argument(edx).Val]
	mov esi, [Std$Type_t(esi).Fields]
	mov ecx, [Std$Integer_smallt(Std$Array_t(esi).Length).Value]
	mov esi, [Std$Array_t(esi).Values]
	rep movsd
	dec ebx
	jnz .fieldcopyloop
	mov edx, [esp + 28]
	mov esi, [Std$Function_argument(edx).Val]
	mov ecx, [Std$Integer_smallt(Std$Array_t(esi).Length).Value]
	mov esi, [Std$Array_t(esi).Values]
	rep movsd
	mov [edi], ebx
	mov ebx, [esp + 24]
	dec ebx
	mov edi, [esp + 4]
	add edi, byte 4
.typecopyloop:
	mov esi, [Std$Function_argument(edx + 8 * ebx).Val]
	mov ecx, [Std$Type_t(esi).Levels]
	mov esi, [Std$Type_t(esi).Types]
	mov ecx, [levels(ecx).Length]
	rep movsd
	dec ebx
	jnz .typecopyloop
	mov edi, [esp]
	add edi, byte 4
	mov [edi], ebx
	mov ebx, [esp + 24]
	add edi, byte 4
	dec ebx
.levelcopyloop:
	mov esi, [Std$Function_argument(edx + 8 * ebx).Val]
	mov esi, [Std$Type_t(esi).Levels]
	mov ecx, [levels(esi).Length]
.levelcopyloop0:
	add esi, byte 4
	mov eax, [esi]
	inc eax
	mov [edi], eax
	add edi, byte 4
	dec ecx
	jnz .levelcopyloop0
	dec ebx
	jnz .levelcopyloop
	; [esp] = Levels
	; [esp + 4] = Types
	; [esp + 8] = Fields
	; [esp + 12] = NoOfFields
	; [esp + 16] = NoOfTypes
	; [esp + 20] = Type
	; [esp + 24] = NoOfArgs
	; [esp + 28] = Args
	mov edi, [esp + 4]
	mov esi, [esp]
	mov ebx, [esp + 16]
	xor eax, eax
	dec ebx
	call .sort
.evenifnoparents:
%if 0
%else
	mov esi, [esp + 8]
	mov esi, [Std$Array_t(esi).Values]
	mov edi, FieldGetter
	push esi
	push edi
	mov esi, [esi]
	test esi, esi
	jz .nofields
.fieldaccessloop:
	push edi
	push dword [esp + 32]
	push esi
	call Std$Symbol$_typetable_put
	pop dword [Std$Symbol_t(eax).Function]
	add [esp + 4], dword 4
	add [esp], dword 8
	mov esi, [esp + 4]
	mov edi, [esp]
	mov esi, [esi]
	test esi, esi
	jnz .fieldaccessloop
.nofields:
	add esp, byte 8
%endif
	mov ecx, [esp + 20]
	pop dword [Std$Type_t(ecx).Levels]
	pop dword [Std$Type_t(ecx).Types]
	pop dword [Std$Type_t(ecx).Fields]
	xor edx, edx
	add esp, byte 20
	xor eax, eax
	pop ebp
	ret
.noparents:
	push eax
	push byte 8
	call Riva$Memory$_alloc_atomic
	mov ebx, [esp + 16]
	mov [esp], eax
	mov [eax], ebx
	mov [eax + 4], dword 0
	push byte 8
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov [levels(eax).Length], dword 1
	mov [levels(eax).Levels], dword 0
	jmp .evenifnoparents

.sort:;(edi = Types, esi = Levels, eax = Low, ebx = High)
	cmp eax, ebx
	jl .nontrivial
	ret
.nontrivial:
	push eax
	push ebx
	mov ecx, [esi + 4 * eax + 4]
	push dword [edi + 4 * eax]
	mov edx, [esi + 4 * ebx + 4]
	push dword [edi + 4 * ebx]
.sortloop:
	cmp edx, ecx
	jae .above
.below:
	pop dword [edi + 4 * eax]
	mov [esi + 4 * eax + 4], edx
	inc eax
	cmp eax, ebx
	je .finish
	push dword [edi + 4 * eax]
	mov edx, [esi + 4 * eax + 4]
	jmp .sortloop
.above:
	pop dword [edi + 4 * ebx]
	mov [esi + 4 * ebx + 4], edx
	dec ebx
	cmp eax, ebx
	je .finish
	push dword [edi + 4 * ebx]
	mov edx, [esi + 4 * ebx + 4]
	jmp .sortloop
.finish:
	pop dword [edi + 4 * ebx]
	mov [esi + 4 * ebx + 4], ecx
	pop ebx
	push eax
	inc eax
	call .sort
	pop ebx
	sub ebx, byte 2
	pop eax
	jmp .sort

cfunction New.invoke
	; ecx = object
	; esi = number of args
	; edi = args
	;int3
	push ebp
	mov ebp, esp
	lea eax, [8 * esi]
	sub esp, eax
	push byte 0
	push ecx
	mov ebx, esi
	mov esi, edi
	lea edi, [esp + 8]
	lea ecx, [2 * ebx]
	rep movsd
	lea esi, [ebx + 1]
	mov edi, esp
	mov ecx, ?CALL
	call Std$Symbol$T.invoke
	mov esp, ebp
	pop ebp
	ret

_function Fields
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Type_t(ecx).Fields]
	xor edx, edx
	xor eax, eax
	ret

%macro field_getter 1
section .dfields noexec
get_field_%1:
	dd Std$Function$AsmT
	dd %%here
section .cfields exec
%%here:
	mov edx, [Std$Function_argument(edi).Val]
	add edx, 4 + (4 * %1)
	mov ecx, [edx]
	xor eax, eax
	ret
%endmacro

%if 0
%else
section .dfields noexec
align 8
FieldGetter:
%assign i 0
%rep 1024
	field_getter i
	%assign i i + 1
%endrep
%endif

%ifdef DOCUMENTING

%define Std$Type$T T

%define type_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
