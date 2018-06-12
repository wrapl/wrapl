%include "Std.inc"
%include "Riva/Memory.inc"

struct _node
	.Value:	resd 1
	.Next:	resd 1
	.Prev:	resd 1
endstruct

struct _list
	.Type:		resd 1
	.Head:		resd 1
	.Tail:		resd 1
	.Cache:		resd 1
	.Array:		resd 1
	.Length:	resd 1
	.Index:		resd 1
	.Lower:		resd 1
	.Upper:		resd 1
	.Access:	resd 1
endstruct


extern T

section .data

;c_data T
;	dd Std$Type$T
;	dd .types
;	dd 0
;	dd 0
;	dd 0
;.types:
;	dd T
;	dd 0

_function New
;@length : Std$Integer$T := 0
;:T
; Returns a new list with Length elements
	push sizeof(_list)
	call Riva$Memory$_alloc
	pop ecx
	mov [_list(eax).Type], dword T
	test esi, esi
	jz .empty_list
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov [_list(eax).Length], ecx
	mov edx, Std$Object$Nil
	dec esi
	cmovnz edx, [Std$Function_argument(edi + 8).Val]
    mov ebx, eax
    mov edi, edx
    mov esi, ecx
    push byte sizeof(_node)
    call Riva$Memory$_alloc
    mov [esp], ebx
    mov [_list(ebx).Head], eax
    mov [_list(ebx).Cache], eax
    inc dword [_list(ebx).Index]
    mov [_node(eax).Value], edi
    dec esi
    jz .finished
    push edx
.loop:
    mov ebx, eax
    mov [esp], dword sizeof(_node)
    call Riva$Memory$_alloc
    mov [_node(eax).Prev], ebx
    mov [_node(ebx).Next], eax
    mov [_node(eax).Value], edi
    dec esi
    jnz .loop
    pop edx
.finished:
    pop ecx
    mov [_list(ecx).Tail], eax
    jmp .common
.empty_list:
	xor esi, esi
	mov [_list(eax).Head], esi
	mov [_list(eax).Tail], esi
	mov [_list(eax).Length], esi
	mov ecx, eax
.common:
	mov [_list(ecx).Upper], edx
	mov [_list(ecx).Lower], edx
	mov [_list(ecx).Access], dword 4
	xor edx, edx
	xor eax, eax
	ret

method "new", VAL, T
	push sizeof(_list)
	call Riva$Memory$_alloc
	pop ecx
	mov [_list(eax).Type], dword T
	xor esi, esi
	mov [_list(eax).Head], esi
	mov [_list(eax).Tail], esi
	mov [_list(eax).Length], esi
	mov ecx, eax
	mov [_list(ecx).Upper], edx
	mov [_list(ecx).Lower], edx
	mov [_list(ecx).Access], dword 4
	xor edx, edx
	xor eax, eax
	ret

_function Make
;@value&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;value&lt;sub&gt;k&lt;/sub&gt;
;:T
; returns a list with <var>value<sub>1</sub></var>, ... , <var>value<sub>k</sub></var> as its elements
	push byte sizeof(_list)
	call Riva$Memory$_alloc
	pop ecx
	mov [_list(eax).Type], dword T
	mov [_list(eax).Length], esi
	mov [_list(eax).Access], dword 4
	test esi, esi
	mov ebx, eax
	mov ecx, eax
	jz .empty_list
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	mov [_list(ebx).Head], eax
	mov [_list(ebx).Cache], eax
	mov [_list(ebx).Index], dword 1
	push ebx
	dec esi
	jz .done
.loop:
	add edi, byte sizeof(Std$Function_argument)
	mov ebx, eax
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	dec esi
	mov [_node(eax).Prev], ebx
	mov [_node(ebx).Next], eax
	jnz .loop
.done:
	pop ecx
	mov [_list(ecx).Tail], eax
.empty_list:
	xor edx, edx
	xor eax, eax
	ret

extern Std$Integer$SmallT

method "[]", TYP, T, TYP, Std$Integer$SmallT
;@list
;@n
;:ANY
; returns an assignable reference to the <var>n</var><sup>th</sup> element of <var>list</var>
; negative indices are taken from the end of the list
; fails if <var>n</var> is outside the range <code>-</code><var>list</var><code>:length</code> .. <var>list</var><code>:length</code>
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov edx, [_list(ebx).Length]
	test edx, edx
	jz .failure
	test eax, eax
	jns .positive
	lea eax, [eax + edx + 1]
