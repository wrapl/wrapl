#include "scanner.h"
#include "missing.h"
#include "debugger.h"
#include "wrapl.h"
#include <Riva.h>
#include <Std.h>
#include <Agg.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "compiler.h"
#include "parser.h"

const char *Tokens[] = {
	"<none>", //tkNONE                            0
	";", //tkSEMICOLON                       1
	"DEF", //tkDEF                             2
	"VAR", //tkVAR                             3
	"MOD", //tkMOD                             4
	"<ident>", //tkIDENT                           5
	"END", //tkEND                             6
	".", //tkDOT                             7
	"<eoi>", //tkEOI                             8
	"IMP", //tkIMP                             9
	"AS", //tkAS                             10
	"USE", //tkUSE                            11
	",", //tkCOMMA                          12
	"*", //tkMULTIPLY                       13
	"!", //tkEXCLAIM                        14
	"<-", //tkASSIGN                         15
	"(", //tkLPAREN                         16
	")", //tkRPAREN                         17
	"RECV", //tkRECV                           18
	"DO", //tkDO                             19
	"RET", //tkRET                            20
	"BACK", //tkBACK                           21
	"FAIL", //tkFAIL                           22
	"SUSP", //tkSUSP                           23
	"WHEN", //tkWHEN                           24
	"REP", //tkREP                            25
	"EXIT", //tkEXIT                           26
	"STEP", //tkSTEP                           27
	"EVERY", //tkEVERY                          28
	"NOT", //tkNOT                            29
	"WHILE", //tkWHILE                          30
	"UNTIL", //tkUNTIL                          31
	"SEND", //tkSEND                           32
	"YIELD", //tkYIELD                          33
	"ALL", //tkALL                            34
	"<", //tkLESS                           35
	">", //tkGREATER                        36
	"TO", //tkTO                             37
	"IS", //tkIS                             38
	"@", //tkAT                             39
	"=", //tkEQUAL                          40
	"//", //tkELSE                           41
	"=>", //tkTHEN                           42
	"&", //tkAND                            43
	"\\", //tkBACKSLASH                      44
	"==", //tkEXACTLY                        45
	"~==", //tkNOTEXACTLY                     46
	"~=", //tkNOTEQ                          47
	"<=", //tkLSEQ                           48
	">=", //tkGREQ                           49
	"IN", //tkIN                             50
	"<:", //tkSUBTYPE                        51
	"+", //tkPLUS                           52
	"-", //tkMINUS                          53
	"/", //tkDIVIDE                         54
	"%", //tkMODULO                         55
	"^", //tkPOWER                          56
	"!!", //tkPARTIAL                        57
	"?", //tkQUERY                          58
	"|", //tkOR                             59
	"OF", //tkOF                             60
	"~", //tkINVERSE                        61
	"#", //tkHASH                           62
	"<symbol>", //tkSYMBOL                         63
	"[", //tkLBRACKET                       64
	"]", //tkRBRACKET                       65
	"<constant>", //tkCONST                          66
	"$", //tkSELF                           67
	"NIL", //tkNIL                            68
	"{", //tkLBRACE                         69
	"}", //tkRBRACE                         70
	"<<=", //tkREFASSIGN                      71
	"..", //tkDOTDOT                         72
	"<block>", //tkSTRBLOCK                       73
	"SKIP", //tkSKIP                           74
	"->", //tkRASSIGN                        75
	"WITH", //tkWITH                           76
	"SEQ", //tkSEQ                            77
	"PAR", //tkPAR                            78
	"INT", //tkINT                            79
	"`", //tkBACKQUOTE                      80
	"SUM", //tkSUM                            81
	"PROD", //tkPROD                           82
	"MAX", //tkMAX                            83
	"MIN", //tkMIN                            84
	"COUNT", //tkCOUNT                          85
	"UNIQ", //tkUNIQ                           86
	"NEW", //tkNEW                            87
	"MUST", //tkMUST                           88
	"/\\", //tkLOGICALAND                     89
	"\\/" //tkLOGICALOR                      90
};

scanner_t::scanner_t(IO$Stream_t *Source) {
	this->Source = Source;
	NextChar = "";
	NextToken.Type = 0;
	NextToken.LineNo = 0;
};

const char *scanner_t::readl(void) {
	//printf("IO$Stream$readi = 0x%x\n", IO$Stream$readi);
	const char *Line = IO$Stream$readi(Source, 0, "\n", 1);
	if (Line == (const char *)0xFFFFFFFF) {
		raise_error(Token.LineNo, SourceErrorMessageT, "Error: error reading source");
	};
	if (Debugger) debug_add_line(DebugInfo, Line);
	return Line;
};

