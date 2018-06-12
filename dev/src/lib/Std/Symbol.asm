%include "Std.inc"
%include "Riva/Memory.inc"

%define KEY_ROTATION 5
%define INCR_ROTATION 2

struct __arg__
	.Types:	resd 1
	.Value:	resd 1
endstruct

struct typeentry
	.Type:	resd 1
	.Next:	resd 1
endstruct

struct valueentry
	.Value:	resd 1
	.Next:	resd 1
endstruct

%define MATCH_VALUE 20
%define MATCH_TYPE 16
%define MATCH_SUBTYPE 12
%define MATCH_ANY 5

%macro _proceed_ 1
%define _arg(N) __arg__(ebp - 8 * N)
%define _bestscore ebp + 4
%define _bestfunction ebp
align 8
proceed %+ %1:;(index, score, table)
	;mov eax, [esp + 8]
	mov edi, [Std$Symbol_t(eax).ValueSize]
	dec edi
	js %%continue0a
	mov ebx, [_arg(%1).Value]
	mov edx, [Std$Symbol_t(eax).ValueTable]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov esi, ebx
	ror esi, INCR_ROTATION
	mov eax, ebx
%%searchloop0:
	lea eax, [eax + 2 * esi + 1]
	and eax, edi
	cmp [valueentry(edx + sizeof(valueentry) * eax).Value], ebx
	jb %%continue0
	ja %%searchloop0
	mov eax, [valueentry(edx + sizeof(valueentry) * eax).Next]
	mov ebx, [esp + 8]
	add ebx, byte MATCH_VALUE
	mov edi, [Std$Symbol_t(eax).Function]
	test edi, edi
%if %1 = 1
	jz %%continue0
%else
	jz %%nofunction0
%endif
	cmp ebx, [_bestscore]
	jna %%nofunction0
	mov [_bestscore], ebx
	mov [_bestfunction], edi
%%nofunction0:
%if %1 = 1
	xor eax, eax
	ret 8
%else
	push eax
	push ebx
%assign %%NEXT %1 - 1
	call proceed %+ %%NEXT
	cmp eax, byte 2
	jae %%continue0
	ret 8
%endif
%%continue0:
	mov eax, [esp + 8]
%%continue0a:
	mov edi, [Std$Symbol_t(eax).TypeSize]
	dec edi
	js near %%continue2
	mov ecx, [_arg(%1).Types]
	push ecx
	mov ebx, [ecx]
	mov edx, [Std$Symbol_t(eax).TypeTable]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov esi, ebx
	ror esi, INCR_ROTATION
	mov eax, ebx
%%searchloop1:
	lea eax, [eax + 2 * esi + 1]
	and eax, edi
	cmp [typeentry(edx + sizeof(typeentry) * eax).Type], ebx
	jb %%continue1
	ja %%searchloop1
	mov eax, [typeentry(edx + sizeof(typeentry) * eax).Next]
	mov ebx, [esp + 8]
	add ebx, byte MATCH_TYPE
	mov ecx, [Std$Symbol_t(eax).Function]
%if %1 = 1
	jecxz %%continue1
%else
	jecxz %%nofunction1
%endif
	cmp ebx, [_bestscore]
	jna %%nofunction1
	mov [_bestscore], ebx
	mov [_bestfunction], ecx
%%nofunction1:
%if %1 = 1
	xor eax, eax
	add esp, byte 4
	ret 8
%else
	push eax
	push ebx
%assign %%NEXT %1 - 1
	call proceed %+ %%NEXT
	cmp eax, byte 2
	jae %%continue1
	add esp, byte 4
	ret 8
%endif
%%continue1:
	pop edx
	add edx, byte 4
	mov ecx, [edx]
	jecxz %%continue2
	push edx
	mov eax, [esp + 12]
	mov ebx, ecx
	mov edi, [Std$Symbol_t(eax).TypeSize]
	dec edi
	mov edx, [Std$Symbol_t(eax).TypeTable]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov esi, ebx
	ror esi, INCR_ROTATION
	mov eax, ebx
%%searchloop2:
	lea eax, [eax + 2 * esi + 1]
	and eax, edi
	cmp [typeentry(edx + sizeof(typeentry) * eax).Type], ebx
	jb %%continue1
	ja %%searchloop2
	mov eax, [typeentry(edx + sizeof(typeentry) * eax).Next]
	mov ebx, [esp + 8]
	add ebx, byte MATCH_SUBTYPE
	mov ecx, [Std$Symbol_t(eax).Function]
