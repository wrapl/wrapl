%ifndef STD_SYMBOL_INC
%define STD_SYMBOL_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Symbol

riva_struct t, Std$Object_t
	.ValueSize:		resd 1
	.ValueSpace:	resd 1
	.ValueTable:	resd 1
	.TypeSize:		resd 1
	.TypeSpace:		resd 1
	.TypeTable:		resd 1
	.Skip:			resd 1
	.Function:		resd 1
	.Name:			resd 1
endstruct

riva_struct arrayt, Std$Object_t
	.Count:			resd 1
	.Values:
endstruct

riva_struct nomethodmessage, Std$Object_t
	.Symbol:		resd 1
	.Count			resd 1
	.Stack:			resd 12
endstruct

riva_extern T
riva_extern T.invoke
riva_extern _valuetable_put
riva_extern _typetable_put

riva_extern ArrayT

riva_extern Set
riva_extern Get

riva_extern nomethod.invoke

riva_extern NoMethodMessageT

%ifdef DOCUMENTING

%macro symbol 1+
symbol __LINE__, %1
%endmacro

%macro method 1+
method __LINE__, %1
%endmacro

%macro methodx 1+
methodx __LINE__, %1
%endmacro

%macro set_method 1+
set_method __LINE__, %1
%endmacro

%macro asymbol 1+
asymbol __LINE__, %1
%endmacro

%macro amethod 1+
amethod __LINE__, %1
%endmacro

%macro _amethod 1+
_amethod __LINE__, %1
%endmacro

%else

%define ANY 1
%define SKP 1
%define VAL 2
%define TYP 3

%macro symbol 2
section .symbols
%1: db %2, 0
%endmacro

%macro method 2+
section .symbols
%%SYMBOL:
	db %1, 0
section .methods
	dd %%SYMBOL
	dd %2, 0
	dd %%FUNCTION
datasect
%%FUNCTION:
	dd Std$Function$AsmT, %%entry
textsect
method %+ __LINE__:
%%entry:
%endmacro

%macro methodx 4+
%ifdef WINDOWS
	global %1
	global %1.invoke
	global %1.unchecked
%else
	global %1:data
	global %1.invoke:function
	global %1.unchecked:data
%endif
datasect
%1:
	dd Std$Function$CheckedAsmT
.unchecked:
	dd Std$Function$AsmT
	dd .invoke
	dd %2
	dd %%file
	dd __LINE__
%%file:
	db __FILE__, 0
section .symbols
%%SYMBOL:
	db %3, 0
section .methods
	dd %%SYMBOL
	dd %4, 0
	dd .unchecked
textsect
.invoke:
%endmacro

%macro set_method 3+
section .symbols
%%SYMBOL:
	db %1, 0
section .methods
	dd %%SYMBOL
	dd %3, 0
	dd %2
%endmacro

%macro asymbol 1
global %1
section .asymbol
%1: db 0
%endmacro

%macro amethod 2+
section .methods
	dd %1
	dd %2, 0
	dd %%FUNCTION
datasect
%%FUNCTION:
	dd Std$Function$AsmT, %%entry
textsect
method %+ __LINE__:
%%entry:
%endmacro

%macro _amethod 3+
section .methods
	dd %1
	dd %3, 0
	dd %2
%endmacro

%endif

%endif
