%include "Std.inc"
%include "Riva/Memory.inc"

%define Std$Integer$T T
%define Std$Integer$SmallT SmallT
%define Std$Integer$BigT BigT

ctype T, Std$Number$T, Std$Function$T
;  base type of all integer types
.invoke: equ 0

ctype SmallT, T, Std$Number$T, Std$Function$T
;  integers that fit into a signed machine word.
.invoke:
	mov eax, [Std$Integer_smallt(ecx).Value]
	dec eax
	jns .nonnegative
	add eax, esi
	inc eax
	js .failure
.nonnegative:
	cmp eax, esi
	jae .failure
	mov ecx, [Std$Function_argument(edi + 8 * eax).Val]
	mov edx, [Std$Function_argument(edi + 8 * eax).Ref]
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

cglobal Zero, T
	dd SmallT
	dd 0

cglobal One, T
	dd SmallT
	dd 1

ctype BigT, T, Std$Number$T, Std$Function$T
;  arbritrary precision integers.
;  note: instances of type <id>BigT</id> should never contain values which can fit into a signed machine word.
.invoke:
	; simply fail (we can't have this many Std$Function_arguments anyway)
	xor eax, eax
	inc eax
	ret

cfunction _alloc_small
	push byte sizeof(Std$Integer_smallt)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword SmallT
	ret

cfunction _alloc_big
	push byte sizeof(Std$Integer_bigt)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword BigT
	ret

extern __gmpz_init_set_str
extern __gmpz_fits_slong_p
extern __gmpz_get_si

cfunction _new_string ;(char *Digits)
	sub esp, byte 12
	mov eax, esp
	push byte 10
	push dword [esp + 20]
	push eax
	call __gmpz_init_set_str
	add esp, byte 12
	push esp
	call __gmpz_fits_slong_p
	add esp, byte 4
	test eax, eax
	jnz .convert_to_small
	call _alloc_big
	pop dword [eax + 4]
	pop dword [eax + 8]
	pop dword [eax + 12]
	ret
.convert_to_small:
	push esp
	call __gmpz_get_si
	add esp, byte 16
	push eax
	push byte sizeof(Std$Integer_smallt)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword SmallT
	pop dword [Std$Integer_smallt(eax).Value]
	ret

cfunction _new_string_base ;(char *Digits, int Base)
	sub esp, byte 12
	mov eax, esp
	push dword [esp + 20]
	push dword [esp + 20]
	push eax
	call __gmpz_init_set_str
	add esp, byte 12
	push esp
	call __gmpz_fits_slong_p
	add esp, byte 4
	test eax, eax
	jnz .convert_to_small
	call _alloc_big
	pop dword [eax + 4]
	pop dword [eax + 8]
	pop dword [eax + 12]
	ret
.convert_to_small:
	push esp
	call __gmpz_get_si
	add esp, byte 16
	push eax
	push byte sizeof(Std$Integer_smallt)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword SmallT
	pop dword [Std$Integer_smallt(eax).Value]
	ret

cfunction _new_small ;(long  Value)
	push byte sizeof(Std$Integer_smallt)
	call Riva$Memory$_alloc_atomic
	pop ecx
	mov [Std$Object_t(eax).Type], dword SmallT
	mov ecx, [esp + 4]
	mov [Std$Integer_smallt(eax).Value], ecx
	ret

extern __gmpz_init_set
cfunction _new_big ;(mpz_t Value)
	push byte sizeof(Std$Integer_bigt)
	call Riva$Memory$_alloc
	pop ecx
	mov [Std$Object_t(eax).Type], dword BigT
	push eax
	lea eax, [Std$Integer_bigt(eax).Value]
	push dword [esp + 8]
	push eax
	call __gmpz_init_set
	add esp, byte 8
	pop eax
	ret

extern __gmp_set_memory_functions

textsect
_realloc:
	push dword [esp + 12]
	push dword [esp + 8]
	call Riva$Memory$_realloc
	add esp, byte 8
_free:
	ret	

struct to_small_small_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

_function ToSmallSmall
; generates the values from <var>from</var> to <var>to</var> inclusive
;@from : SmallT
;@to : SmallT
;:SmallT
	mov eax, [Std$Function_argument(edi + 8).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Integer_smallt(eax).Value]
	cmp ebx, [Std$Integer_smallt(ecx).Value]
	jl .fail
	je .return
.suspend:
	push ecx
	push dword [Std$Integer_smallt(ecx).Value]
	push byte sizeof(to_small_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [to_small_small_state(eax).Current]
	mov [to_small_small_state(eax).Limit], ebx
	mov dword [Std$Function_state(eax).Run], .resume
	pop ecx
	mov ebx, eax
	or eax, byte -1
	xor edx, edx
	ret
.return:
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
	mov ebx, [to_small_small_state(eax).Current]
	inc ebx
	cmp ebx, [to_small_small_state(eax).Limit]
	jg .fail
	je .return2
	mov [to_small_small_state(eax).Current], ebx
	push eax
	call _alloc_small
	mov ecx, eax
	mov [Std$Integer_smallt(ecx).Value], ebx
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret
.return2:
	call _alloc_small
	mov [Std$Integer_smallt(eax).Value], ebx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

_function CompareSmall
;@a:SmallT
;@b:SmallT
;:Std$Object$T
; Compares <var>a</var> and <var>b</var> and return either <id>Std/Object/Less</id>, <id>Std/Object/Equal</id> or <id>Std/Object/Greater</id>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jl .less
	jg .greater
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
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_cmp
extern __gmpz_cmp_si

_function Compare
;@a:T
;@b:T
;:Std$Object$T
; Compares <var>a</var> and <var>b</var> and return either <id>Std/Object/Less</id>, <id>Std/Object/Equal</id> or <id>Std/Object/Greater</id>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi + 8).Val]
	cmp [Std$Object_t(eax).Type], dword SmallT
	jne .first_not_small
	cmp [Std$Object_t(ecx).Type], dword SmallT
	jne .small_big
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jl .less
	jg .greater
	jmp .equal
.small_big:
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi + 8).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jg .less
	jl .greater
	jmp .equal
.first_not_small:
	cmp [Std$Object_t(ecx).Type], dword SmallT
	jne .big_big
.big_small:
	mov eax, [Std$Function_argument(edi + 8).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jl .less
	jg .greater
	jmp .equal
.big_big:
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jl .less
	jg .greater
	;jmp .equal
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
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret

_function HashSmall
;@a:SmallT
;:SmallT
; Returns a hash for <var>a</var>.
	mov ecx, [Std$Function_argument(edi).Val]
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_get_si

_function Hash
;@a:T
;:SmallT
; Returns a hash for <var>a</var>.
	mov ecx, [Std$Function_argument(edi).Val]
	cmp [Std$Object_t(ecx).Type], dword SmallT
	jne .not_small
	xor edx, edx
	xor eax, eax
	ret
.not_small:
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push ecx
	call __gmpz_get_si
	mov [esp], eax
	call _alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

%ifdef DOCUMENTING

%define Std$Integer$T T
%define Std$Integer$SmallT SmallT
%define Std$Integer$BigT BigT

%define integer_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
