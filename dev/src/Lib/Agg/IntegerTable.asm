%include "Std.inc"
%include "Riva/Memory.inc"

ctype T
; A table using <id>Std/Integer/SmallT</id> as keys. Note that instances of <id>T</id> do not support deletion of entries.
.invoke: equ 0

struct table, Std$Object_t
	.Size : resd 1
	.Space : resd 1
	.Entries : resd 1
endstruct

struct node
	.Key : resd 1
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
; Returns the number of entries in <var>table</var>
	mov ebx, [Std$Function_argument(edi).Val]
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [table(ebx).Size]
	sub eax, [table(ebx).Space]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor eax, eax
	xor edx, edx
	ret

method "insert", TYP, T, TYP, Std$Integer$SmallT, ANY
;@table
;@key
;@value
;:T
; Inserts a new entry <code>(key, Std$Object_t)</code> into <var>table</var>. Returns <var>table</var>.
	push dword [Std$Function_argument(edi + 16).Val]
	mov eax, [Std$Function_argument(edi + 8).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push dword [Std$Function_argument(edi).Val]
	call _put
	add esp, byte 12
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

method "[]", TYP, T, TYP, Std$Integer$SmallT
;@table
;@key
;:ANY
; Returns value if <code>(key, Std$Object_t)</code> is an entry in <var>table</var>, fails otherwise.
	push ebp
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov ebx, [Std$Integer_smallt(eax).Value]
	mov ecx, ebx
	rol ecx, 2
	mov eax, [Std$Function_argument(edi).Val]
	mov edx, [table(eax).Size]
	dec edx
	js .empty_table
	mov ebp, [table(eax).Entries]
	mov eax, ebx
	; ecx = increment
	; ebx = hash/key
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], ebx
	jb .not_present
	ja .search_loop
	mov ecx, [node(ebp + 8 * eax).Value]
	lea edx, [node(ebp + 8 * eax).Value]
	xor eax, eax
	test ecx, ecx
	setz al
	pop ebp
	ret
.not_present:
.empty_table:
	xor eax, eax
	inc eax
	pop ebp
	ret

cfunction _put;(integertable Table, int Key, void *Value) -> ()
	push ebx
	push edi
	push esi
	push ebp
	mov ebx, [esp + 24]
	mov ecx, ebx
	rol ecx, 2
	mov esi, [esp + 20]
	mov edx, [table(esi).Size]
	dec edx
	js .empty_table
	mov ebp, [table(esi).Entries]
	mov eax, ebx
	; ecx = increment
	; ebx = hash/key
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], ebx
	jb .not_present
	ja .search_loop
	mov edx, [esp + 28]
	mov [node(ebp + 8 * eax).Value], edx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.empty_table:
	; ebx = hash/key
	; ecx = increment
	push ebx
	push ecx
	push byte 32
	call Riva$Memory$_alloc
	pop edx
	pop ecx
	pop ebx
	mov [table(esi).Size], dword 2
	mov [table(esi).Space], dword 1
	mov [table(esi).Entries], eax
	lea edi, [ebx + 2 * ecx + 1]
	and edi, byte 1
	mov ecx, [esp + 28]
	jnz .odd
	mov [node(eax).Key], ebx
	mov [node(eax).Value], ecx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.odd:
	mov [node(eax + 8).Key], ebx
	mov [node(eax + 8).Value], ecx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
	dec dword [table(esi).Space]
	jle near .grow_table
	mov edi, [esp + 28]
	cmp [node(ebp + 8 * eax).Value], dword 0
	jne .replace_node
.empty_node:
	mov [node(ebp + 8 * eax).Key], ebx
	mov [node(ebp + 8 * eax).Value], edi
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.replace_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [node(ebp + 8 * eax).Value], dword 0
	je .empty_node
	cmp [node(ebp + 8 * eax).Key], ebx
	ja .replace_loop
.replace_node:
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Value]
	mov [node(ebp + 8 * eax).Key], ebx
	mov [node(ebp + 8 * eax).Value], edi
	pop edi
	pop ebx
	mov ecx, ebx
	rol ecx, 2
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	push esi
	mov edx, [table(esi).Size]
	shl edx, 3
	mov edi, [esp + 32]
	push edi
	push ebx
	; node(esp) is our pivot node
	mov edi, ebp
	lea esi, [ebp + edx]
	mov eax, [node(ebp).Key]
	mov ebx, [node(ebp).Value]
	call .sort_section
	pop esi
	mov edx, [table(esi).Size]
	add edx, edx
	push edx
	inc edx
	shl edx, 3
	push edx
	call Riva$Memory$_alloc
	mov edi, eax
	mov ecx, [esp + 4]
	pop edx
	pop edx
	mov [table(esi).Size], edx
	mov [table(esi).Entries], eax
	mov [table(esi).Space], edx
	dec edx
.entry_loop:
	dec dword [table(esi).Space]
	mov edi, [node(ebp).Key]
	mov ecx, edi
	rol ecx, 2
.find_slot_loop:
	lea edi, [edi + 2 * ecx + 1]
	and edi, edx
	cmp [node(eax + 8 * edi).Value], dword 0
	jne .find_slot_loop
	mov ebx, [node(ebp).Key]
	mov ecx, [node(ebp).Value]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Value], ecx
	add ebp, byte 8
	cmp [node(ebp).Value], dword 0
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
	cmp eax, [node(esp + 12).Key]
	ja .add_to_bottom
	jb .add_to_top
	test ebx, ebx
	jnz .add_to_bottom
.add_to_top:
	mov [node(esi).Key], eax
	mov [node(esi).Value], ebx
	sub esi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Value]
	jmp .sort_loop
.add_to_bottom:
	mov [node(edi).Key], eax
	mov [node(edi).Value], ebx
	add edi, byte 8
	cmp esi, edi
	je .finished_section
	mov eax, [node(edi).Key]
	mov ebx, [node(edi).Value]
	jmp .sort_loop
.finished_section:
	mov eax, [node(esp + 12).Key]
	mov ebx, [node(esp + 12).Value]
	mov [node(edi).Key], eax
	mov [node(edi).Value], ebx
	pop esi
	push edi
	add edi, byte 8
	cmp esi, edi
	jbe .no_recurse_1
	push dword [node(edi).Value]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Value]
	call .sort_section
.no_recurse_1:
	pop esi
	pop edi
	sub esi, byte 8
	cmp esi, edi
	jbe .no_recurse_2
	push dword [node(edi).Value]
	push dword [node(edi).Key]
	mov eax, [node(esi).Key]
	mov ebx, [node(esi).Value]
	call .sort_section
.no_recurse_2:
	ret 8

cfunction _get;(integertable Table, int Key) -> (void *)
	push ebx
	push edi
	push esi
	push ebp
	mov ebx, [esp + 24]
	mov ecx, ebx
	rol ecx, 2
	mov eax, [esp + 20]
	mov edx, [table(eax).Size]
	dec edx
	js .empty_table
	mov ebp, [table(eax).Entries]
	mov eax, ebx
	; ecx = increment
	; ebx = hash/key
	; edx = mask
	; eax = index
	; ebp = entries
.search_loop:
	lea eax, [eax + 2 * ecx + 1]
	and eax, edx
	cmp [node(ebp + 8 * eax).Key], ebx
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