%if %1 = 1
	jecxz %%continue1
%else
	jecxz %%nofunction2
%endif
	cmp ebx, [_bestscore]
	jna %%nofunction2
	mov [_bestscore], ebx
	mov [_bestfunction], ecx
%%nofunction2:
%if %1 = 1
	mov eax, 1
	add esp, byte 4
	ret 8
%else
	push eax
	push ebx
%assign %%NEXT %1 - 1
	call proceed %+ %%NEXT
	cmp eax, byte 2
	jae %%continue1
	inc eax
	add esp, byte 4
	ret 8
%endif
%%continue2:
	mov ecx, [esp + 8]
	mov eax, [Std$Symbol_t(ecx).Skip]
	test eax, eax
	jz %%continue3
	mov ebx, [esp + 4]
	add ebx, byte MATCH_ANY
	mov ecx, [Std$Symbol_t(eax).Function]
	jecxz %%nofunction3
	cmp ebx, [_bestscore]
	jna %%nofunction3
	mov [_bestscore], ebx
	mov [_bestfunction], ecx
%%nofunction3:
%if %1 = 1
	mov eax, 2
	ret 8
%else
	push eax
	push ebx
%assign %%NEXT %1 - 1
	call proceed %+ %%NEXT
	inc eax
	ret 8
%endif
%%continue3:
	mov eax, 2
	ret 8
%endmacro

datasect
Proceed:
	dd 0
%assign i 1
%rep 64
	dd proceed %+ i
	%assign i i + 1
%endrep

ctype T, Std$Function$T
; The type of all symbols (a.k.a. multimethods).
cfunction T.invoke
	push esi
	push edi
	mov ebx, esi
	dec esi
	js .noargs
	push ecx
	push ebp
	push byte 0
	push dword [Std$Symbol_t(ecx).Function]
	mov ebp, esp
.argloop:
	mov eax, [Std$Function_argument(edi + 8 * esi).Val]
	mov edx, [Std$Object_t(eax).Type]
	push eax
	push dword [Std$Type_t(edx).Types]
	dec esi
	jns .argloop
	mov eax, ecx
	push eax
	push byte 0
	call [Proceed + 4 * ebx]
	mov esp, ebp
%if 0
	cmp eax, byte 16
	jae .not_field
	mov eax, [esp + 12]
	mov ebx, [__arg__(ebp - 8).Value]
	mov edi, [Std$Object_t(ebx).Type]
	mov edi, [Std$Type_t(edi).Fields]
	mov ecx, [Std$Integer_smallt(Std$Array_t(edi).Length).Value]
	jecxz .not_field
	mov edx, [Std$Array_t(edi).Values]
.field_loop:
	add edx, byte 4
	cmp [edx - 4], eax
	je .field
	dec ecx
	jnz .field_loop
.not_field:
	pop ecx
	pop eax
	pop ebp
	pop ebx
	pop edi
	pop esi
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]
.field:
	sub edx, [Std$Array_t(edi).Values]
	add edx, ebx
	pop ecx
	pop eax
	pop ebp
	add esp, byte 12
	mov ecx, [edx]
	xor eax, eax
	ret
%else
.not_field:
	pop ecx
	pop eax
	pop ebp
	pop ebx
	pop edi
	pop esi
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]
%endif
.noargs:
	mov ebx, ecx
	mov ecx, [Std$Symbol_t(ecx).Function]
	pop edi
	pop esi
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]
%assign i 1
%rep 64
	_proceed_ i
	%assign i i + 1
%endrep

ctype ArrayT
.invoke: equ 0

ctype NoMethodMessageT
; An instance of this type is sent when no signature is found to match a method call.
.invoke: equ 0

global nomethod.invoke
extern Riva$Debug$_stack_trace