void scanner_t::flush() {
	NextChar = "";
	NextToken.Type = 0;
//	NextToken.LineNo = 0;
};

TYPE(SourceErrorMessageT, ErrorMessageT);

static char *scan_cstring_next(scanner_t *Scanner, const char **Next, int Index) {
	char Char;
	switch (Char = *(*Next)++) {
	case 0: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in string");
	case '\"': {
		char *String = new char[Index + 1];
		String[Index] = 0;
		return String;
	};
	case '\\': {
		switch (Char = *(*Next)++) {
		case 0: case '\n':
			Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in string");
		case 'n': Char = '\n'; break;
		case 'r': Char = '\r'; break;
		case 'b': Char = '\b'; break;
		case 't': Char = '\t'; break;
		case 'e': Char = '\e'; break;
		case 'x': {
			char Code;
			switch (Code = *(*Next)++) {
			case '0' ... '9': Char = 16 * (Code - '0'); break;
			case 'A' ... 'F': Char = 16 * (Code - 'A' + 10); break;
			case 'a' ... 'f': Char = 16 * (Code - 'a' + 10); break;
			default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
			};
			switch (Code = *(*Next)++) {
			case '0' ... '9': Char += Code - '0'; break;
			case 'A' ... 'F': Char += Code - 'A' + 10; break;
			case 'a' ... 'f': Char += Code - 'a' + 10; break;
			default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
			};
		};
		};
	};
	// FALLTHROUGH IS DELIBERATE!
	default: {
		char *String = scan_cstring_next(Scanner, Next, Index + 1);
		String[Index] = Char;
		return String;
	};
	};
};

typedef struct string_t {
	char *Chars;
	int Length;
} string_t;

static string_t scan_string_next(scanner_t *Scanner, const char **Next, int Index) {
	char Char;
	switch (Char = *(*Next)++) {
	case 0: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in string");
	case '\"': {
		string_t String = {new char[Index + 1], Index};
		String.Chars[Index] = 0;
		return String;
	};
	case '\\': {
		switch (Char = *(*Next)++) {
		case 0: case '\n':
			Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in string");
		case 'n': Char = '\n'; break;
		case 'r': Char = '\r'; break;
		case 'b': Char = '\b'; break;
		case 't': Char = '\t'; break;
		case 'e': Char = '\e'; break;
		case 'x': {
			char Code;
			switch (Code = *(*Next)++) {
			case '0' ... '9': Char = 16 * (Code - '0'); break;
			case 'A' ... 'F': Char = 16 * (Code - 'A' + 10); break;
			case 'a' ... 'f': Char = 16 * (Code - 'a' + 10); break;
			default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
			};
			switch (Code = *(*Next)++) {
			case '0' ... '9': Char += Code - '0'; break;
			case 'A' ... 'F': Char += Code - 'A' + 10; break;
			case 'a' ... 'f': Char += Code - 'a' + 10; break;
			default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
			};
		};
		};
	};
	// FALLTHROUGH IS DELIBERATE!
	default: {
		string_t String = scan_string_next(Scanner, Next, Index + 1);
		String.Chars[Index] = Char;
		return String;
	};
	};
};

static bool scan_string_block0_next(scanner_t *Scanner, int Index, char **Chars, int *Length) {
    Start: char Char = *(Scanner->NextChar++);
    switch (Char) {
    case 0: {
        Scanner->NextChar = Scanner->readl();
        if (Scanner->NextChar == 0) Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in block string");
        ++Scanner->NextToken.LineNo;
        goto Start;
    };
    case '\\': {
        Char = *(Scanner->NextChar++);
        switch (Char) {
        case 0: case '\n': {
            Scanner->NextChar = Scanner->readl();
            if (Scanner->NextChar == 0) Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in block string");
            ++Scanner->NextToken.LineNo;
            goto Start;
        };
        case 'n': Char = '\n'; break;
        case 'r': Char = '\r'; break;
        case 't': Char = '\t'; break;
		case 'e': Char = '\e'; break;
        case 'x': {
            char Code;
            switch (Code = *(Scanner->NextChar++)) {
            case '0' ... '9': Char = 16 * (Code - '0'); break;
            case 'A' ... 'F': Char = 16 * (Code - 'A' + 10); break;
            case 'a' ... 'f': Char = 16 * (Code - 'a' + 10); break;
            default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
            };
            switch (Code = *(Scanner->NextChar++)) {
            case '0' ... '9': Char += Code - '0'; break;
            case 'A' ... 'F': Char += Code - 'A' + 10; break;
            case 'a' ... 'f': Char += Code - 'a' + 10; break;
            default: Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: invalid character code in string");
            };
        };
        };
        bool Continue = scan_string_block0_next(Scanner, Index + 1, Chars, Length);
		//printf("(1) Writing character \'%c\' to 0x%x [%d]\n", Char, Chars[0] + Index, Index);
        Chars[0][Index] = Char;
        return Continue;
    };
    case '{': {
        if (Index > 0) {
            Chars[0] = Riva$Memory$alloc_atomic(Index + 1);
			//printf("(2) Writing 0 to 0x%x [%d]\n", Chars[0] + Index, Index);
            Chars[0][Index] = 0;
            Length[0] = Index;
            return true;
        } else {
        	Length[0] = 0;
        	return true;
        };
    };
    case '\'': {
        if (Index > 0) {
        	Chars[0] = Riva$Memory$alloc_atomic(Index + 1);
			//printf("(3) Writing 0 to 0x%x [%d]\n", Chars[0] + Index, Index);
            Chars[0][Index] = 0;
            Length[0] = Index;
            return false;
        } else {
        	Length[0] = 0;
        	return false;
        };
    };
    default: {
    	bool Continue = scan_string_block0_next(Scanner, Index + 1, Chars, Length);
		//printf("(4) Writing character \'%c\' to 0x%x [%d]\n", Char, Chars[0] + Index, Index);
        Chars[0][Index] = Char;
        return Continue;
    };
    };
};

