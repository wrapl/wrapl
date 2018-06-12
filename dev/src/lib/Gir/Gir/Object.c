#include <Gtk/GObject/Init.h>
#include <Gtk/GObject/Object.h>
#include <Gtk/GObject/Type.h>
#include <Gtk/GObject/Closure.h>
#include <Gtk/GObject/Value.h>
#include <Util/TypedFunction.h>
#include <Riva.h>

TYPE(T);

Gtk$GObject$Object_t Nil[] = {{T, 0}};

static void finalize(Gtk$GObject$Object_t *Object, void *Data) {
	g_object_unref(Object->Handle);
};

static GQuark RivaQuark;

Gtk$GObject$Object_t *_new(GObject *Handle, Std$Type_t *Type) {
	Gtk$GObject$Object_t *Object = new(Gtk$GObject$Object_t);
	Object->Type = Type;
	Object->Handle = Handle;
	Object->Extra = Std$Object$Nil;
	g_object_set_qdata(Handle, RivaQuark, Object);	
//	g_object_ref_sink(Handle);
//	Riva$Memory$register_finalizer(Object, finalize, 0, 0, 0);
	return Object;
};

Gtk$GObject$Object_t *_to_riva(GObject *Handle) {
	if (Handle == 0) return Nil;
	Gtk$GObject$Object_t *Object = g_object_get_qdata(Handle, RivaQuark);
	if (Object) return Object;
	Std$Type_t *Type = Gtk$GObject$Type$to_riva(G_OBJECT_TYPE(Handle));
	if (Type == 0) return 0;
	return _new(Handle, Type);
};

TYPED_INSTANCE(void, Gtk$GObject$Value$to_value, T, Gtk$GObject$Object_t const *Source, GValue *Dest) {
	void *Object = Source->Handle;
	g_value_init(Dest, G_OBJECT_TYPE(Object));
	g_value_set_object(Dest, Object);
};

METHOD("data", TYP, T) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	Result->Val = *(Result->Ref = &Object->Extra);
	return SUCCESS;
};

METHOD("Connect", TYP, T, TYP, Std$String$T, TYP, Gtk$GObject$Closure$T) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	const char *Signal = Std$String$flatten(Args[1].Val);
	Gtk$GObject$Closure_t *Closure = (Gtk$GObject$Closure_t *)Args[2].Val;
	gboolean After = (Count > 3) && (Args[3].Val == $true);
	gulong ID = g_signal_connect_closure(Object->Handle, Signal, Closure->Handle, After);
	Result->Val = Std$Integer$new_small(ID);
	return SUCCESS;
};

METHOD("Connect", TYP, T, TYP, Std$String$T, TYP, Std$Function$T) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	const char *Signal = Std$String$flatten(Args[1].Val);
	Gtk$GObject$Closure_t *Closure = Gtk$GObject$Closure$from_val(Args[2].Val);
	gboolean After = (Count > 3) && (Args[3].Val == $true);
	gulong ID = g_signal_connect_closure(Object->Handle, Signal, Closure->Handle, After);
	Result->Val = Std$Integer$new_small(ID);
	return SUCCESS;
};

METHOD("Disconnect", TYP, T, TYP, Std$Integer$SmallT) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	gulong ID = ((Std$Integer_smallt *)Args[1].Val)->Value;
	g_signal_handler_disconnect(Object->Handle, ID);
	return SUCCESS;
};

METHOD("GetProperty", TYP, T, TYP, Std$String$T) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	GObjectClass *Class = G_OBJECT_GET_CLASS(Object);
	const char *Prop = Std$String$flatten(Args[1].Val);
	GValue Value[1] = {{0,}};
	GParamSpec *Spec = g_object_class_find_property(Class, Prop);
	g_value_init(Value, Spec->value_type);
	g_object_get_property(Object, Prop, Value);
	Result->Val = Gtk$GObject$Value$to_riva(Value);
	return SUCCESS;
};