local_function nomethod
.invoke:
	;int3
	push byte sizeof(Std$Symbol_nomethodmessage)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov [Std$Object_t(eax).Type], dword NoMethodMessageT
	mov [Std$Symbol_nomethodmessage(eax).Symbol], ebx
	lea eax, [Std$Symbol_nomethodmessage(eax).Stack]
	push dword 12
	lea eax, [Std$Symbol_nomethodmessage(eax).Stack]
	push eax
	lea eax, [esp + 12]
	push eax
	call Riva$Debug$_stack_trace
	add esp, byte 12
	pop ecx
	mov [Std$Symbol_nomethodmessage(ecx).Count], eax
	xor edx, edx
	mov eax, 2
	ret

_function New
;:T
; Returns a new unnamed symbol
	push byte sizeof(Std$Symbol_t)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	mov dword [Std$Symbol_t(eax).Name], .anonymous_str
;	mov dword [Std$Symbol_t(eax).TypeTable], 0
;	mov dword [Std$Symbol_t(eax).TypeSpace], 0
;	mov dword [Std$Symbol_t(eax).TypeSize], 0
;	mov dword [Std$Symbol_t(eax).ValueTable], 0
;	mov dword [Std$Symbol_t(eax).ValueSpace], 0
;	mov dword [Std$Symbol_t(eax).ValueSize], 0
	mov dword [Std$Symbol_t(eax).Function], defaultmethod ;nomethod
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
section .data
.anonymous_str:
	dd Std$String$T
	dd Std$Integer$SmallT
	dd 6, 1
	dd Std$Integer$SmallT, 6
	dd Std$Address$T, .anonymous
	dd 0, 0, 0, 0
.anonymous:
	db "<anon>", 0, 0

cfunction _new_string
	push byte sizeof(Std$Symbol_t)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov [Std$Object_t(eax).Type], dword T
	push dword [esp + 8]
	call Std$String$_new
	mov ecx, eax
	pop eax
	pop eax
	mov dword [Std$Symbol_t(eax).Name], ecx
;	mov dword [Std$Symbol_t(eax).TypeTable], 0
;	mov dword [Std$Symbol_t(eax).TypeSpace], 0
;	mov dword [Std$Symbol_t(eax).TypeSize], 0
;	mov dword [Std$Symbol_t(eax).ValueTable], 0
;	mov dword [Std$Symbol_t(eax).ValueSpace], 0
;	mov dword [Std$Symbol_t(eax).ValueSize], 0
	mov dword [Std$Symbol_t(eax).Function], defaultmethod
	ret

cfunction _new
	push byte sizeof(Std$Symbol_t)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
;	mov dword [Std$Symbol_t(eax).TypeTable], 0
;	mov dword [Std$Symbol_t(eax).TypeSpace], 0
;	mov dword [Std$Symbol_t(eax).TypeSize], 0
;	mov dword [Std$Symbol_t(eax).ValueTable], 0
;	mov dword [Std$Symbol_t(eax).ValueSpace], 0
;	mov dword [Std$Symbol_t(eax).ValueSize], 0
; To me: leave the 0 on the next line ALONE!!!!
;	mov dword [Std$Symbol_t(eax).Function], 0
	ret

symbol ?IN, "in"
symbol ?IS, "is"

_function Set
;@symbol:T
;@function:Std$Function$T
;@signature...
;:Std$Function$T
; Adds a new entry to <var>symbol</var> which matches <var>signature</var> to <var>function</var>.
; Returns <var>function</var>.
	push ebp
	shr esi, 1
	push dword [Std$Function_argument(edi + 8).Val]
	dec esi
	mov eax, [Std$Function_argument(edi).Val]
	jz .notypes
	push esi
.next:
	mov ebx, [Std$Function_argument(edi).Val]
	add edi, byte 16
	mov ebx, [Std$Function_argument(edi).Val]
	push edi
	push dword [Std$Function_argument(edi + 8).Val]
	push eax
	cmp ebx, ?IN
	je .type
	cmp ebx, ?IS
	je .value
.skip:
	add esp, byte 8
	mov ecx, eax
	mov eax, [Std$Symbol_t(eax).Skip]
	test eax, eax
	jnz .done
	push ecx
	call _new
	pop ecx
	mov [Std$Symbol_t(ecx).Skip], eax
	jmp .done
.value:
	call valuetable_put
	jmp .done
.type:
	call typetable_put
.done:
	pop edi
	dec dword [esp]
	jnz .next
	add esp, byte 4
.notypes:
	pop ecx
	mov [Std$Symbol_t(eax).Function], ecx
	xor edx, edx
	xor eax, eax
	pop ebp
	ret

