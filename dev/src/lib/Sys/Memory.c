#include <Std.h>
#include <Riva/Memory.h>

GLOBAL_FUNCTION(Collect, 0) {
// Causes a garbage collection cycle.
	Riva$Memory$collect();
	return SUCCESS;
};

GLOBAL_FUNCTION(Alloc, 1) {
//@size:Std$Integer$SmallT
//:Std$Address$T
// Allocates and returns the address of <var>size</var> bytes of memory in the heap.
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	Result->Val = Std$Address$new(Riva$Memory$alloc(((Std$Integer$smallt *)Args[0].Val)->Value));
	return SUCCESS;
};

GLOBAL_FUNCTION(AllocAtomic, 1) {
//@size:Std$Integer$SmallT
//:Std$Address$T
// Allocates and returns the address of <var>size</var> bytes of pointer free memory in the heap.
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	Result->Val = Std$Address$new(Riva$Memory$alloc_atomic(((Std$Integer$smallt *)Args[0].Val)->Value));
	return SUCCESS;
};

GLOBAL_FUNCTION(AllocUncollectable, 1) {
//@size:Std$Integer$SmallT
//:Std$Address$T
// Allocates and returns the address of <var>size</var> bytes of uncollectable memory in the heap.
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	Result->Val = Std$Address$new(Riva$Memory$alloc_uncollectable(((Std$Integer$smallt *)Args[0].Val)->Value));
	return SUCCESS;
};

static void _finalize(void *Value, Std$Object$t *Finalizer) {
	Std$Address$t Address[1] = {{Std$Address$T, Value}};
	Std$Function$result Result[1];
	Std$Function$call(Finalizer, 1, Result, Address, 0);
};

GLOBAL_FUNCTION(RegisterFinalizer, 2) {
//@address:Std$Address$T
//@finalizer:Std$Function$T
//@data:ANY=NIL
// Attaches a finalizer to <var>address</var>. I.e. <code>finalizer(address, data)</code> should be called at some stage after <var>address</var> becomes unreachable.
	CHECK_EXACT_ARG_TYPE(0, Std$Address$T);
	Riva$Memory$register_finalizer(((Std$Address$t *)Args[0].Val)->Value, _finalize, (char *)Args[1].Val, 0, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

static void _finalize_object(Std$Object$t *Value, Std$Object$t *Finalizer) {
	Std$Function$result Result[1];
	if (Std$Function$call(Finalizer, 1, Result, Value, 0) == MESSAGE) {
		printf("Error in Sys.Module::_finalize_object\n");
	};
};

GLOBAL_FUNCTION(RegisterObjectFinalizer, 2) {
//@object:ANY
//@finalizer:Std$Function$T
//@data:ANY=NIL
// Attaches a finalizer to <var>object</var>. I.e. <code>finalizer(object)</code> should be called at some stage after <var>object</var> becomes unreachable.
	Riva$Memory$register_finalizer_ignore_self(Args[0].Val, _finalize_object, (char *)Args[1].Val, 0, 0);
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_FUNCTION(IsVisible, 1) {
//@address:Std$Address$T
	CHECK_EXACT_ARG_TYPE(0, Std$Address$T);
	if (Riva$Memory$is_visible(((Std$Address$t *)Args[0].Val)->Value)) {
		Result->Arg = Args[0];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

GLOBAL_FUNCTION(GCEnabled, 0) {
	if (!Riva$Memory$gc_disabled()) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
};

typedef struct weakref_t {
	const Std$Type$t *Type;
	Std$Object$t *Object;
} weakref_t;

TYPE(WeakRefT);

GLOBAL_FUNCTION(WeakRefNew, 1) {
//@object:ANY
// Returns a weak reference to <var>object</var>.
	weakref_t *WeakRef = Riva$Memory$alloc_atomic(sizeof(weakref_t));
	WeakRef->Type = WeakRefT;
	WeakRef->Object = Args[0].Val;
	Riva$Memory$register_disappearing_link(&WeakRef->Object, WeakRef->Object);
	Result->Val = WeakRef;
	return SUCCESS;
};

static void *weakref_get(weakref_t *WeakRef) {
	return WeakRef->Object;
};

GLOBAL_METHOD(WeakRefGet, 1, "get", TYP, WeakRefT) {
	Std$Object$t *Object = Riva$Memory$call_with_alloc_lock((Riva$Memory$function)weakref_get, (weakref_t *)Args[0].Val);
	if (Object) {
		Result->Val = Object;
		return SUCCESS;
	} else {
		return FAILURE;
	}
};