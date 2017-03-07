%ifndef RIVA_HEADER_INC
%define RIVA_HEADER_INC

bits 32
section .symbols noexec
section .methods noexec
section .initial noexec
section .typedfn noexec
section .text

%ifndef SECTION_INDEX
	%assign SECTION_INDEX 1
%endif

%macro __newsect 3
%ifdef WINDOWS
	section %1%2 %+ _ %+ SECTION_INDEX class=%3 use32
	%assign SECTION_INDEX SECTION_INDEX + 1
%else
	[section %1%2 %+ _ %+ SECTION_INDEX %3]
	%assign SECTION_INDEX SECTION_INDEX + 1
%endif
%endmacro

%macro textsect 0
%ifdef WINDOWS
	__newsect .t, __LINE__, text
%else
	;__newsect .t, FILE %+ __LINE__, exec
	__newsect .t, __LINE__, exec
%endif
%endmacro

%macro datasect 0
%ifdef WINDOWS
	__newsect .d, __LINE__, data
%else
	;__newsect .d, FILE %+ __LINE__, noexec
	__newsect .d, __LINE__, noexec
%endif
%endmacro

%macro struct 1.nolist
	%define %1(X) X + %1
	%define %1(X, N) X + N * %1 %+ .__size__ + %1
	struc %1
%endmacro

%macro struct 2.nolist
	%define %1(X) X + %1
	%define %1(X, N) X + N * %1 %+ .__size__ + %1
	struc %1
		resb sizeof(%2)
%endmacro

%macro endstruct 0.nolist
	.__size__:
	endstruc
%endmacro

%define sizeof(T) T %+ .__size__

%define resv(T) resb sizeof(T)

%macro riva_module 1
%define RIVA_MODULE %1
%endmacro

%macro riva_extern 1
extern RIVA_MODULE %+ $ %+ %1
%endmacro

%macro riva_struct 1+
struct RIVA_MODULE %+ _ %+ %1
%endmacro

%ifdef DOCUMENTING

%macro cfunction 1+
cfunction __LINE__, %1
%endmacro

%macro cdata 1+
cdata __LINE__, %1
%endmacro

%else

%macro cfunction 1
textsect
	align 8
%ifdef WINDOWS
	global %1
%else
	global %1:function
%endif
	%1:
%endmacro

%macro cdata 1
datasect
	align 8
%ifdef WINDOWS
	global %1
%else
	global %1:data
%endif
	%1:
%endmacro

%endif

%endif
