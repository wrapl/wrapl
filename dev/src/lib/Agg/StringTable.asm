%include "Std.inc"
%include "Riva/Memory.inc"

%define HASH_ROTATION 1

ctype T
; A table using <id>Std/String/T</id> as keys. Note that instances of <id>T</id> do not support deletion of entries.
.invoke: equ 0

struct table, Std$Object_t
	.Size : resd 1
	.Space : resd 1
	.Entries : resd 1
endstruct

struct node
	.Key : resd 1
	.Length : resd 1
	.Hash : resd 1
	.Value : resd 1
endstruct

cfunction _init
	mov eax, [esp + 4]
	mov [Std$Object_t(eax).Type], dword T
	ret

_function New
	push byte sizeof(table)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

method "size", TYP, T
;@table
;:Std$Integer$SmallT
; returns the size of <var>table</var>
	mov ebx, [Std$Function_argument(edi).Val]
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [table(ebx).Size]
	sub eax, [table(ebx).Space]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor eax, eax
	xor edx, edx
	ret

method "copy", TYP, T
	mov ebx, [Std$Function_argument(edi).Val]
	push byte sizeof(table)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov esi, ebx
	mov edi, eax
	mov ecx, sizeof(table)/4
	rep movsd
	mov eax, [table(ebx).Size]
	test eax, eax
	jz .empty_table
	inc eax
	push eax
	shl eax, byte 4
	push eax
	call Riva$Memory$_alloc
	pop ecx
	pop ecx
	mov edi, eax
	mov esi, [table(ebx).Entries]
	shl ecx, byte 2
	rep movsd
.empty_table:
	pop ecx
	mov [table(ecx).Entries], eax
	xor edx, edx
	xor eax, eax
	ret

method "insert", TYP, T, TYP, Std$String$T, ANY
	mov eax, [Std$Function_argument(edi + 8).Val]
	push dword [Std$Function_argument(edi + 16).Val]
	push dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	push eax
	call Std$String$_flatten
	mov [esp], eax
	push dword [Std$Function_argument(edi).Val]
	call _put
	add esp, byte 16
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

method "missing", TYP, T, TYP, Std$String$T, ANY
	mov eax, [Std$Function_argument(edi, 1).Val]
	push byte 0
	push dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	push eax
	call Std$String$_flatten
	mov [esp], eax
	push dword [Std$Function_argument(edi).Val]
	call _slot
	add esp, byte 16
	mov ecx, [eax]
	test ecx, ecx
	jz .missing
	mov edx, [Std$Function_argument(edi, 2).Ref]
	mov [edx], ecx
	xor eax, eax
	inc eax
	ret
.missing:
	mov ecx, Std$Object$Nil
	mov [eax], ecx
	mov edx, eax
	xor eax, eax
	ret

method "[]", TYP, T, TYP, Std$String$T
	push ebp
	mov ebx, esp
	mov ebp, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov edx, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	cmp [Std$String_t(eax).Count], dword 1
	jne .complex_string
	mov esi, [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value]
	jmp .string_ready
.complex_string:
	push byte 0
	sub esp, edx
	mov edi, esp
	lea eax, [Std$String_t(eax).Blocks]
.copy:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .done
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .copy
.done:
	mov [edi], byte 0
	mov esi, esp
.string_ready:
	push esi
	push ebx
	; edx = length of string
	; esi = address of string
	; [esp] = old esp
	; [esp + 4] = address of string
	; [esp + 8] = possibly string or old ebp, no one cares
	mov ecx, edx
	test edx, edx
	jz .empty_string
	xor ebx, ebx
	xor eax, eax
.hash_loop:
	lea ebx, [9 * ebx]
	;rol ebx, HASH_ROTATION
	mov al, [esi]
	xor ebx, eax
	inc esi
	dec edx
	jnz .hash_loop
