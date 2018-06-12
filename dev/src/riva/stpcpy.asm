section .text class=code use32
section .data class=data use32
section .bss class=bss use32

global stpcpy
section .text
align 8
stpcpy:
	mov ecx, [esp + 8]
	mov edx, [esp + 4]
	test edx, 3
	jz short .loop2
.loop:
    mov al, [ecx]
	or al, al
	jz short .finish
	mov [edx], al
	inc ecx
	inc edx
	test edx, 3
	jnz short .loop
.loop2:
    mov eax, [ecx]
	or al, al
	jz short .finish
	or ah, ah
	jz short .last1
	test eax, 0FF0000h
	jz short .last2
	test eax, 0FF000000h
	jz short .last3
	add ecx, 4
	mov [edx], eax
	add edx, 4
	jmp short .loop2
.last3:
    mov [edx], eax
	inc edx
	inc edx
	inc edx
	mov eax, edx
	retn
.last2:
    mov [edx], eax
	inc edx
	inc edx
	mov eax, edx
	retn
.last1:
    mov [edx], ax
	inc edx
	mov eax, edx
	retn
.finish:
    mov [edx], al
	mov eax, edx
	retn

global mempcpy
align 8
mempcpy:
	mov edx, esi
	mov eax, edi
	mov ecx, [esp + 12]
	mov edi, [esp + 8]
	mov esi, [esp + 4]
	shl ecx, 1
	jc .plus1
	shl ecx, 1
	jc .plus2
	rep movsd
	xchg eax, edi
	mov esi, edx
	ret
.plus1:
	shl ecx, 1
	jc .plus3
	rep movsd
	movsb
	xchg eax, edi
	mov esi, edx
	ret
.plus2:
	rep movsd
	movsw
	xchg eax, edi
	mov esi, edx
	ret
.plus3:
	rep movsd
	movsw
	movsb
	xchg eax, edi
	mov esi, edx
	ret