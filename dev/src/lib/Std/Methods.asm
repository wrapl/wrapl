%include "Std.inc"
%include "Riva/Memory.inc"

%define ADDRESS TYP, Std$Address$T
%define BUFFER TYP, Std$Buffer$T
%define SMALLINT TYP, Std$Integer$SmallT
%define BIGINT TYP, Std$Integer$BigT
%define REAL TYP, Std$Real$T
%define STRING TYP, Std$String$T
%define OBJECT TYP, Std$Object$T
%define SYMBOL TYP, Std$Symbol$T
%define ANY SKP

%ifndef DOCUMENTING

%define address_method method
%define array_method method
%define function_method method
%define integer_method method
%define number_method method
%define object_method method
%define rational_method method
%define real_method method
%define string_method method
%define symbol_method method
%define type_method method
%define normal_method method

%endif

local_function Self
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

;_amethod Std$String$New, Self, STRING
;_amethod Std$Integer$New, Self, SMALLINT
;_amethod Std$Integer$New, Self, BIGINT

amethod Std$String$Of, TYP, Std$String$T
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

amethod Std$Integer$Of, TYP, Std$Integer$SmallT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

amethod Std$Integer$Of, TYP, Std$Integer$BigT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

amethod Std$Real$Of, TYP, Std$Real$T
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

symbol ?COMP, "?"
symbol ?HASH, "#"
symbol ?EQUAL, "="

cstring CompareError
	db "Compare Error"
cstrend

object_method "max", ANY, ANY
	push edi
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Greater
	pop edi
	setne al
	lea eax, [edi + 8 * eax]
	mov ecx, [Std$Function_argument(eax).Val]
	mov edx, [Std$Function_argument(eax).Ref]
	xor eax, eax
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 4
	ret

object_method "min", ANY, ANY
	push edi
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Less
	pop edi
	setne al
	lea eax, [edi + 8 * eax]
	mov ecx, [Std$Function_argument(eax).Val]
	mov edx, [Std$Function_argument(eax).Ref]
	xor eax, eax
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 4
	ret

object_method "<", ANY, ANY
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Less
	setne al
	pop edx
	pop ecx
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 8
	ret

object_method ">", ANY, ANY
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Greater
	setne al
	pop edx
	pop ecx
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 8
	ret

object_method "<=", ANY, ANY
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Greater
	sete al
	pop edx
	pop ecx
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 8
	ret

object_method ">=", ANY, ANY
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Less
	sete al
	pop edx
	pop ecx
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 8
	ret

extern Agg$ObjectTable$T

object_method "=", ANY, ANY, TYP, Agg$ObjectTable$T
	mov ecx, [Std$Function_argument(edi, 1).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	je .equal
	push ecx
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	xor eax, eax
	cmp ecx, Std$Object$Equal
	setne al
	pop edx
	pop ecx
	ret
.equal:
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.failure:
	mov ecx, CompareError
	mov eax, 2
.message:
	add esp, byte 8
	ret

object_method "=", ANY, ANY
	mov ecx, [Std$Function_argument(edi, 1).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	jne .notidentical
	mov edx, [Std$Function_argument(edi, 1).Val]
	xor eax, eax
	ret
.notidentical:
	push byte 0
	push byte 0
	push byte 0
	push dword Agg$ObjectTable$T
	mov eax, esp
	push byte 0
	push dword eax
	push dword [Std$Function_argument(edi, 1).Ref]
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi).Ref]
	push dword [Std$Function_argument(edi).Val]
	mov edi, esp
	mov esi, 3
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	add esp, byte 40
	ret