.empty_string:
	cmp ebx, byte 0
	sete bl
	mov edx, [table(ebp).Size]
	dec edx
	js .empty_table
	lea edx, [2 * edx]
	mov ebp, [table(ebp).Entries]
	mov eax, ebx
	; ecx = length
	; ebx = hash
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Hash], ebx
	jb .not_present
	ja .search_loop
	cmp [node(ebp + 8 * eax).Length], ecx
	jb .not_present
	ja .search_loop
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 4]
	push ecx
	repe cmpsb
	pop ecx
	jb .not_present
	ja .search_loop
	lea edx, [node(ebp + 8 * eax).Value]
	pop esp
	mov ecx, [edx]
	xor eax, eax
	pop ebp
	ret
.not_present:
.empty_table:
	pop esp
	xor eax, eax
	inc eax
	pop ebp
	ret

extern Std$Integer$SmallT
extern Std$Address$T

convert_key:
	; converts the key in node(ebx) into an instance of Std.String.T, returns string in ebx
	push eax
	push ecx
	push edx
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc
	pop ecx
	mov edx, [node(ebx).Length]
	mov ecx, [node(ebx).Key]
	mov [Std$Object_t(eax).Type], dword Std$String$T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	mov [Std$String_t(eax).Count], dword 1
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	mov ebx, eax
	pop edx
	pop ecx
	pop eax
	ret

struct key_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

method "keys", TYP, T
;@table
;:ANY
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [table(eax).Size]
	shl ecx, byte 4
	jz .failure
	mov ebx, [table(eax).Entries]
	lea edi, [ebx + ecx]
	jmp .start
.loop:
	add ebx, byte 16
	cmp ebx, edi
	je .failure
.start:
	cmp [node(ebx).Key], dword 0
	je .loop
	push byte sizeof(loop_state)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Function_state(eax).Run], dword .resume
	mov [key_state(eax).Limit], edi
.suspend:
	mov [key_state(eax).Current], ebx
	call convert_key
	mov ecx, ebx
	xor edx, edx
	mov ebx, eax
	or eax, byte -1
	ret
.resume:
	mov ebx, [key_state(eax).Current]
	mov edi, [key_state(eax).Limit]
.loop2:
	add ebx, byte 16
	cmp ebx, edi
	je .failure
	cmp [node(ebx).Key], dword 0
	je .loop2
	jmp .suspend
.failure:
	xor eax, eax
	inc eax
	ret

struct value_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

method "values", TYP, T
;@table
;:ANY
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [table(eax).Size]
	shl ecx, byte 4
	jz .failure
	mov ebx, [table(eax).Entries]
	lea edi, [ebx + ecx]
	jmp .start
.loop:
	add ebx, byte 16
	cmp ebx, edi
	je .failure
.start:
	cmp [node(ebx).Key], dword 0
	je .loop
	push byte sizeof(loop_state)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Function_state(eax).Run], dword .resume
	mov [value_state(eax).Limit], edi
.suspend:
	mov [value_state(eax).Current], ebx
	mov ecx, [node(ebx).Value]
	lea edx, [node(ebx).Value]
	mov ebx, eax
	or eax, byte -1
	ret
.resume:
	mov ebx, [value_state(eax).Current]
	mov edi, [value_state(eax).Limit]
.loop2:
	add ebx, byte 16
	cmp ebx, edi
	je .failure
	cmp [node(ebx).Key], dword 0
	je .loop2
	jmp .suspend
.failure:
	xor eax, eax
	inc eax
	ret

struct loop_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
	.Key:		resd 1
	.Value:		resd 1
endstruct

extern Std$Object$Nil

method "loop", TYP, T, ANY, ANY
;@table
;:ANY
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [table(eax).Size]
	shl ecx, byte 4
	jz .failure
	mov ebx, [table(eax).Entries]
	lea esi, [ebx + ecx]
	jmp .start
.loop:
	add ebx, byte 16
	cmp ebx, esi
	je .failure
.start:
	cmp [node(ebx).Key], dword 0
	je .loop
	push byte sizeof(loop_state)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Function_state(eax).Run], dword .resume
	mov [loop_state(eax).Limit], esi
	mov ecx, [Std$Function_argument(edi + 8).Ref]
	mov edx, [Std$Function_argument(edi + 16).Ref]
	mov [loop_state(eax).Key], ecx
	mov [loop_state(eax).Value], edx