static void scan_string_block0(scanner_t *Scanner) {
	expr_t Head;
    Head.Next = 0;
    expr_t *Tail = &Head;
    char *Chars;
    int Length;
	bool Continue = scan_string_block0_next(Scanner, 0, &Chars, &Length);
    if (Length) {
		if (Continue) {
			Tail = (Tail->Next = new const_expr_t(Scanner->Token.LineNo, Std$String$new_length(Chars, Length)));
		} else {
			Scanner->NextToken.Type = tkCONST;
			Scanner->NextToken.Const = Std$String$new_length(Chars, Length);
			return;
		};
	};
    while (Continue) {
        Scanner->NextToken.Type = 0;
        Tail = (Tail->Next = accept_expr(Scanner));
        Scanner->accept(tkRBRACE);
        Continue = scan_string_block0_next(Scanner, 0, &Chars, &Length);
	    if (Length) Tail = (Tail->Next = new const_expr_t(Scanner->Token.LineNo, Std$String$new_length(Chars, Length)));
	};
	if (Head.Next) {
		Scanner->NextToken.Type = tkSTRBLOCK;
		Scanner->NextToken.Expr = Head.Next;
	} else {
		Scanner->NextToken.Type = tkCONST;
		Scanner->NextToken.Const = Std$String$Empty;
	};
};

static Std$String_t *scan_string_block_next(scanner_t *Scanner, const char *End, int EndLength, const char **Current, int Index) {
	const char *Line = Scanner->readl();
	if (Line) {
		++Scanner->NextToken.LineNo;
	} else {
		Scanner->raise_error(Scanner->NextToken.LineNo, SourceErrorMessageT, "Error: end of input in block string");
	};
	Std$String_t *String;
	const char *Prefix = Line;
	while (*Prefix && *Prefix <= ' ') ++Prefix;
	if (strncmp(Prefix, End, EndLength) == 0) {
		*Current = Prefix + EndLength;
		String = (Std$String_t *)Riva$Memory$alloc_stubborn(sizeof(Std$String_t) + Index * sizeof(Std$String_block));
		String->Type = Std$String$T;
		String->Length.Type = Std$Integer$SmallT;
		String->Count = Index;
	} else {
		String = scan_string_block_next(Scanner, End, EndLength, Current, Index + 1);
		int Length = strlen(Line);
		String->Length.Value += Length;
		String->Blocks[Index].Length.Type = Std$Integer$SmallT;
		String->Blocks[Index].Length.Value = Length;
		String->Blocks[Index].Chars.Type = Std$Address$T;
		String->Blocks[Index].Chars.Value = Line;
	};
	if (Index == 0) Std$String$freeze(String);
	return String;
};

#include "keywords.c"

extern Riva$Module_t Riva$Symbol[];