.positive:
	cmp eax, byte 1
	jb .failure
	jne .l1
	; return Head
	mov edx, [_list(ebx).Head]
	jmp .return
.l1:
	cmp eax, edx
	ja .failure
	jne .l2
	; return Tail
	mov edx, [_list(ebx).Tail]
	jmp .return
.failure:
	xor eax, eax
	inc eax
	ret
.l2:
	mov ecx, eax
	sub ecx, [_list(ebx).Index]
	jne .l3
	; return Cache
	mov edx, [_list(ebx).Cache]
.return:
	mov ecx, [_node(edx).Value]
	lea edx, [_node(edx).Value]
	xor eax, eax
	ret
.l3:
	js .l5
	dec ecx
	jnz .l6
	; return Cache->Next
	mov edx, [_list(ebx).Cache]
	mov edx, [_node(edx).Next]
	mov [_list(ebx).Index], eax
	mov [_list(ebx).Cache], edx
	jmp .return
.l5:
	inc ecx
	jnz .l6
	; return Cache->Prev
	mov edx, [_list(ebx).Cache]
	mov edx, [_node(edx).Prev]
	mov [_list(ebx).Index], eax
	mov [_list(ebx).Cache], edx
	jmp .return
.l6:
	mov edi, [_list(ebx).Array]
	test edi, edi
	jz .l7
	mov ecx, eax
	sub ecx, [_list(ebx).Lower]
	js .l7
	cmp eax, [_list(ebx).Upper]
	ja .l7
	; return Array[Index]
	mov edx, [edi + 4 * ecx]
	jmp .return
.l7:
	dec dword [_list(ebx).Access]
	jnz .l8
	; build array
	push eax
	; edx = length of list
	mov esi, edx
	shl edx, 2
	push edx
	; use atomic since every node should be accessible through the normal linked list
	call Riva$Memory$_alloc
	pop ecx
	mov [_list(ebx).Array], eax
	mov [_list(ebx).Lower], dword 1
	mov [_list(ebx).Upper], esi
	mov edi, eax
	mov ecx, [_list(ebx).Head]
.array_loop:
	mov [eax], ecx
	add eax, byte 4
	dec esi
	mov ecx, [_node(ecx).Next]
	jnz .array_loop
	pop ecx
	dec ecx
	mov edx, [edi + 4 * ecx]
	jmp .return
.l8:
	lea ecx, [eax + eax]
	cmp ecx, [_list(ebx).Index]
	jae .l10
	mov edx, [_list(ebx).Head]
	mov [_list(ebx).Index], eax
	dec eax
.l9:
	mov edx, [_node(edx).Next]
	dec eax
	jnz .l9
	jmp .return2
.l10:
	cmp eax, [_list(ebx).Index]
	jae .l12
	mov edx, [_list(ebx).Cache]
	sub eax, [_list(ebx).Index]
	add [_list(ebx).Index], eax
.l11:
	mov edx, [_node(edx).Prev]
	inc eax
	jnz .l11
	jmp .return2
.l12:
	; edx is still the length
	; ecx is still 2 * index
	add edx, [_list(ebx).Index]
	cmp ecx, edx
	jae .l14
	mov edx, [_list(ebx).Cache]
	sub eax, [_list(ebx).Index]
	add [_list(ebx).Index], eax
.l13:
	mov edx, [_node(edx).Next]
	dec eax
	jnz .l13
	jmp .return2
.l14:
	mov edx, [_list(ebx).Tail]
	mov [_list(ebx).Index], eax
	sub eax, [_list(ebx).Length]
.l15:
	mov edx, [_node(edx).Prev]
	inc eax
	jnz .l15
.return2:
	mov [_list(ebx).Cache], edx
	lea edx, [_node(edx).Value]
	mov ecx, [edx]
	xor eax, eax
	ret

method "push", TYP, T, SKP
;@list
;@value...
;:T
; inserts <code>value...</code> onto the start of <var>list</var> and returns <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	dec esi
	add [_list(ebx).Length], esi
	add [_list(ebx).Lower], esi
	add [_list(ebx).Upper], esi
	add [_list(ebx).Index], esi
	add edi, byte sizeof(Std$Function_argument)
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	mov ecx, [_list(ebx).Head]
	test ecx, ecx
	jz .l1
	mov [_node(ecx).Prev], eax
	mov [_node(eax).Next], ecx
	jmp .l2
