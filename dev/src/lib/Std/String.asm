%include "Std.inc"
%include "Riva/Memory.inc"

ctype T
; The type of all strings.
.invoke: equ 0

cfunction _alloc
	mov eax, [esp + 4]
	lea eax, [(eax + 1) * 4]
	lea eax, [sizeof(Std$String_t) + (sizeof(Std$String_block) / 4) * eax]
	push eax
	call Riva$Memory$_alloc_stubborn
	mov [esp], eax
	mov ecx, [esp + 8]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$String_t(eax).Count], ecx
	lea eax, [Std$String_t(eax).Blocks]
	jecxz .empty	
.loop:
	mov [Std$Object_t(Std$String_block(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Object_t(Std$String_block(eax).Chars).Type], dword Std$Address$T
	add eax, byte sizeof(Std$String_block)
	dec ecx
	jnz .loop
.empty:
	pop eax
	ret

cfunction _freeze
	mov eax, [esp + 4]
	push dword [esp + 4]
	call Riva$Memory$_freeze_stubborn
	pop eax
	mov eax, [esp + 4]
	ret

extern strlen
cfunction _new
	mov eax, [esp + 4]
	test eax, eax
	jnz .notnull
	mov eax, Nil
	ret
.notnull:
	push eax
	call strlen
	mov [esp], eax
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	pop edx
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	test edx, edx
	setnz [Std$String_t(eax).Count]
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	ret

cfunction _copy
	mov eax, [esp + 4]
	test eax, eax
	jnz .notnull
	mov eax, Nil
	ret
.notnull:
	push eax
	call strlen
	mov [esp], eax
	push eax
	inc eax
	push eax
	call Riva$Memory$_alloc_atomic
	pop ecx
	pop ecx
	push edi
	push esi
	mov esi, [esp + 16]
	mov edi, eax
	mov [esp + 16], eax
	rep movsb
	mov [edi], byte 0
	pop esi
	pop edi
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	pop edx
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	test edx, edx
	setnz [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	ret

cfunction _new_length
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov edx, [esp + 8]
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	test edx, edx
	setnz [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	ret

cfunction _copy_length
	mov eax, [esp + 8]
	push eax
	inc eax
	push eax
	call Riva$Memory$_alloc_atomic
	pop ecx
	pop ecx
	push edi
	push esi
	mov esi, [esp + 12]
	mov edi, eax
	mov [esp + 12], eax
	rep movsb
	mov [edi], byte 0
	pop esi
	pop edi
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov edx, [esp + 8]
	mov ecx, [esp + 4]
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	test edx, edx
	setnz [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	ret

cfunction _new_char
	push byte 2
	call Riva$Memory$_alloc_atomic
	add esp, byte 4
	mov ecx, [esp + 4]
	mov [eax], cl
	mov [eax + 1], byte 0
	mov [esp + 4], eax
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov ecx, [esp + 4]
	mov edx, 1
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	mov [Std$String_t(eax).Count], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	ret

cfunction _flatten
	mov edx, [esp + 4]
	cmp edx, Nil
	jne .not_nil
	xor eax, eax
	ret
.not_nil:
	mov ecx, [Std$String_t(edx).Count]
	cmp ecx, byte 1
	ja .not_simple
	je .not_empty
	mov eax, .empty_string
	ret
.not_empty:
	mov ecx, [Std$Integer_smallt(Std$String_t(edx).Length).Value]
	mov eax, [Std$Address_t(Std$String_block(Std$String_t(edx).Blocks).Chars).Value]
	cmp [eax + ecx], byte 0
	jne .not_simple
	ret
.not_simple:
	mov ecx, [Std$Integer_smallt(Std$String_t(edx).Length).Value]
	inc ecx
	push ecx
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	push edi
	push esi
	mov edi, eax
	mov eax, [esp + 16]
	lea eax, [Std$String_t(eax).Blocks]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	jz .finished
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	mov [edi], byte 0
	pop esi
	pop edi
	pop eax
	ret
datasect
.empty_string:
	db 0

cfunction _flatten_to
	push edi
	push esi
	mov eax, [esp + 12]
	mov edi, [esp + 16]
	lea eax, [Std$String_t(eax).Blocks]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	jz .finished
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	mov [edi], byte 0
	pop esi
	pop edi
	ret

cfunction _content
	mov edx, [esp + 4]
	cmp edx, Nil
	jne .not_nil
	xor eax, eax
	ret
.not_nil:
	mov ecx, [Std$String_t(edx).Count]
	cmp ecx, byte 1
	ja .not_simple
	je .not_empty
	mov eax, .empty_string
	ret
.not_empty:
	mov eax, [Std$Address_t(Std$String_block(Std$String_t(edx).Blocks).Chars).Value]
	ret
.not_simple:
	mov ecx, [Std$Integer_smallt(Std$String_t(edx).Length).Value]
	push ecx
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	push edi
	push esi
	mov edi, eax
	mov eax, [esp + 16]
	lea eax, [Std$String_t(eax).Blocks]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	jz .finished
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	pop esi
	pop edi
	pop eax
	ret
datasect
.empty_string:
	db 0

cfunction _content_to
	push edi
	push esi
	mov eax, [esp + 12]
	mov edi, [esp + 16]
	lea eax, [Std$String_t(eax).Blocks]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	jz .finished
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	pop esi
	pop edi
	ret

function Ord, 1
;@str:T
;:Std$Integer$SmallT
; Returns the first byte of <var>str</var> as an integer.
	mov eax, [Std$Function_argument(edi).Val]
	cmp [Std$Integer_smallt(Std$String_t(eax).Length).Value], dword 0
	jne .nonempty
	xor eax, eax
	inc eax
	ret
.nonempty:
	xor edx, edx
	mov eax, [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value]
	mov dl, [eax]
	push edx
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

function Chr, 1
;@byte:Std$Integer$SmallT
;:T
; Returns a string <var>str</var> of length 1 with <code>Ord(str) = byte</code>.
	push byte 2
	call Riva$Memory$_alloc_atomic
	add esp, byte 4
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov [eax], cl
	mov [eax + 1], byte 0
	mov ebx, eax
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov edx, 1
	mov [Std$Object_t(eax).Type], dword T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	mov [Std$String_t(eax).Count], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ebx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

function FromAddress, 2
;@address:Std$Address$T
;@length:Std$Integer$SmallT
;:T
; Returns a string consisting of the <var>length</var> bytes at <var>address</var>.
; The memory at <var>address</var> is not copied and should not be modified after this call.
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi + 8).Val]
	mov [Std$Object_t(eax).Type], dword T
	mov ecx, [Std$Address_t(ecx).Value]
	mov edx, [Std$Integer_smallt(edx).Value]
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], edx
	test edx, edx
	setnz [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], ecx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	push eax
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

function GetBlockAddress, 2
;@t:T
;@n:Std$Integer$SmallT
;:Std$Address$T
; Returns the address of the bytes of block<sub><var>n</var></sub> of <var>t</var>, where block<sub><code>0</code></sub> is the first block.
; Fails if <var>n</var> is larger or equal than the number of blocks in <var>t</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	shl ecx, 4
	lea ecx, [Std$String_block(Std$String_t(eax).Blocks + ecx).Chars]
	cmp [Std$Object_t(ecx).Type], dword 0
	je .failure
	xor edx, edx
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

function GetBlockLength, 2
;@t:T
;@n:Std$Integer$SmallT
;:Std$Integer$SmallT
; Returns the length of the bytes of block<sub><var>n</var></sub> of <var>t</var>, where block<sub><code>0</code></sub> is the first block.
; Fails if <var>n</var> is larger or equal than the number of blocks in <var>t</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	shl ecx, 4
	lea ecx, [Std$String_block(Std$String_t(eax).Blocks + ecx).Length]
	cmp [Std$Object_t(ecx).Type], dword 0
	je .failure
	xor edx, edx
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

struct get_blocks_state, Std$Function_state
	.Block:		resd 1
	.Address:	resd 1
	.Length:	resd 1
endstruct

function GetBlocks, 3
;@t:T
;@addr:ANY
;@len:ANY
; For each block in <var>t</var>, stores the address and length of the block into <var>addr</var> and <var>len</var> respectively and generates <id>NIL</id>.
	mov eax, [Std$Function_argument(edi).Val]
	add eax, byte Std$String_t.Blocks
	cmp [eax], dword 0
	je .failure
	mov ecx, [Std$Function_argument(edi + 8).Ref]
	mov edx, [Std$Function_argument(edi + 16).Ref]
	mov [ecx], eax
	add eax, byte 8
	mov [edx], eax
	add eax, byte 8
	cmp [eax], dword 0
	jne .suspend
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret
.suspend:
	push edx
	push ecx
	push eax
	push byte sizeof(get_blocks_state)
	call Riva$Memory$_alloc
	pop edx
	mov dword [Std$Function_state(eax).Run], .resume
	pop dword [get_blocks_state(eax).Block]
	pop dword [get_blocks_state(eax).Address]
	pop dword [get_blocks_state(eax).Length]
	mov ecx, Std$Object$Nil
	xor edx, edx
	mov ebx, eax
	or eax, byte -1
	ret
.failure:
	xor eax, eax
	inc eax
	ret
.resume:
	mov ecx, [get_blocks_state(eax).Address]
	mov edx, [get_blocks_state(eax).Length]
	mov ebx, [get_blocks_state(eax).Block]
	mov [ecx], ebx
	add ebx, byte 8
	mov [edx], ebx
	add ebx, byte 8
	mov ecx, Std$Object$Nil
	xor edx, edx
	cmp [ebx], dword 0
	jne .resuspend
	xor eax, eax
	ret
.resuspend:
	mov [get_blocks_state(eax).Block], ebx
	mov ebx, eax
	or eax, byte -1
	ret

textsect
compress:
;	int3
	mov ebx, [Std$String_t(eax).Count]
	cmp ebx, byte 1
	jbe .simple
	push ebp
	push eax
	; Length/Count ratio should be > 16????
	shl ebx, 4
	lea edx, [Std$String_t(eax).Blocks - sizeof(Std$String_block)]
	lea ebp, [Std$String_t(eax).Blocks - sizeof(Std$String_block)]
.start1:
	add edx, byte sizeof(Std$String_block)
	add ebp, byte sizeof(Std$String_block)
	cmp [edx], dword 0
	je .finished
	mov eax, edx
	mov ecx, [Std$Integer_smallt(Std$String_block(edx).Length).Value]
	cmp ecx, ebx
	jbe .start2
	cmp edx, ebp
	je .start1
.copy:
	mov eax, [edx]
	mov [ebp], eax
	mov eax, [edx + 4]
	mov [ebp + 4], eax
	mov eax, [edx + 8]
	mov [ebp + 8], eax
	mov eax, [edx + 12]
	mov [ebp + 12], eax
	jmp .start1
.simple:
	ret
.loop1:
	add ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	cmp ecx, ebx
	ja .exit1
.start2:
	add eax, byte sizeof(Std$String_block)
	cmp [eax], dword 0
	jne .loop1
	sub eax, byte sizeof(Std$String_block)
.exit1:
	cmp eax, ebp
	je .start1
	cmp eax, edx
	je .copy
	mov esi, eax
	mov edi, edx
	push ecx
	inc ecx
	push ecx
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	xchg eax, edi
	mov edx, esi
.loop2:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	cmp eax, edx
	jbe .loop2
	mov [edi], byte 0
	pop dword [Std$Address_t(Std$String_block(ebp).Chars).Value]
	pop dword [Std$Integer_smallt(Std$String_block(ebp).Length).Value]
	jmp .start1
.finished:
	xor edx, edx
	mov [ebp], edx
	mov [ebp + 4], edx
	mov [ebp + 8], edx
	mov [ebp + 12], edx
	pop eax
	sub ebp, eax
	sub ebp, byte sizeof(Std$String_t)
	shr ebp, 4
	mov [Std$String_t(eax).Count], ebp
	pop ebp
	ret

%ifdef DOCUMENTING
function Add, 2
;@a:T
;@b:T
;:T
; Returns the concatenation of <var>a</var> and <var>b</var>.
%else
function $Add, 2
%endif
	mov ebx, [Std$Function_argument(edi + 8).Val]
	mov esi, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	test esi, esi
	jnz .first_not_empty
	mov ecx, [Std$Function_argument(edi).Val]
	xor edx, edx
	xor eax, eax
	ret
.first_not_empty:
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	test ecx, ecx
	jnz .second_not_empty
	mov ecx, [Std$Function_argument(edi + 8).Val]
	xor edx, edx
	xor eax, eax
	ret
.second_not_empty:
	add esi, ecx
	mov ecx, [Std$String_t(ebx).Count]
	add ecx, [Std$String_t(eax).Count]
	push esi
	push ecx
	lea ecx, [2 * ecx + 2]
	lea eax, [sizeof(Std$String_t) + ecx * (sizeof(Std$String_block) / 2)]
	push eax
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
	pop dword [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edx, edi
	lea edi, [Std$String_t(eax).Blocks]
	mov ebx, [Std$Function_argument(edx).Val]
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	mov ebx, [Std$Function_argument(edx + 8).Val]
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	call compress
	push eax
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

cglobal Nil, T
; String corresponding to the address <code>0</code>. Needed for bindings to external libraries.
	dd T
	dd 0
	dd Std$Integer$SmallT
	dd 0
	dd 0, 0
	dd 0, 0, 0, 0

cglobal Empty, T
; Empty string <code>""</code>.
	dd T
	dd 0
	dd Std$Integer$SmallT
	dd 0
	dd 0, 0
	dd 0, 0, 0, 0

cglobal Lower, T
; String consisting of lower case letters.
	dd T
	dd 1
	dd Std$Integer$SmallT
	dd 26
	dd 0, 0
	dd Std$Integer$SmallT, 26
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
	db "abcdefghijklmnopqrstuvwxyz"

cglobal Upper, T
; String consisting of upper case letters.
	dd T
	dd 1
	dd Std$Integer$SmallT
	dd 26
	dd 0, 0
	dd Std$Integer$SmallT, 26
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
	db "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

cfunction _add
	push ebx
	push esi
	push edi
	mov ebx, [esp + 20]
	mov esi, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	test esi, esi
	jnz .first_not_empty
	mov eax, [esp + 16]
	pop edi
	pop esi
	pop ebx
	ret
.first_not_empty:
	mov eax, [esp + 16]
	mov ecx, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	test ecx, ecx
	jnz .second_not_empty
	mov eax, [esp + 20]
	pop edi
	pop esi
	pop ebx
	ret
.second_not_empty:
	add esi, ecx
	mov ecx, [Std$String_t(ebx).Count]
	add ecx, [Std$String_t(eax).Count]
	push esi
	push ecx
	lea ecx, [2 * ecx + 2]
	lea eax, [sizeof(Std$String_t) + ecx * (sizeof(Std$String_block) / 2)]
	push eax
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
	pop dword [Std$String_t(eax).Count]
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edx, edi
	lea edi, [Std$String_t(eax).Blocks]
	mov ebx, [esp + 16]
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	mov ebx, [esp + 20]
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	call compress
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	pop edi
	pop esi
	pop ebx
	ret

cfunction _slice
	push ebx
	push edi
	push esi
	mov eax, [esp + 16]
	mov ebx, [esp + 20]
	mov ecx, [esp + 24]
	cmp ebx, ecx
	jl .notempty
	mov eax, .empty_string
	pop esi
	pop edi
	pop ebx
	ret
.notempty:
	xor edx, edx
	lea esi, [Std$String_t(eax).Blocks - sizeof(Std$String_block)]
.findfirstloop:
	add esi, byte sizeof(Std$String_block)
	add edx, [Std$Integer_smallt(Std$String_block(esi).Length).Value]
	cmp edx, ebx
	jbe .findfirstloop
	push esi
	sub edx, [Std$Integer_smallt(Std$String_block(esi).Length).Value]
	mov eax, ebx
	sub eax, edx
	push eax
	sub esi, byte sizeof(Std$String_block)
.findsecondloop:
	add esi, byte sizeof(Std$String_block)
	add edx, [Std$Integer_smallt(Std$String_block(esi).Length).Value]
	cmp edx, ecx
	jb .findsecondloop
	sub edx, ecx
	push edx
	sub esi, [esp + 8]
	lea eax, [sizeof(Std$String_t) + esi + 2 * sizeof(Std$String_block)]
	shr esi, 4
	push ecx
	inc esi
	sub [esp], ebx
	push esi
	push eax
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword T
	pop ecx
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov [Std$String_t(eax).Count], ecx
	mov esi, [esp + 8]
	shl ecx, 2
	lea edi, [Std$String_t(eax).Blocks]
	rep movsd
	pop edx
	sub [Std$Integer_smallt(Std$String_block(edi - sizeof(Std$String_block)).Length).Value], edx
	pop edx
	add [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], edx
	sub [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], edx
	add esp, byte 4
	push eax
	call Riva$Memory$_freeze_stubborn
	pop eax
	pop esi
	pop edi
	pop ebx
	ret
datasect
.empty_string:
	dd T
	dd 0
	dd Std$Integer$SmallT
	dd 0
	dd 0, 0
	dd 0, 0, 0, 0

function Hash, 1
;@t:T
;:Std$Integer$SmallT
; Returns a hash code for <var>t</var>.
	mov edx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$String_t(edx).Hash]
	cmp [Std$Object_t(ecx).Type], dword Std$Integer$SmallT
	je .cached
	lea edi, [Std$String_t(edx).Blocks]
	xor ebx, ebx
	xor eax, eax
	mov edx, [Std$Integer_smallt(Std$String_block(edi).Length).Value]
	test edx, edx
	jz .done
.outer_loop:
	mov esi, [Std$Address_t(Std$String_block(edi).Chars).Value]
.inner_loop:
	lodsb
	lea ebx, [9 * ebx]
	xor ebx, eax
	dec edx
	jnz .inner_loop
	add edi, byte sizeof(Std$String_block)
	mov edx, [Std$Integer_smallt(Std$String_block(edi).Length).Value]
	test edx, edx
	jnz .outer_loop
.done:
	mov [Std$Object_t(ecx).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(ecx).Value], ebx
.cached:
	xor edx, edx
	xor eax, eax
	ret


function Compare, 2
;@a:T
;@b:T
;:Std$Object$T
; Returns <id>Std/Object/Less</id>, <id>Std/Object/Equal</id> or <id>Std/Object/Greater</id> depending on whether <var>a</var> is lexically less than, equal to or greater than <var>b</var>.
   	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi + 8).Val]
	lea eax, [Std$String_t(eax).Blocks]
	lea ebx, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	mov edx, [Std$Integer_smallt(Std$String_block(ebx).Length).Value]
	mov edi, [Std$Address_t(Std$String_block(ebx).Chars).Value]
	test ecx, ecx
	jnz .first_not_empty0
	test edx, edx
	jz .equal
	jmp .less