bool scanner_t::parse(int Type) {
	if (NextToken.Type == 0) {
		const char *Current = NextChar;
		const char *Start;
		scan_loop: {
			//asm("int3");
			Start = Current;
			switch (*Current) {
			case 0: Current = readl();
				if (Current == 0) {
					NextToken.Type = tkEOI; goto scan_done;
				} else {
					NextToken.LineNo++; goto scan_loop;
				};
			case '\x01' ... ' ': ++Current; goto scan_loop;
			case '0' ... '9': Start = Current++; goto scan_integer;
			case 'A' ... 'Z': case 'a' ... 'z': case '_': case '$': Start = Current++; goto scan_identifier;
			case '\xC2' ... '\xDF': /*Should validate UTF-8*/ Start = Current; Current += 2; goto scan_identifier;
			case '\xE0' ... '\xEF': /*Should validate UTF-8*/ Start = Current; Current += 3; goto scan_identifier;
			case '\xF0' ... '\xF4': /*Should validate UTF-8*/ Start = Current; Current += 4; goto scan_identifier;
			case ':': ++Current; goto scan_symbol;
			case '\"': {
				++Current;
				NextToken.Type = tkCONST;
				string_t String = scan_string_next(this, &Current, 0);
				NextToken.Const = Std$String$new_length(String.Chars, String.Length);
				goto scan_done;
			};
			case '\'':
				NextChar = ++Current;
				scan_string_block0(this);
				Current = NextChar;
				goto scan_done;
			case '=': ++Current;
				switch (*Current) {
				case '=': ++Current; NextToken.Type = tkEXACTLY; goto scan_done;
				case '>': ++Current; NextToken.Type = tkTHEN; goto scan_done;
				default: NextToken.Type = tkEQUAL; goto scan_done;
				};
			case '~': ++Current;
				switch (*Current) {
				case '=': ++Current;
					switch (*Current) {
					case '=': ++Current; NextToken.Type = tkNOTEXACTLY; goto scan_done;
					default: NextToken.Type = tkNOTEQ; goto scan_done;
					};
				default: NextToken.Type = tkINVERSE; goto scan_done;
				};
			case '¬': ++Current; NextToken.Type = tkINVERSE; goto scan_done;
			case '}': ++Current; NextToken.Type = tkRBRACE; goto scan_done;
			case '|': ++Current; NextToken.Type = tkOR; goto scan_done;
			case '{': ++Current; NextToken.Type = tkLBRACE; goto scan_done;
			case '^': ++Current; NextToken.Type = tkPOWER; goto scan_done;
			case ']': ++Current; NextToken.Type = tkRBRACKET; goto scan_done;
			case '\\': ++Current;
				switch (*Current) {
				case '/': ++Current; NextToken.Type = tkLOGICALOR; goto scan_done;
				default: NextToken.Type = tkBACKSLASH; goto scan_done;
				};
			case '[': ++Current; NextToken.Type = tkLBRACKET; goto scan_done;
			case '@': ++Current; NextToken.Type = tkAT; goto scan_done;
			case '?': ++Current; NextToken.Type = tkQUERY; goto scan_done;
			case ';': ++Current; NextToken.Type = tkSEMICOLON; goto scan_done;
			case '>': ++Current;
				switch (*Current) {
				case '>': ++Current; goto scan_block_string;
				case '=': ++Current; NextToken.Type = tkGREQ; goto scan_done;
				default: NextToken.Type = tkGREATER; goto scan_done;
				};
			case '/': ++Current;
				switch (*Current) {
				case '/': ++Current; NextToken.Type = tkELSE; goto scan_done;
				case '\\': ++Current; NextToken.Type = tkLOGICALAND; goto scan_done;
				default: NextToken.Type = tkDIVIDE; goto scan_done;
				};
			case '<': ++Current;
				switch (*Current) {
				case '<': ++Current; if (*Current == '=') {
					++Current; NextToken.Type = tkREFASSIGN; goto scan_done;
				} else {
					--Current; NextToken.Type = tkLESS; goto scan_done;
				};
				case '=': ++Current; NextToken.Type = tkLSEQ; goto scan_done;
				case '-': ++Current; NextToken.Type = tkASSIGN; goto scan_done;
				case ':': ++Current; NextToken.Type = tkSUBTYPE; goto scan_done;
				default: NextToken.Type = tkLESS; goto scan_done;
				};
			case '.': Start = Current++;
				switch (*Current) {
				case '0' ... '9': ++Current; goto scan_real_mantissa;
				case '.': ++Current; NextToken.Type = tkDOTDOT; goto scan_done;
				default: NextToken.Type = tkDOT; goto scan_done;
				};
			case '-': Start = Current++;
				switch (*Current) {
				case '0' ... '9': ++Current; goto scan_integer;
				case '.': ++Current; goto scan_real_mantissa;
				case '=': ++Current; goto scan_comment;
				case '-': Current = readl();
					if (Current == 0) {
						NextToken.Type = tkEOI; goto scan_done;
					} else {
						++NextToken.LineNo; goto scan_loop;
					};
				case '>': ++Current; NextToken.Type = tkRASSIGN; goto scan_done;
				default: NextToken.Type = tkMINUS; goto scan_done;
				};
			case ',': ++Current; NextToken.Type = tkCOMMA; goto scan_done;
			case '+': ++Current; NextToken.Type = tkPLUS; goto scan_done;
			case '*': ++Current; NextToken.Type = tkMULTIPLY; goto scan_done;
			case ')': ++Current; NextToken.Type = tkRPAREN; goto scan_done;
			case '(': ++Current; NextToken.Type = tkLPAREN; goto scan_done;
			case '&': ++Current; NextToken.Type = tkAND; goto scan_done;
			case '%': ++Current; NextToken.Type = tkMODULO; goto scan_done;
			case '!': ++Current; NextToken.Type = tkEXCLAIM; goto scan_done;
			case '#': ++Current; NextToken.Type = tkHASH; goto scan_done;
			//case '$': ++Current; NextToken.Type = tkSELF; goto scan_done;
			case '`': ++Current; NextToken.Type = tkBACKQUOTE; goto scan_done;
			default: raise_error(NextToken.LineNo, SourceErrorMessageT, "Error: invalid character \'%c\'", *Current);
			};
			scan_integer: {
				switch(*Current) {
				case '0' ... '9': ++Current; goto scan_integer;
				case '.': ++Current; goto scan_real_mantissa;
				case 'e': case 'E': ++Current; goto scan_real_exponent_sign;
				case '_': goto scan_integer_base;
				case '/': if (('0' <= Current[1]) && (Current[1] <= '9')) {
					++Current; goto scan_rational;
				};
				// Fallthrough is deliberate
				default: {
					int Length = Current - Start;
					char Buffer[Length + 1];
					memcpy(Buffer, Start, Length);
					Buffer[Length] = 0;
					NextToken.Type = tkCONST;
					NextToken.Const = Std$Integer$new_string(Buffer);
					goto scan_done;
				};
				};
			};
			scan_integer_base: {
				int Base = 0;
				for (const char *Digit = Start; Digit < Current; ++Digit) Base = 10 * Base + *Digit - '0';
				Start = ++Current;
				for (;;) switch (*Current) {
				case '0' ... '9': case 'a' ... 'z': case 'A' ... 'Z':
					++Current;
					continue;
				default: {
					int Length = Current - Start;
					char Buffer[Length + 1];
					memcpy(Buffer, Start, Length);
					Buffer[Length] = 0;
					NextToken.Type = tkCONST;
					NextToken.Const = Std$Integer$new_string_base(Buffer, Base);
					goto scan_done;
				};
				};
			};
			scan_rational: {
				switch(*Current) {
				case '0' ... '9': ++Current; goto scan_rational;
				default: {
					int Length = Current - Start;
					char Buffer[Length + 1];
					memcpy(Buffer, Start, Length);
					Buffer[Length] = 0;
					NextToken.Type = tkCONST;
					NextToken.Const = Std$Rational$new_string(Buffer);
					goto scan_done;
				};
				};
			};
			scan_real_mantissa: {
				switch(*Current) {
				case '0' ... '9': ++Current; goto scan_real_mantissa;
				case 'e': case 'E': ++Current; goto scan_real_exponent_sign;
				default: {
					int Length = Current - Start;
					char Buffer[Length + 1];
					memcpy(Buffer, Start, Length);
					Buffer[Length] = 0;
					NextToken.Type = tkCONST;
					NextToken.Const = (Std$Object_t *)Std$Real$new_string(Buffer);
					goto scan_done;
				};
				};
			};
			scan_real_exponent_sign: {
				switch(*Current) {
				case '0' ... '9': ++Current; goto scan_real_exponent;
				case '+': case '-': ++Current; goto scan_real_exponent;
				default: raise_error(NextToken.LineNo, SourceErrorMessageT, "Error: invalid character in literal");
				};
			};
			scan_real_exponent: {
				switch(*Current) {
				case '0' ... '9': ++Current; goto scan_real_exponent;
				default: {
					int Length = Current - Start;
					char Buffer[Length + 1];
					memcpy(Buffer, Start, Length);
					Buffer[Length] = 0;
					NextToken.Type = tkCONST;
					NextToken.Const = (Std$Object_t *)Std$Real$new_string(Buffer);
					goto scan_done;
				};
				};
			};
			scan_identifier: {
				switch(*Current) {
				case 'A' ... 'Z': case 'a' ... 'z': case '_': case '$': case '0' ... '9': ++Current; goto scan_identifier;
				case '\xC2' ... '\xDF': /*Should validate UTF-8*/ Current += 2; goto scan_identifier;
				case '\xE0' ... '\xEF': /*Should validate UTF-8*/ Current += 3; goto scan_identifier;
				case '\xF0' ... '\xF4': /*Should validate UTF-8*/ Current += 4; goto scan_identifier;
				default: {
					int Length = Current - Start;
					char *Identifier = new char[Length + 1];
					memcpy(Identifier, Start, Length);
					Identifier[Length] = 0;
					const struct keyword_t *Keyword = keyword::lookup(Identifier, Length);
					if (Keyword) {
						NextToken.Type = Keyword->Token;
					} else {
						NextToken.Type = tkIDENT;
						NextToken.Ident = Identifier;
					};
					goto scan_done;
				};
				};
			};
			scan_symbol: {
				switch (*Current) {
				case '?': {
					++Current;
					NextToken.Type = tkSYMBOL;
					NextToken.Const = (Std$Object_t *)Std$Symbol$new_string("<anon>");
					goto scan_done;
				};
				case 'A' ... 'Z': case 'a' ... 'z': case '_': Start = Current++; goto scan_symbol_identifier;
				case '\xC2' ... '\xDF': /*Should validate UTF-8*/ Start = Current; Current += 2; goto scan_symbol_identifier;
				case '\xE0' ... '\xEF': /*Should validate UTF-8*/ Start = Current; Current += 3; goto scan_symbol_identifier;
				case '\xF0' ... '\xF4': /*Should validate UTF-8*/ Start = Current; Current += 4; goto scan_symbol_identifier;
				case '\"': {
					++Current;
					int Type; void *Value;
					Riva$Module$import(Riva$Symbol, scan_cstring_next(this, &Current, 0), &Type, &Value);
					NextToken.Type = tkSYMBOL;
					NextToken.Const = (Std$Object_t *)Value;
					goto scan_done;
				};
				default: raise_error(NextToken.LineNo, SourceErrorMessageT, "Error: invalid character in symbol");
				};
			};
			scan_symbol_identifier: {
				switch(*Current) {
				case 'A' ... 'Z': case 'a' ... 'z': case '_': case '0' ... '9': ++Current; goto scan_symbol_identifier;
				case '\xC2' ... '\xDF': /*Should validate UTF-8*/ Current += 2; goto scan_symbol_identifier;
				case '\xE0' ... '\xEF': /*Should validate UTF-8*/ Current += 3; goto scan_symbol_identifier;
				case '\xF0' ... '\xF4': /*Should validate UTF-8*/ Current += 4; goto scan_symbol_identifier;
				default: {
					int Length = Current - Start;
					char *Identifier = new char[Length + 1];
					memcpy(Identifier, Start, Length);
					Identifier[Length] = 0;
					int Type; void *Value;
					Riva$Module$import(Riva$Symbol, Identifier, &Type, &Value);
					NextToken.Type = tkSYMBOL;
					NextToken.Const = (Std$Object_t *)Value;
					goto scan_done;
				};
				};
			};
			scan_block_string: {
				const char *End = Current;
				NextToken.Type = tkCONST;
				NextToken.Const = (Std$Object_t *)scan_string_block_next(this, End, strlen(End) - 1, &Current, 0);
				goto scan_done;
			};
			scan_comment: {
				int Level = 1;
				comment_loop: {
					switch (*Current++) {
					case 0: Start = Current = readl();
						if (Current == 0) {
							raise_error(NextToken.LineNo, SourceErrorMessageT, "Error: end of input in comment");
						} else {
							++NextToken.LineNo;
						};
						goto comment_loop;
					case '=':
						switch (*Current++) {
						case '-': if (--Level == 0) goto scan_loop;
						default: goto comment_loop;
						};
					case '-':
						switch (*Current++) {
						case '-': Start = Current = readl();
							if (Current == 0) {
								// RAISE ERROR HERE!!!
							} else {
								++NextToken.LineNo;
							};
							goto comment_loop;
						case '=': ++Level;
							goto comment_loop;
						};
					};
					goto comment_loop;
				};
			};
		};
	scan_done:
		NextChar = Current;
	};
	if (NextToken.Type == Type) {
		Token = NextToken;
		NextToken.Type = 0;
		return true;
	} else {
		return false;
	};
};

