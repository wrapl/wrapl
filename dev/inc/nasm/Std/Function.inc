%ifndef STD_FUNCTION_INC
%define STD_FUNCTION_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Function

riva_struct argument
	.Val: resa 1
	.Ref: resa 1
endstruct

riva_struct result
	.Val: resa 1
	.Ref: resa 1
	.State: resa 1
endstruct

riva_struct state
	.Run: resa 1
	.Chain: resa 1
	.Resume: resa 1
endstruct

riva_struct asmt, Std$Object_t
	.Invoke: resa 1;(fun, count, args)
endstruct

riva_struct checkedasmt, Std$Object_t
	.Unchecked: resv(Std$Function_asmt)
	.Count: resa 1
	.File: resa 1
	.Line: resa 1
endstruct

riva_struct asmstate, Std$Function_state
	.Handler: resa 1
endstruct

riva_struct ct, Std$Object_t
	.Invoke: resa 1;(native, count, args, result)
endstruct

riva_struct checkedct, Std$Object_t
	.Invoke: resa 1
	.File: resa 1
	.Line: resd 1
	.Count: resd 1
endstruct

riva_struct fewargsmessage, Std$Object_t
	.Func: resa 1
	.Count: resa 1
endstruct

riva_struct argtypemessage, Std$Object_t
	.Func: resa 1
	.Index: resa 1
	.Expected: resa 1
	.Received: resa 1
endstruct

riva_extern T
riva_extern AsmT
riva_extern CheckedAsmT

riva_extern FewArgsMessageT
riva_extern ArgTypeMessageT

riva_extern IteratorT
riva_extern IteratorNext

%ifdef DOCUMENTING

%macro _function 1+
_function __LINE__, %1
%endmacro

%macro function 1+
function __LINE__, %1
%endmacro

%else

%macro _function 1
%ifdef WINDOWS
	global %1
%else
	global %1:data
%endif
datasect
%1:
	da Std$Function$AsmT
	da %%here
textsect
%%here:
%endmacro

%macro function 2
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
	da Std$Function$CheckedAsmT
.unchecked:
	da Std$Function$AsmT
	da .invoke
	da %%file
	dd __LINE__
	dd %2
%%file:
	db __FILE__, 0
textsect
.invoke:
%endmacro

%endif

%macro local_function 1
datasect
%1:
	da Std$Function$AsmT
	da %%here
textsect
%%here:
%endmacro

%endif