object_method "~=", ANY, ANY
	mov ecx, [Std$Function_argument(edi, 1).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	je .equal
	push ecx
	push dword [Std$Function_argument(edi, 1).Ref]
	mov ecx, ?EQUAL
	call Std$Symbol$T.invoke
	cmp eax, byte 1
	je .failure
	jg .message
	add esp, byte 8
.equal:
	xor eax, eax
	inc eax
	ret
.failure:
	xor eax, eax
	pop edx
	pop ecx
	ret
.message:
	add esp, byte 8
	ret

address_method "fill8", ADDRESS, SMALLINT, SMALLINT
;@dst
;@value
;@count
; Write <var>count</var> copies of <var>value</var> to <var>dst</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	mov edi, [Std$Address_t(eax).Value]
	mov eax, [Std$Integer_smallt(ebx).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	rep stosb
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "fill32", ADDRESS, SMALLINT, SMALLINT
;@dst
;@value
;@count
; Write <var>count</var> copies of <var>value</var> to <var>dst</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	mov edi, [Std$Address_t(eax).Value]
	mov eax, [Std$Integer_smallt(ebx).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	rep stosd
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, ADDRESS, SMALLINT
;@dst
;@src
;@length
; Copies <var>length</var> bytes from <var>src</var> to <var>dst</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	mov edi, [Std$Address_t(eax).Value]
	mov esi, [Std$Address_t(ebx).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	rep movsb
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, ADDRESS, SMALLINT, SMALLINT
;@dst
;@src
;@length
;@offset
; Copies <var>length</var> bytes from <var>src</var> to <code>dst + offset</code>.
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	mov edx, [Std$Function_argument(edi, 3).Val]
	mov edi, [Std$Address_t(eax).Value]
	add edi, [Std$Integer_smallt(edx).Value]
	mov esi, [Std$Address_t(ebx).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	rep movsb
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put8", ADDRESS, SMALLINT
;@dst
;@int
; Writes the byte <var>int</var> at <var>dst</var>.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	mov al, [Std$Integer_smallt(eax).Value]
	mov [esi], al
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, OBJECT
;@dst
;@object
; Writes the address of <var>object</var> at <var>dst</var>.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, OBJECT, SMALLINT
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put32", ADDRESS, SMALLINT
;@dst
;@int
; Writes the int <var>int</var> at <var>dst</var>.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "puta", ADDRESS, ADDRESS
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	mov eax, [Std$Address_t(eax).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put8", ADDRESS, SMALLINT, SMALLINT
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	mov al, [Std$Integer_smallt(eax).Value]
	mov [esi], al
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put32", ADDRESS, SMALLINT, SMALLINT
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "puta", ADDRESS, SMALLINT, SMALLINT
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	mov eax, [Std$Address_t(eax).Value]
	mov [esi], eax
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "putf32", ADDRESS, REAL
;@dst
;@real
; Writes the real <var>real</var> at <var>dst</var> as a 32 bit float.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	fld qword [Std$Real_t(eax).Value]
	fstp dword [esi]
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "putf64", ADDRESS, REAL
;@dst
;@real
; Writes the real <var>real</var> at <var>dst</var> as a 64 bit float.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esi]
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "putf32", ADDRESS, REAL, SMALLINT
;@dst
;@real
;@offset
; Writes the real <var>real</var> at <var>dst</var> as a 32 bit float.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	fld qword [Std$Real_t(eax).Value]
	fstp dword [esi]
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "putf64", ADDRESS, REAL, SMALLINT
;@dst
;@real
;@offset
; Writes the real <var>real</var> at <var>dst</var> as a 64 bit float.
	mov esi, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esi]
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, STRING
;@dst
;@string
; Writes the bytes in <var>string</var> at <var>dst</var>.
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$String_t(eax).Blocks]
	mov edi, [Std$Function_argument(edi).Val]
	mov edi, [Std$Address_t(edi).Value]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .finished
	mov esi, [Std$Integer_smallt(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "put", ADDRESS, STRING, SMALLINT
;@dst
;@string
;@offset
; Writes the bytes in <var>string</var> at <var>dst</var>.
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	lea eax, [Std$String_t(eax).Blocks]
	mov edi, [Std$Function_argument(edi).Val]
	mov edi, [Std$Address_t(edi).Value]
	add edi, [Std$Integer_smallt(ebx).Value]
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .finished
	mov esi, [Std$Integer_smallt(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .loop
.finished:
	mov ecx, Std$Object$Nil
	xor edx, edx
	xor eax, eax
	ret

address_method "get", ADDRESS
;@src
;:ANY
; Returns the object whose address is stored at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov edx, [Std$Address_t(eax).Value]
	mov ecx, [edx]
	xor eax, eax
	ret

address_method "get", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Address_t(eax).Value]
	add edx, [Std$Integer_smallt(ebx).Value]
	mov ecx, [edx]
	xor eax, eax
	ret

address_method "get8", ADDRESS
;@src
;:Std$Integer$SmallT
; Returns the byte at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	movzx eax, byte [eax]
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get16", ADDRESS
;@src
;:Std$Integer$SmallT
; Returns the 16 bit int at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	mov eax, [eax]
	and eax, 0xFFFF
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get24", ADDRESS
;@src
;:Std$Integer$SmallT
; Returns the 24 bit int at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	mov eax, [eax]
	and eax, 0xFFFFFF
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get32", ADDRESS
;@src
;:Std$Integer$SmallT
; Returns the 32 bit int at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	push dword [eax]
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "geta", ADDRESS
;@src
;:Std$Integer$SmallT
; Returns the address stored at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	push dword [eax]
	call Std$Address$_alloc
	pop dword [Std$Address_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "getf32", ADDRESS
;@src
;:Std$Real$T
; Returns the 32 bit float at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	fld dword [eax]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "getf64", ADDRESS
;@src
;:Std$Real$T
; Returns the 64 bit float at <var>src</var>.
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Address_t(eax).Value]
	fld qword [eax]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get8", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	movzx eax, byte [eax]
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get16", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	mov eax, [eax]
	and eax, 0xFFFF
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get24", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	mov eax, [eax]
	and eax, 0xFFFFFF
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "get32", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	push dword [eax]
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

address_method "geta", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Address_t(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	push dword [eax]
	call Std$Address$_alloc
	pop dword [Std$Address_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

%if 0
address_method "gets", ADDRESS, SMALLINT
;@src
;@length
; Returns a string composed of the <var>length</var> bytes at <var>src</var>.
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	cmp ebx, MAX_STRING_BLOCK_SIZE
	jg .large
	push ebx
	inc ebx
	push ebx
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov esi, [Std$Function_argument(edi).Val]
	mov esi, [Std$Address_t(esi).Value]
	mov edi, eax
	mov ecx, [esp + 4]
	rep movsb
	mov [edi], byte 0
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword Std$String$T
	mov [Std$Object_t(Std$String_t(ecx).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Address_t(Std$String_block(Std$String_t(ecx).Blocks).Chars).Value]
	pop eax
	mov [Std$Integer_smallt(Std$String_t(ecx).Length).Value], eax
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(ecx).Blocks).Length).Value], eax
	mov [Std$String_t(ecx).Count], dword 1
	mov [Std$Object_t(Std$String_block(Std$String_t(ecx).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Object_t(Std$String_block(Std$String_t(ecx).Blocks).Chars).Type], dword Std$Address$T
	push ecx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret
.large:
	

address_method "gets", ADDRESS, SMALLINT, SMALLINT
;@src
;@length
;@offset
; Returns a string composed of the <var>length</var> bytes at <var>src</var>.
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	push eax
	inc eax
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov esi, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov esi, [Std$Address_t(esi).Value]
	add esi, [Std$Integer_smallt(ebx).Value]
	mov edi, eax
	mov ecx, [esp + 4]
	rep movsb
	mov [edi], byte 0
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov ecx, eax
	mov [Std$Object_t(ecx).Type], dword Std$String$T
	mov [Std$Object_t(Std$String_t(ecx).Length).Type], dword Std$Integer$SmallT
	pop dword [Std$Address_t(Std$String_block(Std$String_t(ecx).Blocks).Chars).Value]
	pop eax
	mov [Std$Integer_smallt(Std$String_t(ecx).Length).Value], eax
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(ecx).Blocks).Length).Value], eax
	mov [Std$String_t(ecx).Count], dword 1
	mov [Std$Object_t(Std$String_block(Std$String_t(ecx).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Object_t(Std$String_block(Std$String_t(ecx).Blocks).Chars).Type], dword Std$Address$T
	push ecx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret
%endif

address_method "+", SMALLINT, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [eax + 4]
	add eax, [ebx + 4]
	push eax
	call Std$Address$_alloc
	mov ecx, eax
	pop dword [eax + 4]
	xor edx, edx
	xor eax, eax
	ret

address_method "+", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [eax + 4]
	add eax, [ebx + 4]
	push eax
	call Std$Address$_alloc
	mov ecx, eax
	pop dword [eax + 4]
	xor edx, edx
	xor eax, eax
	ret

address_method "-", ADDRESS, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [eax + 4]
	sub eax, [ebx + 4]
	push eax
	call Std$Address$_alloc
	mov ecx, eax
	pop dword [eax + 4]
	xor edx, edx
	xor eax, eax
	ret

address_method "-", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [eax + 4]
	sub eax, [ebx + 4]
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [eax + 4]
	xor edx, edx
	xor eax, eax
	ret

extern sprintf

amethod Std$String$Of, ADDRESS
	push byte 12
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov ebx, [Std$Function_argument(edi).Val]
	push dword [Std$Address_t(ebx).Value]
	push .format
	push eax
	call sprintf
	add esp, byte 12
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.format:
	db "#%x", 0

amethod Std$String$Of, TYP, Std$Address$SizedT
	push byte 24
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov ebx, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(Std$Address_sizedt(ebx).Length).Value]
	push dword [Std$Address_t(ebx).Value]
	push .format
	push eax
	call sprintf
	add esp, byte 16
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.format:
	db "#%x:%d", 0, 0

method "length", TYP, Std$Address$SizedT
	mov eax, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Address_sizedt(eax).Length]
	xor edx, edx
	xor eax, eax
	ret

extern Riva$Debug$_stack_trace

type_method "@", ANY, TYP, Std$Type$T
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Object_t(ecx).Type]
	mov eax, [Std$Type_t(eax).Types]
.loop:
	cmp [eax], edx
	je .matched
	add eax, byte 4
	cmp [eax], dword 0
	jne .loop
	push byte sizeof(Std$Symbol_nomethodmessage)
	call Riva$Memory$_alloc
	mov [esp], eax
	mov [Std$Object_t(eax).Type], dword Std$Symbol$NoMethodMessageT
	mov [Std$Symbol_nomethodmessage(eax).Symbol], ebx
	push dword 12
	lea eax, [Std$Symbol_nomethodmessage(eax).Stack]
	push eax
	push esp
	call Riva$Debug$_stack_trace
	add esp, byte 12
	pop ecx
	mov [Std$Symbol_nomethodmessage(ecx).Count], eax
	xor edx, edx
	mov eax, 2
	ret
.matched:
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

datasect
LESS: dd Std$Integer$SmallT, -1
ZERO:
EQUAL: dd Std$Integer$SmallT, 0
ONE:
GREATER: dd Std$Integer$SmallT, 1

set_method "?", Std$Object$Compare, ANY, ANY
set_method "#", Std$Object$Hash, ANY

%if 0
object_method "?", ANY, ANY
	mov ecx, [Std$Function_argument(edi, 1).Val]
	cmp ecx, [Std$Function_argument(edi).Val]
	jne .notidentical
	mov ecx, EQUAL
	xor edx, edx
	xor eax, eax
	ret
.notidentical:
	push byte 0
	push byte 0
	push byte 0
	push dword Agg$ObjectTable$T
	mov eax, esp
	push byte 0
	push dword eax
	push dword [Std$Function_argument(edi, 1).Ref]
	push dword [Std$Function_argument(edi, 1).Val]
	push dword [Std$Function_argument(edi).Ref]
	push dword [Std$Function_argument(edi).Val]
	mov edi, esp
	mov esi, 3
	mov ecx, ?COMP
	call Std$Symbol$T.invoke
	add esp, byte 40
	ret

object_method "#", ANY
	push byte 0
	push byte 0
	push byte 0
	push dword Agg$ObjectTable$T
	mov eax, esp
	push byte 0
	push dword eax
	push dword [Std$Function_argument(edi).Ref]
	push dword [Std$Function_argument(edi).Val]
	mov edi, esp
	mov esi, 2
	mov ecx, ?HASH
	call Std$Symbol$T.invoke
	add esp, byte 32
	ret
%endif

amethod Std$String$Of, VAL, Std$Object$Nil
	mov ecx, .nilstr
	xor edx, edx
	xor eax, eax
	ret
datasect
.nilstr:
	dd Std$String$T
	dd 1
	dd Std$Integer$SmallT
	dd 3
	dd 0, 0
	dd Std$Integer$SmallT, 3
	dd Std$Address$T, .nil
	dd 0, 0, 0, 0
.nil:
	db "NIL", 0

amethod Std$String$Of, SYMBOL
	mov edx, [Std$Function_argument(edi).Val]
	lea edx, [Std$Symbol_t(edx).Name]
	mov ecx, [edx]
	xor eax, eax
	ret

set_method "[]", Std$Symbol$Get, SYMBOL

extern asprintf
amethod Std$String$Of, TYP, Std$Symbol$NoMethodMessageT
	mov edx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Symbol_nomethodmessage(edx).Symbol]
	push byte 0
	push dword [Std$Symbol_t(edx).Name]
	push byte 0
	push dword .messagestr
	mov edi, esp
	mov esi, 2
	mov ecx, Std$String$Add
	call Std$String$Add.invoke
	add esp, byte 16
	ret
datasect
.messagestr:
	dd Std$String$T
	dd 1
	dd Std$Integer$SmallT
	dd 11
	dd 0, 0
	dd Std$Integer$SmallT, 11
	dd Std$Address$T, .str
	dd 0, 0, 0, 0
.str:
	db "no method: ", 0

amethod Std$String$Of, TYP, Std$Function$FewArgsMessageT
	sub esp, byte 4
	mov edx, [Std$Function_argument(edi).Val]
	mov eax, esp
	push dword [Std$Function_fewargsmessage(edx).Count]
	mov edx, [Std$Function_fewargsmessage(edx).Func]
	push dword [Std$Function_checkedct(edx).Count]
	push dword [Std$Function_checkedct(edx).Line]
	push dword [Std$Function_checkedct(edx).File]
	push dword .format
	push eax
	call asprintf
	add esp, byte 24
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.format: db "%s:%d: expected %d args, received %d", 0

amethod Std$String$Of, TYP, Std$Function$ArgTypeMessageT
	sub esp, byte 4
	mov edx, [Std$Function_argument(edi).Val]
	mov eax, esp
	push dword [Std$Function_argtypemessage(edx).Index]
	mov edx, [Std$Function_argtypemessage(edx).Func]
	push dword [Std$Function_checkedct(edx).Line]
	push dword [Std$Function_checkedct(edx).File]
	push dword .format
	push eax
	call asprintf
	add esp, byte 20
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.format: db "%s:%d: argument %d has incorrect type", 0

cfunction bin2ascii_test
; Code borrowed from http://board.flatassembler.net/topic.php?t=3924
; Written by El Tangas based on code by Terje Mathisen
; Convert an unsigned 32-bit number into a 10-digit ascii value
.magic1: equ 0a7c5ac47h
.magic2: equ 068db8badh
	mov eax, .magic1
	mul esi
	shr esi, 3
	add esi, eax
	adc edx, 0
	shr esi, 20 ;separate remainder
	mov ebx, edx
	shl ebx, 12
	and edx, 0FFFF0000h ;mask quotient
	and ebx, 0FFFFFFFh ;remove quotient nibble from remainder.
	mov eax, .magic2
	mul edx
	add esi, ebx
	mov eax, edx
	shr edx, 28
	and eax, 0FFFFFFFh
	lea esi, [4 * esi + esi + 5] ;multiply by 5 and round up
	add dl, '0'
	mov ebx, esi
	and esi, 07FFFFFFh
	shr ebx, 27
	mov [edi], dl
	add bl, '0'
	lea eax, [4 * eax + eax + 5] ;mul by 5 and round up
	mov [edi + 5], bl
	lea esi, [4 * esi + esi]
	mov edx, eax
	and eax, 07FFFFFFh
	shr edx, 27
	lea ebx, [esi + 0c0000000h]
	shr ebx, 26
	and esi, 03FFFFFFh
	add dl, '0'
	lea eax, [4 * eax + eax]
	mov [edi + 1], dl
	lea esi, [4 * esi + esi]
	mov [edi + 6], bl
	lea edx, [eax + 0c0000000h]
	shr edx, 26
	and eax, 03FFFFFFh
	lea ebx, [esi + 60000000h]
	and esi, 01FFFFFFh
	shr ebx, 25
	lea eax, [4 * eax + eax]
	mov [edi + 2], dl
	lea esi, [4 * esi + esi]
	mov [edi + 7], bl
	lea edx, [eax + 60000000h]
	shr edx, 25
	and eax, 01FFFFFFh
	lea ebx, [esi + 30000000h]
	mov [edi + 3], dl
	shr ebx, 24
	and esi, 00FFFFFFh
	mov [edi + 8], bl
	lea edx, [4 * eax + eax + 30000000h]
	shr edx, 24
	lea ebx, [4 * esi + esi + 18000000h]
	shr ebx, 23
	mov [edi + 4], dl
	mov [edi + 9], bl
	ret

amethod Std$String$Of, SMALLINT
	mov ebx, 10
	mov eax, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(eax).Value]
	test esi, esi
	jnz .nonzero
	mov ecx, .zerostr
	xor edx, edx
	xor eax, eax
	ret
.nonzero:
	push byte 33
	call Riva$Memory$_alloc_atomic
	pop edi
	lea edi, [eax + 32]
	mov [edi], byte 0
	test esi, esi
	js .negative
.positive:
	mov eax, esi
.loop:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.negative:
	mov eax, esi
	neg eax
.loop2:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop2
	dec edi
	mov [edi], byte '-'
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.digits: db "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	align 4
cstring .zerostr
	db "0"
cstrend

;	mov ebx, [Std$Function_argument(edi).Val]
;	mov esi, [Std$Integer_smallt(ebx).Value]
;	test esi, esi
;	js .negative
;	jnz .positive
;	mov ecx, .zerostr
;	xor edx, edx
;	xor eax, eax
;	ret
;.positive:
;	push byte 0
;	push byte 0
;	push byte 0
;	push byte 0
;	mov edi, esp
;	call bin2ascii_test
;	mov eax, esp
;	dec eax
;.loop1:
;	inc eax
;	cmp byte [eax], '0'
;	je .loop1
;	jmp .finished
;.negative:
;	push byte 0
;	push byte 0
;	push byte 0
;	push byte 0
;	neg esi
;	lea edi, [esp + 1]
;	call bin2ascii_test
;	mov eax, esp
;.loop2:
;	inc eax
;	cmp byte [eax], '0'
;	je .loop2
;	dec eax
;	mov byte [eax], '-'
;.finished:
;	mov [esp], eax
;	call Std$String$_copy
;	add esp, byte 16
;	mov ecx, eax
;	xor edx, edx
;	xor eax, eax
;	ret
;cstring .zerostr
;	db "0"
;cstrend

amethod Std$String$Of, SMALLINT, SMALLINT
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	mov eax, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(eax).Value]
	test esi, esi
	jnz .nonzero
	mov ecx, .zerostr
	xor edx, edx
	xor eax, eax
	ret
.nonzero:
	push byte 33
	call Riva$Memory$_alloc_atomic
	pop edi
	lea edi, [eax + 32]
	mov [edi], byte 0
	test esi, esi
	js .negative
.positive:
	mov eax, esi
.loop:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.negative:
	mov eax, esi
	neg eax
.loop2:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop2
	dec edi
	mov [edi], byte '-'
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.digits: db "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	align 4
cstring .zerostr
	db "0"
cstrend

integer_method "repr", SMALLINT, SMALLINT
	mov ebx, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(ebx).Value]
	test esi, esi
	jnz .nonzero
	mov ecx, .zerostr
	xor edx, edx
	xor eax, eax
	ret