.suspend:
	mov [loop_state(eax).Current], ebx
	mov edi, [node(ebx).Value]
	call convert_key
	mov [ecx], ebx
	mov [edx], edi
	mov ecx, dword Std$Object$Nil
	xor edx, edx
	mov ebx, eax
	or eax, byte -1
	ret
.resume:
	mov ebx, [loop_state(eax).Current]
	mov esi, [loop_state(eax).Limit]
	mov ecx, [loop_state(eax).Key]
	mov edx, [loop_state(eax).Value]
.loop2:
	add ebx, byte 16
	cmp ebx, esi
	je .failure
	cmp [node(ebx).Key], dword 0
	je .loop2
	jmp .suspend
.failure:
	xor eax, eax
	inc eax
	ret

cfunction _put;(stringtable Table, const char *Key, int Length, void *Value) -> ()
	push ebx
	push edi
	push esi
	push ebp
	mov esi, [esp + 24]
	mov edx, [esp + 28]
	mov ecx, edx
	test edx, edx
	jz .empty_string
	xor ebx, ebx
	xor eax, eax
.hash_loop:
	lea ebx, [9 * ebx]
	;rol ebx, HASH_ROTATION
	mov al, [esi]
	xor ebx, eax
	inc esi
	dec edx
	jnz .hash_loop
.empty_string:
	cmp ebx, byte 0
	sete bl
	mov eax, [esp + 20]
	mov edx, [table(eax).Size]
	dec edx
	js .empty_table
	lea edx, [2 * edx]
	mov ebp, [table(eax).Entries]
	mov eax, ebx
	; ecx = length
	; ebx = hash
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Hash], ebx
	jb .not_present
	ja .search_loop
	cmp [node(ebp + 8 * eax).Length], ecx
	jb .not_present
	ja .search_loop
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	jb .not_present
	ja .search_loop
	mov edx, [esp + 32]
	mov [node(ebp + 8 * eax).Value], edx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.empty_table:
	; ebx = hash
	; ecx = length
	push ebx
	push ecx
	push byte 48
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	pop ebx
	mov edx, [esp + 20]
	mov [table(edx).Size], dword 2
	mov [table(edx).Space], dword 1
	mov [table(edx).Entries], eax
	lea edi, [ebx + 4 * ecx + 2]
	and edi, byte 2
	mov [node(eax + 8 * edi).Hash], ebx
	mov [node(eax + 8 * edi).Length], ecx
	mov ebx, [esp + 24]
	mov ecx, [esp + 32]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Value], ecx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
	mov esi, [esp + 20]
	dec dword [table(esi).Space]
	jle near .grow_table
	push dword [esp + 24]
	push dword [esp + 36]
	cmp [node(ebp + 8 * eax).Key], dword 0
	jne .replace_node
.empty_node:
	pop edi
	pop esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Value], edi
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.replace_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], dword 0
	je .empty_node
	cmp [node(ebp + 8 * eax).Hash], ebx
	ja .replace_loop
	jb .replace_node
	cmp [node(ebp + 8 * eax).Length], ecx
	ja .replace_loop
	jb .replace_node
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	ja .replace_loop
.replace_node:
	pop edi
	pop esi
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Value]
	push dword [node(ebp + 8 * eax).Hash]
	push dword [node(ebp + 8 * eax).Length]
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Value], edi
	pop ecx
	pop ebx
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	push esi
	mov edx, [table(esi).Size]
	shl edx, 4
	mov esi, [esp + 28]
	mov edi, [esp + 36]
	push edi
	push ebx
	push ecx
	push esi
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [node(ebp).Key]
	mov ebx, [node(ebp).Hash]
	mov ecx, [node(ebp).Length]
	mov edx, [node(ebp).Value]
	call .sort_section
	pop esi
	mov edx, [table(esi).Size]
	add edx, edx
	push edx
	inc edx
	shl edx, 4
	push edx
	call Riva$Memory$_alloc
	pop edx
	pop edx
	mov [table(esi).Size], edx
	mov [table(esi).Entries], eax
	mov [table(esi).Space], edx
	dec edx
	add edx, edx
