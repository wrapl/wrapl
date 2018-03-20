%include "Std.inc"
%include "Riva/Memory.inc"

ctype FewArgsMessageT
; Message sent when a function is called with too few arguments
.invoke: equ 0

ctype ArgTypeMessageT
; Message sent when a function is called with incorrect argument types
.invoke: equ 0

ctype T
; Base type of all functions
.invoke: equ 0

ctype StatusT, Std$Integer$SmallT, Std$Integer$T, Std$Number$T
.invoke: equ 0

cglobal Suspend, StatusT
	dd StatusT, -1

cglobal Success, StatusT
	dd StatusT, 0

cglobal Failure, StatusT
	dd StatusT, 1

cglobal Message, StatusT
	dd StatusT, 2

cglobal Nil, T
	dd T

ctype AsmT, T
; Type of functions written in assembly.
.invoke:
; <ccode>struct {Std$Type_t *Type; void *Invoke;};</ccode>
; <var>Invoke</var> is an assembly function with the following parameters:
; <var>ecx</var> = invoked function
; <var>esi</var> = number of arguments
; <var>edi</var> = pointer to array of arguments
; The result is returned as follows:
; <var>eax</var> = status [<code>-1 = SUSP</code>, <code>0 = RET</code>, <code>1 = FAIL</code>, <code>2 = SEND</code>. No other values are allowed]
; <var>ebx</var> = suspended state, if <code>eax = 1</code>
; <var>ecx</var> = returned value, if <code>eax = -1</code>, <code>0</code> or <code>2</code>
; <var>edx</var> = returned reference, if <code>eax = -1</code> or <code>0</code>
	jmp [Std$Function_asmt(ecx).Invoke]

ctype CheckedAsmT, AsmT, T
; Type of functions written in assembly with a checked minimum number of arguments
.invoke:
	cmp esi, [Std$Function_checkedasmt(ecx).Count]
	jb .toofewargs
	jmp [Std$Function_asmt(Std$Function_checkedasmt(ecx).Unchecked).Invoke]
.toofewargs:
	push ecx
	push byte sizeof(Std$Function_fewargsmessage)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword FewArgsMessageT
	pop dword [Std$Function_fewargsmessage(eax).Func]
	mov dword [Std$Function_fewargsmessage(eax).Count], esi
	mov ecx, eax
	xor edx, edx
	mov eax, 2
	ret

ctype CT, T
; Type of functions written in C.
cfunction CT.invoke
	push byte 0
	push byte 0
	push dword Std$Object$Nil
	mov ebx, esp
	and esp, byte 0xF0
	push ebx
	push edi
	push esi
	push ecx
	call [Std$Function_ct(ecx).Invoke]
	add esp, byte 12
	pop esp
	pop ecx
	pop edx
	pop ebx
	ret

ctype CheckedCT, CT, T
; Type of functions written in C with checked minimum number of arguments.
.invoke:
	cmp esi, [Std$Function_checkedct(ecx).Count]
	jb .toofewargs
	push byte 0
	push byte 0
	push dword Std$Object$Nil
	mov ebx, esp
	and esp, byte 0xF0
	push ebx
	push edi
	push esi
	push ecx
	call [Std$Function_checkedct(ecx).Invoke]
	add esp, byte 12
	pop esp
	pop ecx
	pop edx
	pop ebx
	ret
.toofewargs:
	push ecx
	push byte sizeof(Std$Function_fewargsmessage)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword FewArgsMessageT
	pop dword [Std$Function_fewargsmessage(eax).Func]
	mov dword [Std$Function_fewargsmessage(eax).Count], esi
	mov ecx, eax
	xor edx, edx
	mov eax, 2
	ret

cfunction _new_arg_type_message
	push byte sizeof(Std$Function_argtypemessage)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword ArgTypeMessageT
	mov ecx, [esp + 4]
	mov edx, [esp + 8]
	mov [Std$Function_argtypemessage(eax).Func], ecx
	mov [Std$Function_argtypemessage(eax).Index], edx
	mov ecx, [esp + 12]
	mov edx, [esp + 16]
	mov [Std$Function_argtypemessage(eax).Expected], ecx
	mov [Std$Function_argtypemessage(eax).Received], edx
	ret

cfunction _new_c
	push byte sizeof(Std$Function_ct)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword CT
	mov [Std$Function_ct(eax).Invoke], ecx
	ret

struct cstate, Std$Function_state
	.Invoke:	resd 1
endstruct

cfunction _resume_c
	push byte 0
	push dword Std$Object$Nil
	push eax
	push esp
	call [cstate(eax).Invoke]
	add esp, byte 4
	pop ebx
	pop ecx
	pop edx
	ret

cfunction _call
	push ebx
	push esi
	push edi
	lea edi, [esp + 28]
	mov esi, [esp + 20]
	mov ecx, [esp + 16]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	mov edi, [esp + 24]
	;cmp eax, byte 0
	;jl .state
	;xor ebx, ebx