.nonzero:
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	push byte 40
	call Riva$Memory$_alloc_atomic
	pop edi
	lea edi, [eax + 39]
	mov [edi], byte 0
	test esi, esi
	js .negative
.positive:
	mov eax, esi
.loop:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.negative:
	mov eax, esi
	neg eax
.loop2:
	xor edx, edx
	div ebx
	dec edi
	mov cl, [.digits + edx]
	test eax, eax
	mov [edi], cl
	jnz .loop2
	dec edi
	mov [edi], byte '-'
	push edi
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.digits: db "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	align 4
cstring .zerostr
	db "0"
cstrend

integer_method "hex", SMALLINT
	push byte 10
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov ebx, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(ebx).Value]
	push dword .format
	push eax
	call sprintf
	add esp, byte 12
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
datasect
.format:
	db "%x", 0, 0

extern __gmpz_get_str

amethod Std$String$Of, BIGINT
	mov ebx, [Std$Function_argument(edi).Val]
	lea ebx, [Std$Integer_bigt(ebx).Value]
	push ebx
	push 10
	push 0
	call __gmpz_get_str
	add esp, byte 12
	push eax
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

amethod Std$String$Of, BIGINT, SMALLINT
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea ebx, [Std$Integer_bigt(ebx).Value]
	push ebx
	push dword [Std$Integer_bigt(eax).Value]
	push 0
	call __gmpz_get_str
	add esp, byte 12
	push eax
	call Std$String$_new
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_init_set_str
amethod Std$Integer$Of, STRING
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push byte 0
	sub esp, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edi, esp
	lea eax, [Std$String_t(eax).Blocks]
.copy:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .done
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .copy
.done:
	mov [edi], byte 0
	mov edx, esp
	push byte 10
	push edx
	push ebx
	call __gmpz_init_set_str
	test eax, eax
	js .failure
	mov esp, ebx
	jmp finish_integer
.failure:
	xor eax, eax
	lea esp, [ebx + 12]
	inc eax
	ret

integer_method "base", STRING, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi, 1).Val]
	push byte 0
	mov edx, [Std$Integer_smallt(edx).Value]
	sub esp, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edi, esp
	lea eax, [Std$String_t(eax).Blocks]
.copy:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .done
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .copy
.done:
	mov [edi], byte 0
	mov eax, esp
	push edx
	push eax
	push ebx
	call __gmpz_init_set_str
	test eax, eax
	js .failure
	mov esp, ebx
	jmp finish_integer
.failure:
	xor eax, eax
	lea esp, [ebx + 12]
	inc eax
	ret

amethod Std$Integer$Of, STRING, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi, 1).Val]
	push byte 0
	mov edx, [Std$Integer_smallt(edx).Value]
	sub esp, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edi, esp
	lea eax, [Std$String_t(eax).Blocks]
.copy:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .done
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .copy
.done:
	mov [edi], byte 0
	mov eax, esp
	push edx
	push eax
	push ebx
	call __gmpz_init_set_str
	test eax, eax
	js .failure
	mov esp, ebx
	jmp finish_integer
.failure:
	xor eax, eax
	lea esp, [ebx + 12]
	inc eax
	ret

extern strtod
amethod Std$Real$Of, STRING
	mov eax, [Std$Function_argument(edi).Val]
	push ebx
	mov ebx, esp
	push byte 0
	sub esp, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov edi, esp
	lea eax, [Std$String_t(eax).Blocks]
.copy:
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	jecxz .done
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	rep movsb
	add eax, byte sizeof(Std$String_block)
	jmp .copy
.done:
	mov [edi], byte 0
	mov edi, esp
	push ebx
	push edi
	call strtod
	mov esp, ebx
	pop ebx
	cmp ebx, edi
	je .failure
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret
.failure:
	fstp st0
	xor eax, eax
	inc eax
	ret

struct symbol_array_values_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

symbol_method "values", TYP, Std$Symbol$ArrayT
	mov eax, [Std$Function_argument(edi).Val]
	mov esi, [Std$Symbol_arrayt(eax).Count]
	lea ebx, [Std$Symbol_arrayt(eax).Values]
	dec esi
	js .fail
	jz .return
	lea esi, [ebx + 4 * esi]
	push byte sizeof(symbol_array_values_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [symbol_array_values_state(eax).Limit], esi
	mov dword [Std$Function_state(eax).Run], .resume
.suspend:
	mov edx, ebx
	mov ecx, [ebx]
	add ebx, byte 4
	mov [symbol_array_values_state(eax).Current], ebx
	mov ebx, eax
	or eax, byte -1
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
	mov ebx, [symbol_array_values_state(eax).Current]
	cmp ebx, [symbol_array_values_state(eax).Limit]
	jb .suspend
.return:
	mov edx, ebx
	mov ecx, [ebx]
	xor eax, eax
	ret

array_method "length", TYP, Std$Array$T
	mov eax, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Array_t(eax).Length]
	xor edx, edx
	xor eax, eax
	ret


struct array_values_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
endstruct

array_method "values", TYP, Std$Array$T
	mov eax, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	mov ebx, [Std$Array_t(eax).Values]
	dec esi
	js .fail
	jz .return
	lea esi, [ebx + 4 * esi]
	push byte sizeof(array_values_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [array_values_state(eax).Limit], esi
	mov dword [Std$Function_state(eax).Run], .resume
.suspend:
	mov edx, ebx
	mov ecx, [ebx]
	add ebx, byte 4
	mov [array_values_state(eax).Current], ebx
	mov ebx, eax
	or eax, byte -1
	ret
.fail:
	xor eax, eax
	inc eax
	ret
.resume:
	mov ebx, [array_values_state(eax).Current]
	cmp ebx, [array_values_state(eax).Limit]
	jb .suspend
.return:
	mov edx, ebx
	mov ecx, [ebx]
	xor eax, eax
	ret

array_method "[]", TYP, Std$Array$T, SMALLINT
;@array
;@index
;:ANY
; Returns the <var>index</var><sup>th</sup> element of <var>array</var>. Indexing begins at <code>1</code>.
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	dec eax
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Array_t(ecx).Values]
	lea edx, [edx + 4 * eax]
	mov ecx, [edx]
	xor eax, eax
	ret

array_method "apply", TYP, Std$Array$T, TYP, Std$Function$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov esi, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	shr esi, 1
	mov edi, [Std$Array_t(eax).Values]
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

array_method "apply", TYP, Std$Function$T, TYP, Std$Array$T
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov esi, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	mov edi, [Std$Array_t(eax).Values]
	mov eax, [Std$Object_t(ecx).Type]
	jmp [Std$Type_t(eax).Invoke]

set_method "^", Std$Function$IteratorNext, TYP, Std$Function$IteratorT

extern __gmpz_init_set_si
extern __gmpz_mul_si

integer_method "-", SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	neg eax
	jo .overflow
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	call Std$Integer$_alloc_big
	push eax
	lea esi, [Std$Integer_bigt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	push esi
	push esi
	call __gmpz_neg
	add esp, byte 16
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

integer_method "abs", SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	neg eax
	jo .overflow
	jns .done
	neg eax
.done:
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	call Std$Integer$_alloc_big
	push eax
	lea esi, [Std$Integer_bigt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	push esi
	push esi
	call __gmpz_abs
	add esp, byte 16
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

integer_method "+", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	add eax, [Std$Integer_smallt(ebx).Value]
	jo .overflow
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	sub esp, byte 12
	mov esi, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	push esi
	push ebx
	push ebx
	call __gmpz_add
	mov esp, ebx
	jmp finish_integer

integer_method "-", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	sub eax, [Std$Integer_smallt(ebx).Value]
	jo .overflow
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	sub esp, byte 12
	mov esi, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	push esi
	push ebx
	push ebx
	call __gmpz_sub
	mov esp, ebx
	jmp finish_integer

integer_method "*", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	imul eax, [Std$Integer_smallt(ebx).Value]
	jo .overflow
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	call Std$Integer$_alloc_big
	push eax
	lea esi, [Std$Integer_bigt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	push eax
	push esi
	push esi
	call __gmpz_mul_si
	add esp, byte 20
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

integer_method "div", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cdq
	idiv dword [Std$Integer_smallt(ebx).Value]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

set_method "#", Std$Rational$Hash, TYP, Std$Rational$T
set_method "?", Std$Rational$Compare, TYP, Std$Rational$T, TYP, Std$Rational$T

extern __gmpq_set_si
extern __gmpq_canonicalize
integer_method "/", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cdq
	idiv dword [Std$Integer_smallt(ebx).Value]
	test edx, edx
	jnz .rational
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.rational:
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(ebx).Value]
	push dword [Std$Integer_smallt(edx).Value]
	call Std$Rational$_new_small_small
	add esp, byte 8
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "%", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cdq
	idiv dword [Std$Integer_smallt(ebx).Value]
	mov [Std$Integer_smallt(ecx).Value], edx
	xor edx, edx
	xor eax, eax
	ret

integer_method "~", SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	not eax
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "and", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	and eax, [Std$Integer_smallt(ebx).Value]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "covers", SMALLINT, SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Integer_smallt(ecx).Value]
	and ebx, [Std$Integer_smallt(eax).Value]
	cmp ebx, [Std$Integer_smallt(eax).Value]
	jne .failure
	xor edx, edx
	xor eax, eax
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "or", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	or eax, [Std$Integer_smallt(ebx).Value]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "xor", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	xor eax, [Std$Integer_smallt(ebx).Value]
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "sar", SMALLINT, SMALLINT
sar_small_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ebx).Value]
	neg ecx
	jz .noshift
	jns near sal_small_small.run
	neg ecx
