%ifndef STD_BUFFER_INC
%define STD_BUFFER_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Buffer

riva_struct t, Std$Object_t
	.Value:	resd 1
	.Length: resd 1
endstruct

riva_extern T

riva_extern Compare
riva_extern Hash

riva_extern _alloc
riva_extern _new

%endif
