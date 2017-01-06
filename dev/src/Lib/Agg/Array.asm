%include "Std.inc"

extern Std$Type$T
extern Std$Integer$SmallT
extern Std$Object$Nil
extern Riva$Memory$_alloc

c_type T
; A fixed length array of objects
.invoke: equ 0

func New, 1
; creates a new array with length elements, all initialized to <id>NIL</id>
;@length : Std$Integer$SmallT
;:T
	mov eax, [argument(edi).Val]
	mov eax, [small_int(eax).Value]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push byte sizeof(array)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [array(eax).Values]
	pop ecx
	mov [value(eax).Type], dword T
	mov [value(array(eax).Length).Type], dword Std$Integer$SmallT
	mov [small_int(array(eax).Length).Value], ecx
	push eax
	mov edi, [array(eax).Values]
	mov eax, Std$Object$Nil
	rep stosd
.empty:
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

c_func _new
	mov eax, [esp + 4]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	mov [esp], eax
	push edi
	mov edi, eax
	mov ecx, [esp + 8]
	mov eax, Std$Object$Nil
	rep stosd
	pop edi
	push byte sizeof(array)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [value(eax).Type], dword T
	mov [value(array(eax).Length).Type], dword Std$Integer$SmallT
	pop dword [array(eax).Values]
	pop dword [small_int(array(eax).Length).Value]
	ret