.run:
	sar eax, cl
	jz .zero
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(eax).Value]
	xor edx, edx
	xor eax, eax
	ret
.noshift:
	mov ecx, [Std$Function_argument(edi).Val]
	xor edx, edx
	xor eax, eax
	ret
.zero:
	mov ecx, ZERO
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_mul_2exp
integer_method "sal", SMALLINT, SMALLINT
sal_small_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	test eax, eax
	jz .zero
	mov ecx, [Std$Integer_smallt(ebx).Value]
	neg ecx
	jz .noshift
	jns sar_small_small.run
	neg ecx
.run:
	cmp ecx, 32
	jae .overflow
.loop:
	add eax, eax
	jo .overflow
	dec ecx
	jnz .loop
	push eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	xor eax, eax
	ret
.zero:
	mov ecx, ZERO
	xor edx, edx
	xor eax, eax
	ret
.noshift:
	mov ecx, [Std$Function_argument(edi).Val]
	xor edx, edx
	xor eax, eax
	ret
.overflow:
	call Std$Integer$_alloc_big
	push eax
	lea esi, [Std$Integer_bigt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push esi
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	push eax
	push esi
	push esi
	call __gmpz_mul_2exp
	add esp, byte 20
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

integer_method "shl", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ebx).Value]
	shl eax, cl
	pop ecx
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "shr", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ebx).Value]
	shr eax, cl
	pop ecx
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "rol", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ebx).Value]
	rol eax, cl
	pop ecx
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "ror", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ebx).Value]
	ror eax, cl
	pop ecx
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "gcd", SMALLINT, SMALLINT
	call Std$Integer$_alloc_small
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	
	; The following code was taken from http://groups.google.com/group/comp.lang.asm.x86/msg/783fbd0cc6bb29ae?pli=1
	; by definition: gcd(a,0) = a, gcd(0,b) = b, gcd(0,0) = 1 !
	mov ecx, eax
	or ecx, ebx
	bsf ecx, ecx ;extract the greatest common power of 2 of a and b...
	jnz notBoth0
	mov eax, 1 ;if a = 0 and b = 0, return 1
	jmp done
notBoth0:
	mov edi, ecx ;...and remember it for later use
	test eax, eax
	jnz aNot0
	mov eax, ebx ;if a = 0, return b
	jmp done
aNot0:
	test ebx, ebx
	jz done ; if b = 0, return a
	bsf ecx, eax ; "simplify" a as much as possible
	shr eax, cl
	bsf ecx, ebx ; "simplify" b as much as possible
	shr ebx, cl
mainLoop:
	mov ecx, ebx
	sub ecx, eax ; b - a
	sbb edx, edx ; edx = 0 if b >= a or -1 if a > b
	and ecx, edx ;ecx = 0 if b >= a or b - a if a > b
	add eax, ecx ;a-new = min(a,b)
	sub ebx, ecx ;b-new = max(a,b)
	sub ebx, eax ;we're now sure that that difference is >= 0
	bsf ecx, eax ;"simplify" as much as possible by 2
	shr eax, cl
	bsf ecx, ebx ;"simplify" as much as possible by 2
	shr ebx, cl
	jnz mainLoop ;keep looping until ebx = 0
	mov ecx, edi ;multiply back with the common power of 2
	shl eax, cl
done: 
	pop ecx
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	xor eax, eax
	ret 

address_method "<", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jge .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "<", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jge .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

address_method ">", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jle .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jle .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

address_method "is0", ADDRESS
	mov ecx, [Std$Function_argument(edi).Val]
	xor eax, eax
	cmp [Std$Integer_smallt(ecx).Value], dword 0
	setne al
	xor edx, edx
	ret

integer_method "is0", SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	xor eax, eax
	cmp [Std$Integer_smallt(ecx).Value], dword 0
	setne al
	xor edx, edx
	ret

integer_method "is0", BIGINT
	xor eax, eax
	inc eax
	ret

address_method "=", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jne .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "=", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jne .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

address_method "~=", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	je .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "~=", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	je .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

address_method "<=", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jg .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "<=", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jg .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

address_method ">=", ADDRESS, ADDRESS
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jl .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">=", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jl .fail
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

set_method "?", Std$Address$Compare, ADDRESS, ADDRESS
set_method "?", Std$Integer$CompareSmall, SMALLINT, SMALLINT

extern __gmpz_cmp
extern __gmpz_cmp_si

integer_method "in", SMALLINT, SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp ecx, byte 32
	jae .failure
	xor edx, edx
	inc edx
	shl edx, cl
	and edx, eax
	jz .failure
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.failure:
	mov eax, 1
	ret

integer_method "floor", SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

integer_method "ceil", SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

integer_method "floor", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

integer_method "ceil", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret

extern __gmpz_init

extern __gmpz_neg
integer_method "-", BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_neg
	add esp, byte 8
	jmp finish_integer

extern __gmpz_com
integer_method "~", BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_com
	add esp, byte 8
	jmp finish_integer

extern __gmpz_abs
integer_method "abs", BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_abs
	add esp, byte 8
	jmp finish_integer

extern __gmpz_com
integer_method "~", BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_com
	add esp, byte 8
	jmp finish_integer

extern __gmpz_sqrt
integer_method "isqrt", BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_sqrt
	add esp, byte 8
	jmp finish_integer

extern __gmpz_add
integer_method "+", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_add
	add esp, byte 12
	jmp finish_integer

extern __gmpz_sub
integer_method "-", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_sub
	add esp, byte 12
	jmp finish_integer

extern __gmpz_mul
integer_method "*", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mul
	add esp, byte 12
	jmp finish_integer

extern __gmpz_tdiv_q
integer_method "div", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_tdiv_q
	add esp, byte 12
	jmp finish_integer

extern __gmpz_mod
integer_method "%", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mod
	add esp, byte 12
	jmp finish_integer

extern __gmpz_gcd
integer_method "gcd", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_gcd
	add esp, byte 12
	jmp finish_integer

extern __gmpz_cmp
integer_method "<", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jge .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jle .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">=", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jl .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "<=", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jg .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "=", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	jne .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "~=", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp
	add esp, byte 8
	cmp eax, byte 0
	je .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "?", BIGINT, BIGINT
	mov eax, [Std$Function_argument(edi, 1).Val]
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


extern __gmpz_and
integer_method "and", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_and
	add esp, byte 12
	jmp finish_integer

extern __gmpz_ior
integer_method "or", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_ior
	add esp, byte 12
	jmp finish_integer

extern __gmpz_xor
integer_method "xor", BIGINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_xor
	add esp, byte 12
	jmp finish_integer

extern __gmpz_add_ui
integer_method "+", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_add
	mov esp, ebx
	jmp finish_integer

extern __gmpz_sub_ui
integer_method "-", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_sub
	mov esp, ebx
	jmp finish_integer

extern __gmpz_mul_si
integer_method "*", BIGINT, SMALLINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mul_si
	add esp, byte 12
	jmp finish_integer

extern __gmpz_tdiv_q_ui
extern __gmpz_neg
integer_method "div", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_tdiv_q
	mov esp, ebx
	jmp finish_integer

extern __gmpz_fdiv_ui
integer_method "%", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_fdiv_ui
	add esp, byte 8
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "and", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_and
	mov esp, ebx
	jmp finish_integer

integer_method "or", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_ior
	mov esp, ebx
	jmp finish_integer

integer_method "xor", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push ebx
	push eax
	push ebx
	call __gmpz_xor
	mov esp, ebx
	jmp finish_integer

extern __gmpz_mul_2exp
integer_method "sal", BIGINT, SMALLINT
sal_big_small:
	sub esp, byte 12
	mov ebx, esp
	push esp
	call __gmpz_init
	add esp, byte 4
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(eax).Value]
	neg ecx
	jns near sar_big_small.finish
	neg ecx
.finish:
	push ecx
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mul_2exp
	add esp, byte 12
	jmp finish_integer

extern __gmpz_tdiv_q_2exp
integer_method "sar", BIGINT, SMALLINT
sar_big_small:
	sub esp, byte 12
	mov ebx, esp
	push esp
	call __gmpz_init
	add esp, byte 4
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(eax).Value]
	neg ecx
	jns near sal_big_small.finish
	neg ecx
.finish:
	push ecx
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_tdiv_q_2exp
	add esp, byte 12
	jmp finish_integer

extern __gmpz_mul_2exp
integer_method "shl", BIGINT, SMALLINT
shl_big_small:
	sub esp, byte 12
	mov ebx, esp
	push esp
	call __gmpz_init
	add esp, byte 4
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(eax).Value]
	neg ecx
	jns near shr_big_small.finish
	neg ecx
.finish:
	push ecx
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mul_2exp
	add esp, byte 12
	jmp finish_integer

extern __gmpz_fdiv_q_2exp
integer_method "shr", BIGINT, SMALLINT
shr_big_small:
	sub esp, byte 12
	mov ebx, esp
	push esp
	call __gmpz_init
	add esp, byte 4
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(eax).Value]
	neg ecx
	jns near shl_big_small.finish
	neg ecx
.finish:
	push ecx
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_fdiv_q_2exp
	add esp, byte 12
	jmp finish_integer

extern __gmpz_fac_ui
integer_method "fact", SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init
	call __gmpz_fac_ui
	add esp, byte 8
	jmp finish_integer

extern __gmpz_bin_uiui
integer_method "bin", SMALLINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init
	call __gmpz_bin_uiui
	add esp, byte 12
	jmp finish_integer

extern __gmpz_bin_ui
integer_method "bin", BIGINT, SMALLINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_init
	call __gmpz_bin_ui
	add esp, byte 12
	jmp finish_integer

extern __gmpz_gcd_ui
integer_method "gcd", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	test eax, eax
	jz .return_big
	push eax
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push byte 0
	call __gmpz_gcd_ui
	add esp, byte 12
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.return_big:
	mov ecx, [Std$Function_argument(edi).Val]
	xor edx, edx
	xor eax, eax
	ret

integer_method "gcd", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	test eax, eax
	jz .return_big
	push eax
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push byte 0
	call __gmpz_gcd_ui
	add esp, byte 12
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.return_big:
	mov ecx, [Std$Function_argument(edi, 1).Val]
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_cmp_si
integer_method "<", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jge .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jle .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">=", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jl .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "<=", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jg .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "=", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jne .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "~=", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	je .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "?", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
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


integer_method "+", SMALLINT, BIGINT
add_small_big:
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	push ebx
	call __gmpz_add
	mov esp, ebx
	jmp finish_integer

integer_method "-", SMALLINT, BIGINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	push ebx
	call __gmpz_sub
	mov esp, ebx
	jmp finish_integer

extern __gmpz_mul_si
integer_method "*", SMALLINT, BIGINT
	sub esp, byte 12
	push esp
	call __gmpz_init
	add esp, byte 4
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	call __gmpz_mul_si
	add esp, byte 12
	jmp finish_integer