METHOD("SetProperty", TYP, T, TYP, Std$String$T, ANY) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GValue Value[1] = {{0,}};
	Gtk$GObject$Value$to_gtk(Args[2].Val, Value);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("SetPropertyBoxed", TYP, T, TYP, Std$String$T, TYP, Gtk$GObject$Type$T, ANY) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GType *Type = ((Gtk$GObject$Type_t *)Args[2].Val)->Value;
	void *Address = ((Std$Address_t *)Args[3].Val)->Value;
	GValue Value[1] = {{0,}};
	g_value_init(Value, Type);
	g_value_set_boxed(Value, Address);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("SetPropertyEnum", TYP, T, TYP, Std$String$T, TYP, Gtk$GObject$Type$T, TYP, Std$Integer$SmallT) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GType *Type = ((Gtk$GObject$Type_t *)Args[2].Val)->Value;
	int Int = ((Std$Integer_smallt *)Args[3].Val)->Value;
	GValue Value[1] = {{0,}};
	g_value_init(Value, Type);
	g_value_set_enum(Value, Int);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("Get", TYP, T, TYP, Std$String$T) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	GObjectClass *Class = G_OBJECT_GET_CLASS(Object);
	const char *Prop = Std$String$flatten(Args[1].Val);
	GValue Value[1] = {{0,}};
	GParamSpec *Spec = g_object_class_find_property(Class, Prop);
	g_value_init(Value, Spec->value_type);
	g_object_get_property(Object, Prop, Value);
	Result->Val = Gtk$GObject$Value$to_riva(Value);
	return SUCCESS;
};

METHOD("Set", TYP, T, TYP, Std$String$T, ANY) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GValue Value[1] = {{0,}};
	Gtk$GObject$Value$to_gtk(Args[2].Val, Value);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("SetBoxed", TYP, T, TYP, Std$String$T, TYP, Gtk$GObject$Type$T, ANY) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GType *Type = ((Gtk$GObject$Type_t *)Args[2].Val)->Value;
	void *Address = ((Std$Address_t *)Args[3].Val)->Value;
	GValue Value[1] = {{0,}};
	g_value_init(Value, Type);
	g_value_set_boxed(Value, Address);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("SetEnum", TYP, T, TYP, Std$String$T, TYP, Gtk$GObject$Type$T, TYP, Std$Integer$SmallT) {
	GObject *Object = ((Gtk$GObject$Object_t *)Args[0].Val)->Handle;
	const char *Prop = Std$String$flatten(Args[1].Val);
	GType *Type = ((Gtk$GObject$Type_t *)Args[2].Val)->Value;
	int Int = ((Std$Integer_smallt *)Args[3].Val)->Value;
	GValue Value[1] = {{0,}};
	g_value_init(Value, Type);
	g_value_set_enum(Value, Int);
	g_object_set_property(Object, Prop, Value);
	return SUCCESS;
};

METHOD("GetData", TYP, T, TYP, Std$String$T) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	const char *Prop = Std$String$flatten(Args[1].Val);
	if (Result->Val = g_object_get_data(Object->Handle, Prop)) {
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("SetData", TYP, T, TYP, Std$String$T, ANY) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	const char *Prop = Std$String$flatten(Args[1].Val);
	g_object_set_data(Object->Handle, Prop, Args[2].Val);
	return SUCCESS;
};

METHOD("GetType", TYP, T) {
	Gtk$GObject$Object_t *Object = (Gtk$GObject$Object_t *)Args[0].Val;
	Gtk$GObject$Type_t *Type = new(Gtk$GObject$Type_t);
	Type->Type = Gtk$GObject$Type$T;
	Type->Value = G_OBJECT_TYPE(Object->Handle);
	Result->Val = (Std$Object_t *)Type;
	return SUCCESS;
};

METHOD("=", TYP, T, TYP, T) {
    Gtk$GObject$Object_t *A = (Gtk$GObject$Object_t *)Args[0].Val;
    Gtk$GObject$Object_t *B = (Gtk$GObject$Object_t *)Args[1].Val;
    if (A->Handle == B->Handle) {
        Result->Arg = Args[1];
        return SUCCESS;
    } else {
        return FAILURE;
    };
};

METHOD("~=", TYP, T, TYP, T) {
    Gtk$GObject$Object_t *A = (Gtk$GObject$Object_t *)Args[0].Val;
    Gtk$GObject$Object_t *B = (Gtk$GObject$Object_t *)Args[1].Val;
    if (A->Handle != B->Handle) {
        Result->Arg = Args[1];
        return SUCCESS;
    } else {
        return FAILURE;
    };
};

INITIAL() {
	RivaQuark = g_quark_from_static_string("<<riva>>");
};
