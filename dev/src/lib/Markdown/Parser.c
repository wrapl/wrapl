#include <Std.h>
#include <Riva.h>
#include <Html/Entities.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <setjmp.h>
#include <md4c.h>

typedef struct parser_t {
	const Std$Type$t *Type;
	Std$Object$t *UserData;
	Std$Object$t *EnterBlockFn, *LeaveBlockFn;
	Std$Object$t *EnterSpanFn, *LeaveSpanFn;
	Std$Object$t *TextFn;
	MD_RENDERER Renderer[1];
	jmp_buf OnError[1];
} parser_t;

TYPE(T, IO$Stream$WriterT, IO$Stream$T);

static Std$Object$t *attributes_to_string(MD_ATTRIBUTE *Attributes) {
	int Total = 0;
	int NumParts = 0;
	while (Attributes->substr_offsets[NumParts] != Attributes->size) ++NumParts;
	char *Parts[NumParts];
	const char *Text = Attributes->text;
	for (int I = 0; I < NumParts; ++I) {
		int Offset = Attributes->substr_offsets[I];
		int Length = Attributes->substr_offsets[I + 1] - Offset;
		switch (Attributes->substr_types[I]) {
		case MD_TEXT_NORMAL: {
			Parts[I] = Text + Offset;
			Total += Length;
			break;
		}
		case MD_TEXT_ENTITY: {
			Std$String$t *Entity = Agg$StringTable$get(Html$Entities$ByName, Text + Offset, Length);
			Parts[I] = Std$String$flatten(Entity);
			Total += Std$String$get_length(Entity);
			break;
		}
		case MD_TEXT_NULLCHAR:
			Parts[I] = "\xEF\xBF\xBD";
			Total += strlen("\xEF\xBF\xBD");
			break;
		}
	}
	char *Chars = Riva$Memory$alloc_atomic(Total + 1);
	char *P = Chars;
	for (int I = 0; I < NumParts; ++I) P = stpcpy(P, Parts[I]);
	return Std$String$new_length(Chars, Total);
}

SYMBOL($doc, "doc");
SYMBOL($quote, "quote");
SYMBOL($ul, "ul");
SYMBOL($ol, "ol");
SYMBOL($li, "li");
SYMBOL($hr, "hr");
SYMBOL($h, "h");
SYMBOL($code, "code");
SYMBOL($html, "html");
SYMBOL($p, "p");
SYMBOL($table, "table");
SYMBOL($thead, "thead");
SYMBOL($tbody, "tbody");
SYMBOL($tr, "tr");
SYMBOL($th, "th");
SYMBOL($td, "td");
SYMBOL($default, "default");
SYMBOL($left, "left");
SYMBOL($center, "center");
SYMBOL($right, "right");