.l1:
	mov [_list(ebx).Tail], eax
	mov [_list(ebx).Cache], eax
	mov [_list(ebx).Index], esi
.l2:
	dec esi
	jz .l4
	push ebx
.l3:
	mov ebx, eax
	add edi, byte sizeof(Std$Function_argument)
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	mov [_node(eax).Next], ebx
	mov [_node(ebx).Prev], eax
	dec esi
	jnz .l3
	pop ebx
.l4:
	mov [_list(ebx).Head], eax
	mov [_list(ebx).Access], dword 4
	mov ecx, ebx
	xor edx, edx
	xor eax, eax
	ret

method "put", TYP, T, SKP
;@list
;@value...
;:T
; inserts <code>value...</code> onto the end of <var>list</var> and returns <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	dec esi
	add [_list(ebx).Length], esi
	add edi, byte sizeof(Std$Function_argument)
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	mov ecx, [_list(ebx).Tail]
	test ecx, ecx
	jz .l1
	mov [_node(ecx).Next], eax
	mov [_node(eax).Prev], ecx
	jmp .l2
.l1:
	mov [_list(ebx).Head], eax
	mov [_list(ebx).Cache], eax
	mov [_list(ebx).Index], dword 1
.l2:
	dec esi
	jz .l4
	push ebx
.l3:
	mov ebx, eax
	add edi, byte sizeof(Std$Function_argument)
	push byte sizeof(_node)
	call Riva$Memory$_alloc
	mov edx, [Std$Function_argument(edi).Val]
	pop ecx
	mov [_node(eax).Value], edx
	mov [_node(eax).Prev], ebx
	mov [_node(ebx).Next], eax
	dec esi
	jnz .l3
	pop ebx
.l4:
	mov [_list(ebx).Tail], eax
	mov [_list(ebx).Access], dword 4
	mov ecx, ebx
	xor edx, edx
	xor eax, eax
	ret

method "pop", TYP, T
;@list
;:ANY
; removes and returns the first element of <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	mov edx, [_list(ebx).Head]
	test edx, edx
	jnz .l1
	xor eax, eax
	inc eax
	ret
.l1:
	mov eax, [_node(edx).Next]
	test eax, eax
	mov [_list(ebx).Head], eax
	jz .l2
	mov [_node(eax).Prev], dword 0
	cmp [_list(ebx).Cache], edx
	je .l6
	dec dword [_list(ebx).Index]
	jmp .l3
.l6:
	mov [_list(ebx).Cache], eax
	jmp .l3
.l2:
	mov [_list(ebx).Tail], eax
.l3:
	dec dword [_list(ebx).Length]
	mov eax, [_list(ebx).Array]
	test eax, eax
	jz .l5
	dec dword [_list(ebx).Upper]
	cmp [_list(ebx).Lower], dword 1
	jne .l4
	add dword [_list(ebx).Array], 4
	jmp .l5
.l4:
	dec dword [_list(ebx).Lower]
.l5:
	mov [_list(ebx).Access], dword 4
	mov ecx, [_node(edx).Value]
	xor edx, edx
	xor eax, eax
	ret

method "pull", TYP, T
;@list
;:ANY
; removes and returns the last element of <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	mov edx, [_list(ebx).Tail]
	test edx, edx
	jnz .l1
	xor eax, eax
	inc eax
	ret
.l1:
	mov eax, [_node(edx).Prev]
	test eax, eax
	mov [_list(ebx).Tail], eax
	jz .l2
	mov [_node(eax).Next], dword 0
	cmp [_list(ebx).Cache], edx
	jne .l3
	mov [_list(ebx).Cache], eax
	dec dword [_list(ebx).Index]
	jmp .l3
.l2:
	mov [_list(ebx).Head], eax
.l3:
	dec dword [_list(ebx).Length]
	mov eax, [_list(ebx).Array]
	test eax, eax
	jz .l5
	mov eax, [_list(ebx).Upper]
	cmp eax, [_list(ebx).Length]
	jbe .l5
	dec dword [_list(ebx).Upper]
.l5:
	mov [_list(ebx).Access], dword 4
	mov ecx, [_node(edx).Value]
	xor edx, edx
	xor eax, eax
	ret