integer_method "div", SMALLINT, BIGINT
datasect
	mov ecx, .zero
	xor edx, edx
	xor eax, eax
	ret
.zero:
	dd Std$Integer$SmallT
	dd 0

integer_method "%", SMALLINT, BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(ecx).Value]
	test eax, eax
	js add_small_big
	xor edx, edx
	xor eax, eax
	ret

integer_method "and", SMALLINT, BIGINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	push ebx
	call __gmpz_and
	mov esp, ebx
	jmp finish_integer

integer_method "or", SMALLINT, BIGINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	push ebx
	call __gmpz_ior
	mov esp, ebx
	jmp finish_integer

integer_method "xor", SMALLINT, BIGINT
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	push ebx
	call __gmpz_init_set_si
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push ebx
	push ebx
	call __gmpz_xor
	mov esp, ebx
	jmp finish_integer

integer_method "<", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jl .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jg .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method ">=", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jge .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "<=", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jle .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "=", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jne .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "~=", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	je .fail
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

integer_method "?", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_cmp_si
	add esp, byte 8
	cmp eax, byte 0
	jg .less
	jl .greater
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


extern __gmpz_tstbit
integer_method "in", SMALLINT, BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	push dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_tstbit
	add esp, byte 8
	cmp eax, byte 0
	je .fail
	mov ecx, [Std$Function_argument(edi).Val]
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

extern __gmpz_fits_sint_p
extern __gmpz_get_si
cfunction finish_integer
	push esp
	call __gmpz_fits_sint_p
	add esp, byte 4
	test eax, eax
	jnz .convert_to_small
	call Std$Integer$_alloc_big
	pop dword [eax + 4]
	pop dword [eax + 8]
	pop dword [eax + 12]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.convert_to_small:
	push esp
	call __gmpz_get_si
	add esp, byte 16
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

struct repeat_value_state, Std$Function_state
	.Value:	resd 1
endstruct

cfunction resume_repeat_value
	mov ecx, [repeat_value_state(eax).Value]
	mov ebx, eax
	or eax, byte -1
	xor edx, edx
	ret

set_method "to", Std$Integer$ToSmallSmall, SMALLINT, SMALLINT

struct to_small_small_small_state, Std$Function_state
	.Current:	resd 1
	.Limit:		resd 1
	.Increment:	resd 1
endstruct

integer_method "to", SMALLINT, SMALLINT, SMALLINT
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	test ebx, ebx
	js near downto_small_small_small
	jz near repeat_small
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jl .fail
	je .return