function Get, 1
;@symbol:T
;@signature...
;:Std$Function$T
; Returns the entry in <var>symbol</var> which matches <var>signature</var> to <var>function</var> or fails if no matching entry is found.
	;int3
	mov eax, [Std$Function_argument(edi).Val]
	add edi, byte 8
	push edi
	push esi
	dec dword [esp]
	jz .finished
.loop:
	mov edx, [esp + 4]
	add [esp + 4], dword 16
	mov ebx, [Std$Function_argument(edx).Val]
	cmp ebx, ?IN
	je .type
	cmp ebx, ?IS
	je .value
.skip:
	mov eax, [Std$Symbol_t(eax).Skip]
	test eax, eax
	jz .failure
	dec dword [esp]
	jz .failure
	dec dword [esp]
	jz .finished
	jmp .loop
.type:
	dec dword [esp]
	jz .failure
	mov edi, [Std$Symbol_t(eax).TypeSize]
	dec edi
	js .failure
	mov ebx, [Std$Function_argument(edx + 8).Val]
	mov edx, [Std$Symbol_t(eax).TypeTable]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov esi, ebx
	ror esi, INCR_ROTATION
	mov eax, ebx
.searchloop1:
	lea eax, [eax + 2 * esi + 1]
	and eax, edi
	cmp [typeentry(edx + sizeof(typeentry) * eax).Type], ebx
	jb .failure
	ja .searchloop1
	mov eax, [typeentry(edx + sizeof(typeentry) * eax).Next]
	dec dword [esp]
	jz .finished
	jmp .loop
.value:
	dec dword [esp]
	jz .failure
	mov edi, [Std$Symbol_t(eax).ValueSize]
	dec edi
	js .failure
	mov ebx, [Std$Function_argument(edx + 8).Val]
	mov edx, [Std$Symbol_t(eax).ValueTable]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov esi, ebx
	ror esi, INCR_ROTATION
	mov eax, ebx
.searchloop2:
	lea eax, [eax + 2 * esi + 1]
	and eax, edi
	cmp [valueentry(edx + sizeof(valueentry) * eax).Value], ebx
	jb .failure
	ja .searchloop2
	mov eax, [valueentry(edx + sizeof(valueentry) * eax).Next]
	dec dword [esp]
	jz .finished
	jmp .loop
.finished:
	add esp, byte 8
	lea edx, [Std$Symbol_t(eax).Function]
	mov ecx, [Std$Symbol_t(eax).Function]
	jecxz .failure2
	xor eax, eax
	ret
.failure:
	add esp, byte 8
.failure2:
	xor eax, eax
	inc eax
	ret

cfunction _add_methods;(methods)
	;int3
	push ebp
	push esi
	push edi
	push ebx
	mov edi, [esp + 20]
.loop:
	mov eax, [edi]
	add edi, byte 4
	test eax, eax
	jz .loop
	cmp eax, byte -1
	jne .add
	pop ebx
	pop edi
	pop esi
	pop ebp
	ret
.add:
	mov ebx, [edi]
	add edi, byte 4
	jmp [.select + 4 * ebx]
.end:
	mov ebx, [edi]
	add edi, byte 4
	mov [Std$Symbol_t(eax).Function], ebx
	jmp .loop
.skp:
	mov ecx, eax
	mov eax, [Std$Symbol_t(eax).Skip]
	test eax, eax
	jnz .add
	push ecx
	call _new
	pop ecx
	mov [Std$Symbol_t(ecx).Skip], eax
	jmp .add
.val:
	add edi, byte 4
	push edi
	push dword [edi - 4]
	push eax
	call valuetable_put
	pop edi
	jmp .add
.typ:
	add edi, byte 4
	push edi
	push dword [edi - 4]
	push eax
	call typetable_put
	pop edi
	jmp .add
datasect
.select:
	dd .end
	dd .skp
	dd .val
	dd .typ

cfunction _typetable_put;(table, type)
typetable_put:
	mov ebx, [esp + 8]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov ecx, ebx
	ror ecx, INCR_ROTATION
	mov esi, [esp + 4]
	mov edx, [Std$Symbol_t(esi).TypeSize]
	dec edx
	js .empty_table
	mov ebp, [Std$Symbol_t(esi).TypeTable]
	mov eax, ebx
	; ecx = increment
	; ebx = hash/key
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [typeentry(ebp + 8 * eax).Type], ebx
	jb .not_present
	ja .search_loop
	mov eax, [typeentry(ebp + 8 * eax).Next]
	ret 8
