%include "Std.inc"
%include "Riva/Memory.inc"

struct bstate, Std$Function_state
	.Val:		resd 1
	.Ref:		resd 1
	.Code:		resd 1
	.Handler:	resd 1
endstruct

struct dstate, bstate
	.UpState:	resd 1
	.Function:	resd 1
	.Thread:	resd 1
endstruct

struct debugger
	.add_module:		resd 1
	.add_line:			resd 1
	.add_global:		resd 1
	.add_function:		resd 1
	.set_locals_offset:	resd 1
	.add_local:			resd 1
	.enter_function:	resd 1
	.exit_function:		resd 1
	.alloc_local:		resd 1
	.break_line:		resd 1
endstruct

struct closure, Std$Object_t
	.Entry:		resd 1
	.Scopes:
endstruct

struct variadic, Std$Object_t
	.Length: resv(Std$Integer_smallt)
	.Args:
endstruct

ctype VariadicT
; Used to collect extra arguments to a variadic function.
.invoke: equ 0

cfunction create_variadic
	lea eax, [sizeof(variadic) + 8 * ebx]
	push eax
	call Riva$Memory$_alloc
	mov [esp], edi
	mov [Std$Object_t(eax).Type], dword VariadicT
	mov [Std$Object_t(variadic(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(variadic(eax).Length).Value], ebx
	lea edi, [variadic(eax).Args]
	lea ecx, [2 * ebx]
	rep movsd
	pop edi
	ret

function VariadicNew, 1
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(eax).Value]
	lea eax, [sizeof(variadic) + 8 * ebx]
	push eax
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword VariadicT
	mov [Std$Object_t(variadic(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(variadic(eax).Length).Value], ebx
	lea edi, [variadic(eax).Args]
	mov ecx, Std$Object$Nil
	dec ebx
	js .noargs
.loop:
	mov [Std$Function_argument(edi).Val], ecx
	mov [Std$Function_argument(edi).Ref], edi
	add edi, byte 8
	dec ebx
	jns .loop
.noargs:
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

method "length", TYP, VariadicT
;@args
;:Std$Integer$SmallT
; Returns the number of arguments in <var>args</var>.
	mov eax, [Std$Function_argument(edi).Val]
	lea ecx, [variadic(eax).Length]
	xor edx, edx
	xor eax, eax
	ret

methodx VariadicIndex, 2, "[]", TYP, VariadicT, TYP, Std$Integer$SmallT
;@args
;@n
;:ANY
; Returns the <var>n</var>th argument in <var>args</var>.
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov edi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	dec eax
	jns .nonneg
	add eax, [Std$Integer_smallt(variadic(edi).Length).Value]
	inc eax
.nonneg:
	cmp eax, [Std$Integer_smallt(variadic(edi).Length).Value]
	jge .failure
	mov ecx, [Std$Function_argument(variadic(edi).Args + 8 * eax).Val]
	mov edx, [Std$Function_argument(variadic(edi).Args + 8 * eax).Ref]
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

methodx VariadicSet, 3, "set", TYP, VariadicT, TYP, Std$Integer$SmallT, ANY
;@args
;@n
;@arg
;:ANY
; Sets the <var>n</var>th argument in <var>args</var> to <var>arg</var>.
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov ecx, [Std$Function_argument(edi + 16).Val]
	mov edx, [Std$Function_argument(edi + 16).Ref]
	mov edi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	dec eax
	jns .nonneg
	add eax, [Std$Integer_smallt(variadic(edi).Length).Value]
	inc eax
.nonneg:
	cmp eax, [Std$Integer_smallt(variadic(edi).Length).Value]
	jge .failure
	mov [Std$Function_argument(variadic(edi).Args + 8 * eax).Val], ecx
	mov [Std$Function_argument(variadic(edi).Args + 8 * eax).Ref], edx
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

method "apply", TYP, VariadicT, TYP, Std$Function$T
;@args
;@func
; Calls <var>func</var> with <var>args</var>.
	mov edx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov esi, [Std$Integer_smallt(variadic(edx).Length).Value]
	mov eax, [Std$Object_t(ecx).Type]
	lea edi, [variadic(edx).Args]
	jmp [Std$Type_t(eax).Invoke]

method "apply", TYP, Std$Function$T, TYP, VariadicT
;@func
;@args
; Calls <var>func</var> with <var>args</var>.
	mov edx, [Std$Function_argument(edi + 8).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(variadic(edx).Length).Value]
	mov eax, [Std$Object_t(ecx).Type]
	lea edi, [variadic(edx).Args]
	jmp [Std$Type_t(eax).Invoke]

method "apply", TYP, VariadicT, TYP, Std$Function$T, ANY
;@args
;@more...
	mov ebx, [Std$Function_argument(edi).Val]
	push dword [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Integer_smallt(variadic(ebx).Length).Value]
	lea eax, [eax + esi - 2]
	lea eax, [8 * eax]
	push eax
	call Riva$Memory$_alloc
	pop ecx
	lea edx, [esi - 2]
	lea ecx, [edx * 2]
	lea esi, [edi + 16]
	mov edi, eax
	rep movsd
	mov ecx, [Std$Integer_smallt(variadic(ebx).Length).Value]
	add edx, ecx
	lea esi, [variadic(ebx).Args]
	add ecx, ecx
	rep movsd
	pop ecx
	mov esi, edx
	mov edi, eax
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

method "apply", TYP, Std$Function$T, TYP, VariadicT, ANY
;@args
;@more...
	push dword [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Integer_smallt(variadic(ebx).Length).Value]
	lea eax, [eax + esi - 2]
	lea eax, [8 * eax]
	push eax
	call Riva$Memory$_alloc
	pop ecx
	push edi
	push esi
	mov edi, eax
	mov edx, [Std$Integer_smallt(variadic(ebx).Length).Value]
	lea esi, [variadic(ebx).Args]
	lea ecx, [edx * 2]
	rep movsd
	pop ecx
	pop esi
	sub ecx, byte 2
	add esi, byte 16
	add edx, ecx
	add ecx, ecx
	rep movsd
	pop ecx
	mov esi, edx
	mov edi, eax
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

method "prepend", TYP, VariadicT, ANY
;@args
;@more...
; Creates a copy of <var>args</var> with the extra arguments <var>more...</var> at the beginning
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(variadic(ebx).Length).Value]
	lea eax, [eax + esi - 1]
	lea eax, [sizeof(variadic) + 8 * eax]
	push eax
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword VariadicT
	mov [Std$Object_t(variadic(eax).Length).Type], dword Std$Integer$SmallT
	lea ecx, [esi - 1]
	mov [Std$Integer_smallt(variadic(eax).Length).Value], ecx
	add ecx, ecx
	lea esi, [edi + 8]
	lea edi, [variadic(eax).Args]
	rep movsd
	mov ecx, [Std$Integer_smallt(variadic(ebx).Length).Value]
	add [Std$Integer_smallt(variadic(eax).Length).Value], ecx
	lea esi, [variadic(ebx).Args]
	add ecx, ecx
	rep movsd
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

method "append", TYP, VariadicT, ANY
;@args
;@more...
; Creates a copy of <var>args</var> with the extra arguments <var>more...</var> at the end
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(variadic(ebx).Length).Value]
	lea eax, [eax + esi - 1]
	lea eax, [sizeof(variadic) + 8 * eax]
	push eax
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword VariadicT
	mov [Std$Object_t(variadic(eax).Length).Type], dword Std$Integer$SmallT
	push edi
	push esi
	lea edi, [variadic(eax).Args]
	mov ecx, [Std$Integer_smallt(variadic(ebx).Length).Value]
	mov [Std$Integer_smallt(variadic(eax).Length).Value], ecx
	lea esi, [variadic(ebx).Args]
	add ecx, ecx
	rep movsd
	pop ecx
	pop esi
	dec ecx
	add [Std$Integer_smallt(variadic(eax).Length).Value], ecx
	add esi, byte 8
	add ecx, ecx
	rep movsd
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

struct values_state, Std$Function_state
	.Current:	resd 1
	.Left:		resd 1
endstruct

method "values", TYP, VariadicT
;@args
;:ANY
; Generates the arguments in <var>args</var>.
	mov edi, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(variadic(edi).Length).Value]
	dec ebx
	js .failure
	lea edi, [variadic(edi).Args]
	jz .return
	push byte sizeof(values_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov dword [Std$Function_state(eax).Run], .resume
	mov [values_state(eax).Left], ebx
.suspend:
	mov [values_state(eax).Current], edi
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	mov ebx, eax
	or eax, byte -1
	ret
.return:
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret
.resume:
	mov edi, [values_state(eax).Current]
	add edi, byte sizeof(Std$Function_argument)
	dec dword [values_state(eax).Left]
	js .failure
	jz .return
	jmp .suspend

struct values_keys_state, Std$Function_state
	.CurrentValue:	resd 1
	.CurrentKey:	resd 1
	.KeyVar:		resd 1
	.Left:			resd 1
endstruct

method "values", TYP, VariadicT, TYP, Std$Symbol$ArrayT, ANY
;@values
;@keys
;@key+
;:ANY
; Generates the arguments in <var>args</var>, while filling <var>key</var> from <var>keys</var> in parallel.
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov edx, [Std$Function_argument(edi + 16).Ref]
	mov edi, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(variadic(edi).Length).Value]
	cmp ebx, [Std$Symbol_arrayt(ecx).Count]
	jne .invalid_assign
	lea esi, [Std$Symbol_arrayt(ecx).Values]
	dec ebx
	js .failure
	test edx, edx
	jz .invalid_assign
	lea edi, [variadic(edi).Args]
	jz .return
	push edx
	push byte sizeof(values_keys_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop edx
	mov dword [Std$Function_state(eax).Run], .resume
	mov [values_keys_state(eax).Left], ebx
	mov [values_keys_state(eax).KeyVar], edx
.suspend:
	mov [values_keys_state(eax).CurrentKey], esi
	mov [values_keys_state(eax).CurrentValue], edi
	mov ecx, [esi]
	mov [edx], ecx
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	mov ebx, eax
	or eax, byte -1
	ret
.return:
	mov ecx, [esi]
	mov [edx], ecx
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret
.resume:
	mov edi, [values_keys_state(eax).CurrentValue]
	add edi, byte sizeof(Std$Function_argument)
	mov esi, [values_keys_state(eax).CurrentKey]
	add esi, byte 4
	mov edx, [values_keys_state(eax).KeyVar]
	dec dword [values_keys_state(eax).Left]
	js .failure
	jz .return
	jmp .suspend
.invalid_assign:
	mov ecx, IncorrectAssignMessage
	xor edx, edx
	mov eax, 2
	ret

 method "values", TYP, VariadicT, VAL, Std$Object$Nil, ANY
	xor eax, eax
	inc eax
	ret

cfunction run_state
	push ebp
	mov ebp, eax
	mov ecx, [bstate(eax).Val]
	mov edx, [bstate(eax).Ref]
	jmp [bstate(eax).Code]

struct trap
	.State:		resd 1
	.Run:		resd 1
endstruct

extern compile_prefunc

ctype WraplPreT, Std$Function$T
.invoke:
	push ecx
	push dword [closure(ecx).Entry]
	call compile_prefunc
	pop ecx
	pop ecx
	mov dword [Std$Object_t(ecx).Type], WraplT
	jmp [closure(ecx).Entry]

ctype WraplT, Std$Function$T
.invoke:
	jmp [closure(ecx).Entry]

global backtrack
textsect
invoke_suspend:
	mov [Std$Function_state(ebx).Resume], esi
	mov eax, [trap(edi).State]
	mov [Std$Function_state(ebx).Chain], eax
	mov [trap(edi).State], ebx
invoke_return:
	;TRAP TEST!!!
	;jmp esi
	ret
	
invoke_message:
	pop esi
	jmp [bstate(ebp).Handler]	

invoke_backtrack:
	pop esi
backtrack:
	mov eax, [trap(edi).State]
	test eax, eax
	jz .failure
	mov esi, [Std$Function_state(eax).Chain]
	mov [trap(edi).State], esi
	push dword [Std$Function_state(eax).Resume]
	jmp [Std$Function_state(eax).Run]
	
.failure:
	jmp [trap(edi).Run]

invoke_message_debug:
	pop esi
	push eax
	push edx
	push ecx
	push dword 4
	push dword 0
	push dword [dstate(ebp).Function]
	mov eax, [Debugger]
	call [debugger(eax).break_line]
	add esp, 12
	pop ecx
	pop edx
	pop eax
	jmp [bstate(ebp).Handler]

global return_table
datasect
	dd invoke_suspend
return_table:
	dd invoke_return
	dd invoke_backtrack
	dd invoke_message

extern Debugger
global return_table_debug
datasect
	dd invoke_suspend
return_table_debug:
	dd invoke_return
	dd invoke_backtrack
	dd invoke_message_debug

cfunction send_message
	jmp dword [bstate(ebp).Handler]

cfunction resend_message
	;mov eax, 2
	pop ebp
	ret

cfunction alloc_local
	push byte 4
	call Riva$Memory$_alloc
	pop ecx
;	mov [eax], dword Std$Object$Nil
	ret

struct exec, Std$Object_t
	.$esp:	resd 1
	.$ebp:	resd 1
	.$eip:	resd 1
endstruct

ctype ExecT
; Marks a state in the execution of a Wrapl function that can be restarted.
; Calling an instance of <id>ExecT</id> will restart execution at the state stored in it.
.invoke:
	mov ebx, [exec(ecx).$eip]
	test ebx, ebx
	jz .fail
	mov esp, [exec(ecx).$esp]
	mov ebp, [exec(ecx).$ebp]
	mov [esp], ebx
	xor edx, edx
	mov [exec(ecx).$eip], edx
	mov ecx, Std$Object$Nil
	cmp esi, byte 1
	jbe .no_arg
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
.no_arg:
	xor eax, eax
	ret
.fail:
	mov eax, 1
	ret

function ExecNew, 0
;:ExecT
; Returns a new empty <id>ExecT</id>.
	push byte sizeof(exec)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword ExecT
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

function ExecMark, 1
;@exec:ExecT
; Stores the current execution state into <var>exec</var>.
; Fails when called initially, succeeds if restarted by <id>ExecJump</id>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [esp]
	mov [exec(eax).$eip], ebx
	mov [exec(eax).$ebp], ebp
	mov [exec(eax).$esp], esp
	mov eax, 1
	ret

method "mark", TYP, ExecT
;@exec
; Stores the current execution state into <var>exec</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [esp]
	mov [exec(eax).$eip], ebx
	mov [exec(eax).$ebp], ebp
	mov [exec(eax).$esp], esp
	mov eax, 1
	ret

method "reset", TYP, ExecT
;@exec
; Resets <var>exec</var>.
	mov eax, [Std$Function_argument(edi).Val]
	xor edx, edx
	mov [exec(eax).$eip], edx
	mov ecx, eax
	xor eax, eax
	ret

function ExecJump, 1
;@exec:ExecT
;@arg:ANY=NIL
; Restarts current execution from the state in <var>exec</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [exec(eax).$eip]
	test ebx, ebx
	jz .fail
	mov esp, [exec(eax).$esp]
	mov ebp, [exec(eax).$ebp]
	mov [esp], ebx
	mov ecx, dword Std$Object$Nil
	xor edx, edx
	mov [exec(eax).$eip], edx
	cmp esi, byte 2
	jbe .no_arg
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov edx, [Std$Function_argument(edi + 8).Ref]
.no_arg:
	xor eax, eax
	ret
.fail:
	mov eax, 1
	ret

ctype IncorrectTypeMessageT
.invoke: equ 0

amethod Std$String$Of, TYP, IncorrectTypeMessageT
	mov ecx, .String
	xor edx, edx
	xor eax, eax
	ret
section .data
.String:
	dd Std$String$T
	dd 1
	dd Std$Integer$SmallT, 25
	dd 0, 0
	dd Std$Integer$SmallT, 25
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
	db "Object has incorrect type", 0

cdata IncorrectTypeMessage
	dd IncorrectTypeMessageT

cfunction incorrect_type
	mov ecx, IncorrectTypeMessage
	xor edx, edx
	mov eax, 2
	jmp [bstate(ebp).Handler]


ctype IncorrectAssignMessageT
.invoke: equ 0

amethod Std$String$Of, TYP, IncorrectAssignMessageT
	mov ecx, .String
	xor edx, edx
	xor eax, eax
	ret
section .data
.String:
	dd Std$String$T
	dd 1
	dd Std$Integer$SmallT, 24
	dd 0, 0
	dd Std$Integer$SmallT, 24
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
	db "Assignment to location 0", 0

cdata IncorrectAssignMessage
	dd IncorrectAssignMessageT

cdata incorrect_assign
	mov ecx, IncorrectAssignMessage
	xor edx, edx
	mov eax, 2
	jmp [bstate(ebp).Handler]

struct select_string_case
	.Length: resd 1
	.String: resd 1
	.Jump: resd 1
endstruct

cfunction select_string
;	int3
	pop edx
	mov eax, [Std$Object_t(ecx).Type]
	mov eax, [Std$Type_t(eax).Types]
.type_check:
	cmp [eax], dword Std$String$T
	je .type_ok
	cmp [eax], dword 0
	je incorrect_type
	add eax, byte 4
	jmp .type_check
.type_ok:
;	add edx, byte 0x03
;	and edx, byte 0xFC
	push ebp
	mov ebp, esp
	cmp [Std$String_t(ecx).Count], dword 1
	mov ebx, [Std$Address_t(Std$String_block(Std$String_t(ecx).Blocks).Chars).Value]
	mov eax, [Std$Integer_smallt(Std$String_block(Std$String_t(ecx).Blocks).Length).Value]
        ;int3
	jbe .simplestring
	;jb .default
	;je .simplestring
	mov eax, [Std$Integer_smallt(Std$String_t(ecx).Length).Value]
	sub esp, eax
	and esp, byte -4
	mov edi, esp
	lea ebx, [Std$String_t(ecx).Blocks]
.copyloop:
	mov ecx, [Std$Integer_smallt(Std$String_block(ebx).Length).Value]
	jecxz .copied
	mov esi, [Std$Address_t(Std$String_block(ebx).Chars).Value]
	rep movsb
	add ebx, byte sizeof(Std$String_block)
	jmp .copyloop
.copied:
	mov ebx, esp
.simplestring:
	; ebx = string
	; eax = length
	push edx
;	test eax, eax
;	jz .default
.loop:
	add edx, byte sizeof(select_string_case)
	mov ecx, [select_string_case(edx).Length]
	cmp ecx, eax
	ja .loop
	jb .default
	mov esi, [select_string_case(edx).String]
	mov edi, ebx
	repz cmpsb
	jnz .loop
	mov esp, ebp
	pop ebp
	jmp [select_string_case(edx).Jump]
.default:
	pop edx
	mov esp, ebp
	pop ebp
	jmp [select_string_case(edx).Jump]

cdata CmovSupport
	dd 0

cfunction detect_cpu_features
	push ebx
	mov eax, 0x1
	cpuid
	and edx, 0x8000
	setnz byte [CmovSupport]
	pop ebx
	ret

struct code, Std$Object_t
	.Entry:		resd 1
	.Frame:		resd 1
endstruct

ctype CodeT, Std$Function$T
.invoke:
	push ebp
	mov ebp, [code(ecx).Frame]
	xor edx, edx
	jmp [code(ecx).Entry]


extern debug_enter_impl
cfunction debug_enter
	mov [bstate(ebp).Val], ecx
	mov [bstate(ebp).Ref], edx
	push ebp
	call debug_enter_impl
	add esp, byte 4
	mov edx, [bstate(ebp).Ref]
	mov ecx, [bstate(ebp).Val]
	ret

extern debug_break_impl
cfunction debug_break
	mov [bstate(ebp).Val], ecx
	mov [bstate(ebp).Ref], edx
	push eax
	push ebp
	call debug_break_impl
	add esp, byte 8
	mov edx, [bstate(ebp).Ref]
	mov ecx, [bstate(ebp).Val]
	ret

extern debug_message_impl
cfunction debug_message
	mov [bstate(ebp).Val], ecx
	mov [bstate(ebp).Ref], edx
	push ecx
	push eax
	push ebp
	call debug_message_impl
	add esp, byte 12
	mov edx, [bstate(ebp).Ref]
	mov ecx, [bstate(ebp).Val]
	ret

extern debug_exit_impl
cfunction debug_exit
	mov [bstate(ebp).Val], ecx
	mov [bstate(ebp).Ref], edx
	push ebp
	call debug_exit_impl
	add esp, byte 4
	mov edx, [bstate(ebp).Ref]
	mov ecx, [bstate(ebp).Val]
	ret