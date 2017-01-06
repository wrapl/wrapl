%ifndef STD_STRING_INC
%define STD_STRING_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$String

riva_struct block
	.Length:	resv(Std$Integer_smallt)
	.Chars:		resv(Std$Address_t)
endstruct

riva_struct t, Std$Object_t
	.Count:		resd 1
	.Length:	resv(Std$Integer_smallt)
	.Hash:		resv(Std$Integer_smallt)
	.Blocks:
endstruct

riva_extern T
riva_extern Nil
riva_extern Empty
riva_extern New
riva_extern Add
riva_extern Add.invoke

riva_extern Compare
riva_extern Hash

riva_extern FromAddress

riva_extern _new
riva_extern _copy
riva_extern _new_length
riva_extern _copy_length
riva_extern _flatten

%macro cstring 1
datasect
%1:
	dd Std$String$T
	dd 1
	dd Std$Integer$SmallT, .end - .chars
	dd 0, 0
	dd Std$Integer$SmallT, .end - .chars
	dd Std$Address$T, .chars
	dd 0, 0, 0, 0
.chars:
%endmacro

%macro cstrend 0
.end: db 0
%endmacro

%endif
