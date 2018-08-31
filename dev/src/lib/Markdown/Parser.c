#include <Std.h>
#include <Riva.h>
#include <Html/Entities.h>
#include <setjmp.h>
#include <md4c.h>

typedef struct parser_t {
	const Std$Type$t *Type;
	Std$Object$t *EnterBlockFn, *LeaveBlockFn;
	Std$Object$t *EnterSpanFn, *LeaveSpanFn;
	Std$Object$t *TextFn;
	MD_RENDERER Renderer[1];
	jmp_buf OnError[1];
} parser_t;

TYPE(T);


static int enter_block_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	Std$Function$result Result[1];
	switch (BlockType) {
	case MD_BLOCK_DOC:
	case MD_BLOCK_QUOTE:

	/* <ul>...</ul>
	 * Detail: Structure MD_BLOCK_UL_DETAIL. */
	case MD_BLOCK_UL:

	/* <ol>...</ol>
	 * Detail: Structure MD_BLOCK_OL_DETAIL. */
	case MD_BLOCK_OL:

	/* <li>...</li> */
	case MD_BLOCK_LI:

	/* <hr> */
	case MD_BLOCK_HR:

	/* <h1>...</h1> (for levels up to 6)
	 * Detail: Structure MD_BLOCK_H_DETAIL. */
	case MD_BLOCK_H:

	/* <pre><code>...</code></pre>
	 * Note the text lines within code blocks are terminated with '\n'
	 * instead of explicit MD_TEXT_BR. */
	case MD_BLOCK_CODE: {
		MD_BLOCK_CODE_DETAIL *CodeDetail = (MD_BLOCK_CODE_DETAIL *)Detail;
		for (int I = 0; I < CodeDetail->info.size; ++I) {
			switch (CodeDetail->info.substr_types[I]) {
			case MD_TEXT_NORMAL:
			case MD_TEXT_ENTITY: {
				int Offset = CodeDetail->info.substr_offsets[I];
				int Length = CodeDetail->info.substr_offsets[I + 1] - Offset;
				Std$String$t *Entity = Agg$StringTable$get(Html$Entities$ByName, CodeDetail->info.text + Offset, Length);
				break;
			}
			case MD_TEXT_NULLCHAR:
				break;
			}
		}
		break;
	}

	/* Raw HTML block. This itself does not correspond to any particular HTML
	 * tag. The contents of it _is_ raw HTML source intended to be put
	 * in verbatim form to the HTML output. */
	case MD_BLOCK_HTML:

	/* <p>...</p> */
	case MD_BLOCK_P:

	/* <table>...</table> and its contents.
	 * Detail: Structure MD_BLOCK_TD_DETAIL (used with MD_BLOCK_TH and MD_BLOCK_TD)
	 * Note all of these are used only if extension MD_FLAG_TABLES is enabled. */
	case MD_BLOCK_TABLE:
	case MD_BLOCK_THEAD:
	case MD_BLOCK_TBODY:
	case MD_BLOCK_TR:
	case MD_BLOCK_TH:
	case MD_BLOCK_TD:
		break;
	}
	Std$Function$argument Arguments[] = {

	};
	if (Std$Function$invoke(Parser->EnterBlockFn, 2, Result, Arguments) == MESSAGE) {
		longjmp(Parser->OnError, Result->Val);
	}
	return 0;
}

static int leave_block_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	return 0;
}

static int enter_span_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	return 0;
}

static int leave_span_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	return 0;
}

static int text_fn(MD_BLOCKTYPE BlockType, void *Detail, parser_t *Parser) {
	return 0;
}


GLOBAL_FUNCTION(New, 1) {
	parser_t *Parser = new(parser_t);
	Parser->Type = T;
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
