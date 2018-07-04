#include <Std.h>
#include <Riva.h>
#include <IO/Stream.h>
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

METHOD("@", TYP, TextNodeT, VAL, Std$String$T) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$String$new(Node->Handle->v.text.text);
	return SUCCESS;
}

METHOD("text", TYP, TextNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	Result->Val = Std$String$new(Node->Handle->v.text.text);
	return SUCCESS;
}

METHOD("@", TYP, ElementNodeT, VAL, Std$String$T) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	char *Buffer = Riva$Memory$alloc_atomic(4096);
	char *End = Buffer;
	*End++ = '<';
	GumboTag Tag = Node->Handle->v.element.tag;
	End = stpcpy(End, gumbo_normalized_tagname(Tag));
	GumboAttribute **Attributes = (GumboAttribute **)Node->Handle->v.element.attributes.data;
	for (int I = 0; I < Node->Handle->v.element.attributes.length; ++I) {
		GumboAttribute *Attribute = Attributes[I];
		*End++ = ' ';
		End = stpcpy(End, Attribute->name);
		*End++ = '=';
		*End++ = '\"';
		End = stpcpy(End, Attribute->value);
		*End++ = '\"';
	}
	End += sprintf(End, ">[%d]", Node->Handle->v.element.children.length);
	Result->Val = Std$String$copy_length(Buffer, End - Buffer);
	return SUCCESS;
}

METHOD("tag", TYP, ElementNodeT) {
	gumbo_node_t *Node = (gumbo_node_t *)Args[0].Val;
	GumboTag Tag = Node->Handle->v.element.tag;
	Result->Val = Std$String$new(gumbo_normalized_tagname(Tag));
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
	Std$Function_cstate State;
	GumboVector *Vector;
	int Index;
} vector_values_generator;

typedef struct vector_values_resume_data {
	vector_values_generator *Generator;
	Std$Function_argument Result;
} vector_values_resume_data;

static Std$Function$status resume_vector_values(vector_values_resume_data *Data) {
	int Index = Data->Generator->Index;
	gumbo_node_t *Node = new(gumbo_node_t);
	Node->Handle = Data->Generator->Vector->data[Index];
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
	Data->Result.Val = (Std$Object$t *)Node;
	Data->Result.Ref = 0;
	if (Index < Data->Generator->Vector->length) {
		Data->Generator->Index = Index;
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
	Generator->State.Invoke = (Std$Function_cresumefn)resume_vector_values;
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
	Generator->State.Invoke = (Std$Function_cresumefn)resume_vector_values;
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
};

typedef struct element_find_generator {
	Std$Function_cstate State;
	find_node_t *FindNode;
	Std$Object$t *Filter;
} element_find_generator;

static Std$Function$status resume_element_find(Std$Function$cresumedata *Data) {
	element_find_generator *Generator = (element_find_generator *)Data->State;
	find_node_t *FindNode = Generator->FindNode;
	for (;;) {
		if (!FindNode) return FAILURE;
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
		switch (Std$Function$call(Generator->Filter, 1, &Data->Result, Node, 0)) {
		case SUSPEND: case SUCCESS:
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
	for (;;) {
		if (!FindNode) return FAILURE;
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
		switch (Std$Function$call(Args[1].Val, 1, Result, Node, 0)) {
		case SUSPEND: case SUCCESS: {
			element_find_generator *Generator = new(element_find_generator);
			Generator->State.Run = Std$Function$resume_c;
			Generator->State.Invoke = (Std$Function_cresumefn)resume_element_find;
			Generator->FindNode = FindNode;
			Generator->Filter = Args[1].Val;
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