void scanner_t::unparse() {
	NextToken = Token;
};

TYPE(ParseErrorMessageT, ErrorMessageT);

void scanner_t::accept(int Type) {
	if (!parse(Type)) raise_error(NextToken.LineNo, ParseErrorMessageT, "Error: expected \"%s\" not \"%s\"", Tokens[Type], Tokens[NextToken.Type]);
};

void scanner_t::raise_error(int LineNo, const Std$Type_t *Type, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	int Length = vasprintf((char **)&Error.Message, Format, Args);
	va_end(Args);
	Error.LineNo = LineNo;
	Error.Type = Type;
	longjmp(Error.Handler, 1);
};

SYMBOL($AT, "@");
ASYMBOL(Image);
// Converts an object to it's representation in Wrapl.

AMETHOD(Image, TYP, Std$Number$T) {
//:Std$String$T
	return Std$Function$call((Std$Object_t *)$AT, 2, Result, Args[0].Val, 0, Std$String$T, 0);
};

AMETHOD(Image, TYP, Std$String$T) {
//:Std$String$T
	static char Hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	const Std$String_t *In = (Std$String_t *)Args[0].Val;
	if (In->Length.Value == 0) {
		Result->Val = Std$String$new("\"\"");
		return SUCCESS;
	};
	Std$String_t *Out = Std$String$alloc(In->Count + 2);
	const Std$String_block *Src = In->Blocks;
	Std$String_block *Dst = Out->Blocks;
	int Length = In->Length.Value + 2;
	Dst->Length.Value = 1;
	Dst->Chars.Value = "\"";
	Dst++;
	for (int I = In->Count; --I >= 0;) {
		int SrcLength = Src->Length.Value;
		int DstLength = SrcLength;
		const char *SrcChars = Src->Chars.Value;
		for (int J = 0; J < SrcLength; ++J) {
			unsigned char Char = SrcChars[J];
			if (Char == '\"') {
				DstLength += 1;
			} else if (Char == '\\') {
				DstLength += 1;
			} else if (Char == '\'') {
				DstLength += 1;
			} else if (Char == '\t') {
				DstLength += 1;
			} else if (Char == '\r') {
				DstLength += 1;
			} else if (Char == '\n') {
				DstLength += 1;
			} else if ((Char < ' ') || (Char >= 0x80)) {
				DstLength += 3;
			};
		};
		Src++;
		char *DstChars = Riva$Memory$alloc_atomic(DstLength + 1);
		if (DstLength == SrcLength) {
			memcpy(DstChars, SrcChars, SrcLength);
		} else {
			Length += DstLength - SrcLength;
			char *Tmp = DstChars;
			for (int J = 0; J < SrcLength; ++J) {
				unsigned char Char = SrcChars[J];
				if (Char == '\"') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\\') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\'') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\t') {
					*(Tmp++) = '\\';
					*(Tmp++) = 't';
				} else if (Char == '\r') {
					*(Tmp++) = '\\';
					*(Tmp++) = 'r';
				} else if (Char == '\n') {
					*(Tmp++) = '\\';
					*(Tmp++) = 'n';
				} else if ((Char < ' ') || (Char >= 0x80)) {
					*(Tmp++) = '\\';
					*(Tmp++) = 'x';
					*(Tmp++) = Hex[Char / 16];
					*(Tmp++) = Hex[Char % 16];
				} else {
					*(Tmp++) = Char;
				};
			};
		};
		DstChars[DstLength] = 0;
		Dst->Length.Type = Std$Integer$SmallT;
		Dst->Length.Value = DstLength;
		Dst->Chars.Type = Std$Address$T;
		Dst->Chars.Value = DstChars;
		Dst++;
	};
	Dst->Length.Value = 1;
	Dst->Chars.Value = "\"";
	Out->Length.Value = Length;
	Std$String$freeze(Out);
	Result->Val = (Std$Object_t *)Out;
	return SUCCESS;
};

