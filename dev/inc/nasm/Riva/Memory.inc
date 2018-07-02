%ifndef RIVA_INC
%define RIVA_INC

%include "Riva-Header.inc"
riva_module Riva$Memory

riva_extern _collect
riva_extern _strdup
riva_extern _alloc
riva_extern _alloc_uncollectable
riva_extern _alloc_atomic
riva_extern _alloc_large
riva_extern _alloc_code
riva_extern _calloc
riva_extern _realloc
riva_extern _free
riva_extern _size
riva_extern _alloc_stubborn
riva_extern _change_stubborn
riva_extern _freeze_stubborn

%endif