.entry_loop:
	dec dword [table(esi).Space]
	mov ecx, [node(ebp).Length]
	mov edi, [node(ebp).Hash]
.find_slot_loop:
	lea edi, [edi + 4 * ecx + 2]
	and edi, edx
	cmp [node(eax + 8 * edi).Key], dword 0
	jne .find_slot_loop
	mov ebx, [node(ebp).Key]
	mov ecx, [node(ebp).Hash]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Hash], ecx
	mov ebx, [node(ebp).Length]
	mov ecx, [node(ebp).Value]
	mov [node(eax + 8 * edi).Length], ebx
	mov [node(eax + 8 * edi).Value], ecx
	add ebp, byte 16
	cmp [node(ebp).Key], dword 0
	jne .entry_loop
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.sort_section:
	; eax-edx = current node
	; node(esp + 4) = pivot node
	; edi = bottom free slot
	; esi = top free slot
	push edi
	push esi
.sort_loop:
	cmp ebx, [node(esp + 12).Hash]
	ja .add_to_bottom
	jb .add_to_top
	cmp ecx, [node(esp + 12).Length]
	ja .add_to_bottom
	jb .add_to_top
	push ecx
	push esi
	push edi
	mov esi, eax
	mov edi, [node(esp + 24).Key]
	repe cmpsb
	pop edi
	pop esi
	pop ecx
	ja .add_to_bottom
.add_to_top:
	mov [node(esi).Key], eax
	mov [node(esi).Hash], ebx
	mov [node(esi).Length], ecx
	mov [node(esi).Value], edx
	sub esi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	jmp .sort_loop
.add_to_bottom:
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	add edi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(edi).Key]
	mov ebx, [node(edi).Hash]
	mov ecx, [node(edi).Length]
	mov edx, [node(edi).Value]
	jmp .sort_loop
.finished_section:
	mov eax, [node(esp + 12).Key]
	mov ebx, [node(esp + 12).Hash]
	mov ecx, [node(esp + 12).Length]
	mov edx, [node(esp + 12).Value]
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	pop esi
	push edi
	add edi, byte 16
	cmp esi, edi
	jbe .no_recurse_1
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 16
	cmp esi, edi
	jbe .no_recurse_2
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_2:
	ret 16

cfunction _get;(stringtable Table, const char *Key, int Length) -> (void *)
	push ebx
	push edi
	push esi
	push ebp	
	mov edx, [esp + 28]
	mov esi, [esp + 24]
	mov ebp, [esp + 20]
.string_ready:
	; edx = length of string
	; esi = address of string
	mov ecx, edx
	test edx, edx
	jz .empty_string
	xor ebx, ebx
	xor eax, eax
.hash_loop:
	lea ebx, [9 * ebx]
	;rol ebx, HASH_ROTATION
	mov al, [esi]
	xor ebx, eax
	inc esi
	dec edx
	jnz .hash_loop
.empty_string:
	cmp ebx, byte 0
	sete bl
	mov edx, [table(ebp).Size]
	dec edx
	js .empty_table
	lea edx, [2 * edx]
	mov ebp, [table(ebp).Entries]
	mov eax, ebx
	; ecx = length
	; ebx = hash
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Hash], ebx
	jb .not_present
	ja .search_loop
	cmp [node(ebp + 8 * eax).Length], ecx
	jb .not_present
	ja .search_loop
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	jb .not_present
	ja .search_loop
	mov eax, [node(ebp + 8 * eax).Value]
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
.empty_table:
	xor eax, eax
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret

cfunction _inc;(stringtable Table, const char *Key, int Length, int Value) -> (int Value*)
	push ebx
	push edi
	push esi
	push ebp
	mov esi, [esp + 24]
	mov edx, [esp + 28]
	mov ecx, edx
	test edx, edx
	jz .empty_string
	xor ebx, ebx
	xor eax, eax