.empty_table:
	; ebx = hash/key
	; ecx = increment
	push ecx
	push byte 24
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	mov [Std$Symbol_t(esi).TypeSize], dword 2
	mov [Std$Symbol_t(esi).TypeSpace], dword 1
	mov [Std$Symbol_t(esi).TypeTable], eax
	lea edi, [ebx + 2 * ecx + 1]
	mov esi, eax
	call _new
	and edi, byte 1
	mov [typeentry(esi + 8 * edi).Type], ebx
	mov [typeentry(esi + 8 * edi).Next], eax
	ret 8
.not_present:
	dec dword [Std$Symbol_t(esi).TypeSpace]
	jle near .grow_table
	mov edi, eax
	mov esi, edx
	call _new
	push eax
	cmp [typeentry(ebp + 8 * edi).Type], dword 0
	jne .replace_node
.empty_node:
	mov [typeentry(ebp + 8 * edi).Type], ebx
	mov [typeentry(ebp + 8 * edi).Next], eax
	pop eax
	ret 8
.replace_loop:
	lea edi, [edi + 2 * ecx + 1]
	and edi, esi
	cmp [typeentry(ebp + 8 * edi).Type], dword 0
	je .empty_node
	cmp [typeentry(ebp + 8 * edi).Type], ebx
	ja .replace_loop
.replace_node:
	push dword [typeentry(ebp + 8 * edi).Type]
	push dword [typeentry(ebp + 8 * edi).Next]
	mov [typeentry(ebp + 8 * edi).Type], ebx
	mov [typeentry(ebp + 8 * edi).Next], eax
	pop eax
	pop ebx
	mov ecx, ebx
	ror ecx, INCR_ROTATION
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	call _new
	push eax
	push esi
	mov edx, [Std$Symbol_t(esi).TypeSize]
	shl edx, 3
	push eax
	push ebx
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [typeentry(ebp).Type]
	mov ebx, [typeentry(ebp).Next]
	call .sort_section
	pop esi
	mov edx, [Std$Symbol_t(esi).TypeSize]
	add edx, edx
	push edx
	inc edx
	shl edx, 3
	push edx
	call Riva$Memory$_alloc
	pop edx
	pop edx
	mov [Std$Symbol_t(esi).TypeSize], edx
	mov [Std$Symbol_t(esi).TypeTable], eax
	mov [Std$Symbol_t(esi).TypeSpace], edx
	dec edx
	mov ebx, [typeentry(ebp).Type]
.entry_loop:
	dec dword [Std$Symbol_t(esi).TypeSpace]
	mov edi, ebx
	mov ecx, edi
	ror ecx, INCR_ROTATION
.find_slot_loop:
	lea edi, [edi + 2 * ecx + 1]
	and edi, edx
	cmp [typeentry(eax + 8 * edi).Type], dword 0
	jne .find_slot_loop
	mov ecx, [typeentry(ebp).Next]
	mov [typeentry(eax + 8 * edi).Type], ebx
	mov [typeentry(eax + 8 * edi).Next], ecx
	add ebp, byte 8
	mov ebx, [typeentry(ebp).Type]
	test ebx, ebx
	jne .entry_loop
	pop eax
	ret 8
.sort_section:
	; eax-edx = current node
	; node(esp + 4) = pivot node
	; edi = bottom free slot
	; esi = top free slot
	push edi
	push esi
.sort_loop:
	cmp eax, [typeentry(esp + 12).Type]
	ja .add_to_bottom
	mov [typeentry(esi).Type], eax
	mov [typeentry(esi).Next], ebx
	sub esi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [typeentry(esi).Type]
	mov ebx, [typeentry(esi).Next]
	jmp .sort_loop
.add_to_bottom:
	mov [typeentry(edi).Type], eax
	mov [typeentry(edi).Next], ebx
	add edi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [typeentry(edi).Type]
	mov ebx, [typeentry(edi).Next]
	jmp .sort_loop