.suspend:
	push ecx
	push ebx
	push eax
	push dword [Std$Integer_smallt(ecx).Value]
	push byte sizeof(to_small_small_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [to_small_small_small_state(eax).Current]
	pop dword [to_small_small_small_state(eax).Limit]
	pop dword [to_small_small_small_state(eax).Increment]
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
	mov ecx, [to_small_small_small_state(eax).Current]
	add ecx, [to_small_small_small_state(eax).Increment]
	cmp ecx, [to_small_small_small_state(eax).Limit]
	jg .fail
	je .return2
	mov [to_small_small_small_state(eax).Current], ecx
	push eax
	push ecx
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret
.return2:
	push ecx
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
downto_small_small_small:
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	cmp eax, [Std$Integer_smallt(ecx).Value]
	jg .fail
	je .return
.suspend:
	push ecx
	push ebx
	push eax
	push dword [Std$Integer_smallt(ecx).Value]
	push byte sizeof(to_small_small_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [to_small_small_small_state(eax).Current]
	pop dword [to_small_small_small_state(eax).Limit]
	pop dword [to_small_small_small_state(eax).Increment]
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
	mov ecx, [to_small_small_small_state(eax).Current]
	add ecx, [to_small_small_small_state(eax).Increment]
	cmp ecx, [to_small_small_small_state(eax).Limit]
	jl .fail
	je .return2
	mov [to_small_small_small_state(eax).Current], ecx
	push eax
	push ecx
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret
.return2:
	push ecx
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
repeat_small:
	push byte sizeof(repeat_value_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ecx, [Std$Function_argument(edi).Val]
	mov [repeat_value_state(eax).Value], ecx
	mov dword [Std$Function_state(eax).Run], resume_repeat_value
	mov ebx, eax
	or eax, byte -1
	xor edx, edx
	ret

struct up_small_state, Std$Function_state
	.Current:	resd 1
endstruct

integer_method "up", SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
.suspend:
	push ecx
	push dword [Std$Integer_smallt(ecx).Value]
	push byte sizeof(up_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [up_small_state(eax).Current]
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
	mov ecx, [up_small_state(eax).Current]
	inc ecx
	mov [up_small_state(eax).Current], ecx
	push eax
	push ecx
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret

struct down_small_state, Std$Function_state
	.Current:	resd 1
endstruct

integer_method "down", SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
.suspend:
	push ecx
	push dword [Std$Integer_smallt(ecx).Value]
	push byte sizeof(up_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [down_small_state(eax).Current]
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
	mov ecx, [down_small_state(eax).Current]
	dec ecx
	mov [down_small_state(eax).Current], ecx
	push eax
	push ecx
	call Std$Integer$_alloc_small
	mov ecx, eax
	pop dword [Std$Integer_smallt(ecx).Value]
	xor edx, edx
	or eax, byte -1
	pop ebx
	ret

extern sprintf
amethod Std$String$Of, REAL
	sub esp, byte 8
	mov eax, esp
	mov ebx, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(ebx).Value]
	sub esp, byte 8
	fstp qword [esp]
	push .format
	push eax
	call asprintf
	add esp, byte 16
	mov [esp + 4], eax
	call Std$String$_new_length
	add esp, byte 8
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.format:
	db "%.20g", 0

extern __gmpz_init_set_d
amethod Std$Integer$Of, REAL
	sub esp, byte 12
	mov ebx, esp
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	sub esp, byte 8
	fstp qword [esp]
	push ebx
	call __gmpz_init_set_d
	add esp, byte 12
	jmp finish_integer

amethod Std$Real$Of, SMALLINT
	call Std$Real$_alloc
	mov ecx, eax
	mov eax, [Std$Function_argument(edi).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	fstp qword [Std$Real_t(ecx).Value]
	xor eax, eax
	xor edx, edx
	ret

extern __gmpz_get_d
amethod Std$Real$Of, BIGINT, VAL, Std$Real$T
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	call __gmpz_get_d
	add esp, byte 4
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor eax, eax
	xor edx, edx
	ret

real_method "+", REAL, REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fadd qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "-", REAL, REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fsub qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "*", REAL, REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fmul qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "/", REAL, REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fdiv qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "%", REAL, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
.loop:
	fprem
	fstsw ax
	sahf
	jpe .loop
	fstp st1
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "reduce", REAL, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
.loop:
	fprem1
	fstsw ax
	sahf
	jpe .loop
	fstp st1
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "abs", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fabs
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "-", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fchs
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern floor
real_method "floor", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	sub esp, byte 8
	fstp qword [esp]
	call floor
	add esp, byte 8
	call Std$Integer$_alloc_small
	fistp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern ceil
real_method "ceil", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	sub esp, byte 8
	fstp qword [esp]
	call ceil
	add esp, byte 8
	call Std$Integer$_alloc_small
	fistp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern round
real_method "round", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	call Std$Integer$_alloc_small
	fistp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "sin", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fsin
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "cos", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fcos
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "tan", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fsincos
	fdivp st1, st0
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern asin
real_method "asin", REAL
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call asin
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern acos
real_method "acos", REAL
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call acos
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "atan", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld1
	fld qword [Std$Real_t(eax).Value]
	fpatan
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "atan", REAL, REAL
;@x
;@y
;:Std$Real$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(ebx).Value]
	fld qword [Std$Real_t(eax).Value]
	fpatan
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "sqrt", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fsqrt
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "sqrt", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	fsqrt
	sub esp, byte 4
	fist dword [esp]
	fild dword [esp]
	add esp, byte 4
	fcomip st1
	je .integer
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.integer:
	call Std$Integer$_alloc_small
	fistp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "isqrt", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	fsqrt
	call Std$Integer$_alloc_small
	fistp dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern exp
real_method "exp", REAL
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call exp
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "exp", SMALLINT
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fild qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call exp
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern log
real_method "ln", REAL
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call log
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "ln", SMALLINT
	sub esp, byte 8
	mov eax, [Std$Function_argument(edi).Val]
	fild qword [Std$Real_t(eax).Value]
	fstp qword [esp]
	call log
	add esp, byte 8
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern pow
real_method "^", REAL, REAL
	sub esp, byte 16
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	fstp qword [esp + 8]
	fstp qword [esp]
	call pow
	add esp, byte 16
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern pow
real_method "^", REAL, SMALLINT
	sub esp, byte 16
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	fstp qword [esp + 8]
	fstp qword [esp]
	call pow
	add esp, byte 16
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "radians", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [.constant]
	fmul qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.constant:
	dq 0.01745329251994329577

real_method "degrees", REAL
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [.constant]
	fmul qword [Std$Real_t(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret
.constant:
	dq 57.29577951308232087612

;0x01 = less
;0x40 = equal
;0x00 = greater

%macro real_compare_finish 1
%ifidn %1, S
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
%elifidn %1, F
	mov eax, 1
	ret
%else
	%error "Invalid use of real comparision macros"
%endif
%endmacro

%macro real_compare 3
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fcomp qword [Std$Real_t(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	real_compare_finish %1
.equal:
	real_compare_finish %2
.less:
	real_compare_finish %3
%endmacro

real_method "<", REAL, REAL
	real_compare F, F, S

real_method "<=", REAL, REAL
	real_compare F, S, S

real_method ">", REAL, REAL
	real_compare S, F, F

real_method ">=", REAL, REAL
	real_compare S, S, F

real_method "=", REAL, REAL
	real_compare F, S, F

real_method "~=", REAL, REAL
	real_compare S, F, S

set_method "?", Std$Real$Compare, REAL, REAL
set_method "#", Std$Real$Hash, REAL

real_method "?", REAL, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	ficomp dword [Std$Integer_smallt(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret
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

%macro real_small_compare 3
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	ficomp dword [Std$Integer_smallt(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	real_compare_finish %1
.equal:
	real_compare_finish %2
.less:
	real_compare_finish %3
%endmacro

real_method "<", REAL, SMALLINT
	real_small_compare F, F, S

real_method "<=", REAL, SMALLINT
	real_small_compare F, S, S

real_method ">", REAL, SMALLINT
	real_small_compare S, F, F

real_method ">=", REAL, SMALLINT
	real_small_compare S, S, F

real_method "=", REAL,SMALLINT
	real_small_compare F, S, F

real_method "~=", REAL, SMALLINT
	real_small_compare S, F, S

real_method "+", REAL, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fiadd dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "-", REAL, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fisub dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "*", REAL, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fimul dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "/", REAL, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fidiv dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "?", SMALLINT, REAL
	mov eax, [Std$Function_argument(edi).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fcomp qword [Std$Real_t(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	mov ecx, Std$Object$Greater
	xor edx, edx
	xor eax, eax
	ret
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

%macro small_real_compare 3
	mov eax, [Std$Function_argument(edi).Val]
	fild dword [Std$Integer_smallt(eax).Value]
	mov eax, [Std$Function_argument(edi, 1).Val]
	fcomp qword [Std$Real_t(eax).Value]
	xor eax, eax
	fnstsw ax
	shr ah, 1
	jc .less
	and ah, 0x20
	jnz .equal
.greater:
	real_compare_finish %1
.equal:
	real_compare_finish %2
.less:
	real_compare_finish %3
%endmacro

real_method "<", SMALLINT, REAL
	small_real_compare F, F, S

real_method "<=", SMALLINT, REAL
	small_real_compare F, S, S

real_method ">", SMALLINT, REAL
	small_real_compare S, F, F

real_method ">=", SMALLINT, REAL
	small_real_compare S, S, F

real_method "=", SMALLINT, REAL
	small_real_compare F, S, F

real_method "~=", SMALLINT, REAL
	small_real_compare S, F, S

real_method "+", SMALLINT, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fiadd dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "-", SMALLINT, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fisubr dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "*", SMALLINT, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fimul dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

real_method "/", SMALLINT, REAL
	mov eax, [Std$Function_argument(edi, 1).Val]
	fld qword [Std$Real_t(eax).Value]
	mov eax, [Std$Function_argument(edi).Val]
	fidivr dword [Std$Integer_smallt(eax).Value]
	call Std$Real$_alloc
	fstp qword [Std$Real_t(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

string_method "new", VAL, Std$String$T
	mov ecx, Std$String$Empty;
	xor edx, edx
	xor eax, eax
	ret

string_method "_flatten", STRING
	mov edx, [Std$Function_argument(edi).Val]
	cmp edx, Std$String$Nil
	je .unchanged
	mov ecx, [Std$String_t(edx).Count]
	cmp ecx, byte 1
	ja .not_simple
	jb .unchanged
.not_empty:
	mov ecx, [Std$Integer_smallt(Std$String_t(edx).Length).Value]
	mov eax, [Std$Address_t(Std$String_block(Std$String_t(edx).Blocks).Chars).Value]
	cmp [eax + ecx], byte 0
	je .unchanged
.not_simple:
	mov ecx, [Std$Integer_smallt(Std$String_t(edx).Length).Value]
	lea ebx, [Std$String_t(edx).Blocks]
	push ecx
	inc ecx
	push ecx
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov edi, eax
.loop:
	mov ecx, [Std$Integer_smallt(Std$String_block(ebx).Length).Value]
	test ecx, ecx
	jz .finished
	mov esi, [Std$Address_t(Std$String_block(ebx).Chars).Value]
	rep movsb
	add ebx, byte sizeof(Std$String_block)
	jmp .loop
.unchanged:
	mov ecx, edx
	xor edx, edx
	xor eax, eax
	ret
.finished:
	mov [edi], byte 0
	call Std$String$_new_length
	add esp, byte 8
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

string_method "_count", STRING
	mov ebx, [Std$Function_argument(edi).Val]
	mov ebx, [Std$String_t(ebx).Count]
	call Std$Integer$_alloc_small
	mov [Std$Integer_smallt(eax).Value], ebx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

string_method "*", STRING, SMALLINT
	mov ebx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	mul ecx
	jc .failure0
	push eax
	mov eax, [Std$String_t(ebx).Count]
	mul ecx
	jc .failure1
	push eax
	push ecx
	shl eax, 4
	lea eax, [eax + sizeof(Std$String_t) + sizeof(Std$String_block)]
	push eax
	call Riva$Memory$_alloc_stubborn
	mov [esp], eax
	lea edi, [Std$String_t(eax).Blocks]
.loop:
	dec dword [esp + 4]
	js .done
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	jmp .loop
.done:
	pop ecx
	pop eax
	pop dword [Std$String_t(ecx).Count]
	pop dword [Std$Integer_smallt(Std$String_t(ecx).Length).Value]
	mov [Std$Object_t(Std$String_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov [Std$Object_t(ecx).Type], dword Std$String$T
	push ecx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor eax, eax
	xor edx, edx
	ret
.failure1:
	pop edx
.failure0:
	xor eax, eax
	inc eax
	ret

string_method "*", SMALLINT, STRING
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	mul ecx
	jc .failure0
	push eax
	mov eax, [Std$String_t(ebx).Count]
	mul ecx
	jc .failure1
	push eax
	push ecx
	shl eax, 4
	lea eax, [eax + sizeof(Std$String_t) + sizeof(Std$String_block)]
	push eax
	call Riva$Memory$_alloc_stubborn
	mov [esp], eax
	lea edi, [Std$String_t(eax).Blocks]
.loop:
	dec dword [esp + 4]
	js .done
	lea esi, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$String_t(ebx).Count]
	shl ecx, 2
	rep movsd
	jmp .loop
.done:
	pop ecx
	pop eax
	pop dword [Std$String_t(ecx).Count]
	pop dword [Std$Integer_smallt(Std$String_t(ecx).Length).Value]
	mov [Std$Object_t(Std$String_t(ecx).Length).Type], dword Std$Integer$SmallT
	mov [Std$Object_t(ecx).Type], dword Std$String$T
	push ecx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor eax, eax
	xor edx, edx
	ret
.failure1:
	pop edx
.failure0:
	xor eax, eax
	inc eax
	ret

string_method "left", STRING, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ebx, [Std$Function_argument(edi).Val]
	cmp eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	jbe .nopad
	push eax
	sub eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	push eax
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov eax, [Std$String_t(ebx).Count]
	inc eax
	push eax
	shl eax, 4
	add eax, byte sizeof(Std$String_t) + sizeof(Std$String_block)
	push eax
	call Riva$Memory$_alloc_stubborn
	pop edx
	pop ecx
	mov [Std$String_t(eax).Count], ecx
	lea edi, [Std$String_t(eax).Blocks]
	lea esi, [Std$String_t(ebx).Blocks]
	lea ecx, [4 * ecx - 4]
	rep movsd
	pop edx
	pop ecx
	mov [Std$Object_t(Std$String_block(edi).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(edi).Chars).Value], edx
	mov [Std$Object_t(Std$String_block(edi).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(edi).Length).Value], ecx
	mov dword [Std$Object_t(Std$String_t(eax).Length).Type], Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov dword [Std$Object_t(eax).Type], Std$String$T
	mov ebx, eax
	mov edi, edx
	mov al, ' '
	rep stosb
.nopad:
	push ebx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

string_method "right", STRING, SMALLINT
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ebx, [Std$Function_argument(edi).Val]
	cmp eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	jbe .nopad
	push eax
	sub eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	push eax
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov eax, [Std$String_t(ebx).Count]
	inc eax
	push eax
	shl eax, 4
	add eax, byte sizeof(Std$String_t) + sizeof(Std$String_block)
	push eax
	call Riva$Memory$_alloc_stubborn
	pop edx
	pop ecx
	mov [Std$String_t(eax).Count], ecx
	lea edi, [Std$String_t(eax).Blocks + sizeof(Std$String_block)]
	lea esi, [Std$String_t(ebx).Blocks]
	lea ecx, [4 * ecx - 4]
	rep movsd
	pop edx
	pop ecx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], ecx
	mov dword [Std$Object_t(Std$String_t(eax).Length).Type], Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov dword [Std$Object_t(eax).Type], Std$String$T
	mov ebx, eax
	mov edi, edx
	mov al, ' '
	rep stosb
.nopad:
	push ebx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

string_method "left", STRING, SMALLINT, STRING
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ebx, [Std$Function_argument(edi).Val]
	cmp eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	jbe .nopad
	push byte ' '
	mov edx, [Std$Function_argument(edi, 2).Val]
	cmp [Std$Integer_smallt(Std$String_t(edx).Length).Value], dword 0
	je .default
	mov edx, [Std$Address_t(Std$String_block(Std$String_t(edx).Blocks).Chars).Value]
	mov edx, [edx]
	mov [esp], edx
.default:
	push eax
	sub eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	push eax
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov eax, [Std$String_t(ebx).Count]
	inc eax
	push eax
	shl eax, 4
	add eax, byte sizeof(Std$String_t) + sizeof(Std$String_block)
	push eax
	call Riva$Memory$_alloc_stubborn
	pop edx
	pop ecx
	mov [Std$String_t(eax).Count], ecx
	lea edi, [Std$String_t(eax).Blocks]
	lea esi, [Std$String_t(ebx).Blocks]
	lea ecx, [4 * ecx - 4]
	rep movsd
	pop edx
	pop ecx
	mov [Std$Object_t(Std$String_block(edi).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(edi).Chars).Value], edx
	mov [Std$Object_t(Std$String_block(edi).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(edi).Length).Value], ecx
	mov dword [Std$Object_t(Std$String_t(eax).Length).Type], Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov dword [Std$Object_t(eax).Type], Std$String$T
	mov ebx, eax
	mov edi, edx
	pop eax
	rep stosb
.nopad:
	push ebx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

string_method "right", STRING, SMALLINT, STRING
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ebx, [Std$Function_argument(edi).Val]
	cmp eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	jbe .nopad
	push byte ' '
	mov edx, [Std$Function_argument(edi, 2).Val]
	cmp [Std$Integer_smallt(Std$String_t(edx).Length).Value], dword 0
	je .default
	mov edx, [Std$Address_t(Std$String_block(Std$String_t(edx).Blocks).Chars).Value]
	mov edx, [edx]
	mov [esp], edx
.default:
	push eax
	sub eax, [Std$Integer_smallt(Std$String_t(ebx).Length).Value]
	push eax
	push eax
	call Riva$Memory$_alloc_atomic
	mov [esp], eax
	mov eax, [Std$String_t(ebx).Count]
	inc eax
	push eax
	shl eax, 4
	add eax, byte sizeof(Std$String_t) + sizeof(Std$String_block)
	push eax
	call Riva$Memory$_alloc_stubborn
	pop edx
	pop ecx
	mov [Std$String_t(eax).Count], ecx
	lea edi, [Std$String_t(eax).Blocks + sizeof(Std$String_block)]
	lea esi, [Std$String_t(ebx).Blocks]
	lea ecx, [4 * ecx - 4]
	rep movsd
	pop edx
	pop ecx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Type], dword Std$Address$T
	mov [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], edx
	mov [Std$Object_t(Std$String_block(Std$String_t(eax).Blocks).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], ecx
	mov dword [Std$Object_t(Std$String_t(eax).Length).Type], Std$Integer$SmallT
	pop dword [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	mov dword [Std$Object_t(eax).Type], Std$String$T
	mov ebx, eax
	mov edi, edx
	pop eax
	rep stosb
.nopad:
	push ebx
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret

string_method "ord", STRING
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

string_method "chr", SMALLINT
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
	mov [Std$Object_t(eax).Type], dword Std$String$T
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

set_method "+", Std$Function_checkedasmt(Std$String$Add).Unchecked, STRING, STRING

string_method "[]", STRING, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	mov edx, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	dec ebx
	jns .firstpositive
	add ebx, edx
	inc ebx
	js near .outofbounds
.firstpositive:
	cmp ebx, edx
	jge near .outofbounds
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
	push byte sizeof(Std$String_t) + 2 * sizeof(Std$String_block)
	call Riva$Memory$_alloc_stubborn
	add esp, byte 4
	mov [Std$Object_t(eax).Type], dword Std$String$T
	mov [Std$Object_t(Std$String_t(eax).Length).Type], dword Std$Integer$SmallT
	mov [Std$Integer_smallt(Std$String_t(eax).Length).Value], byte 1
	mov [Std$String_t(eax).Count], byte 1
	mov esi, [esp + 4]
	mov ecx, 4
	lea edi, [Std$String_t(eax).Blocks]
	rep movsd
	pop edx
	add [Std$Address_t(Std$String_block(Std$String_t(eax).Blocks).Chars).Value], edx
	mov [Std$Integer_smallt(Std$String_block(Std$String_t(eax).Blocks).Length).Value], dword 1
	add esp, byte 4
	push eax
	call Riva$Memory$_freeze_stubborn
	pop ecx
	xor edx, edx
	xor eax, eax
	ret
.outofbounds:
	mov eax, 1
	ret

string_method "[]", STRING, SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov edx, [Std$Integer_smallt(Std$String_t(eax).Length).Value]
	dec ebx
	jns .firstpositive
	add ebx, edx
	inc ebx
	js near .outofbounds
.firstpositive:
	cmp ebx, edx
	jge near .outofbounds
	dec ecx
	jns .secondpositive
	add ecx, edx
	inc ecx
	js near .outofbounds
.secondpositive:
	cmp ecx, edx
	jg near .outofbounds
	cmp ebx, ecx
	ja .outofbounds
	jb .notempty
	mov ecx, .empty_string
	xor edx, edx
	xor eax, eax
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
	mov [Std$Object_t(eax).Type], dword Std$String$T
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
	pop ecx
	xor edx, edx
	xor eax, eax
	ret
.outofbounds:
	mov eax, 1
	ret
datasect
.empty_string:
	dd Std$String$T
	dd 0
	dd Std$Integer$SmallT
	dd 0
	dd 0, 0
	dd 0, 0, 0, 0

symbol ?init, "init"

type_method "fields", TYP, Std$Type$T
	mov ecx, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Type_t(ecx).Fields]
	xor edx, edx
	xor eax, eax
	ret

;type_method "new", TYP, Std$Type$T
;	push dword [Std$Function_argument(edi).Val]
;	push dword [Std$Function_argument(edi).Ref]
;	push edi
;	push esi
;	xor esi, esi
;	mov ecx, [Std$Function_argument(edi).Val]
;	call Std$Type$T.invoke
;	pop esi
;	mov edi, [esp]
;	push ecx
;	mov [Std$Function_argument(edi).Val], ecx
;	mov [Std$Function_argument(edi).Ref], dword 0
;	mov ecx, ?init
;	call Std$Symbol$T.invoke
;	pop ecx
;	pop edi
;	pop dword [Std$Function_argument(edi).Ref]
;	pop dword [Std$Function_argument(edi).Val]
;	xor edx, edx
;	xor eax, eax
;	ret

type_method "new", TYP, Std$Type$T
	;int3
	mov ebx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Type_t(ebx).Fields]
	mov eax, [Std$Integer_smallt(Std$Array_t(eax).Length).Value]
	push eax
	lea eax, [4 * eax + 4]
	push eax
	call Riva$Memory$_alloc
	add esp, byte 4
	mov [Std$Object_t(eax).Type], ebx
	pop ecx
	push edi
	lea edi, [eax + 4]
	mov ebx, eax
	mov eax, Std$Object$Nil
	rep stosd
	pop edi
	push dword [Std$Function_argument(edi).Ref]
	push dword [Std$Function_argument(edi).Val]
	push edi
	mov [Std$Function_argument(edi).Val], ebx
	mov [Std$Function_argument(edi).Ref], dword 0
	push ebx
	mov ecx, ?init
	call Std$Symbol$T.invoke
	cmp eax, byte 2
	je .error
	pop ecx
	pop edi
	pop dword [Std$Function_argument(edi).Val]
	pop dword [Std$Function_argument(edi).Ref]
	xor edx, edx
	xor eax, eax
	ret
.error:
	pop edi
	pop edi
	pop dword [Std$Function_argument(edi).Val]
	pop dword [Std$Function_argument(edi).Ref]
	xor edx, edx
	ret

object_method "init", ANY
	;int3
	mov ebx, [Std$Function_argument(edi).Val]
	add edi, byte sizeof(Std$Function_argument)
	sub esi, byte 2
	js .nofields
	mov ecx, ebx
.copyfields:
	add ecx, byte 4
	mov edx, [Std$Function_argument(edi).Val]
	add edi, byte sizeof(Std$Function_argument)
	cmp [Std$Object_t(edx).Type], dword Std$Symbol$ArrayT
	je .namedfields
	dec esi
	mov [ecx], edx
	jns .copyfields
.nofields:
	mov ecx, ebx
	xor edx, edx
	xor eax, eax
	ret	
.namedfields:
	mov esi, edi
	push ebp
	mov ebp, [Std$Object_t(ebx).Type]
	mov ebp, [Std$Type_t(ebp).Fields]
	lea edx, [Std$Symbol_arrayt(edx).Values]
	mov eax, [edx]
	add edx, byte 4
	test eax, eax
	jz .nonamedfields
.namedfieldloop:
	mov ecx, [Std$Integer_smallt(Std$Array_t(ebp).Length).Value]
	mov edi, [Std$Array_t(ebp).Values]
	lea edi, [edi + 4 * ecx]
.nameloop:
	sub edi, byte 4
	cmp eax, [edi]
	je .found
	dec ecx
	jns .nameloop
.nameerror:
	pop ebp
	mov ebx, eax
	jmp Std$Symbol$nomethod.invoke
.found:
	mov eax, [Std$Function_argument(esi).Val]
	add esi, byte sizeof(Std$Function_argument)
	sub edi, [Std$Array_t(ebp).Values]
	mov [ebx + 4 * ecx], eax
	mov eax, [edx]
	add edx, byte 4
	test eax, eax
	jnz .namedfieldloop
.nonamedfields:
	pop ebp
	mov ecx, ebx
	xor edx, edx
	xor eax, eax
	ret	

;object_method "init", ANY
;	mov ecx, [Std$Function_argument(edi).Val]
;	mov edx, [Std$Function_argument(edi).Ref]
;	xor eax, eax
;	ret

%macro string_compare_finish 1
%ifidn %1, S
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi, 1).Ref]
	xor eax, eax
	ret
%elifidn %1, F
	mov eax, 1
	ret
%else
	%error "Invalid use of string comparision macros"
%endif
%endmacro

%macro string_compare 3
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$String_t(eax).Blocks]
	lea ebx, [Std$String_t(ebx).Blocks]
	push edi
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
	pop edi
	string_compare_finish %1
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
	pop edi
	string_compare_finish %2
.less:
	pop edi
	string_compare_finish %3
%endmacro

string_method "<", STRING, STRING
	string_compare F, F, S

string_method "<=", STRING, STRING
	string_compare F, S, S

string_method ">", STRING, STRING
	string_compare S, F, F

string_method ">=", STRING, STRING
	string_compare S, S, F

string_method "=", STRING, STRING
	string_compare F, S, F

string_method "~=", STRING, STRING
	string_compare S, F, S

set_method "?", Std$Function_checkedasmt(Std$String$Compare).Unchecked, STRING, STRING
set_method "#", Std$Address$Hash, ADDRESS
set_method "#", Std$Integer$HashSmall, SMALLINT

set_method "floor", Std$Integer$HashSmall, SMALLINT

integer_method "#", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push ecx
	call __gmpz_get_si
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

integer_method "bitcount", SMALLINT
bits_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
.run:
	mov edx, eax
	and edx, 0x55555555
	sub eax, edx
	shr eax, 1
	add eax, edx
	mov edx, eax
	and edx, 0x33333333
	sub eax, edx
	shr eax, 2
	add eax, edx
	mov edx, eax
	and edx, 0x0F0F0F0F
	sub eax, edx
	shr eax, 4
	add eax, edx
	mov edx, eax
	and edx, 0x00FF00FF
	sub eax, edx
	shr eax, 8
	add eax, edx
	mov edx, eax
	and edx, 0x0000FFFF
	sub eax, edx
	shr eax, 16
	add eax, edx
	push eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

struct bit_small_state, Std$Function_state
	.Source:	resd 1
endstruct

integer_method "bits", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Integer_smallt(eax).Value]
	xor eax, eax
	shr ecx, 1
	setc al
	push eax
	push ecx
	push byte sizeof(bit_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov dword [Std$Function_state(eax).Run], .resume
	pop dword [bit_small_state(eax).Source]
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	or eax, byte -1
	ret
.resume:
	mov ebx, eax
	call Std$Integer$_alloc_small
	mov ecx, eax
	xor eax, eax
	shr dword [bit_small_state(ebx).Source], 1
	setc al
	mov [Std$Integer_smallt(ecx).Value], eax
	xor edx, edx
	or eax, byte -1
	ret

integer_method "scan0", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	not eax
	lea edx, [eax - 1]
	xor eax, edx
	jmp bits_small.run

integer_method "scan1", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	lea edx, [eax - 1]
	xor eax, edx
	jmp bits_small.run

struct scan_small_state, Std$Function_state
	.Source:	resd 1
	.Last:		resd 1
	.Limit:		resd 1
endstruct

section .text
resume_ones_small:
	mov ebx, eax
	mov eax, [scan_small_state(ebx).Source]
	mov ecx, [scan_small_state(ebx).Last]
.loop:
	inc ecx
	sar eax, 1
	jc .suspend
	jnz .loop
	inc eax
	ret
.suspend:
	cmp ecx, [scan_small_state(ebx).Limit]
	ja .failure
	mov [scan_small_state(ebx).Source], eax
	mov [scan_small_state(ebx).Last], ecx
	push ecx
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "ones", SMALLINT
ones_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
.run:
	xor ecx, ecx
.loop:
	sar eax, 1
	jc .suspend
	lea ecx, [ecx + 1]
	jnz .loop
	inc eax
	ret
.suspend:
	push ecx
	push eax
	push byte sizeof(scan_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [scan_small_state(eax).Source]
	mov dword [scan_small_state(eax).Limit], 0xFFFFFFFF
	mov ecx, [esp]
	mov [scan_small_state(eax).Last], ecx
	mov ebx, eax
	mov dword [Std$Function_state(ebx).Run], resume_ones_small
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret

integer_method "ones", SMALLINT, SMALLINT
ones_small_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
.run:
	cmp ecx, 32
	jae .failure
	sar eax, cl
.loop:
	sar eax, 1
	jc .suspend
	lea ecx, [ecx + 1]
	jnz .loop
	inc eax
	ret
.suspend:
	push ecx
	push eax
	push byte sizeof(scan_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [scan_small_state(eax).Source]
	mov dword [scan_small_state(eax).Limit], 0xFFFFFFFF
	mov ecx, [esp]
	mov [scan_small_state(eax).Last], ecx
	mov ebx, eax
	mov dword [Std$Function_state(ebx).Run], resume_ones_small
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "ones", SMALLINT, SMALLINT, SMALLINT
ones_small_small_small:
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov ebx, [Std$Integer_smallt(ebx).Value]
.run:
	cmp ecx, 32
	jae .failure
	sar eax, cl
.loop:
	sar eax, 1
	jc .suspend
	lea ecx, [ecx + 1]
	jnz .loop
	inc eax
	ret
.suspend:
	cmp ecx, ebx
	ja .failure
	push ecx
	push eax
	push byte sizeof(scan_small_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	pop dword [scan_small_state(eax).Source]
	mov [scan_small_state(eax).Limit], ebx
	mov ecx, [esp]
	mov [scan_small_state(eax).Last], ecx
	mov ebx, eax
	mov dword [Std$Function_state(ebx).Run], resume_ones_small
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "zeros", SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	not eax
	jmp ones_small.run

integer_method "zeros", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	not eax
	jmp ones_small_small.run

integer_method "zeros", SMALLINT, SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov ebx, [Std$Function_argument(edi, 2).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	mov ebx, [Std$Integer_smallt(ebx).Value]
	not eax
	jmp ones_small_small_small.run

integer_method "[]", SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Integer_smallt(eax).Value]
	mov ecx, [Std$Integer_smallt(ecx).Value]
	cmp ecx, 31
	jbe .noclip
	mov ecx, 31
.noclip:
	shr eax, cl
	and eax, 1
	lea ecx, [ZERO + 8 * eax]
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_popcount
integer_method "bitcount", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push ecx
	call __gmpz_popcount
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

struct bit_big_state, Std$Function_state
	.Source:	resd 1
	.Index:		resd 1
endstruct

integer_method "bits", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push ecx
	push byte 0
	push ecx
	call __gmpz_tstbit
	mov [esp], eax
	push byte sizeof(bit_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	add esp, byte 4
	mov dword [Std$Function_state(ebx).Run], .resume
	pop dword [bit_big_state(ebx).Source]
	mov ecx, eax
	xor edx, edx
	or eax, byte -1
	ret
.resume:
	mov ebx, eax
	inc dword [bit_big_state(ebx).Index]
	push dword [bit_big_state(ebx).Index]
	push dword [bit_big_state(ebx).Source]
	call __gmpz_tstbit
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	add esp, byte 4
	mov ecx, eax
	xor edx, edx
	or eax, byte -1
	ret

extern __gmpz_scan0
integer_method "scan0", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push byte 0
	push ecx
	call __gmpz_scan0
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	pop ecx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

extern __gmpz_scan1
integer_method "scan1", BIGINT
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push byte 0
	push ecx
	call __gmpz_scan1
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	pop ecx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

struct scan_big_state, Std$Function_state
	.Source:	resd 1
	.Last:		resd 1
	.Limit:		resd 1
endstruct

section .text
resume_ones_big:
	mov ebx, eax
	push dword [scan_big_state(ebx).Last]
	push dword [scan_big_state(ebx).Source]
	call __gmpz_scan1
	add esp, byte 8
	cmp eax, [scan_big_state(ebx).Limit]
	ja .failure
.suspend:
	push eax
	inc eax
	mov [scan_big_state(ebx).Last], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "ones", BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push byte 0
	push eax
	call __gmpz_scan1
	add esp, byte 8
	cmp eax, byte -1
	je .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	mov dword [scan_big_state(ebx).Limit], 0xFFFFFFFE
	mov dword [Std$Function_state(ebx).Run], resume_ones_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 4
	xor eax, eax
	inc eax
	ret

integer_method "ones", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push dword [Std$Integer_smallt(ebx).Value]
	push eax
	call __gmpz_scan1
	add esp, byte 8
	cmp eax, byte -1
	je .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	mov dword [scan_big_state(ebx).Limit], 0xFFFFFFFE
	mov dword [Std$Function_state(ebx).Run], resume_ones_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 4
	xor eax, eax
	inc eax
	ret

integer_method "ones", BIGINT, SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push dword [Std$Integer_smallt(ecx).Value]
	push eax
	push dword [Std$Integer_smallt(ebx).Value]
	push eax
	call __gmpz_scan1
	add esp, byte 8
	cmp eax, [esp + 4]
	ja .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	pop dword [scan_big_state(ebx).Limit]
	mov dword [Std$Function_state(ebx).Run], resume_ones_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 8
	xor eax, eax
	inc eax
	ret

section .text
resume_zeros_big:
	mov ebx, eax
	push dword [scan_big_state(ebx).Last]
	push dword [scan_big_state(ebx).Source]
	call __gmpz_scan0
	add esp, byte 8
	cmp eax, [scan_big_state(ebx).Limit]
	ja .failure
.suspend:
	push eax
	inc eax
	mov [scan_big_state(ebx).Last], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	xor eax, eax
	inc eax
	ret

integer_method "zeros", BIGINT
	mov eax, [Std$Function_argument(edi).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push byte 0
	push eax
	call __gmpz_scan0
	add esp, byte 8
	cmp eax, byte -1
	je .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	mov dword [scan_big_state(ebx).Limit], 0xFFFFFFFE
	mov dword [Std$Function_state(ebx).Run], resume_zeros_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 4
	xor eax, eax
	inc eax
	ret

integer_method "zeros", BIGINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push eax
	push dword [Std$Integer_smallt(ebx).Value]
	push eax
	call __gmpz_scan0
	add esp, byte 8
	cmp eax, byte -1
	je .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	mov dword [scan_big_state(ebx).Limit], 0xFFFFFFFE
	mov dword [Std$Function_state(ebx).Run], resume_zeros_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 4
	xor eax, eax
	inc eax
	ret

integer_method "zeros", BIGINT, SMALLINT, SMALLINT
	mov eax, [Std$Function_argument(edi).Val]
	mov ebx, [Std$Function_argument(edi, 1).Val]
	mov ecx, [Std$Function_argument(edi, 2).Val]
	lea eax, [Std$Integer_bigt(eax).Value]
	push dword [Std$Integer_smallt(ecx).Value]
	push eax
	push dword [Std$Integer_smallt(ebx).Value]
	push eax
	call __gmpz_scan0
	add esp, byte 8
	cmp eax, byte -1
	je .failure
	push eax
	push byte sizeof(scan_big_state)
	call Riva$Memory$_alloc
	add esp, byte 4
	mov ebx, eax
	call Std$Integer$_alloc_small
	pop edx
	mov [Std$Integer_smallt(eax).Value], edx
	inc edx
	mov [scan_big_state(ebx).Last], edx
	pop dword [scan_big_state(ebx).Source]
	pop dword [scan_big_state(ebx).Limit]
	mov dword [Std$Function_state(ebx).Run], resume_zeros_big
	mov ecx, eax
	or eax, byte -1
	xor edx, edx
	ret
.failure:
	add esp, byte 8
	xor eax, eax
	inc eax
	ret

integer_method "[]", BIGINT, SMALLINT
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	lea ecx, [Std$Integer_bigt(ecx).Value]
	push dword [Std$Integer_smallt(eax).Value]
	push ecx
	call __gmpz_tstbit
	mov [esp], eax
	call Std$Integer$_alloc_small
	pop dword [Std$Integer_smallt(eax).Value]
	pop edx
	mov ecx, eax
	xor edx, edx
	xor eax, eax
	ret

set_method "#", Std$String$Hash, STRING

type_method "in", ANY, TYP, Std$Type$T
	mov ecx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Object_t(ecx).Type]
	mov edx, [Std$Type_t(edx).Types]
	sub edx, byte 4
.loop:
	add edx, byte 4
	mov ebx, [edx]
	test ebx, ebx
	jz .fail
	cmp ebx, eax
	jne .loop
	mov edx, [Std$Function_argument(edi).Ref]
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

type_method "<", TYP, Std$Type$T, TYP, Std$Type$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Type_t(eax).Types]
.loop:
	add eax, byte 4
	mov ebx, [eax]
	test ebx, ebx
	jz .fail
	cmp ebx, ecx
	jne .loop
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

type_method ">", TYP, Std$Type$T, TYP, Std$Type$T
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Type_t(ecx).Types]
.loop:
	add eax, byte 4
	mov ebx, [eax]
	test ebx, ebx
	jz .fail
	cmp ebx, edx
	jne .loop
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

type_method "<=", TYP, Std$Type$T, TYP, Std$Type$T
	mov eax, [Std$Function_argument(edi).Val]
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov eax, [Std$Type_t(eax).Types]
	sub eax, byte 4
.loop:
	add eax, byte 4
	mov ebx, [eax]
	test ebx, ebx
	jz .fail
	cmp ebx, ecx
	jne .loop
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

type_method ">=", TYP, Std$Type$T, TYP, Std$Type$T
	mov ecx, [Std$Function_argument(edi, 1).Val]
	mov edx, [Std$Function_argument(edi).Val]
	mov eax, [Std$Type_t(ecx).Types]
	sub eax, byte 4
.loop:
	add eax, byte 4
	mov ebx, [eax]
	test ebx, ebx
	jz .fail
	cmp ebx, edx
	jne .loop
	xor edx, edx
	xor eax, eax
	ret
.fail:
	xor eax, eax
	inc eax
	ret

string_method "length", STRING
;@string
; Returns the length of string
	mov ecx, [Std$Function_argument(edi).Val]
	lea ecx, [Std$String_t(ecx).Length]
	xor edx, edx
	xor eax, eax
	ret

cfunction match_substring;(Std$String_block *StartBlock, int StartOffset, Std$String_t *String, Std$String_block **EndBlock, int *EndOffset)
	push edi
	push esi
	push ebx
	; esp + 16 = first argument
	mov ebx, [esp + 24]
	mov eax, [esp + 16]
	lea ebx, [Std$String_t(ebx).Blocks]
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	sub ecx, [esp + 20]
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	add esi, [esp + 20]
	mov edx, [Std$Integer_smallt(Std$String_block(ebx).Length).Value]
	mov edi, [Std$Address_t(Std$String_block(ebx).Chars).Value]
	test ecx, ecx
	jnz .first_not_empty0
	test edx, edx
	jz .success
	jmp .failure
.first_not_empty0:
	test edx, edx
	jz .failure
.compare_loop:
	cmpsb
	jne .failure
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
.success:
	mov ebx, [esp + 28]
	sub ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	mov [ebx], eax
	neg ecx
	mov ebx, [esp + 32]
	mov [ebx], ecx
	xor eax, eax
	inc eax
	pop ebx
	pop esi
	pop edi
	ret
.reload_first:
	add eax, byte sizeof(Std$String_block)
	mov ecx, [Std$Integer_smallt(Std$String_block(eax).Length).Value]
	test ecx, ecx
	mov esi, [Std$Address_t(Std$String_block(eax).Chars).Value]
	jnz .first_not_empty
	dec edx
	jnz .failure
	cmp [Std$Integer_smallt(Std$String_block(ebx + sizeof(Std$String_block)).Length).Value], dword 0
	je .success
.failure:
	xor eax, eax
	pop ebx
	pop esi
	pop edi
	ret

