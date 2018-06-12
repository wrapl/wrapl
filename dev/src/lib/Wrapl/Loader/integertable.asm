%include "Std.inc"
%include "Riva/Memory.inc"

struct table
	.Size : resd 1
	.Space : resd 1
	.Entries : resd 1
endstruct

struct node
	.Key : resd 1
	.Value : resd 1
endstruct

cfunction integertable_init;(integertable Table) -> ()
	mov eax, [esp + 4]
	mov [table(eax).Size], dword 0
	mov [table(eax).Space], dword 0
	ret

cfunction integertable_put;(integertable Table, int Key, void *Value) -> ()
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
	ja .not_present
	jb .search_loop
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
	mov edx, -1
	jnz .odd
	mov [node(eax).Key], ebx
	mov [node(eax).Value], ecx
	mov [node(eax + 8).Key], edx
	mov [node(eax + 8).Value], edx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.odd:
	mov [node(eax).Key], edx
	mov [node(eax).Value], edx
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
	cmp [node(ebp + 8 * eax).Key], dword -1
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
	cmp [node(ebp + 8 * eax).Key], dword -1
	je .empty_node
	cmp [node(ebp + 8 * eax).Key], ebx
	jb .replace_loop
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
	push eax
	mov eax, -1
	shl ecx, 1
	rep stosd
	pop eax
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
	cmp [node(eax + 8 * edi).Key], dword -1
	jne .find_slot_loop
	mov ebx, [node(ebp).Key]
	mov ecx, [node(ebp).Value]
	mov [node(eax + 8 * edi).Key], ebx
	mov [node(eax + 8 * edi).Value], ecx
	add ebp, byte 8
	cmp [node(ebp).Key], dword -1
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
	jb .add_to_bottom
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

cfunction integertable_get;(integertable Table, int Key) -> (void *)
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
	ja .not_present
	jb .search_loop
	mov eax, [node(ebp + 8 * eax).Value]
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
.empty_table:
	xor eax, eax
	dec eax
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