.finished_section:
	mov eax, [typeentry(esp + 12).Type]
	mov ebx, [typeentry(esp + 12).Next]
	mov [typeentry(edi).Type], eax
	mov [typeentry(edi).Next], ebx
	pop esi
	push edi
	add edi, byte 8
	cmp esi, edi
	jbe .no_recurse_1
	push dword [typeentry(edi).Next]
	push dword [typeentry(edi).Type]
	mov eax, [typeentry(esi).Type]
	mov ebx, [typeentry(esi).Next]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 8
	cmp esi, edi
	jbe .no_recurse_2
	push dword [typeentry(edi).Next]
	push dword [typeentry(edi).Type]
	mov eax, [typeentry(esi).Type]
	mov ebx, [typeentry(esi).Next]
	call .sort_section
.no_recurse_2:
	ret 8

cfunction _valuetable_put;(table, value)
valuetable_put:
	mov ebx, [esp + 8]
	lea ebx, [9 * ebx]
	ror ebx, KEY_ROTATION
	mov ecx, ebx
	ror ecx, INCR_ROTATION
	mov esi, [esp + 4]
	mov edx, [Std$Symbol_t(esi).ValueSize]
	dec edx
	js .empty_table
	mov ebp, [Std$Symbol_t(esi).ValueTable]
	mov eax, ebx
	; ecx = increment
	; ebx = hash/key
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [valueentry(ebp + 8 * eax).Value], ebx
	jb .not_present
	ja .search_loop
	mov eax, [valueentry(ebp + 8 * eax).Next]
	ret 8
.empty_table:
	; ebx = hash/key
	; ecx = increment
	push ecx
	push byte 24
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	mov [Std$Symbol_t(esi).ValueSize], dword 2
	mov [Std$Symbol_t(esi).ValueSpace], dword 1
	mov [Std$Symbol_t(esi).ValueTable], eax
	lea edi, [ebx + 2 * ecx + 1]
	mov esi, eax
	call _new
	and edi, byte 1
	mov [typeentry(esi + 8 * edi).Type], ebx
	mov [typeentry(esi + 8 * edi).Next], eax
	ret 8
.not_present:
	dec dword [Std$Symbol_t(esi).ValueSpace]
	jle near .grow_table
	mov edi, eax
	mov esi, edx
	call _new
	push eax
	cmp [valueentry(ebp + 8 * edi).Value], dword 0
	jne .replace_node
.empty_node:
	mov [valueentry(ebp + 8 * edi).Value], ebx
	mov [valueentry(ebp + 8 * edi).Next], eax
	pop eax
	ret 8
.replace_loop:
	lea edi, [edi + 2 * ecx + 1]
	and edi, esi
	cmp [valueentry(ebp + 8 * edi).Value], dword 0
	je .empty_node
	cmp [valueentry(ebp + 8 * edi).Value], ebx
	ja .replace_loop
.replace_node:
	push dword [valueentry(ebp + 8 * edi).Value]
	push dword [valueentry(ebp + 8 * edi).Next]
	mov [valueentry(ebp + 8 * edi).Value], ebx
	mov [valueentry(ebp + 8 * edi).Next], eax
	pop eax
	pop ebx
	mov ecx, ebx
	ror ecx, INCR_ROTATION
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	call _new
	push eax
	push esi
	mov edx, [Std$Symbol_t(esi).ValueSize]
	shl edx, 3
	push eax
	push ebx
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [valueentry(ebp).Value]
	mov ebx, [valueentry(ebp).Next]
	call .sort_section
	pop esi
	mov edx, [Std$Symbol_t(esi).ValueSize]
	add edx, edx
	push edx
	inc edx
	shl edx, 3
	push edx
	call Riva$Memory$_alloc
	pop edx
	pop edx
	mov [Std$Symbol_t(esi).ValueSize], edx
	mov [Std$Symbol_t(esi).ValueTable], eax
	mov [Std$Symbol_t(esi).ValueSpace], edx
	dec edx
.entry_loop:
	dec dword [Std$Symbol_t(esi).ValueSpace]
	mov edi, [valueentry(ebp).Value]
	mov ecx, edi
	ror ecx, INCR_ROTATION