.hash_loop:
	lea ebx, [9 * ebx]
	;rol ebx, HASH_ROTATION
	mov al, [esi]
	xor ebx, eax
	inc esi
	dec edx
	jnz .hash_loop
.empty_string:
	cmp ebx, byte 0
	sete bl
	mov eax, [esp + 20]
	mov edx, [table(eax).Size]
	dec edx
	js .empty_table
	lea edx, [2 * edx]
	mov ebp, [table(eax).Entries]
	mov eax, ebx
	; ecx = length
	; ebx = hash
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Hash], ebx
	jb .not_present
	ja .search_loop
	cmp [node(ebp + 8 * eax).Length], ecx
	jb .not_present
	ja .search_loop
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	jb .not_present
	ja .search_loop
	mov edx, [esp + 32]
	mov eax, [node(ebp + 8 * eax).Value]
	add eax, edx
	mov [node(ebp + 8 * eax).Value], eax
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.empty_table:
	; ebx = hash
	; ecx = length
	push ebx
	push ecx
	push byte 48
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	pop ebx
	mov edx, [esp + 20]
	mov [table(edx).Size], dword 2
	mov [table(edx).Space], dword 1
	mov [table(edx).Entries], eax
	lea edi, [ebx + 4 * ecx + 2]
	and edi, byte 2
	mov [node(eax + 8 * edi).Hash], ebx
	mov [node(eax + 8 * edi).Length], ecx
	mov ebx, [esp + 24]
	mov ecx, [esp + 32]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Value], ecx
	pop ebp
	pop esi
	pop edi
	pop ebx
	mov eax, [esp + 16]
	ret
.not_present:
	mov esi, [esp + 20]
	dec dword [table(esi).Space]
	jle near .grow_table
	cmp [node(ebp + 8 * eax).Key], dword 0
	jne .replace_node
.empty_node:
	mov esi, [esp + 24]
	mov edi, [esp + 32]
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Value], edi
	pop ebp
	pop esi
	pop edi
	pop ebx
	mov eax, [esp + 16]
	ret
.replace_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], dword 0
	je .empty_node
	cmp [node(ebp + 8 * eax).Hash], ebx
	ja .replace_loop
	jb .replace_node
	cmp [node(ebp + 8 * eax).Length], ecx
	ja .replace_loop
	jb .replace_node
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	ja .replace_loop
.replace_node:
	mov esi, [esp + 24]
	mov edi, [esp + 32]
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Hash]
	push dword [node(ebp + 8 * eax).Length]
	push dword [node(ebp + 8 * eax).Value]
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Value], edi
	pop dword [esp + 44]
	pop ecx
	pop ebx
	pop dword [esp + 24]
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	push esi
	mov edx, [table(esi).Size]
	shl edx, 4
	mov esi, [esp + 28]
	mov edi, [esp + 36]
	push edi
	push ebx
	push ecx
	push esi
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [node(ebp).Key]
	mov ebx, [node(ebp).Hash]
	mov ecx, [node(ebp).Length]
	mov edx, [node(ebp).Value]
	call .sort_section
	pop esi
	mov edx, [table(esi).Size]
	add edx, edx
	push edx
	inc edx
	shl edx, 4
	push edx
	call Riva$Memory$_alloc
	pop edx
	pop edx
	mov [table(esi).Size], edx
	mov [table(esi).Entries], eax
	mov [table(esi).Space], edx
	dec edx
	add edx, edx
.entry_loop:
	dec dword [table(esi).Space]
	mov ecx, [node(ebp).Length]
	mov edi, [node(ebp).Hash]
.find_slot_loop:
	lea edi, [edi + 4 * ecx + 2]
	and edi, edx
	cmp [node(eax + 8 * edi).Key], dword 0
	jne .find_slot_loop
	mov ebx, [node(ebp).Key]
	mov ecx, [node(ebp).Hash]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Hash], ecx
	mov ebx, [node(ebp).Length]
	mov ecx, [node(ebp).Value]
	mov [node(eax + 8 * edi).Length], ebx
	mov [node(eax + 8 * edi).Value], ecx
	add ebp, byte 16
	cmp [node(ebp).Key], dword 0
	jne .entry_loop
	pop ebp
	pop esi
	pop edi
	pop ebx
	mov eax, [esp + 16]
	ret
