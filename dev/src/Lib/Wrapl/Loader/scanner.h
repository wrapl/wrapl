#ifndef SCANNER_H
#define SCANNER_H

#define tkNONE                            0
#define tkSEMICOLON                       1
#define tkDEF                             2
#define tkVAR                             3
#define tkMOD                             4
#define tkIDENT                           5
#define tkEND                             6
#define tkDOT                             7
#define tkEOI                             8
#define tkIMP                             9
#define tkAS                             10
#define tkUSE                            11
#define tkCOMMA                          12
#define tkMULTIPLY                       13
#define tkEXCLAIM                        14
#define tkASSIGN                         15
#define tkLPAREN                         16
#define tkRPAREN                         17
#define tkRECV                           18
#define tkDO                             19
#define tkRET                            20
#define tkBACK                           21
#define tkFAIL                           22
#define tkSUSP                           23
#define tkWHEN                           24
#define tkREP                            25
#define tkEXIT                           26
#define tkSTEP                           27
#define tkEVERY                          28
#define tkNOT                            29
#define tkWHILE                          30
#define tkUNTIL                          31
#define tkSEND                           32
#define tkYIELD                          33
#define tkALL                            34
#define tkLESS                           35
#define tkGREATER                        36
#define tkTO                             37
#define tkIS                             38
#define tkAT                             39
#define tkEQUAL                          40
#define tkELSE                           41
#define tkTHEN                           42
#define tkAND                            43
#define tkBACKSLASH                      44
#define tkEXACTLY                        45
#define tkNOTEXACTLY                     46
#define tkNOTEQ                          47
#define tkLSEQ                           48
#define tkGREQ                           49
#define tkIN                             50
#define tkSUBTYPE                        51
#define tkPLUS                           52
#define tkMINUS                          53
#define tkDIVIDE                         54
#define tkMODULO                         55
#define tkPOWER                          56
#define tkPARTIAL                        57
#define tkQUERY                          58
#define tkOR                             59
#define tkOF                             60
#define tkINVERSE                        61
#define tkHASH                           62
#define tkSYMBOL                         63
#define tkLBRACKET                       64
#define tkRBRACKET                       65
#define tkCONST                          66
#define tkSELF                           67
#define tkNIL                            68
#define tkLBRACE                         69
#define tkRBRACE                         70
#define tkREFASSIGN                      71
#define tkDOTDOT                         72
#define tkSTRBLOCK                       73
#define tkSKIP                           74
#define tkRASSIGN                        75
#define tkWITH                           76
#define tkSEQ                            77
#define tkPAR                            78
#define tkINT                            79
#define tkBACKQUOTE                      80
#define tkSUM                            81
#define tkPROD                           82
#define tkMAX                            83
#define tkMIN                            84
#define tkCOUNT                          85
#define tkUNIQ                           86
#define tkNEW                            87
#define tkMUST                           88
#define tkLOGICALAND                     89
#define tkLOGICALOR                      90

extern const char *Tokens[];

#include <IO/Stream.h>
#include <setjmp.h>
#include "debugger.h"

extern const Std$Type_t ParseErrorMessageT[];

struct scanner_t {
	IO$Stream_t *Source;
	const char *NextChar;

	struct {
		int Type;
		int LineNo;
		union {
			Std$Object_t *Const;
			const char *Ident;
			struct expr_t *Expr;
		};
	} Token, NextToken;

	struct {
		jmp_buf Handler;
		const Std$Type_t *Type;
		const char *Message;
		int LineNo;
	} Error;

	debug_module_t *DebugInfo;

	scanner_t(IO$Stream_t *Source);
	const char *readl(void);
	void flush();
	bool parse(int Type);
	void accept(int Type);
	void unparse();
	__attribute__ ((noreturn)) void raise_error(int LineNo, const Std$Type_t *Type, const char *Format, ...);
};

#endif

