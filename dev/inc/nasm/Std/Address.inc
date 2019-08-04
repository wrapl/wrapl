%ifndef STD_ADDRESS_INC
%define STD_ADDRESS_INC

%include "Std/Object.inc"

%include "Riva-Header.inc"
riva_module Std$Address

riva_struct t, Std$Object_t
	.Value:	resd 1
endstruct

riva_extern T

riva_extern Compare
riva_extern Hash

riva_extern _alloc
riva_extern _new

riva_struct sizedt, Std$Address_t
	.Length: resb sizeof(Std$Integer_smallt)
endstruct

riva_extern SizedT

riva_extern CompareSized
riva_extern HashSized

riva_extern _alloc_sized
riva_extern _new_sized

%endif