AMETHOD(Image, TYP, Std$Symbol$T) {
//:Std$String$T
	static char Hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	const Std$String_t *In = ((Std$Symbol_t *)Args[0].Val)->Name;
	Std$String_t *Out = Std$String$alloc(In->Count + 2);
	const Std$String_block *Src = In->Blocks;
	Std$String_block *Dst = Out->Blocks;
	int Length = In->Length.Value + 2;
	Dst->Length.Value = 2;
	Dst->Chars.Value = ":\"";
	Dst++;
	for (int I = In->Count; --I >= 0;) {
		int SrcLength = Src->Length.Value;
		int DstLength = SrcLength;
		const char *SrcChars = Src->Chars.Value;
		for (int J = 0; J < SrcLength; ++J) {
			unsigned char Char = SrcChars[J];
			if (Char == '\"') {
				DstLength += 1;
			} else if (Char == '\\') {
				DstLength += 1;
			} else if (Char == '\'') {
				DstLength += 1;
			} else if (Char == '\t') {
				DstLength += 1;
			} else if (Char == '\r') {
				DstLength += 1;
			} else if (Char == '\n') {
				DstLength += 1;
			} else if ((Char < ' ') || (Char >= 0x80)) {
				DstLength += 3;
			};
		};
		Src++;
		char *DstChars = Riva$Memory$alloc_atomic(DstLength + 1);
		if (DstLength == SrcLength) {
			memcpy(DstChars, SrcChars, SrcLength);
		} else {
			Length += DstLength - SrcLength;
			char *Tmp = DstChars;
			for (int J = 0; J < SrcLength; ++J) {
				unsigned char Char = SrcChars[J];
				if (Char == '\"') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\\') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\'') {
					*(Tmp++) = '\\';
					*(Tmp++) = Char;
				} else if (Char == '\t') {
					*(Tmp++) = '\\';
					*(Tmp++) = 't';
				} else if (Char == '\r') {
					*(Tmp++) = '\\';
					*(Tmp++) = 'r';
				} else if (Char == '\n') {
					*(Tmp++) = '\\';
					*(Tmp++) = 'n';
				} else if ((Char < ' ') || (Char >= 0x80)) {
					*(Tmp++) = '\\';
					*(Tmp++) = 'x';
					*(Tmp++) = Hex[Char / 16];
					*(Tmp++) = Hex[Char % 16];
				} else {
					*(Tmp++) = Char;
				};
			};
		};
		DstChars[DstLength] = 0;
		Dst->Length.Type = Std$Integer$SmallT;
		Dst->Length.Value = DstLength;
		Dst->Chars.Type = Std$Address$T;
		Dst->Chars.Value = DstChars;
		Dst++;
	};
	Dst->Length.Value = 1;
	Dst->Chars.Value = "\"";
	Out->Length.Value = Length;
	Std$String$freeze(Out);
	Result->Val = (Std$Object_t *)Out;
	return SUCCESS;
};

