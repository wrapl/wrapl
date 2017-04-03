#include <Gtk/GObject/Closure.h>
#include <Gtk/GObject/Object.h>
#include <Gtk/GObject/Value.h>
#include <Riva.h>

#include <stdio.h>

TYPE(T);

SYMBOL(AS, "@");

typedef struct val_closure_t {
	GClosure Parent;
	Std$Object_t *Function;
	Gtk$GObject$Closure_t *Closure;
} val_closure_t;

static void __marshal_val(val_closure_t *Closure, GValue *Result, guint NoOfArgs, const GValue *Args, gpointer Hint, gpointer Data) {
	Std$Function_argument Args0[NoOfArgs];
	for (guint I = 0; I < NoOfArgs; ++I) {
		Args0[I].Val = Gtk$GObject$Value$to_riva(Args + I);
		Args0[I].Ref = 0;
	};
	Std$Function_result Result0;
	switch (Std$Function$invoke(Closure->Function, NoOfArgs, &Result0, Args0)) {
	case MESSAGE:
		if (Std$Function$call(AS, 2, &Result0, Result0.Val, 0, Std$String$T, 0) < FAILURE) {
			printf("Warning: Closure sent message: %s.\n", Std$String$flatten(Result0.Val));
		} else {
			printf("Warning: Closure sent message: <unknown>.\n");
		};
	case FAILURE:
		return;
	case SUSPEND:
	case SUCCESS:
		break;
	};
	if (Result) Gtk$GObject$Value$to_gtk(Result0.Val, Result);
};

Gtk$GObject$Closure_t *_from_val(Std$Object_t *Function) {
	val_closure_t *Handle = (val_closure_t *)g_closure_new_simple(sizeof(val_closure_t), 0);
	Handle->Function = Function;
	g_closure_set_marshal((GClosure *)Handle, (GClosureMarshal)__marshal_val);
	Gtk$GObject$Closure_t *Closure = new(Gtk$GObject$Closure_t);
	Closure->Type = T;
	Closure->Handle = (GClosure *)Handle;
	Handle->Closure = Closure;
	return Closure;
};

typedef struct ref_closure_t {
	GClosure Parent;
	Std$Object_t **Function;
	Gtk$GObject$Closure_t *Closure;
} ref_closure_t;

static void __marshal_ref(ref_closure_t *Closure, GValue *Result, guint NoOfArgs, const GValue *Args, gpointer Hint, gpointer Data) {
	Std$Function_argument Args0[NoOfArgs];
	for (guint I = 0; I < NoOfArgs; ++I) {
		Args0[I].Val = Gtk$GObject$Value$to_riva(Args + I);
		Args0[I].Ref = 0;
	};
	Std$Function_result Result0;
	switch (Std$Function$invoke(Closure->Function[0], NoOfArgs, &Result0, Args0)) {
	case MESSAGE:
		if (Std$Function$call(AS, 2, &Result0, Result0.Val, 0, Std$String$T, 0) < FAILURE) {
			printf("Warning: Closure sent message: %s.\n", Std$String$flatten(Result0.Val));
		} else {
			printf("Warning: Closure sent message: <unknown>.\n");
		};
	case FAILURE:
		return;
	case SUSPEND:
	case SUCCESS:
		break;
	};
	if (Result) Gtk$GObject$Value$to_gtk(Result0.Val, Result);
};

Gtk$GObject$Closure_t *_from_ref(Std$Object_t **Function) {
	ref_closure_t *Handle = (ref_closure_t *)g_closure_new_simple(sizeof(ref_closure_t), 0);
	Handle->Function = Function;
	g_closure_set_marshal((GClosure *)Handle, (GClosureMarshal)__marshal_ref);
	Gtk$GObject$Closure_t *Closure = new(Gtk$GObject$Closure_t);
	Closure->Type = T;
	Closure->Handle = (GClosure *)Handle;
	Handle->Closure = Closure;
	return Closure;
};

GLOBAL_FUNCTION(New, 1) {
	Result->Val = (Std$Object_t *)_from_val(Args[0].Val);
	return SUCCESS;
};
