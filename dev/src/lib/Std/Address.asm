%include "Std.inc"
%include "Riva/Memory.inc"

ctype T
; An address in memory.
.invoke: equ 0

cglobal Nil, T
	dd T, 0, Std$Integer$SmallT, 0

cfunction _alloc
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Address_t(eax).Length).Type], dword Std$Integer$SmallT
	ret

cfunction _new ;(long  Value, long Size)
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Address_t(eax).Length).Type], dword Std$Integer$SmallT
	mov ecx, [esp + 4]
	mov edx, [esp + 8]
	mov [Std$Address_t(eax).Value], ecx
	mov [Std$Integer_smallt(Std$Address_t(eax).Length).Value], edx
	ret

_function _FromVal
;@value
;:T
; Returns the address of the data of <var>value</var>.
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$Address_t(eax).Length).Type], dword Std$Integer$SmallT
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
	mov [Std$Object_t(Std$Address_t(eax).Length).Type], dword Std$Integer$SmallT
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
	mov [Std$Object_t(Std$Address_t(eax).Length).Type], dword Std$Integer$SmallT
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Ref]
	mov [Std$Address_t(ecx).Value], eax
	mov [Std$Integer_smallt(Std$Address_t(eax).Length).Value], byte 4
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

%macro allocator 2

function %1, 1
;@length:Std$Integer$SmallT
;@data:T
;:T
; Allocates <var>length</var> bytes of memory and returns its address.
	cmp esi, byte 1
	je .allocate
	push dword sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword T
	mov [Std$Object_t(Std$Address_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov [Std$Integer_smallt(Std$Address_t(ecx).Length).Value], eax
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	mov [Std$Address_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret
.allocate:
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(eax).Value]
	push ebx
	call Riva$Memory$_ %+ %2
	mov [esp], eax
	push byte sizeof(Std$Address_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword T
	pop dword [Std$Address_t(ecx).Value]
	mov [Std$Object_t(Std$Address_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$Address_t(ecx).Length).Value], ebx
	xor eax, eax
	xor edx, edx
	ret

%endmacro

allocator New, alloc
allocator NewAtomic, alloc_atomic
allocator NewLarge, alloc_large
allocator NewAtomicLarge, alloc_atomic_large

asymbol Of

%ifdef DOCUMENTING

%define T T

%define address_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
