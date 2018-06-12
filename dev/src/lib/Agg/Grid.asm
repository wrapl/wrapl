%include "Std.inc"
%include "Riva/Memory.inc"

struct _grid, Std$Object_t
	.Degree:	resd 1
	.Dims:		resd 1
	.Data:		resd 1
endstruct

struct _dim
	.Limit:		resd 1
	.Stride:	resd 1
endstruct

ctype T
.invoke: equ 0

function New, 1
;@size&lt;sub&gt;1&lt;/sub&gt;,&#160;...,&#160;size&lt;sub&gt;k&lt;/sub&gt;
;:T
; returns a grid with dimensions <var>size<sub>1</sub></var> x ... x <var>size<sub>k</sub></var>.
	lea eax, [(esi + 1) * sizeof(_dim)]
	push esi
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	lea ebx, [eax + esi * sizeof(_dim)]
	xor edx, edx
	dec esi
	mov [_dim(ebx).Limit], edx
	mov [_dim(ebx).Stride], edx
	mov eax, 4
.loop1:
	sub ebx, byte sizeof(_dim)
	mov ecx, [Std$Function_argument(edi + 8 * esi).Val]
	mov [_dim(ebx).Stride], eax
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov [_dim(ebx).Limit], ecx
	imul ecx
	dec esi
	jns .loop1
	mov ebx, eax
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	mov ecx, ebx
	shr ecx, 2
	mov edi, eax
	mov eax, Std$Object$Nil
	rep stosd
	push byte sizeof(_grid)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword T
	pop dword [_grid(eax).Data]
	pop dword [_grid(eax).Dims]
	pop dword [_grid(eax).Degree]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
