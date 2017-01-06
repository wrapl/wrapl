%ifndef STD_RATIONAL_INC
%define STD_RATIONAL_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Rational

riva_struct t, Std$Object_t
	.Value: resd 6
endstruct

riva_extern T
riva_extern Compare
riva_extern Hash
riva_extern New

riva_extern _new_small_small

%endif
