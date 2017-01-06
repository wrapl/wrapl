%include "Std.inc"
%include "Riva/Memory.inc"

struct table
	resb 27
.Mask: resd 1
	resb 3
.Entries: resd 1
	resb 8
.Entries2: resd 1
	resb 2
.Space: resd 1
.Entries3: resd 1
endstruct

struct node
	.Key:	resd 1
	.Value:	resd 1
endstruct

cfunction _set
	push ebx
	push edi
	push esi
	push ebp
	mov ebx, [esp + 24]
	mov ecx, ebx
	ror ecx, 4
	mov eax, [esp + 20]
	mov edx, [table(eax).Mask]
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
	mov edx, [esp + 28]
	mov [node(ebp + 8 * eax).Value], edx
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.not_present:
	mov esi, [esp + 20]
	dec dword [table(esi).Space]
	jle near .grow_table
	cmp [node(ebp + 8 * eax).Value], dword 0
	jne .replace_node
.empty_node:
	mov edi, [esp + 28]
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
	mov edi, [esp + 28]
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Value]
	mov [node(ebp + 8 * eax).Key], ebx
	mov [node(ebp + 8 * eax).Value], edi
	pop dword [esp + 32]
	pop ebx
	mov ecx, ebx
	ror ecx, 4
	jmp .replace_loop
.grow_table:
	; First sort table entries so that they are decreasing
	push esi
	mov edx, [table(esi).Mask]
	inc edx
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
	mov edx, [table(esi).Mask]
	inc edx
	add edx, edx
	push edx
	inc edx
	shl edx, 3
	push edx
	call Riva$Memory$_alloc
	mov edi, eax
	mov ecx, [esp + 4]
	
	;push eax
	;mov eax, -1
	;shl ecx, 1
	;rep stosd
	;pop eax
	
	pop edx
	pop edx
	mov [table(esi).Space], edx
	dec edx
	mov [table(esi).Mask], edx
.entry_loop:
	dec dword [table(esi).Space]
	mov edi, [node(ebp).Key]
	mov ecx, edi
	ror ecx, 4
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
	mov [table(esi).Entries], eax
	mov [table(esi).Entries3], eax
	add eax, byte 4
	mov [table(esi).Entries2], eax
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

cfunction _get
	push ebx
	push edi
	push esi
	push ebp
	mov esi, [esp + 24]
	mov esi, [Std$Type_t(esi).Types]
	mov ebx, [esi]
.type_loop:
	mov ecx, ebx
	ror ecx, 4
	mov eax, [esp + 20]
	mov edx, [table(eax).Mask]
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
	add esi, byte 4
	mov ebx, [esi]
	test ebx, ebx
	jnz .type_loop
	xor eax, eax
	dec eax
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
	
cfunction _add_typedfns
;	int3
	push ebx
	mov ebx, [esp + 8]
	mov eax, [ebx]
	cmp eax, byte -1
	je .finished
.loop:
	push dword [ebx + 8]
	push dword [ebx + 4]
	push dword [ebx]
	call _set
	add esp, byte 12
	add ebx, byte 12
	mov eax, [ebx]
	cmp eax, byte -1
	jne .loop
.finished:
	pop ebx
	ret

