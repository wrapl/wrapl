%include "Std.inc"

ctype T
.invoke: equ 0

%ifdef DOCUMENTING

%define Std$Number$T T

%define number_method method
pushfile "Methods.asm"
%include "Methods.asm"
popfile

%endif