.first_not_empty0:
	test edx, edx
	jz .greater
.compare_loop:
	cmpsb
	jb .less
	ja .greater
	dec ecx
	jz .reload_first
.first_not_empty:
	dec edx
	jnz .compare_loop
	add ebx, byte sizeof(Std$String_block)
	mov edx, [Std$Integer_smallt(Std$String_block(ebx).Length).Value]
	test edx, edx
	mov edi, [Std$Address_t(Std$String_block(ebx).Chars).Value]
	jnz .compare_loop
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret
.reload_first:
	add eax, byte sizeof(Std$String_block)
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	jnz .first_not_empty
	dec edx
	jnz .less
	cmp [Std$Integer_smallt(Std$String_block(ebx + sizeof(Std$String_block)).Length).Value], dword 0
	jne .less
.equal:
	mov ecx, Std$Object$Equal
	xor edx, edx
	xor eax, eax
	ret
.less:
	mov ecx, Std$Object$Less
	xor edx, edx
	xor eax, eax
	ret

extern Of

_function Create
;@value&lt;sub&gt;1&lt;/sub&gt;,...,value&lt;sub&gt;k&lt;/sub&gt;
;:T
; returns the concatenation of <var>value<sub>1</sub></var>, ... , <var>value<sub>k</sub></var> after converting them (if necessary) to strings.
	dec esi
	;js .failure
	jz near .one
	push ebp
	push edi
	mov ebp, esp
