%include "Std.inc"
%include "Riva/Memory.inc"

ctype T
; An address in memory.
.invoke: equ 0

cglobal Nil, T
	dd T, 0

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

_function _FromVal
;@value
;:T
; Returns the address of the data of <var>value</var>.
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	add eax, byte 4
	mov [Std$Address_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret


function FromVal, 1
;@value
;:T
; Returns the address of the object <var>value</var>.
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov [Std$Address_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

function FromRef, 1
;@variable+
;:T
; Returns the address where <var>variable</var> is stored.
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Ref]
	mov [Std$Address_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

function ToVal, 1
;@address:T
;:ANY
; Returns the object at <var>address</var>
    mov eax, [Std$Function_argument(edi).Val]
    mov ecx, [Std$Address_t(eax).Value]
    xor edx, edx
    xor eax, eax
    ret

function ToRef, 1
;@address:T
;:ANY
; Returns an assignable reference to the memory at <var>address</var>
    mov eax, [Std$Function_argument(edi).Val]
    mov edx, [Std$Address_t(eax).Value]
    mov ecx, [edx]
    xor eax, eax
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

%define address_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile


%endif
