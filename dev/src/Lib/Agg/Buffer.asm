%include "Std.inc"
%include "Riva/Memory.inc"
%include "Agg/Buffer.inc"

ctype T, Std$Address$T
.invoke: equ 0

cglobal Nil, T
	dd T, 0, 0

cfunction _alloc
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Agg$Buffer_t(eax).Length).Type], dword Std$Integer$SmallT
	ret

cfunction _new ;(long  Value)
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, [esp + 4]
	mov [Agg$Buffer_t(eax).Value], ecx
	mov ecx, [esp + 8]
	mov [Std$Object_t(Agg$Buffer_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Agg$Buffer_t(eax).Length).Value], ecx
	ret

function New, 1
;@length:Std$Integer$SmallT
;@data:Std$Address$T
;:T
; Allocates <var>length</var> bytes of memory and returns its address.
	cmp esi, byte 1
	je .allocate
	push dword sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword T
	mov [Std$Object_t(Agg$Buffer_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov [Std$Integer_smallt(Agg$Buffer_t(ecx).Length).Value], eax
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	mov [Agg$Buffer_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret
.allocate:
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword T
	pop dword [Agg$Buffer_t(ecx).Value]
	mov [Std$Object_t(Agg$Buffer_t(ecx).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Agg$Buffer_t(ecx).Length).Value]
	xor eax, eax
	xor edx, edx
	ret

function NewAtomic, 1
;@length:Std$Integer$SmallT
;:T
; Allocates <var>length</var> bytes of memory and returns its address.
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	call Riva$Memory$_alloc_atomic
	push eax
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	pop dword [Std$Address_t(eax).Value]
	mov [Std$Object_t(Agg$Buffer_t(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Agg$Buffer_t(eax).Length).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

method "length", TYP, T
	mov eax, [Std$Function_argument(edi).Val]
	lea ecx, [Agg$Buffer_t(eax).Length]
	xor edx, edx
	xor eax, eax
	ret

_function Compare
;@a:T
;@b:T
;:Std$Object$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov eax, [Agg$Buffer_t(eax).Value]
	cmp eax, [Agg$Buffer_t(ecx).Value]
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
	mov ecx, [Agg$Buffer_t(ecx).Value]
	mov [Std$Integer_smallt(eax).Value], ecx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

%macro typed_buffer 2

ctype %1 %+ T, T, Std$Address$T
.invoke: equ 0

cfunction %1 %+ _new ;(long  Value)
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword %1 %+ T
	mov ecx, [esp + 4]
	mov [Agg$Buffer_t(eax).Value], ecx
	mov ecx, [esp + 8]
	mov [Std$Object_t(Agg$Buffer_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Agg$Buffer_t(eax).Length).Value], ecx
	ret

function %1 %+ New, 1
;@length:Std$Integer$SmallT
;@data:Std$Address$T
;:%1 %+ T
; Creates an %1 array length <var>length</var> elements.
	cmp esi, byte 1
	je .allocate
	push dword sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword %1 %+ T
	mov [Std$Object_t(Agg$Buffer_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov [Std$Integer_smallt(Agg$Buffer_t(ecx).Length).Value], eax
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	mov [Agg$Buffer_t(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret
.allocate:
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(eax).Value]
	lea eax, [ebx * %2]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(Agg$Buffer_t)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword %1 %+ T
	pop dword [Agg$Buffer_t(ecx).Value]
	mov [Std$Object_t(Agg$Buffer_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Agg$Buffer_t(ecx).Length).Value], ebx
	xor eax, eax
	xor edx, edx
	ret

method "size", TYP, %1 %+ T
	call Std$Integer$_alloc_small
	mov edx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Integer_smallt(Agg$Buffer_t(edx).Length).Value]
	lea edx, [%2 * edx]
	mov [Std$Integer_smallt(eax).Value], edx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

%endmacro

typed_buffer Int8, 1
typed_buffer Int16, 2
typed_buffer Int32, 4
typed_buffer Float32, 4
typed_buffer Float64, 8