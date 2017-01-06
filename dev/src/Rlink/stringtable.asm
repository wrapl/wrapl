bits 32
;section .text class=code
;section .data class=data
;section .bss class=bss

%macro struct 1.nolist
	%define %1(X) X + %1
	struc %1
%endmacro

%macro struct 2.nolist
	%define %1(X) X + %1
	struc %1
		resb sizeof(%2)
%endmacro

%macro endstruct 0.nolist
	.__size__:
	endstruc
%endmacro

%define sizeof(T) T %+ .__size__

struct table
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

global stringtable_init
global stringtable_put
global stringtable_get
extern calloc

section .text
stringtable_init:;(stringtable Table) -> ()
	mov eax, [esp + 4]
	mov [table(eax).Size], dword 0
	mov [table(eax).Space], dword 0
	ret

stringtable_put:;(stringtable Table, const char *Key, void *Value) -> ()
	push ebx
	push edi
	push esi
	push ebp
	mov eax, [esp + 24]
	xor edx, edx
	xor ecx, ecx
	xor ebx, ebx
	add dl, [eax]
	jz .empty_string
.hash_loop:
	ror ebx, 13
	xor ebx, edx
	inc eax
	inc ecx
	mov dl, [eax]
	test dl, dl
	jnz .hash_loop
.empty_string:
	test ebx, ebx
	jnz .nonzero_hash
	dec ebx
.nonzero_hash:
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
	mov edx, [esp + 28]
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
	push byte 1
	push byte 64
	call calloc
	add esp, byte 8
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
	mov ecx, [esp + 28]
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
	cmp [node(ebp + 8 * eax).Key], dword 0
	jne .replace_node
.empty_node:
	mov esi, [esp + 24]
	mov edi, [esp + 28]
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
	mov esi, [esp + 24]
	mov edi, [esp + 28]
	push dword [node(ebp + 8 * eax).Key]
	push dword [node(ebp + 8 * eax).Hash]
	push dword [node(ebp + 8 * eax).Length]
	push dword [node(ebp + 8 * eax).Value]
	mov [node(ebp + 8 * eax).Key], esi
	mov [node(ebp + 8 * eax).Hash], ebx
	mov [node(ebp + 8 * eax).Length], ecx
	mov [node(ebp + 8 * eax).Value], edi
	pop dword [esp + 40]
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
	mov edi, [esp + 32]
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
	push byte 1
	push edx
	call calloc
	add esp, byte 8
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

stringtable_get:;(stringtable Table, const char *Key) -> (void *)
	push ebx
	push edi
	push esi
	push ebp
	mov eax, [esp + 24]
	xor edx, edx
	xor ecx, ecx
	xor ebx, ebx
	add dl, [eax]
	jz .empty_string
.hash_loop:
	ror ebx, 13
	xor ebx, edx
	inc eax
	inc ecx
	mov dl, [eax]
	test dl, dl
	jnz .hash_loop
.empty_string:
	test ebx, ebx
	jnz .nonzero_hash
	dec ebx
.nonzero_hash:
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
