#include <Std.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>
#include <girepository.h>

TYPE(StructT);
TYPE(ObjectT);

#define Dst			Dynasm
#define Dst_DECL	struct dasm_State *Dynasm
#define Dst_REF		Dynasm

static void dasm_m_grow(Dst_DECL, void **pp, size_t *szp, int need) {
	size_t sz = *szp;
	if (sz >= need) return;
	if (sz < 16) sz = 16;
	while (sz < need) sz += (sz >> 1);
	*pp = Riva$Memory$realloc(*pp, sz);
	*szp = sz;
};

static void dasm_m_free(Dst_DECL, void *p, size_t sz) {
};

#include "dasm_proto.h"
#include "dasm_x86.h"
#include "GirInternal.c"

typedef struct gir_namespace_t gir_namespace_t;

TYPE(NamespaceT);

static int namespace_import(const char *Namespace, const char *Name, int *Type, void **Value) {
	
};

static int global_import(void *Module, const char *Name, int *Type, void **Value) {
	GError *Error = 0;
	GITypelib *Handle = g_irepository_require(NULL, Name, NULL, 0, &Error);
	if (!Handle) return 0;
	Riva$Module$t *Namespace = Riva$Module$new();
	Riva$Module$set_import_func(Riva$Module$get_default_provider(Module), Riva$Memory$strdup(Name), namespace_import);
	Type[0] = 0;
	Value[0] = Namespace;
	return 1;
};

INIT(Sys$Module$t *Module) {
	Riva$Module$set_import_func(Riva$Module$get_default_provider(Module), 0, global_import);
};
