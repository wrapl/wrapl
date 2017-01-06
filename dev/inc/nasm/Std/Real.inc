%ifndef STD_REAL_INC
%define STD_REAL_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Real

riva_struct t, Std$Object_t
	.Value:	resq 1
endstruct

riva_extern T
riva_extern New
riva_extern Compare
riva_extern Hash

riva_extern _alloc

%endif
