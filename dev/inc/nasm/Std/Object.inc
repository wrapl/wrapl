%ifndef STD_OBJECT_INC
%define STD_OBJECT_INC

%include "Riva-Header.inc"
riva_module Std$Object

riva_struct t
	.Type:	resd 1
endstruct

riva_struct noncallablemessage, Std$Object_t
	.Count			resd 1
	.Stack:			resd 12
endstruct

riva_extern T
riva_extern Nil

riva_extern Create

riva_extern Compare
riva_extern Less
riva_extern Equal
riva_extern Greater

riva_extern Hash

riva_extern NonCallableMessageT
riva_extern _default_invoke

%macro initial 0
section .initial
	dd %%entry
textsect
%%entry:
%endmacro

%ifdef DOCUMENTING

%macro module 1+
module __LINE__, %1
%endmacro

%macro constant 1+
module __LINE__, %1
%endmacro

%macro cglobal 1+
cglobal __LINE__, %1
%endmacro

%else

%macro module 1+
%endmacro

%macro constant 1
section .constants
%1: dd %%INIT
textsect
%%INIT:
%endmacro

%macro cglobal 2
datasect
	align 8
%ifdef WINDOWS
	global %1
%else
	global %1:data
%endif
	%1:
%endmacro

%endif

%endif