STRING(SpaceIsSpace, " IS ");
STRING(CommaSpace, ", ");

AMETHOD(Image, TYP, Agg$List$T) {
//:Std$String$T
	Agg$List_node *Node = ((Agg$List_t *)Args[0].Val)->Head;
	if (Node) {
		Std$Object_t *Final;
		Std$Function_result Buffer;
		switch (Std$Function$call((Std$Object_t *)Image, 1, &Buffer, Node->Value, 0)) {
		case SUSPEND: case SUCCESS:
			Final = Std$String$add(Std$String$new("["), Buffer.Val);
			break;
		case FAILURE:
			Result->Val = Std$String$new("Image error");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
		while (Node = Node->Next) {
			Final = Std$String$add(Final, CommaSpace);
			switch (Std$Function$call((Std$Object_t *)Image, 1, &Buffer, Node->Value, 0)) {
			case SUSPEND: case SUCCESS:
				Final = Std$String$add(Final, Buffer.Val);
				break;
			case FAILURE:
				Result->Val = Std$String$new("Image error");
				return MESSAGE;
			case MESSAGE:
				Result->Val = Buffer.Val;
				return MESSAGE;
			};
		};
		Result->Val = Std$String$add(Final, Std$String$new("]"));
		return SUCCESS;
	} else {
		Result->Val = Std$String$new("[]");
		return SUCCESS;
	};
};

AMETHOD(Image, TYP, Agg$Table$T) {
//:Std$String$T
	Agg$Table$trav *Trav = Agg$Table$trav_new();
	Std$Object$t *Node = Agg$Table$trav_first(Trav, Args[0].Val);
	if (!Node) {
		Result->Val = Std$String$new("{}");
		return SUCCESS;
	};
	
	Std$Object_t *Final = Std$String$new("{");
	bool Comma = false;
	do {
		Std$Function_result Buffer;
		switch (Std$Function$call((Std$Object_t *)Image, 1, &Buffer, Agg$Table$node_key(Node), 0)) {
		case SUSPEND: case SUCCESS:
			if (Comma) Final = Std$String$add(Final, CommaSpace);
			Final = Std$String$add(Final, Buffer.Val);
			break;
		case FAILURE:
			Result->Val = Std$String$new("Image error");
			return MESSAGE;
		case MESSAGE:
			Result->Val = Buffer.Val;
			return MESSAGE;
		};
		Std$Object$t *Value = Agg$Table$node_value(Node);
		if (Value != Std$Object$Nil) {
			switch (Std$Function$call((Std$Object_t *)Image, 1, &Buffer, Value, 0)) {
			case SUSPEND: case SUCCESS:
				Final = Std$String$add(Final, SpaceIsSpace);
				Final = Std$String$add(Final, Buffer.Val);
				break;
			case FAILURE:
				Result->Val = Std$String$new("Image error");
				return MESSAGE;
			case MESSAGE:
				Result->Val = Buffer.Val;
				return MESSAGE;
			};
		};
		Comma = true;
		Node = Agg$Table$trav_next(Trav);
	} while (Node);
	Result->Val = Std$String$add(Final, Std$String$new("}"));
	return SUCCESS;
};

AMETHOD(Image, VAL, Std$Object$Nil) {
//:Std$String$T
	Result->Val = (Std$Object_t *)Std$String$new("NIL");
	return SUCCESS;
};
