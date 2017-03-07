%include "Std.inc"
%include "Riva/Memory.inc"

ctype T, Std$Address$T
.invoke: equ 0

cglobal Nil, T
	dd T, 0, 0

cfunction _alloc
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	ret

cfunction _new ;(long  Value)
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, [esp + 4]
	mov [Std$Address_t(eax).Value], ecx
	ret

function New, 1
;@size:Std$Integer$SmallT
;:T
; Allocates <var>size</var> bytes of memory and returns its address.
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	pop dword [Std$Address_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

function NewAtomic, 1
;@size:Std$Integer$SmallT
;:T
; Allocates <var>size</var> bytes of memory and returns its address.
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	pop dword [Std$Address_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

_function Compare
;@a:T
;@b:T
;:Std$Object$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Address_t(eax).Value]
	cmp eax, [Std$Address_t(ecx).Value]
	jl .less
	jg .greater
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
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret
   
_function Hash
;@a:T
;:Std$Integer$SmallT
	call Std$Integer$_alloc_small
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Address_t(ecx).Value]
	mov [Std$Integer_smallt(eax).Value], ecx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

%ifdef DOCUMENTING

%define Std$Address$T T

%define buffer_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif