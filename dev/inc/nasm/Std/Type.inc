%ifndef STD_TYPE_INC
%define STD_TYPE_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Type

riva_struct t, Std$Object_t
	.Types: resa 1
	.Invoke: resa 1; invoke(value, args, count)
	.Fields: resa 1
	.Levels: resa 1
endstruct

riva_extern T
riva_extern T.invoke

%ifdef DOCUMENTING

%macro ctype 1+
ctype __LINE__, %1
%endmacro

%else

%macro ctype 1-2+ 0
datasect
	align 8
%ifdef WINDOWS
	global %1
%else
	global %1:data
%endif
%1: da Std$Type$T
	da .types
	da .invoke
	da .fields
	da .levels
.types:
	da %1, %2, 0
.fields:
	da Std$Array$T, 0, Std$Integer$SmallT, 0
.levels:
	da %0
%assign i 0
%rep %0
	da i
	%assign i i + 1
%endrep
textsect
%endmacro

%endif

%endif
