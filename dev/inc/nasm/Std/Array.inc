%ifndef STD_ARRAY_INC
%define STD_ARRAY_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Array

riva_struct t, Std$Object_t
	.Values: resd 1
	.Length: resv(Std$Integer_smallt)
endstruct

riva_extern T
riva_extern _new

%endif