;.state:
;	faster version of above, assuming eax = -1, 0, 1 or 2 and valid states have address % 4 = 0
	and ebx, eax
	and bl, 0xFC
	mov [Std$Function_result(edi).State], ebx
	mov [Std$Function_result(edi).Val], ecx
	mov [Std$Function_result(edi).Ref], edx
	pop edi
	pop esi
	pop ebx
	ret

cfunction _invoke
	push ebx
	push esi
	push edi
	mov edi, [esp + 28]
	mov esi, [esp + 20]
	mov ecx, [esp + 16]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	mov edi, [esp + 24]
;	cmp eax, byte 0
;	jl .state
;	xor ebx, ebx
;.state:
	and ebx, eax
	and bl, 0xFC
	mov [Std$Function_result(edi).State], ebx
	mov [Std$Function_result(edi).Val], ecx
	mov [Std$Function_result(edi).Ref], edx
	pop edi
	pop esi
	pop ebx
	ret

cfunction _resume
	push ebx
	push esi
	push edi
	mov edi, [esp + 16]
	mov eax, [Std$Function_result(edi).State]
	call [Std$Function_state(eax).Run]
	mov edi, [esp + 16]
;	cmp eax, byte 0
;	jl .state
;	xor ebx, ebx
;.state:
	and ebx, eax
	and bl, 0xFC
	mov [Std$Function_result(edi).State], ebx
	mov [Std$Function_result(edi).Val], ecx
	mov [Std$Function_result(edi).Ref], edx
	pop edi
	pop esi
	pop ebx
	ret

function Identity, 1
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Val]
	xor eax, eax
	ret

struct constant0, Std$Object_t
    .Value: resd 1
endstruct

ctype ConstantT, T
.invoke:
    lea edx, [constant0(ecx).Value]
    mov ecx, [constant0(ecx).Value]
    xor eax, eax
    ret

function ConstantNew, 1
;@x
;:T
; returns a function which always returns the value <var>x</var>.
    push byte sizeof(constant0)
    call Riva$Memory$_alloc
    add esp, byte 4
    mov ecx, [Std$Function_argument(edi).Val]
    mov [Std$Object_t(eax).Type], dword ConstantT
    mov [constant0(eax).Value], ecx
    mov ecx, eax
    xor edx, edx
    xor eax, eax
    ret

cfunction _constant_new
	push byte sizeof(constant0)
    call Riva$Memory$_alloc
    add esp, byte 4
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword ConstantT
    mov [constant0(eax).Value], ecx
	ret

function Fail, 0
	mov eax, 1
	ret

ctype MessageT, T
.invoke:
    xor edx, edx
    mov ecx, [constant0(ecx).Value]
    mov eax, 2
    ret

function MessageNew, 1
;@x
;:T
; returns a function which always sends the message <var>x</var>.
    push byte sizeof(constant0)
    call Riva$Memory$_alloc
    add esp, byte 4
    mov ecx, [Std$Function_argument(edi).Val]
    mov [Std$Object_t(eax).Type], dword MessageT
    mov [constant0(eax).Value], ecx
    mov ecx, eax
    xor edx, edx
    xor eax, eax
    ret

cfunction _message_new
	push byte sizeof(constant0)
    call Riva$Memory$_alloc
    add esp, byte 4
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword MessageT
    mov [constant0(eax).Value], ecx
	ret

struct variable, Std$Object_t
    .Address: resd 1
endstruct

ctype VariableT, T
.invoke:
    mov edx, [variable(ecx).Address]
    mov ecx, [edx]
    xor eax, eax
    ret

function VariableNew, 1
;@v+
;:T
; returns a function which always returns the variable <var>v</var>.
    push byte sizeof(variable)
    call Riva$Memory$_alloc
    add esp, byte 4
    mov edx, [Std$Function_argument(edi).Ref]
    mov [Std$Object_t(eax).Type], dword VariableT
    mov [variable(eax).Address], edx
    mov ecx, eax
    xor edx, edx
    xor eax, eax
    ret

_function Fold
;@func1
;@func2
; returns the repeated application of <var>func1</var> to the values produced by <var>func2</var>
	push dword [Std$Function_argument(edi).Val]
	xor esi, esi
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	cmp eax, byte -1
	je .atleast2
	add esp, byte 4
	ret
.atleast2:
	sub esp, byte 16
.loop:
	mov [esp], ecx
	mov [esp + 4], edx
	mov eax, ebx
	call [Std$Function_state(eax).Run]
	jmp [.table + eax * 4]
.return:
	mov [esp + 8], ecx
	mov [esp + 12], edx
	mov edi, esp
	mov esi, 2
	mov ecx, [esp + 16]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
.message:
	add esp, byte 20
	ret
.suspend:
	mov [esp + 8], ecx
	mov [esp + 12], edx
	mov edi, esp
	mov esi, 2
	mov ecx, [esp + 16]
	mov eax, [Std$Object_t(ecx).Type]
	push ebx
	call [Std$Type_t(eax).Invoke]
	pop ebx
	cmp eax, 1
	jl .loop
	add esp, byte 20
	ret
