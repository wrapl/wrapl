#include <Std.h>
#include <Riva.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <gumbo.h>

typedef struct gumbo_t {
	const Std$Type$t *Type;
	GumboOutput *Handle;
} gumbo_t;

typedef struct gumbo_node_t {
	const Std$Type$t *Type;
	GumboNode *Handle;
} gumbo_node_t;

typedef struct gumbo_vector_t {
	const Std$Type$t *Type;
	GumboVector *Handle;
} gumbo_vector_t;

TYPE(T);

TYPE(NodeT);
TYPE(DocumentNodeT, NodeT);
TYPE(ElementNodeT, NodeT);
TYPE(TextNodeT, NodeT);
TYPE(CDataNodeT, TextNodeT, NodeT);
TYPE(CommentNodeT, TextNodeT, NodeT);
TYPE(WhitespaceNodeT, TextNodeT, NodeT);
TYPE(TemplateNodeT, ElementNodeT, NodeT);

TYPE(VectorT);

static Std$Object$t *Tags[GUMBO_TAG_LAST];

static void *riva_gumbo_allocator(void *User, size_t Size) {
	return Riva$Memory$alloc(Size);
}

static void riva_gumbo_deallocator(void *User, void *Ptr) {
}

GLOBAL_FUNCTION(Parse, 1) {
//@html:Std.String.T
//:T
// Parses <var>html</var> and returns a new html tree.
	CHECK_ARG_TYPE(0, Std$String$T);
	static GumboOptions Options = {
		riva_gumbo_allocator,
		riva_gumbo_deallocator,
		0,
		4,
		false,
		-1,
		GUMBO_TAG_LAST,
		GUMBO_NAMESPACE_HTML
	};
	gumbo_t *Gumbo = new(gumbo_t);
	Gumbo->Type = T;
	Gumbo->Handle = gumbo_parse_with_options(&Options, Std$String$flatten(Args[0].Val), Std$String$get_length(Args[0].Val));
	Result->Val = (Std$Object$t *)Gumbo;
	return SUCCESS;
}

METHOD("document", TYP, T) {
//@t
//:NodeT
	gumbo_t *Gumbo = (gumbo_t *)Args[0].Val;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Gumbo->Handle->document;
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
}

METHOD("root", TYP, T) {
//@t
//:NodeT
	gumbo_t *Gumbo = (gumbo_t *)Args[0].Val;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Gumbo->Handle->root;
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
}

METHOD("parent", TYP, NodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	gumbo_node_t *Parent = new(gumbo_node_t);
	Parent->Handle = Node->Handle->parent;
	switch (Parent->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Parent->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Parent->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Parent->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Parent->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Parent->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Parent->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Parent->Type = TemplateNodeT; break;
	default: Parent->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Parent;
	return SUCCESS;
}

METHOD("index", TYP, NodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Node->Handle->index_within_parent + 1);
	return SUCCESS;
}

AMETHOD(Std$String$Of, TYP, TextNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$String$new(Node->Handle->v.text.text);
	return SUCCESS;
}

METHOD("text", TYP, TextNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$String$new(Node->Handle->v.text.text);
	return SUCCESS;
}

METHOD("text", TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	int Length = 0;
	for (int I = 0; I < Node->Handle->v.element.children.length; ++I) {
		GumboNode *Child = Node->Handle->v.element.children.data[I];
		switch (Child->type) {
		case GUMBO_NODE_TEXT:
		case GUMBO_NODE_CDATA:
			Length += strlen(Child->v.text.text);
			break;
		}
	}
	char *Buffer = Riva$Memory$alloc_atomic(Length + 1);
	char *End = Buffer;
	for (int I = 0; I < Node->Handle->v.element.children.length; ++I) {
		GumboNode *Child = Node->Handle->v.element.children.data[I];
		switch (Child->type) {
		case GUMBO_NODE_TEXT:
		case GUMBO_NODE_CDATA:
			End = stpcpy(End, Child->v.text.text);
			break;
		}
	}
	*End = 0;
	Result->Val = Std$String$new_length(Buffer, Length);
	return SUCCESS;
}

