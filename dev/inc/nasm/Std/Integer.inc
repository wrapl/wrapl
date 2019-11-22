%ifndef STD_INTEGER_INC
%define STD_INTEGER_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Integer

riva_struct smallt, Std$Object_t
	.Value:	resd 1
endstruct

riva_struct bigt, Std$Object_t
	.Value:	resd 3
endstruct

riva_extern T
riva_extern SmallT
riva_extern BigT
riva_extern ToSmallSmall
riva_extern Of
riva_extern Compare
riva_extern Hash

riva_extern CompareSmall
riva_extern HashSmall

riva_extern _alloc_small
riva_extern _alloc_big

riva_extern _new_small
riva_extern _new_big
riva_extern _new_string

%endif