.loop:
	mov ecx, [Std$Function_argument(edi + 8 * esi).Val]
	cmp [Std$Object_t(ecx).Type], dword T
	je .already_string
	push esi
	push edi

	push byte 0
	push ecx
	mov esi, 1
	mov edi, esp
	mov ecx, Of
	call Std$Symbol$T.invoke
	add esp, byte 8

	cmp eax, byte 0
	jg .error
	pop edi
	pop esi
.already_string:
	push ecx
	dec esi
	jns .loop
	
	xor eax, eax
	xor edx, edx
	mov esi, esp
.loop2:
	mov ecx, [esi]
	add eax, [Std$String_t(ecx).Count]
	add edx, [Std$Integer_smallt(Std$String_t(ecx).Length).Value]
	add esi, byte 4
	cmp esi, ebp
	jne .loop2
	
	push eax
	inc eax
	push edx
	shl eax, 4
	add eax, byte sizeof(Std$String_t)
	push eax
	call Riva$Memory$_alloc_stubborn
	pop edx
	mov dword [Std$Object_t(eax).Type], T
	mov dword [Std$Object_t(Std$String_t(eax).Length).Type], Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	pop dword [Std$String_t(eax).Count]
	lea edi, [Std$String_t(eax).Blocks]

.loop3:
	pop ecx
	lea esi, [Std$String_t(ecx).Blocks]
	mov ecx, [Std$String_t(ecx).Count]
	shl ecx, 2
	rep movsd
	cmp esp, ebp
	jne .loop3
	
	call compress
	push eax
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	pop edi
	pop ebp
	ret

.error:
	mov esp, ebp
	pop edi
	pop ebp
	ret
.one:
	mov ecx, [Std$Function_argument(edi).Val]
	cmp [Std$Object_t(ecx).Type], dword T
	je .already_string1

	push byte 0
	push ecx
	mov esi, 1
	mov edi, esp
	mov ecx, Of
	call Std$Symbol$T.invoke
	add esp, byte 8

	cmp eax, byte 0
	jg .error1
.already_string1:
	xor edx, edx
	xor eax, eax
.error1:
	ret

%ifdef DOCUMENTING

%define Std$String$T T

%define string_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
