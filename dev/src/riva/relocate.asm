global init_text_section

extern relocate_text_section

section .text
init_text_section:
%ifdef X86
	sub dword [esp], 5
	pushad
	push dword [esp + 32]
	;int3
	call relocate_text_section
	;int3
	pop eax
	popad
	ret
%elifdef X64
	sub dword [rsp], 5
	push rax
	push rcx
	push rdx
	push r8
	push r9
	push r10
	push r11
	push rsi
	push rdi
	push qword [rsp + 9 * 8]
	call relocate_text_section
	pop rax
	pop rdi
	pop rsi
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	pop rax
	ret 
%endif
