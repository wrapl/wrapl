%include "Std.inc"
%include "Riva/Memory.inc"

struct _table
	.Type:			resd 1
	.Root:			resd 1
	.Count:			resd 1
	.Generation:	resd 1
	.Compare:		resd 1
	.Hash:			resd 1
	.Default:		resd 1
endstruct

struct _node
	.Type:		resd 1
	.Link:
	.Left:		resd 1
	.Right:		resd 1
	.Key:		resd 1
	.Value:		resd 1
	.Hash:		resd 1
	.Balance:	resd 1
endstruct

extern T
extern NodeType

global avl_compare_asm
textsect
avl_compare_asm:
	; eax = Table, edx = A, ecx = B
	push ebx
	push edi
	push esi
	push byte 0
	push ecx
	push byte 0
	push edx
	mov edi, esp
	mov esi, 2
	mov ecx, [_table(eax).Compare]
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	mov eax, [Std$Integer_smallt(ecx).Value]
	add esp, byte 16
	pop esi
	pop edi
	pop ebx
	ret

global avl_find_asm
textsect
avl_find_asm:
	; eax = Tree, edx = Key, ecx = Hash
	push ebx
	push edi
	push esi
	push ebp
	push dword [_table(eax).Compare]
	mov ebp, ecx
	push byte 0
	push byte 0
	push byte 0
	push edx
	mov eax, [_table(eax).Root]
.loop:
	test eax, eax
	jz .done
	cmp ebp, [_node(eax).Hash]
	jb .left
	ja .right
	mov edx, [_node(eax).Key]
	mov edi, esp
	mov esi, 2
	mov ecx, [esp + 16]
	push eax
	mov [Std$Function_argument(edi + 8).Val], edx
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	pop eax
	cmp [Std$Integer_smallt(ecx).Value], dword 0
	jl .left
	jg .right
	lea eax, [_node(eax).Value]
.done:
	add esp, byte 20
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.left:
	mov eax, [_node(eax).Left]
	jmp .loop
.right:
	mov eax, [_node(eax).Right]
	jmp .loop


global avl_probe_asm
textsect
avl_probe_asm:
	; eax = Tree, edx = Key, ecx = Hash
;	int3
	push ebx
	push edi
	push esi
	push ebp
	mov ebp, ecx

	sub esp, byte 32
	mov ecx, esp
	push eax
	push dword [_table(eax).Compare]
	push byte 0
	push byte 0
	push byte 0
	push edx
	mov edx, eax
	; top of stack = args for comparison
	; [esp + 16] = Compare
	; [esp + 20] = table
	; [esp + 24] = da[32]
	mov esi, [_table(eax).Root]
	mov edi, eax
	lea ebx, [_table(eax).Root]
	; [ebx] = p, edx = q, ecx = da + k, esi = y, edi = z
.find_loop:
	mov eax, [ebx]
	test eax, eax
	jz .insert
	cmp byte [_node(eax).Balance], 0

	lea ebx, [esp + 24]
	cmovnz edi, edx
	cmovnz esi, eax
	cmovnz ecx, ebx
	
	cmp ebp, [_node(eax).Hash]
	jb .left
	ja .right
	
	push eax
	push ecx
	push edx
	push edi
	push esi
	mov edx, [_node(eax).Key]
	lea edi, [esp + 20]
	mov esi, 2
	mov ecx, [esp + 36]
	mov [Std$Function_argument(edi + 8).Val], edx
	mov eax, [Std$Object_t(ecx).Type]
	call [Std$Type_t(eax).Invoke]
	cmp [Std$Integer_smallt(ecx).Value], dword 0
	pop esi
	pop edi
	pop edx
	pop ecx
	pop eax
	
	jl .left
	jg .right
	lea eax, [_node(eax).Value]
.done:
	add esp, byte 56
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.left:
	mov byte [ecx], 0
	inc ecx
	mov edx, eax
	lea ebx, [_node(eax).Left]
	jmp .find_loop