.find_slot_loop:
	lea edi, [edi + 2 * ecx + 1]
	and edi, edx
	cmp [valueentry(eax + 8 * edi).Value], dword 0
	jne .find_slot_loop
	mov ebx, [valueentry(ebp).Value]
	mov ecx, [valueentry(ebp).Next]
	mov [valueentry(eax + 8 * edi).Value], ebx
	mov [valueentry(eax + 8 * edi).Next], ecx
	add ebp, byte 8
	cmp [valueentry(ebp).Value], dword 0
	jne .entry_loop
	pop eax
	ret 8
.sort_section:
	; eax-edx = current node
	; node(esp + 4) = pivot node
	; edi = bottom free slot
	; esi = top free slot
	push edi
	push esi
.sort_loop:
	cmp eax, [valueentry(esp + 12).Value]
	ja .add_to_bottom
	mov [valueentry(esi).Value], eax
	mov [valueentry(esi).Next], ebx
	sub esi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [valueentry(esi).Value]
	mov ebx, [valueentry(esi).Next]
	jmp .sort_loop
.add_to_bottom:
	mov [valueentry(edi).Value], eax
	mov [valueentry(edi).Next], ebx
	add edi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [valueentry(edi).Value]
	mov ebx, [valueentry(edi).Next]
	jmp .sort_loop
.finished_section:
	mov eax, [valueentry(esp + 12).Value]
	mov ebx, [valueentry(esp + 12).Next]
	mov [valueentry(edi).Value], eax
	mov [valueentry(edi).Next], ebx
	pop esi
	push edi
	add edi, byte 8
	cmp esi, edi
	jbe .no_recurse_1
	push dword [valueentry(edi).Next]
	push dword [valueentry(edi).Value]
	mov eax, [valueentry(esi).Value]
	mov ebx, [valueentry(esi).Next]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 8
	cmp esi, edi
	jbe .no_recurse_2
	push dword [valueentry(edi).Next]
	push dword [valueentry(edi).Value]
	mov eax, [valueentry(esi).Value]
	mov ebx, [valueentry(esi).Next]
	call .sort_section
.no_recurse_2:
	ret 8

local_function defaultmethod
	;int3
	;ebx = symbol
	;edi = args
	;esi = number of args
	push ebp
	mov ebp, esp
	mov ecx, esi
	jecxz .no_args
	mov edx, edi
.arg_loop:
	push dword [Std$Function_argument(edx).Ref]
	push dword [Std$Function_argument(edx).Val]
	add edx, byte sizeof(Std$Function_argument)
	dec ecx
	jnz .arg_loop
.no_args
	push byte 0
	push ebx
	mov edi, esp
	inc esi
	mov ecx, $Default
	call T.invoke
	mov esp, ebp
	pop ebp
	ret

local_function nodefaultmethod
	;int3
	push byte sizeof(Std$Symbol_nomethodmessage)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov ebx, [Std$Function_argument(edi).Val]
	mov [Std$Object_t(eax).Type], dword NoMethodMessageT
	mov [Std$Symbol_nomethodmessage(eax).Symbol], ebx
	push dword 12
	lea eax, [Std$Symbol_nomethodmessage(eax).Stack]
	push eax
	lea eax, [esp + 12]
	push eax
	call Riva$Debug$_stack_trace
	add esp, byte 12
	pop ecx
	mov [Std$Symbol_nomethodmessage(ecx).Count], eax
	xor edx, edx
	mov eax, 2
	ret

%ifdef DOCUMENTING
cglobal Default, T
; <id>Default</id> is called whenever a symbol call fails to match any signature. I.e. if <code>symbol</code> has no signature matching <code>args...</code>, then the call <code>symbol(args...)</code> is then treated as <code>Default(symbol, args...)</code>. Since <id>Default</id> is an instance of <id>T</id>, this allows for a certain degree of meta-programming.
; If <id>Default</id> itself fails to match a signature, then it sends an instance <id>NoMethodMessageT</id>.
%else
cglobal $Default, T
%endif
	dd T
	dd 0, 0, 0
	dd 0, 0, 0
	dd 0
	dd nodefaultmethod
	dd .name
.name:
	dd Std$String$T
	dd Std$Integer$SmallT
	dd 9, 1
	dd Std$Integer$SmallT, 9
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
	db "<default>", 0, 0

%ifdef DOCUMENTING

%define Std$Symbol$T T
%define Std$Symbol$NoMethodMessageT NoMethodMessageT

%define symbol_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