.sort_section:
	; eax-edx = current node
	; node(esp + 4) = pivot node
	; edi = bottom free slot
	; esi = top free slot
	push edi
	push esi
.sort_loop:
	cmp ebx, [node(esp + 12).Hash]
	ja .add_to_bottom
	jb .add_to_top
	cmp ecx, [node(esp + 12).Length]
	ja .add_to_bottom
	jb .add_to_top
	push ecx
	push esi
	push edi
	mov esi, eax
	mov edi, [node(esp + 24).Key]
	repe cmpsb
	pop edi
	pop esi
	pop ecx
	ja .add_to_bottom
.add_to_top:
	mov [node(esi).Key], eax
	mov [node(esi).Hash], ebx
	mov [node(esi).Length], ecx
	mov [node(esi).Value], edx
	sub esi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	jmp .sort_loop
.add_to_bottom:
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	add edi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(edi).Key]
	mov ebx, [node(edi).Hash]
	mov ecx, [node(edi).Length]
	mov edx, [node(edi).Value]
	jmp .sort_loop
.finished_section:
	mov eax, [node(esp + 12).Key]
	mov ebx, [node(esp + 12).Hash]
	mov ecx, [node(esp + 12).Length]
	mov edx, [node(esp + 12).Value]
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	pop esi
	push edi
	add edi, byte 16
	cmp esi, edi
	jbe .no_recurse_1
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 16
	cmp esi, edi
	jbe .no_recurse_2
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_2:
	ret 16

cfunction _slot;(stringtable Table, const char *Key, int Length, int Value) -> (int Value*)
	push ebx
	push edi
	push esi
	push ebp
.restart_on_grow:
	mov esi, [esp + 24]
	mov edx, [esp + 28]
	mov ecx, edx
	test edx, edx
	jz .empty_string
	xor ebx, ebx
	xor eax, eax
.hash_loop:
	lea ebx, [9 * ebx]
	;rol ebx, HASH_ROTATION
	mov al, [esi]
	xor ebx, eax
	inc esi
	dec edx
	jnz .hash_loop
.empty_string:
	cmp ebx, byte 0
	sete bl
	mov eax, [esp + 20]
	mov edx, [table(eax).Size]
	dec edx
	js .empty_table
	lea edx, [2 * edx]
	mov ebp, [table(eax).Entries]
	mov eax, ebx
	; ecx = length
	; ebx = hash
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Hash], ebx
	jb .not_present
	ja .search_loop
	cmp [node(ebp + 8 * eax).Length], ecx
	jb .not_present
	ja .search_loop
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	jb .not_present
	ja .search_loop
	lea eax, [node(ebp + 8 * eax).Value]
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.empty_table:
	; ebx = hash
	; ecx = length
	push ebx
	push ecx
	push byte 48
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	pop ebx
	mov edx, [esp + 20]
	mov [table(edx).Size], dword 2
	mov [table(edx).Space], dword 1
	mov [table(edx).Entries], eax
	lea edi, [ebx + 4 * ecx + 2]
	and edi, byte 2
	mov [node(eax + 8 * edi).Hash], ebx
	mov [node(eax + 8 * edi).Length], ecx
	mov ebx, [esp + 24]
	mov ecx, [esp + 32]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Value], ecx
	lea eax, [node(eax + 8 * edi).Value]
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
	mov esi, [esp + 20]
	dec dword [table(esi).Space]
	jle near .grow_table
	
	push dword [esp + 24]
	push dword [esp + 36]
	cmp [node(ebp + 8 * eax).Key], dword 0
	jne .replace_node
.empty_node:
	pop edi
	pop esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Value], edi
	jmp .restart_on_grow
