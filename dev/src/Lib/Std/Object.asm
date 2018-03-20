%include "Std.inc"

ctype T
;  the base type of all types
.invoke:
	mov ecx, dword Nil
	xor edx, edx
	xor eax, eax
	ret

cglobal Nil, T
	dd T

_function New
;@object : T
;: T
;  creates a new object of the same type as <var>object</var>
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Object_t(ecx).Type]
	add edi, byte sizeof(Std$Function_argument)
	dec esi
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

_function Create
;@type : Std$Type$T
;@values...
;: T
;  Creates a new object of type <var>type</var>. If present, fields are initialized from <var>values</var>.
	mov ecx, [Std$Function_argument(edi).Val]
	add edi, byte sizeof(Std$Function_argument)
	dec esi
	push ecx
	mov eax, [Std$Type_t(ecx).Fields]
	mov eax, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push edi
	lea edi, [eax + 4]
	mov ecx, [esp + 8]
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
	cmp [Std$Object_t(edx).Type], dword Std$Symbol$ArrayT
	je .namedfields
	dec esi
	mov [ecx], edx
	jns .copyfields
.nofields:
	pop ecx
	pop edx
	pop dword [Std$Object_t(ecx).Type]
	xor edx, edx
	xor eax, eax
	ret	
.namedfields:
	mov esi, edi
	mov ebx, [esp]
	push ebp
	mov ebp, [esp + 12]
	mov ebp, [Std$Type_t(ebp).Fields]
	lea edx, [Std$Symbol_arrayt(edx).Values]
	mov eax, [edx]
	add edx, byte 4
	test eax, eax
	jz .nonamedfields
.namedfieldloop:
	mov ecx, [Std$Integer_smallt(Std$Array_t(ebp).Length).Value]
	mov edi, [Std$Array_t(ebp).Values]
	repne scasd
	jne .nameerror
	neg ecx
	mov eax, [Std$Function_argument(esi).Val]
	add ecx, [Std$Integer_smallt(Std$Array_t(ebp).Length).Value]
	add esi, byte sizeof(Std$Function_argument)
	mov [ebx + 4 * ecx], eax
	mov eax, [edx]
	add edx, byte 4
	test eax, eax
	jnz .namedfieldloop
.nonamedfields:
	pop ebp
	jmp .nofields
.nameerror:
	pop ebp
	add esp, byte 12
	mov ebx, eax
	jmp Std$Symbol$nomethod.invoke

_function IsEqual
;@a:T
;@b:T
;:T
; Returns <var>b</var> if <code>a == b</code>
	mov ecx, [Std$Function_argument(edi + 8).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	jne .fail
	mov edx, [Std$Function_argument(edi + 8).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

_function IsNotEq
;@a:T
;@b:T
;:T
; Returns <var>b</var> if <code>a ~== b</code>
	mov ecx, [Std$Function_argument(edi + 8).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	je .fail
	mov edx, [Std$Function_argument(edi + 8).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

cglobal Less, T
; Result of comparison.
	dd T, -1

cglobal Equal, T
; Result of comparison.
	dd T, 0

cglobal Greater, T
; Result of comparison.
	dd T, 1

_function Compare
;@a:T
;@b:T
;:T
; Compares <var>a</var> and <var>b</var> and return either <id>Std/Object/Less</id>, <id>Std/Object/Equal</id> or <id>Std/Object/Greater</id>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov edx, [Std$Object_t(eax).Type]
	cmp edx, [Std$Object_t(ecx).Type]
	jl .less
	jg .greater
	cmp eax, ecx
	jl .less
	jg .greater
.equal:
	mov ecx, Equal
	xor edx, edx
	xor eax, eax
	ret
.less:
	mov ecx, Less
	xor edx, edx
	xor eax, eax
	ret
.greater:
	mov ecx, Greater
	xor edx, edx
	xor eax, eax
	ret

_function Hash
;@a:T
;:Std$Integer$SmallT
; Returns a hash for <var>a</var>.
	call Std$Integer$_alloc_small
	mov ecx, [Std$Function_argument(edi).Val]
	mov [Std$Integer_smallt(eax).Value], ecx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

_function Field
	mov edx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	lea edx, [edx + 4 * eax]
	mov ecx, [edx]
	xor eax, eax
	ret

struct finalizer
	.Func:	resd 1
	.Arg:	resd 2
endstruct

cfunction _finalize
	mov eax, [esp + 8]
	mov ecx, [esp + 4]
	push ebx
	push edi
	push esi
	push dword [Std$Function_argument(finalizer(eax).Arg).Ref]
	push dword [Std$Function_argument(finalizer(eax).Arg).Val]
	push byte 0
	push ecx
	mov edi, esp
	mov esi, 2
	mov ecx, [finalizer(eax).Func]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	add esp, byte 16
	pop esi
	pop edi
	pop ebx
	ret

extern Riva$Memory$_alloc
extern Riva$Memory$_register_finalizer

function RegisterFinalizer, 2
;@object:T
;@finalizer:Std$Function$T
;@data:ANY=NIL
; Attaches a finalizer to <var>object</var>. I.e. <code>finalizer(object, data)</code> should be called at some stage after <var>object</var> becomes unreachable.
	push byte sizeof(finalizer)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi + 8).Val]
	mov [finalizer(eax).Func], edx
	mov ecx, Nil
	xor edx, edx
	cmp esi, byte 2
	cmova ecx, [Std$Function_argument(edi + 16).Val]
	cmova edx, [Std$Function_argument(edi + 16).Ref]
	mov [Std$Function_argument(finalizer(eax).Arg).Val], ecx
	mov [Std$Function_argument(finalizer(eax).Arg).Ref], edx
	push byte 0
	push byte 0
	push eax
	push dword _finalize
	push dword [Std$Function_argument(edi).Val]
	call Riva$Memory$_register_finalizer
	add esp, byte 24
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

cfunction _in
	mov eax, [esp + 4]
	mov eax, [Std$Object_t(eax).Type]
	mov ecx, [esp + 8]
	cmp eax, ecx
	je .true
	mov eax, [Std$Type_t(eax).Types]
	xor edx, edx
.loop:
	cmp [eax], ecx
	je .true
	cmp [eax], edx
	je .false
	add eax, byte 4
	jmp .loop
.true:
	mov eax, 1
	ret
.false:
	xor eax, eax
	ret

ctype NonCallableMessageT
.invoke: equ _default_invoke

extern Riva$Debug$_stack_trace
cfunction _default_invoke
	push byte sizeof(Std$Object_noncallablemessage)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov [Std$Object_t(eax).Type], dword NonCallableMessageT
	lea eax, [Std$Object_noncallablemessage(eax).Stack]
	push dword 12
	lea eax, [Std$Object_noncallablemessage(eax).Stack]
	push eax
	lea eax, [esp + 12]
	push eax
	call Riva$Debug$_stack_trace
	add esp, byte 12
	pop ecx
	mov [Std$Object_noncallablemessage(ecx).Count], eax
	xor edx, edx
	mov eax, 2
	ret

%ifdef DOCUMENTING

%define Std$Object$T T
%define Std$Object$Nil Nil

%define object_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