method "length", TYP, T
;@list
;:Std$Integer$SmallT
; returns the length of <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [_list(ebx).Length]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor eax, eax
	xor edx, edx
	ret

method "size", TYP, T
;@list
;:Std$Integer$SmallT
; returns the length of <var>list</var>
	mov ebx, [Std$Function_argument(edi).Val]
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [_list(ebx).Length]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor eax, eax
	xor edx, edx
	ret

struct keys_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

method "keys", TYP, T
;@list
;:Std$Integer$SmallT
; Equivalent to <code>1:to(list:length)</code>.
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [_list(ebx).Length]
	cmp eax, byte 1
	jl .fail
	je .return
.suspend:
	push eax
	push byte sizeof(keys_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [keys_state(eax).Current], dword 1
	pop dword [keys_state(eax).Limit]
	mov dword [Std$Function_state(eax).Run], .resume
	mov ecx, .One
	mov ebx, eax
	or eax, byte -1
	xor edx, edx
	ret
.return:
	mov ecx, .One
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
	mov ecx, [keys_state(eax).Current]
	inc ecx
	cmp ecx, [keys_state(eax).Limit]
	jg .fail
	je .return2
	mov [keys_state(eax).Current], ecx
	push eax
	push ecx
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret
.return2:
	push ecx
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.One:
	dd Std$Integer$SmallT, 1

struct values_state, Std$Function_state
	.Current:	resd 1
endstruct

method "values", TYP, T
;@list
;:ANY
; Generates the values in <var>list</var>.
	mov ebx, [Std$Function_argument(edi).Val]
	mov ebx, [_list(ebx).Head]
	test ebx, ebx
	jz .failure
	push byte sizeof(values_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [values_state(eax).Current], ebx
	mov dword [Std$Function_state(eax).Run], .resume
	mov ecx, [_node(ebx).Value]
	lea edx, [_node(ebx).Value]
	mov ebx, eax
	or eax, byte -1
	ret
.resume:
	mov ebx, [values_state(eax).Current]
	mov ebx, [_node(ebx).Next]
	test ebx, ebx
	jz .failure
	mov [values_state(eax).Current], ebx
	mov ecx, [_node(ebx).Value]
	lea edx, [_node(ebx).Value]
	mov ebx, eax
	or eax, byte -1
	ret
.failure:
	xor eax, eax
	inc eax
	ret

method "rvalues", TYP, T
;@list
;:ANY
; Generates the values in <var>list</var> in reverse order.
	mov ebx, [Std$Function_argument(edi).Val]
	mov ebx, [_list(ebx).Tail]
	test ebx, ebx
	jz .failure
	push byte sizeof(values_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [values_state(eax).Current], ebx
	mov dword [Std$Function_state(eax).Run], .resume
	mov ecx, [_node(ebx).Value]
	lea edx, [_node(ebx).Value]
	mov ebx, eax
	or eax, byte -1
	ret
.resume:
	mov ebx, [values_state(eax).Current]
	mov ebx, [_node(ebx).Prev]
	test ebx, ebx
	jz .failure
	mov [values_state(eax).Current], ebx
	mov ecx, [_node(ebx).Value]
	lea edx, [_node(ebx).Value]
	mov ebx, eax
	or eax, byte -1
	ret
.failure:
	xor eax, eax
	inc eax
	ret

method "reverse", TYP, T
;@list
;:T
; Reverses the order of the entries in <var>list</var> in place. Returns <var>list</var>.
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [_list(ecx).Head]
	test eax, eax
	jz .empty
	mov [_list(ecx).Tail], eax
	xor ebx, ebx
.loop:
	;ebx = previous node
	;eax = current node
	;edx = next node
	mov edx, [_node(eax).Next]
	mov [_node(eax).Next], ebx
	mov [_node(eax).Prev], edx
	test edx, edx
	mov ebx, eax
	mov eax, edx
	jnz .loop
	mov edx, [_list(ecx).Length]
	mov [_list(ecx).Head], ebx
	inc edx
	mov [_list(ecx).Access], dword 4
	sub edx, [_list(ecx).Index]
	mov [_list(ecx).Array], dword 0
	mov [_list(ecx).Index], edx
.empty:
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

symbol ?EQUAL, "="

method "[]=", TYP, T, TYP, Std$Integer$SmallT, ANY
;@list
;@n
;@value
;:ANY
; compares the <var>n</var><sup>th</sup> element of <var>list</var> to <var>value</var>
; negative indices are taken from the end of the list
; fails if <var>n</var> is outside the range <code>-</code><var>list</var><code>:length</code> .. <var>list</var><code>:length</code>
	push dword [Std$Function_argument(edi + 16).Ref]
	push dword [Std$Function_argument(edi + 16).Val]
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov edx, [_list(ebx).Length]
	test edx, edx
	jz .failure
	test eax, eax
	jns .positive
	lea eax, [eax + edx + 1]
.positive:
	cmp eax, byte 1
	jb .failure
	jne .l1
	; return Head
	mov edx, [_list(ebx).Head]
	jmp .return
.l1:
	cmp eax, edx
	ja .failure
	jne .l2
	; return Tail
	mov edx, [_list(ebx).Tail]
	jmp .return
.failure:
	add esp, byte 8
	xor eax, eax
	inc eax
	ret
.l2:
	mov ecx, eax
	sub ecx, [_list(ebx).Index]
	jne .l3
	; return Cache
	mov edx, [_list(ebx).Cache]
.return:
	lea edx, [_node(edx).Value]
	push edx
	push dword [edx]
	mov edi, esp
	mov esi, 2
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	add esp, 16
	ret
.l3:
	js .l5
	dec ecx
	jnz .l6
	; return Cache->Next
	mov edx, [_list(ebx).Cache]
	mov edx, [_node(edx).Next]
	mov [_list(ebx).Index], eax
	mov [_list(ebx).Cache], edx
	jmp .return
.l5:
	inc ecx
	jnz .l6
	; return Cache->Prev
	mov edx, [_list(ebx).Cache]
	mov edx, [_node(edx).Prev]
	mov [_list(ebx).Index], eax
	mov [_list(ebx).Cache], edx
	jmp .return
.l6:
	mov edi, [_list(ebx).Array]
	test edi, edi
	jz .l7
	mov ecx, eax
	sub ecx, [_list(ebx).Lower]
	js .l7
	cmp eax, [_list(ebx).Upper]
	ja .l7
	; return Array[Index]
	mov edx, [edi + 4 * ecx]
	jmp .return
.l7:
	dec dword [_list(ebx).Access]
	jnz .l8
	; build array
	push eax
	; edx = length of list
	mov esi, edx
	shl edx, 2
	push edx
	; use atomic since every node should be accessible through the normal linked list
	call Riva$Memory$_alloc
	pop ecx
	mov [_list(ebx).Array], eax
	mov [_list(ebx).Lower], dword 1
	mov [_list(ebx).Upper], esi
	mov edi, eax
	mov ecx, [_list(ebx).Head]
.array_loop:
	mov [eax], ecx
	add eax, byte 4
	dec esi
	mov ecx, [_node(ecx).Next]
	jnz .array_loop
	pop ecx
	dec ecx
	mov edx, [edi + 4 * ecx]
	jmp .return
.l8:
	lea ecx, [eax + eax]
	cmp ecx, [_list(ebx).Index]
	jae .l10
	mov edx, [_list(ebx).Head]
	mov [_list(ebx).Index], eax
	dec eax
.l9:
	mov edx, [_node(edx).Next]
	dec eax
	jnz .l9
	jmp .return2
.l10:
	cmp eax, [_list(ebx).Index]
	jae .l12
	mov edx, [_list(ebx).Cache]
	sub eax, [_list(ebx).Index]
	add [_list(ebx).Index], eax
.l11:
	mov edx, [_node(edx).Prev]
	inc eax
	jnz .l11
	jmp .return2
.l12:
	; edx is still the length
	; ecx is still 2 * index
	add edx, [_list(ebx).Index]
	cmp ecx, edx
	jae .l14
	mov edx, [_list(ebx).Cache]
	sub eax, [_list(ebx).Index]
	add [_list(ebx).Index], eax
.l13:
	mov edx, [_node(edx).Next]
	dec eax
	jnz .l13
	jmp .return2
.l14:
	mov edx, [_list(ebx).Tail]
	mov [_list(ebx).Index], eax
	sub eax, [_list(ebx).Length]
.l15:
	mov edx, [_node(edx).Prev]
	inc eax
	jnz .l15
.return2:
	mov [_list(ebx).Cache], edx
	lea edx, [_node(edx).Value]
	push edx
	push dword [edx]
	mov edi, esp
	mov esi, 2
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	add esp, 16
	ret