.replace_loop:
	lea eax, [eax + 4 * ecx + 2]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], dword 0
	je .empty_node
	cmp [node(ebp + 8 * eax).Hash], ebx
	ja .replace_loop
	jb .replace_node
	cmp [node(ebp + 8 * eax).Length], ecx
	ja .replace_loop
	jb .replace_node
	mov esi, [node(ebp + 8 * eax).Key]
	mov edi, [esp + 24]
	push ecx
	repe cmpsb
	pop ecx
	ja .replace_loop
.replace_node:
	pop edi
	pop esi
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Value]
	push dword [node(ebp + 8 * eax).Hash]
	push dword [node(ebp + 8 * eax).Length]
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Value], edi
	pop ecx
	pop ebx
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	push esi
	mov edx, [table(esi).Size]
	shl edx, 4
	mov esi, [esp + 28]
	mov edi, [esp + 36]
	push edi
	push ebx
	push ecx
	push esi
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [node(ebp).Key]
	mov ebx, [node(ebp).Hash]
	mov ecx, [node(ebp).Length]
	mov edx, [node(ebp).Value]
	call .sort_section
	pop esi
	mov edx, [table(esi).Size]
	add edx, edx
	push edx
	inc edx
	shl edx, 4
	push edx
	call Riva$Memory$_alloc
	pop edx
	pop edx
	mov [table(esi).Size], edx
	mov [table(esi).Entries], eax
	mov [table(esi).Space], edx
	dec edx
	add edx, edx
.entry_loop:
	dec dword [table(esi).Space]
	mov ecx, [node(ebp).Length]
	mov edi, [node(ebp).Hash]
.find_slot_loop:
	lea edi, [edi + 4 * ecx + 2]
	and edi, edx
	cmp [node(eax + 8 * edi).Key], dword 0
	jne .find_slot_loop
	mov ebx, [node(ebp).Key]
	mov ecx, [node(ebp).Hash]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Hash], ecx
	mov ebx, [node(ebp).Length]
	mov ecx, [node(ebp).Value]
	mov [node(eax + 8 * edi).Length], ebx
	mov [node(eax + 8 * edi).Value], ecx
	add ebp, byte 16
	cmp [node(ebp).Key], dword 0
	jne .entry_loop
	jmp .restart_on_grow
.sort_section:
	; eax-edx = current node
	; node(esp + 4) = pivot node
	; edi = bottom free slot
	; esi = top free slot
	push edi
	push esi
.sort_loop:
	cmp ebx, [node(esp + 12).Hash]
	ja .add_to_bottom
	jb .add_to_top
	cmp ecx, [node(esp + 12).Length]
	ja .add_to_bottom
	jb .add_to_top
	push ecx
	push esi
	push edi
	mov esi, eax
	mov edi, [node(esp + 24).Key]
	repe cmpsb
	pop edi
	pop esi
	pop ecx
	ja .add_to_bottom
.add_to_top:
	mov [node(esi).Key], eax
	mov [node(esi).Hash], ebx
	mov [node(esi).Length], ecx
	mov [node(esi).Value], edx
	sub esi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	jmp .sort_loop
.add_to_bottom:
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	add edi, byte 16
	cmp esi, edi
	je .finished_section
	mov eax, [node(edi).Key]
	mov ebx, [node(edi).Hash]
	mov ecx, [node(edi).Length]
	mov edx, [node(edi).Value]
	jmp .sort_loop
.finished_section:
	mov eax, [node(esp + 12).Key]
	mov ebx, [node(esp + 12).Hash]
	mov ecx, [node(esp + 12).Length]
	mov edx, [node(esp + 12).Value]
	mov [node(edi).Key], eax
	mov [node(edi).Hash], ebx
	mov [node(edi).Length], ecx
	mov [node(edi).Value], edx
	pop esi
	push edi
	add edi, byte 16
	cmp esi, edi
	jbe .no_recurse_1
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 16
	cmp esi, edi
	jbe .no_recurse_2
	push dword [node(edi).Value]
	push dword [node(edi).Hash]
	push dword [node(edi).Length]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Hash]
	mov ecx, [node(esi).Length]
	mov edx, [node(esi).Value]
	call .sort_section
.no_recurse_2:
	ret 16