static int enter_block_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	Std$Function$result Result[1];
	Std$Function$argument Arguments[5] = {0,};
	int NumArguments = 2;
	Arguments[0].Val = Parser->UserData;
	Arguments[0].Ref = &Parser->UserData;
	switch (BlockType) {
	case MD_BLOCK_DOC: {
		Arguments[1].Val = $doc;
		break;
	}
	case MD_BLOCK_QUOTE: {
		Arguments[1].Val = $quote;
		break;
	}
	case MD_BLOCK_UL: {
		MD_BLOCK_UL_DETAIL *ULDetail = (MD_BLOCK_UL_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $ul;
		Arguments[2].Val = ULDetail->is_tight ? $true : $false;
		Arguments[3].Val = Std$String$new_char(ULDetail->mark);
		break;
	}
	case MD_BLOCK_OL: {
		MD_BLOCK_OL_DETAIL *OLDetail = (MD_BLOCK_OL_DETAIL *)Detail;
		NumArguments = 5;
		Arguments[1].Val = $ul;
		Arguments[2].Val = Std$Integer$new_small(OLDetail->start);
		Arguments[3].Val = OLDetail->is_tight ? $true : $false;
		Arguments[4].Val = Std$String$new_char(OLDetail->mark_delimiter);
		break;
	}
	case MD_BLOCK_LI: {
		Arguments[1].Val = $li;
		break;
	}
	case MD_BLOCK_HR: {
		Arguments[1].Val = $hr;
		break;
	}
	case MD_BLOCK_H: {
		MD_BLOCK_H_DETAIL *HDetail = (MD_BLOCK_H_DETAIL *)Detail;
		NumArguments = 3;
		Arguments[1].Val = $h;
		Arguments[2].Val = Std$Integer$new_small(HDetail->level);
		break;
	}
	case MD_BLOCK_CODE: {
		MD_BLOCK_CODE_DETAIL *CodeDetail = (MD_BLOCK_CODE_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $code;
		Arguments[2].Val = attributes_to_string(&CodeDetail->info);
		Arguments[3].Val = attributes_to_string(&CodeDetail->lang);
		break;
	}
	case MD_BLOCK_HTML: {
		Arguments[1].Val = $html;
		break;
	}
	case MD_BLOCK_P: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TABLE: {
		Arguments[1].Val = $table;
		break;
	}
	case MD_BLOCK_THEAD: {
		Arguments[1].Val = $thead;
		break;
	}
	case MD_BLOCK_TBODY: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TR: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TH: {
		NumArguments = 3;
		Arguments[1].Val = $th;
		switch (((MD_BLOCK_TD_DETAIL *)Detail)->align) {
		case MD_ALIGN_DEFAULT: Arguments[2].Val = $default; break;
		case MD_ALIGN_LEFT: Arguments[2].Val = $left; break;
		case MD_ALIGN_CENTER: Arguments[2].Val = $center; break;
		case MD_ALIGN_RIGHT: Arguments[2].Val = $right; break;
		}
		break;
	}
	case MD_BLOCK_TD: {
		NumArguments = 3;
		Arguments[1].Val = $td;
		switch (((MD_BLOCK_TD_DETAIL *)Detail)->align) {
		case MD_ALIGN_DEFAULT: Arguments[2].Val = $default; break;
		case MD_ALIGN_LEFT: Arguments[2].Val = $left; break;
		case MD_ALIGN_CENTER: Arguments[2].Val = $center; break;
		case MD_ALIGN_RIGHT: Arguments[2].Val = $right; break;
		}
		break;
	}
	}
	if (Std$Function$invoke(Parser->EnterBlockFn, NumArguments, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}

static int leave_block_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	Std$Function$result Result[1];
	Std$Function$argument Arguments[5] = {0,};
	int NumArguments = 2;
	Arguments[0].Val = Parser->UserData;
	Arguments[0].Ref = &Parser->UserData;
	switch (BlockType) {
	case MD_BLOCK_DOC: {
		Arguments[1].Val = $doc;
		break;
	}
	case MD_BLOCK_QUOTE: {
		Arguments[1].Val = $quote;
		break;
	}
	case MD_BLOCK_UL: {
		MD_BLOCK_UL_DETAIL *ULDetail = (MD_BLOCK_UL_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $ul;
		Arguments[2].Val = ULDetail->is_tight ? $true : $false;
		Arguments[3].Val = Std$String$new_char(ULDetail->mark);
		break;
	}
	case MD_BLOCK_OL: {
		MD_BLOCK_OL_DETAIL *OLDetail = (MD_BLOCK_OL_DETAIL *)Detail;
		NumArguments = 5;
		Arguments[1].Val = $ol;
		Arguments[2].Val = Std$Integer$new_small(OLDetail->start);
		Arguments[3].Val = OLDetail->is_tight ? $true : $false;
		Arguments[4].Val = Std$String$new_char(OLDetail->mark_delimiter);
		break;
	}
	case MD_BLOCK_LI: {
		Arguments[1].Val = $li;
		break;
	}
	case MD_BLOCK_HR: {
		Arguments[1].Val = $hr;
		break;
	}
	case MD_BLOCK_H: {
		MD_BLOCK_H_DETAIL *HDetail = (MD_BLOCK_H_DETAIL *)Detail;
		NumArguments = 3;
		Arguments[1].Val = $h;
		Arguments[2].Val = Std$Integer$new_small(HDetail->level);
		break;
	}
	case MD_BLOCK_CODE: {
		MD_BLOCK_CODE_DETAIL *CodeDetail = (MD_BLOCK_CODE_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $code;
		Arguments[2].Val = attributes_to_string(&CodeDetail->info);
		Arguments[3].Val = attributes_to_string(&CodeDetail->lang);
		break;
	}
	case MD_BLOCK_HTML: {
		Arguments[1].Val = $html;
		break;
	}
	case MD_BLOCK_P: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TABLE: {
		Arguments[1].Val = $table;
		break;
	}
	case MD_BLOCK_THEAD: {
		Arguments[1].Val = $thead;
		break;
	}
	case MD_BLOCK_TBODY: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TR: {
		Arguments[1].Val = $p;
		break;
	}
	case MD_BLOCK_TH: {
		NumArguments = 3;
		Arguments[1].Val = $th;
		switch (((MD_BLOCK_TD_DETAIL *)Detail)->align) {
		case MD_ALIGN_DEFAULT: Arguments[2].Val = $default; break;
		case MD_ALIGN_LEFT: Arguments[2].Val = $left; break;
		case MD_ALIGN_CENTER: Arguments[2].Val = $center; break;
		case MD_ALIGN_RIGHT: Arguments[2].Val = $right; break;
		}
		break;
	}
	case MD_BLOCK_TD: {
		NumArguments = 3;
		Arguments[1].Val = $td;
		switch (((MD_BLOCK_TD_DETAIL *)Detail)->align) {
		case MD_ALIGN_DEFAULT: Arguments[2].Val = $default; break;
		case MD_ALIGN_LEFT: Arguments[2].Val = $left; break;
		case MD_ALIGN_CENTER: Arguments[2].Val = $center; break;
		case MD_ALIGN_RIGHT: Arguments[2].Val = $right; break;
		}
		break;
	}
	}
	if (Std$Function$invoke(Parser->LeaveBlockFn, NumArguments, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}

SYMBOL($em, "em");
SYMBOL($strong, "strong");
SYMBOL($a, "a");
SYMBOL($img, "img");
SYMBOL($del, "del");

static int enter_span_fn(MD_SPANTYPE SpanType, void *Detail, parser_t *Parser) {
	Std$Function$result Result[1];
	Std$Function$argument Arguments[5];
	Arguments[0].Val = Parser->UserData;
	Arguments[0].Ref = &Parser->UserData;
	int NumArguments = 2;
	switch (SpanType) {
	case MD_SPAN_EM: {
		Arguments[1].Val = $em;
		break;
	}
	case MD_SPAN_STRONG: {
		Arguments[1].Val = $strong;
		break;
	}
	case MD_SPAN_A: {
		MD_SPAN_A_DETAIL *ADetail = (MD_SPAN_A_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $a;
		Arguments[2].Val = attributes_to_string(&ADetail->href);
		Arguments[3].Val = attributes_to_string(&ADetail->title);
		break;
	}
	case MD_SPAN_IMG: {
		MD_SPAN_IMG_DETAIL *ImgDetail = (MD_SPAN_IMG_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $img;
		Arguments[2].Val = attributes_to_string(&ImgDetail->src);
		Arguments[3].Val = attributes_to_string(&ImgDetail->title);
		break;
	}
	case MD_SPAN_CODE: {
		Arguments[1].Val = $code;
		break;
	}
	case MD_SPAN_DEL: {
		Arguments[1].Val = $del;
		break;
	}
	}
	if (Std$Function$invoke(Parser->EnterSpanFn, NumArguments, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}

static int leave_span_fn(MD_SPANTYPE SpanType, void *Detail, parser_t *Parser) {
	Std$Function$result Result[1];
	Std$Function$argument Arguments[5];
	Arguments[0].Val = Parser->UserData;
	Arguments[0].Ref = &Parser->UserData;
	int NumArguments = 2;
	switch (SpanType) {
	case MD_SPAN_EM: {
		Arguments[1].Val = $em;
		break;
	}
	case MD_SPAN_STRONG: {
		Arguments[1].Val = $strong;
		break;
	}
	case MD_SPAN_A: {
		MD_SPAN_A_DETAIL *ADetail = (MD_SPAN_A_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $a;
		Arguments[2].Val = attributes_to_string(&ADetail->href);
		Arguments[3].Val = attributes_to_string(&ADetail->title);
		break;
	}
	case MD_SPAN_IMG: {
		MD_SPAN_IMG_DETAIL *ImgDetail = (MD_SPAN_IMG_DETAIL *)Detail;
		NumArguments = 4;
		Arguments[1].Val = $img;
		Arguments[2].Val = attributes_to_string(&ImgDetail->src);
		Arguments[3].Val = attributes_to_string(&ImgDetail->title);
		break;
	}
	case MD_SPAN_CODE: {
		Arguments[1].Val = $code;
		break;
	}
	case MD_SPAN_DEL: {
		Arguments[1].Val = $del;
		break;
	}
	}
	if (Std$Function$invoke(Parser->LeaveSpanFn, NumArguments, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}

SYMBOL($text, "text");
STRING(NullChar, "\xEF\xBF\xBD");
STRING(SoftBr, "\n");
SYMBOL($br, "br");

static int text_fn(MD_TEXTTYPE TextType, const char *Text, int Size, parser_t *Parser) {
	Std$Function$result Result[1];
	Std$Function$argument Arguments[5] = {0,};
	Arguments[0].Val = Parser->UserData;
	Arguments[0].Ref = &Parser->UserData;
	int NumArguments = 2;
	switch (TextType) {
	case MD_TEXT_NORMAL: {
		NumArguments = 3;
		Arguments[1].Val = $text;
		Arguments[2].Val = Std$String$copy_length(Text, Size);
		break;
	}
	case MD_TEXT_NULLCHAR: {
		NumArguments = 3;
		Arguments[1].Val = $text;
		Arguments[2].Val = NullChar;
		break;
	}
	case MD_TEXT_BR: {
		Arguments[1].Val = $br;
		break;
	}
	case MD_TEXT_SOFTBR: {
		NumArguments = 3;
		Arguments[1].Val = $text;
		Arguments[2].Val = SoftBr;
		break;
	}
	case MD_TEXT_ENTITY: {
		NumArguments = 3;
		Arguments[1].Val = $text;
		Arguments[2].Val = Agg$StringTable$get(Html$Entities$ByName, Text, Size);
		break;
	}
	case MD_TEXT_CODE: {
		NumArguments = 3;
		Arguments[1].Val = $code;
		Arguments[2].Val = Std$String$copy_length(Text, Size);
		break;
	}
	case MD_TEXT_HTML: {
		NumArguments = 3;
		Arguments[1].Val = $html;
		Arguments[2].Val = Std$String$copy_length(Text, Size);
		break;
	}
	}
	if (Std$Function$invoke(Parser->TextFn, NumArguments, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}


GLOBAL_FUNCTION(New, 1) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
	Parser->UserData = Std$Object$Nil;
	Parser->EnterBlockFn = Std$Object$Nil;
	Parser->LeaveBlockFn = Std$Object$Nil;
	Parser->EnterSpanFn = Std$Object$Nil;
	Parser->LeaveSpanFn = Std$Object$Nil;
	Parser->TextFn = Std$Object$Nil;
	Parser->Renderer->enter_block = (void *)enter_block_fn;
	Parser->Renderer->leave_block = (void *)leave_block_fn;
	Parser->Renderer->enter_span = (void *)enter_span_fn;
	Parser->Renderer->leave_span = (void *)leave_span_fn;
	Parser->Renderer->text = (void *)text_fn;
	Parser->Renderer->flags = MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH;
	Result->Val = (Std$Object$t *)Parser;
	return SUCCESS;
}

METHOD("parse", TYP, T, TYP, Std$String$T) {
	parser_t *Parser = Args[0].Val;
	Std$String$t *String = Args[1].Val;
	Std$Object$t *Message = setjmp(Parser->OnError);
	if (Message) {
		Result->Val = Message;
		return MESSAGE;
	}
	int Status = md_parse(Std$String$flatten(String), String->Length.Value, Parser->Renderer, Parser);
	if (Status == -1) {
		Result->Val = Std$String$new("Markdown error");
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

TYPED_INSTANCE(int, IO$Stream$write, T, parser_t *Parser, const char *Source, int Length, int Block) {
	int Status = md_parse(Source, Length, Parser->Renderer, Parser);
	if (Status == -1) return -1;
	return Length;
}

METHOD("enter_block", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->EnterBlockFn);
	return SUCCESS;
}

METHOD("leave_block", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->LeaveBlockFn);
	return SUCCESS;
}

METHOD("enter_span", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->EnterSpanFn);
	return SUCCESS;
}

METHOD("leave_span", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->LeaveSpanFn);
	return SUCCESS;
}

METHOD("text", TYP, T) {
	parser_t *Parser = Args[0].Val;
	Result->Val = *(Result->Ref = &Parser->TextFn);
	return SUCCESS;
}
