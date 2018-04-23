#include <Gir/GObject/Riva.h>
#include <Gir/GObject/Object.h>
#include <Gir/GObject/Type.h>
#include <Gir/GObject/Closure.h>
#include <Gir/GObject/Value.h>
#include <Gir/GObject/Interface.h>
#include <Sys/Module.h>
#include <Riva.h>

static GType InterfaceGType;

ASYMBOL($RIVA);

TYPEF(T, ($RIVA));

GLOBAL_FUNCTION(New, 1) {
//@name : Std$String$T
//@modules... : Sys$Module$T
//:Std$Type$T
// Creates a new type which implements specified GObject interfaces.
// <var>modules...</var> should be the modules which correspond to the required interfaces.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	GType ImplementationGType = g_type_register_static_simple(
		InterfaceGType,
		Std$String$flatten(Args[0].Val),
		sizeof(GObjectClass),
		(GClassInitFunc)0,
		sizeof(Gir$GObject$Interface$t),
		(GInstanceInitFunc)0,
		0
	);
	Std$Type$t **Types = (Std$Type$t **)Riva$Memory$alloc((Count + 2) * sizeof(Std$Type$t *));
	unsigned long *Levels = (unsigned long *)Riva$Memory$alloc((Count + 2) * sizeof(unsigned long));
	for (int I = 1; I < Count; ++I) {
		if (Args[I].Val->Type != Sys$Module$T) {
			Result->Val = Std$String$new("Parameter is not a module");
			return MESSAGE;
		}
		Gir$GObject$Interface_infot *Interface;
		int Temp;
		if (!Riva$Module$import(Args[I].Val, "InterfaceInfo", &Temp, &Interface)) {
			Result->Val = Std$String$new("Module does not specify an interface");
			return MESSAGE;
		};
		Types[I] = Interface->Riva;
		Levels[I + 1] = I;
		g_type_add_interface_static(ImplementationGType, Interface->Type, &Interface->Info);
	};
	
	Std$Type$t *ImplementationRivaType = new(Std$Type$t);
	Types[0] = ImplementationRivaType;
	Types[Count] = Gir$GObject$Object$T;
	Types[Count + 1] = 0;
	ImplementationRivaType->Type = Std$Type$T;
	ImplementationRivaType->Types = Types;
	ImplementationRivaType->Invoke = Std$Type$default_invoke,
	ImplementationRivaType->Fields = Std$Array$new(0);
	Levels[0] = Count + 1;
	Levels[1] = 0;
	Levels[Count + 1] = Count;
	ImplementationRivaType->Levels = Levels;
	
	Gir$GObject$Type$register_type(ImplementationGType, ImplementationRivaType);
	
	Gir$GObject$Type$t *Type = new(Gir$GObject$Type$t);
	Type->Type = T;
	Type->Value = ImplementationGType;
	Result->Val = Type;
	return SUCCESS;
};

GLOBAL_FUNCTION(Implement, 2) {
	CHECK_EXACT_ARG_TYPE(0, T);
	Gir$GObject$Type$t *Type = Args[0].Val;
	Gir$GObject$Interface$t *Implementation = g_object_new(Type->Value, 0);
	Implementation->Extra = Args[1].Val;
	switch (Std$Function$call($RIVA, 1, Result, Args[1].Val, 0)) {
	case SUSPEND: case SUCCESS:
		Result->Ref[0] = Implementation;
		break;
	case FAILURE:
		return FAILURE;
	case MESSAGE:
		return MESSAGE;
	};
	Result->Val = Gir$GObject$Riva$object_to_riva(Implementation);
	Result->Ref = 0;
	return SUCCESS;
};

GObject *_implementation(Std$Object$t *Object) {
	printf("_implementation\n");
	Std$Function_result Result;
	Std$Function$call($RIVA, 1, &Result, Object, 0);
	return ((Gir$GObject$Object$t *)Result.Val)->Handle;
};

INITIAL() {
	InterfaceGType = g_type_register_static_simple(
		G_TYPE_OBJECT,
		"RivaInterface",
		sizeof(GObjectClass),
		(GClassInitFunc)0,
		sizeof(Gir$GObject$Interface$t),
		(GInstanceInitFunc)0,
		0
	);
};