AMETHOD(Std$String$Of, TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	const char *Tag = gumbo_normalized_tagname(Node->Handle->v.element.tag);
	GumboAttribute **Attributes = (GumboAttribute **)Node->Handle->v.element.attributes.data;
	int Length = 1 + strlen(Tag);
	char CountBuffer[12];
	int CountLength = sprintf(CountBuffer, "%d", Node->Handle->v.element.children.length);
	for (int I = 0; I < Node->Handle->v.element.attributes.length; ++I) {
		GumboAttribute *Attribute = Attributes[I];
		Length += 1 + strlen(Attribute->name);
		Length += 2 + strlen(Attribute->value);
		Length += 1;
	}
	Length += 3 + CountLength;
	char *Buffer = Riva$Memory$alloc_atomic(Length + 1);
	char *End = Buffer;
	*End++ = '<';
	End = stpcpy(End, Tag);
	for (int I = 0; I < Node->Handle->v.element.attributes.length; ++I) {
		GumboAttribute *Attribute = Attributes[I];
		*End++ = ' ';
		End = stpcpy(End, Attribute->name);
		*End++ = '=';
		*End++ = '\"';
		End = stpcpy(End, Attribute->value);
		*End++ = '\"';
	}
	*End++ = '>';
	*End++ = '[';
	End = stpcpy(End, CountBuffer);
	*End++ = ']';
	*End = 0;
	Result->Val = Std$String$new_length(Buffer, Length);
	return SUCCESS;
}

static int write_node(GumboNode *Node, IO$Stream$t *Stream, IO$Stream$writefn write) {
	switch (Node->type) {
	case GUMBO_NODE_DOCUMENT: {
		break;
	}
	case GUMBO_NODE_ELEMENT: {
		write(Stream, "<", 1, 1);
		const char *TagName = gumbo_normalized_tagname(Node->v.element.tag);
		write(Stream, TagName, strlen(TagName), 1);
		for (int I = 0; I < Node->v.element.attributes.length; ++I) {
			GumboAttribute *Attribute = Node->v.element.attributes.data[I];
			write(Stream, " ", 1, 1);
			write(Stream, Attribute->original_name.data, Attribute->original_name.length, 1);
			write(Stream, "=", 1, 1);
			write(Stream, Attribute->original_value.data, Attribute->original_value.length, 1);
		}
		if (Node->v.element.children.length) {
			write(Stream, ">", 1, 1);
			for (int I = 0; I < Node->v.element.children.length; ++I) {
				write_node(Node->v.element.children.data[I], Stream, write);
			}
			write(Stream, "</", 2, 1);
			write(Stream, TagName, strlen(TagName), 1);
			write(Stream, ">", 1, 1);
		} else {
			write(Stream, "/>", 2, 1);
		}
		break;
	}
	case GUMBO_NODE_TEXT:
	case GUMBO_NODE_CDATA:
	case GUMBO_NODE_WHITESPACE: {
		write(Stream, Node->v.text.original_text.data, Node->v.text.original_text.length, 1);
		break;
	}
	case GUMBO_NODE_COMMENT: {
		write(Stream, "<!--", 4, 1);
		write(Stream, Node->v.text.original_text.data, Node->v.text.original_text.length, 1);
		write(Stream, "-->", 3, 1);
		break;
	}
	case GUMBO_NODE_TEMPLATE: {
		break;
	}
	}
	return 0;
}

