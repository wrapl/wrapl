bits 32

global init_text_section

extern relocate_text_section

section .text
init_text_section:
	sub dword [esp], 5
	pushad
	push dword [esp + 32]
	;int3
	call relocate_text_section
	;int3
	pop eax
	popad
	ret