.failure:
	pop ecx
	pop edx
	add esp, byte 12
	xor eax, eax
	ret
datasect
	dd .suspend
.table:
	dd .return
	dd .failure
	dd .message

struct iterator, Std$Object_t
	.State:		resd 1
endstruct

struct initial_state
	.Run:		resd 1
	.Function:	resd 1
	.Count:		resd 1
	.Args:
endstruct

ctype IteratorT
.invoke: equ 0

function IteratorNew, 1
;@fun:T
;@args...
;:IteratorT
; Creates an <id>IteratorT</id> which encapsulates the function call <code>fun(args)</code>.
	dec esi
	js .failure
	lea eax, [sizeof(initial_state) + 8 * esi]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	mov ecx, [Std$Function_argument(edi).Val]
	mov [initial_state(eax).Run], dword .initial_run
	mov [initial_state(eax).Function], ecx
	mov [initial_state(eax).Count], esi
	lea ecx, [esi + esi]
	lea esi, [edi + sizeof(Std$Function_argument)]
	lea edi, [initial_state(eax).Args]
	rep movsd
	push byte sizeof(iterator)
	call Riva$Memory$_alloc
	pop ecx
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword IteratorT
	pop dword [iterator(ecx).State]
	xor edx, edx
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret
.initial_run:
	mov ecx, [initial_state(eax).Function]
	mov esi, [initial_state(eax).Count]
	lea edi, [initial_state(eax).Args]
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

_function IteratorNext
;@iter:IteratorT
;:ANY
; returns the next value produced by the function call encapsulated in <var>iter</var>.
	mov eax, [Std$Function_argument(edi).Val]
	push eax
	mov eax, [iterator(eax).State]
	call [Std$Function_state(eax).Run]
	pop edi
	cmp eax, byte -1
	je .suspend
	mov [iterator(edi).State], dword .final_state
	ret
.suspend:
	mov [iterator(edi).State], ebx
	xor eax, eax
	ret
.final_run:
	xor eax, eax
	inc eax
	ret
datasect
.final_state:
	dd .final_run

struct generate_state, Std$Function_state
	.Remain:	resd 1
	.Args:		resd 1
endstruct

_function Generate
;@v&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;v&lt;sub&gt;k&lt;/sub&gt;
;:ANY
; Generates <var>v<sub>1</sub></var>, ..., <var>v<sub>k</sub></var>
	dec esi
	js .fail
	jz .return
.suspend:
	push byte sizeof(generate_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [generate_state(eax).Remain], esi
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	add edi, byte sizeof(Std$Function_argument)
	mov [generate_state(eax).Args], edi
	mov dword [Std$Function_state(eax).Run], .resume
	mov ebx, eax
	or eax, byte -1
	ret
.return:
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
	dec dword [generate_state(eax).Remain]
	mov edi, [generate_state(eax).Args]
	js .fail
	jz .return
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	add edi, byte sizeof(Std$Function_argument)
	mov [generate_state(eax).Args], edi
	mov ebx, eax
	or eax, byte -1
	ret

struct cycle_state, Std$Function_state
	.Total:		resd 1
	.Current:	resd 1
	.Args:		resd 1
endstruct

struct cycle_single_state, Std$Function_state
	.Val:	resd 1
	.Ref:	resd 1
endstruct

_function Cycle
;@v&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;v&lt;sub&gt;k&lt;/sub&gt;
;:ANY
; Generates <var>v<sub>1</sub></var>, ..., <var>v<sub>k</sub></var>, ..., <var>v<sub>1</sub></var>, ...
	dec esi
	js .fail
	jz .single
.suspend:
	push byte sizeof(cycle_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [cycle_state(eax).Total], esi
	mov [cycle_state(eax).Args], edi
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	mov dword [Std$Function_state(eax).Run], .resume
	mov ebx, eax
	or eax, byte -1
	ret
.single:
	push byte sizeof(cycle_single_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	mov [cycle_single_state(eax).Val], ecx
	mov [cycle_single_state(eax).Ref], edx
	mov dword [Std$Function_state(eax).Run], .resume_single
	mov ebx, eax
	or eax, byte -1
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
%ifdef CMOVSUPPORT
	xor edx, edx
%endif
	mov edi, [cycle_state(eax).Current]
	inc edi
	cmp edi, [cycle_state(eax).Total]
%ifdef CMOVSUPPORT
	cmova edi, edx
%else
	jbe .continue
	xor edi, edi
.continue:
%endif
	mov [cycle_state(eax).Current], edi
	shl edi, byte 3
	add edi, [cycle_state(eax).Args]
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	add edi, byte sizeof(Std$Function_argument)
	mov ebx, eax
	or eax, byte -1
	ret
.resume_single:
	mov ecx, [cycle_single_state(eax).Val]
	mov edx, [cycle_single_state(eax).Ref]
	mov ebx, eax
	or eax, byte -1
	ret

%ifdef DOCUMENTING

%define Std$Function$T T
%define Std$Function$FewArgsMessageT FewArgsMessageT

%define function_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