METHOD("write", TYP, IO$Stream$T, TYP, NodeT) {
	IO$Stream$t *Stream = Args[0].Val;
	IO$Stream$writefn write = Util$TypedFunction$get(IO$Stream$write, Stream->Type);
	gumbo_node_t *Node = (gumbo_node_t *)Args[1].Val;
	write_node(Node->Handle, Stream, write);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("tag", TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Tags[Node->Handle->v.element.tag];
	return SUCCESS;
}

METHOD("[]", TYP, ElementNodeT, TYP, Std$String$T) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	const char *Name = Std$String$flatten(Args[1].Val);
	GumboAttribute *Attribute = gumbo_get_attribute(&Node->Handle->v.element.attributes, Name);
	if (Attribute) {
		Result->Val = Std$String$new(Attribute->value);
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

METHOD(".", TYP, ElementNodeT, TYP, Std$String$T) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	const char *Name = Std$String$flatten(Args[1].Val);
	GumboAttribute *Attribute = gumbo_get_attribute(&Node->Handle->v.element.attributes, Name);
	if (Attribute) {
		Result->Val = Std$String$new(Attribute->value);
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

METHOD("children", TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	gumbo_vector_t *Vector = new(gumbo_vector_t);
	Vector->Type = VectorT;
	Vector->Handle = &Node->Handle->v.element.children;
	Result->Val = (Std$Object$t *)Vector;
	return SUCCESS;
}

METHOD("length", TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Node->Handle->v.element.children.length);
	return SUCCESS;
}

METHOD("children", TYP, DocumentNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	gumbo_vector_t *Vector = new(gumbo_vector_t);
	Vector->Type = VectorT;
	Vector->Handle = &Node->Handle->v.document.children;
	Result->Val = (Std$Object$t *)Vector;
	return SUCCESS;
}

METHOD("length", TYP, VectorT) {
	gumbo_vector_t *Vector = (gumbo_vector_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Vector->Handle->length);
	return SUCCESS;
}

METHOD("[]", TYP, VectorT, TYP, Std$Integer$SmallT) {
	gumbo_vector_t *Vector = (gumbo_vector_t *)Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	if (Index <= 0) Index += Vector->Handle->length + 1;
	if (Index <= 0 || Index > Vector->Handle->length) return FAILURE;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Vector->Handle->data[Index - 1];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
}

typedef struct vector_values_generator {
	Std$Function$cstate State;
	GumboVector *Vector;
	int Index;
} vector_values_generator;

static Std$Function$status resume_vector_values(Std$Function$result *Result) {
	vector_values_generator *Generator = (vector_values_generator *)Result->State;
	int Index = Generator->Index;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Generator->Vector->data[Index];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	++Index;
	Result->Val = (Std$Object$t *)Node;
	Result->Ref = 0;
	if (Index < Generator->Vector->length) {
		Generator->Index = Index;
		return SUSPEND;
	} else {
		return SUCCESS;
	}
}

METHOD("values", TYP, VectorT) {
//@vector
//:ANY
	gumbo_vector_t *Vector = (gumbo_vector_t *)Args[0].Val;
	if (Vector->Handle->length == 0) return FAILURE;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Vector->Handle->data[0];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	if (Vector->Handle->length == 1) return SUCCESS;
	vector_values_generator *Generator = new(vector_values_generator);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = (Std$Function$cresumefn)resume_vector_values;
	Generator->Index = 1;
	Generator->Vector = Vector->Handle;
	Result->State = Generator;
	return SUSPEND;
}

METHOD("[]", TYP, ElementNodeT, TYP, Std$Integer$SmallT) {
	gumbo_node_t *Parent = (gumbo_node_t *)Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	if (Index <= 0) Index += Parent->Handle->v.element.children.length + 1;
	if (Index <= 0 || Index > Parent->Handle->v.element.children.length) return FAILURE;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Parent->Handle->v.element.children.data[Index - 1];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
}

METHOD("values", TYP, ElementNodeT) {
//@vector
//:ANY
	gumbo_node_t *Element = (gumbo_node_t *)Args[0].Val;
	if (Element->Handle->v.element.children.length == 0) return FAILURE;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Element->Handle->v.element.children.data[0];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	if (Element->Handle->v.element.children.length == 1) return SUCCESS;
	vector_values_generator *Generator = new(vector_values_generator);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = (Std$Function$cresumefn)resume_vector_values;
	Generator->Index = 1;
	Generator->Vector = &Element->Handle->v.element.children;
	Result->State = Generator;
	return SUSPEND;
}

typedef struct find_node_t find_node_t;

struct find_node_t {
	find_node_t *Next;
	GumboVector *Vector;
	int Index;
	Std$Object$t *Info;
};

typedef struct element_find_generator {
	Std$Function$cstate State;
	find_node_t *FindNode;
	Std$Object$t *Filter;
} element_find_generator;

static Std$Function$status resume_element_find(Std$Function$result *Result) {
	element_find_generator *Generator = (element_find_generator *)Result->State;
	find_node_t *FindNode = Generator->FindNode;
	for (;;) {
		if (!FindNode) return FAILURE;
		Std$Object$t *InfoVal = FindNode->Info;
		Std$Object$t **InfoRef = &InfoVal;
		int Index = FindNode->Index;
		gumbo_node_t *Node = new(gumbo_node_t);
		Node->Handle = FindNode->Vector->data[Index];
		if (++Index == FindNode->Vector->length) {
			FindNode = FindNode->Next;
		} else {
			FindNode->Index = Index;
		}
		switch (Node->Handle->type) {
		case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
		case GUMBO_NODE_ELEMENT: {
			Node->Type = ElementNodeT;
			if (Node->Handle->v.element.children.length) {
				find_node_t *NewNode = new(find_node_t);
				NewNode->Next = FindNode;
				NewNode->Index = 0;
				NewNode->Vector = &Node->Handle->v.element.children;
				NewNode->Info = InfoVal;
				InfoRef = &NewNode->Info;
				FindNode = NewNode;
			}
			break;
		}
		case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
		case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
		case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
		case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
		case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
		default: Node->Type = NodeT; break;
		}
		Std$Function$result Result0;
		switch (Std$Function$call(Generator->Filter, 2, &Result0, Node, 0, InfoVal, InfoRef)) {
		case SUSPEND: case SUCCESS:
			Result->Arg = Result0.Arg;
			Generator->FindNode = FindNode;
			return SUSPEND;
		case FAILURE:
			break;
		case MESSAGE:
			return MESSAGE;
		}
	}
}

METHOD("find", TYP, ElementNodeT, ANY) {
	gumbo_node_t *Element = (gumbo_node_t *)Args[0].Val;
	if (Element->Handle->v.element.children.length == 0) return FAILURE;
	find_node_t *FindNode = new(find_node_t);
	FindNode->Index = 0;
	FindNode->Vector = &Element->Handle->v.element.children;
	Std$Object$t *Filter;
	if (Count > 2) {
		FindNode->Info = Args[1].Val;
		Filter = Args[2].Val;
	} else {
		FindNode->Info = Std$Object$Nil;
		Filter = Args[1].Val;
	}
	for (;;) {
		if (!FindNode) return FAILURE;
		Std$Object$t *InfoVal = FindNode->Info;
		Std$Object$t **InfoRef = &InfoVal;
		int Index = FindNode->Index;
		gumbo_node_t *Node = new(gumbo_node_t);
		Node->Handle = FindNode->Vector->data[Index];
		if (++Index == FindNode->Vector->length) {
			FindNode = FindNode->Next;
		} else {
			FindNode->Index = Index;
		}
		switch (Node->Handle->type) {
		case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
		case GUMBO_NODE_ELEMENT: {
			Node->Type = ElementNodeT;
			if (Node->Handle->v.element.children.length) {
				find_node_t *NewNode = new(find_node_t);
				NewNode->Next = FindNode;
				NewNode->Index = 0;
				NewNode->Vector = &Node->Handle->v.element.children;
				NewNode->Info = InfoVal;
				InfoRef = &NewNode->Info;
				FindNode = NewNode;
			}
			break;
		}
		case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
		case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
		case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
		case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
		case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
		default: Node->Type = NodeT; break;
		}
		switch (Std$Function$call(Filter, 2, Result, Node, 0, InfoVal, InfoRef)) {
		case SUSPEND: case SUCCESS: {
			element_find_generator *Generator = new(element_find_generator);
			Generator->State.Run = Std$Function$resume_c;
			Generator->State.Invoke = (Std$Function$cresumefn)resume_element_find;
			Generator->FindNode = FindNode;
			Generator->Filter = Filter;
			Result->State = Generator;
			return SUSPEND;
		}
		case FAILURE:
			break;
		case MESSAGE:
			return MESSAGE;
		}

	}
}

METHOD("[]", TYP, DocumentNodeT, TYP, Std$Integer$SmallT) {
	gumbo_node_t *Parent = (gumbo_node_t *)Args[0].Val;
	int Index = Std$Integer$get_small(Args[1].Val);
	if (Index <= 0) Index += Parent->Handle->v.document.children.length + 1;
	if (Index <= 0 || Index > Parent->Handle->v.document.children.length) return FAILURE;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Parent->Handle->v.document.children.data[Index - 1];
	switch (Node->Handle->type) {
	case GUMBO_NODE_DOCUMENT: Node->Type = DocumentNodeT; break;
	case GUMBO_NODE_ELEMENT: Node->Type = ElementNodeT; break;
	case GUMBO_NODE_TEXT: Node->Type = TextNodeT; break;
	case GUMBO_NODE_CDATA: Node->Type = CDataNodeT; break;
	case GUMBO_NODE_COMMENT: Node->Type = CommentNodeT; break;
	case GUMBO_NODE_WHITESPACE: Node->Type = WhitespaceNodeT; break;
	case GUMBO_NODE_TEMPLATE: Node->Type = TemplateNodeT; break;
	default: Node->Type = NodeT; break;
	}
	Result->Val = (Std$Object$t *)Node;
	return SUCCESS;
}

extern int Riva$Symbol[];

INITIAL() {
	for (GumboTag Tag = 0; Tag < GUMBO_TAG_LAST; ++Tag) {
		int Type; void *Value;
		const char *TagName = gumbo_normalized_tagname(Tag);
		Riva$Module$import(Riva$Symbol, TagName, &Type, &Value);
		Tags[Tag] = Value;
	}
}