.right:
	mov byte [ecx], 1
	inc ecx
	mov edx, eax
	lea ebx, [_node(eax).Right]
	jmp .find_loop
.insert:
	; top of stack = args for comparison
	; [esp + 16] = Compare
	; ebx = &q->avl_link[dir]
	; esi = y, edi = z
	push byte sizeof(_node);
	call Riva$Memory$_alloc
	pop ecx
	mov [ebx], eax
	mov [Std$Object_t(eax).Type], dword NodeType
	pop dword [_node(eax).Key]
	add esp, byte 16
	; [esp] = table
	; [esp + 4] = da[32]
	; edi = z, esi = y
	mov [_node(eax).Hash], ebp
	mov ebx, [esp]
	inc dword [_table(ebx).Count]
	mov ebp, eax
	mov eax, esi
	test eax, eax
	jnz .balance
.l4:
	lea eax, [_node(ebp).Value]
	add esp, byte 36
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
.balance:
	lea ecx, [esp + 4]
.loop:
	cmp eax, ebp
	je .l1
	cmp byte [ecx], 0
	jnz .l2
	dec byte [_node(eax).Balance]
	mov eax, [_node(eax).Left]
	inc ecx
	jmp .loop
.l2:
	inc byte [_node(eax).Balance]
	mov eax, [_node(eax).Right]
	inc ecx
	jmp .loop
.l1:
	; [esp] = table
	; [esp + 4] = da[32]
	; ebp = n, esi = y, edi = z
%define Y esi
%define X ebx
%define W ecx
%define Z edi
	cmp byte [_node(Y).Balance], -2
	jne .l3
	mov X, [_node(Y).Left]
	cmp byte [_node(X).Balance], -1
	jne .l5
	mov W, X
	mov edx, [_node(X).Right]
	mov [_node(Y).Left], edx
	mov [_node(X).Right], Y
	mov byte [_node(Y).Balance], 0
	jmp .l0
.l5:
	mov W, [_node(X).Right]
	mov edx, [_node(W).Left]
	mov [_node(X).Right], edx
	mov [_node(W).Left], X
	mov edx, [_node(W).Right]
	mov [_node(Y).Left], edx
	mov [_node(W).Right], Y
	movsx edx, byte [_node(W).Balance]
	mov edx, [.t1 + 2 * edx]
	mov [_node(X).Balance], dl
	mov [_node(Y).Balance], dh
	jmp .l0
.l3:
	cmp byte [_node(Y).Balance], 2
	jne .l4
	mov X, [_node(Y).Right]
	cmp byte [_node(X).Balance], 1
	jne .l6
	mov W, X
	mov edx, [_node(X).Left]
	mov [_node(Y).Right], edx
	mov [_node(X).Left], Y
	mov byte [_node(Y).Balance], 0
	jmp .l0
.l6:
	mov W, [_node(X).Left]
	mov edx, [_node(W).Right]
	mov [_node(X).Left], edx
	mov [_node(W).Right], X
	mov edx, [_node(W).Left]
	mov [_node(Y).Right], edx
	mov [_node(W).Left], Y
	movsx edx, byte [_node(W).Balance]
	mov edx, [.t2 + 2 * edx]
	mov [_node(X).Balance], dl
	mov [_node(Y).Balance], dh
	jmp .l0
.l0:
	mov byte [_node(W).Balance], 0
	; [esp] = table
	; [esp + 4] = da[32]
	; ebp = n, esi = y, edi = z
	xor edx, edx
	cmp Y, [_node(Z).Left]
	setne dl
	mov [_node(Z).Link + 4 * edx], W
	mov eax, [esp]
	inc dword [_table(eax).Generation]
	lea eax, [_node(ebp).Value]
	add esp, byte 36
	pop ebp
	pop esi
	pop edi
	pop ebx
	ret
datasect
align 4
	db 0, 1 
.t1:
	db 0, 0
	db -1, 0
	db 1, 0
.t2:
	db 0, 0
	db 0, -1
