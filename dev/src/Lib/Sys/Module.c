#include <Sys/Module.h>
#include <Std.h>
#include <Agg.h>
#include <Riva/Memory.h>
#include <string.h>
#include <stdio.h>

TYPE(T);
//  A handle to a loaded module.

typedef struct info_t {
	const Std$Type_t *Type;
	Std$Object_t *Module;
	Std$Object_t *Import;
} info_t;

TYPE(InfoT);
// Information about an exported/imported symbol.

GLOBAL_FUNCTION(GetInfo, 1) {
//@value
// Returns information about an exported value/reference.
	const char *ModuleName, *SymbolName;
	if (Riva$Module$lookup((void *)Args[0].Ref ?: (void *)Args[0].Val, &ModuleName, &SymbolName)) {
		info_t *Info = new(info_t);
		Info->Type = InfoT;
		Info->Module = Std$String$new(ModuleName);
		Info->Import = Std$String$new(SymbolName);
		Result->Val = (Std$Object_t *)Info;
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("module", TYP, InfoT) {
	info_t *Info = (info_t *)Args[0].Val;
	Result->Val = Info->Module;
	return SUCCESS;
};

METHOD("import", TYP, InfoT) {
	info_t *Info = (info_t *)Args[0].Val;
	Result->Val = Info->Import;
	return SUCCESS;
};

GLOBAL_FUNCTION(FromVal, 1) {
//@val
//:Agg$List$T
// Returns the a name for <var>val</var> as a list <code>[module, import]</code> if is a value imported from a loaded module. Fails otherwise.
	const char *ModuleName, *SymbolName;
	if (Riva$Module$lookup(Args[0].Val, &ModuleName, &SymbolName)) {
		Result->Val = Agg$List$new(2, Std$String$new(ModuleName), Std$String$new(SymbolName));
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

GLOBAL_FUNCTION(FromRef, 1) {
//@ref
//:Agg$List$T
// Returns the a name for <var>ref</var> as a list <code>[module, import]</code> if is a reference imported from a loaded module. Fails otherwise.
	const char *ModuleName, *SymbolName;
	if (Riva$Module$lookup(Args[0].Ref, &ModuleName, &SymbolName)) {
		Result->Val = Agg$List$new(2, Std$String$new(ModuleName), Std$String$new(SymbolName));
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

Sys$Module_t *_new(const char *Name) {
	if (Name == 0) Name = "Anonymous Module";
	Sys$Module_t *Module = Riva$Module$new(Name);
	return Module;
};

Sys$Module_t *_load(const char *Path, const char *Name) {
	Sys$Module_t *Module = Riva$Module$load(Path, Name);
	if (Module) {
		return Module;
	} else {
		return 0;
	};
};

int _import(Sys$Module_t *Module, const char *Name, int *IsRef, void **Data) {
	return Riva$Module$import(Module, Name, IsRef, Data);
};

void _export(Sys$Module_t *Module, const char *Name, int IsRef, void *Data) {
	Riva$Module$export(Module, Name, IsRef, Data);
};

void _set_path(Sys$Module_t *Module, const char *Path) {
	Riva$Module$set_path(Module, Path);
};

const char *_get_path(Sys$Module_t *Module) {
	return Riva$Module$get_path(Module);
};

GLOBAL_FUNCTION(AddDirectory, 1) {
//@directory:Std$String$T
// Adds <var>directory</var> to the module search path.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Riva$Module$add_directory(Std$String$flatten(Args[0].Val));
	return SUCCESS;
};

GLOBAL_FUNCTION(New, 0) {
//:T
//  Creates an returns an anonymous module.
	const char *Name;
	if (Count > 0) {
		Name = Std$String$flatten(Args[0].Val);
	} else {
		Name = "Anonymous Module";
	};
	Sys$Module_t *Module = Riva$Module$new(Name);
	Result->Val = (Std$Object_t *)Module;
	return SUCCESS;
};

GLOBAL_FUNCTION(Load, 2) {
//@path : Std$String$T
//@name : Std$String$T
//:T
//  Loads and returns a handle to the module <var>name</var> from <var>path</var>.
//  <var>path</var> can be <code>NIL</code> to load <var>name</var> relative to the current directory.
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	const char *Path = (Args[0].Val == Std$Object$Nil) ? 0 :Std$String$flatten(Args[0].Val);
	const char *Name = Std$String$flatten(Args[1].Val);
	Riva$Module_t *Module = Riva$Module$load(Path, Name);
	if (Module) {
		Result->Val = (Std$Object_t *)Module;
		return SUCCESS;
	} else {
		Result->Val = Std$String$new("Module not found");
		return MESSAGE;
	};
};

GLOBAL_FUNCTION(LoadFile, 1) {
//@file:Std$String$T
//:T
//  Loads and returns a handle to the module located in <var>file</var>.
//  The loaded module is not cached by the module system.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	const char *Name = Std$String$flatten(Args[0].Val);
	const char *Type = (Count > 1) ? Std$String$flatten(Args[1].Val) : 0;
	Riva$Module_t *Module = Riva$Module$load_file(Name, Type);
	if (Module) {
		Result->Val = (Std$Object_t *)Module;
		return SUCCESS;
	} else {
		Result->Val = Std$String$new("Module not found");
		return MESSAGE;
	};
};

static int function_import(Std$Object_t *Function, const char *Symbol, int *IsRef, void **Data) {
	Std$Function_result Result;
	if (Std$Function$call(Function, 1, &Result, Std$String$new(Symbol), 0) < FAILURE) {
		if (Result.Ref) {
			*IsRef = 1;
			*Data = Result.Ref;
		} else {
			*IsRef = 0;
			*Data = Result.Val;
		};
		return 1;
	} else {
		return 0;
	};
};

METHOD("set_import_func", TYP, T, TYP, Std$Function$T) {
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	Riva$Module$set_import_func(Riva$Module$get_default_provider(Module), Args[1].Val, (Riva$Module$import_func)function_import);
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD("import", TYP, T, TYP, Std$String$T) {
//@module
//@id
//:Std$Object$T
//  Returns the imported value <code>module.id</code>.
	const char *Symbol = Std$String$flatten(Args[1].Val);
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	int IsRef; void *Data;
	if (Riva$Module$import(Module, Symbol, &IsRef, &Data)) {
		if (IsRef) {
			Result->Val = *(Result->Ref = Data);
		} else {
			Result->Val = Data;
			Result->Ref = 0;
		};
		return SUCCESS;
	} else {
/*
		Result->Val = Std$String$new("Import not found");
		return MESSAGE;
*/
		return FAILURE;
	};
}

METHOD("export", TYP, T, TYP, Std$String$T, ANY) {
//@module
//@id
//@value
// Adds the export <var>value</var> to <var>module</var> with the name <var>id</var>.
	const char *Symbol = Std$String$flatten(Args[1].Val);
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	if (Args[2].Ref) {
		Riva$Module$export(Module, Symbol, 1, Args[2].Ref);
	} else {
		Riva$Module$export(Module, Symbol, 0, Args[2].Val);
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

METHOD(".", TYP, T, TYP, Std$String$T) {
//@module
//@id
//:Std$Object$T
//  Returns the imported value <code>module.id</code>.
	const char *Symbol = Std$String$flatten(Args[1].Val);
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	int IsRef; void *Data;
	if (Riva$Module$import(Module, Symbol, &IsRef, &Data)) {
		if (IsRef) {
			Result->Val = *(Result->Ref = Data);
		} else {
			Result->Val = Data;
			Result->Ref = 0;
		};
		return SUCCESS;
	} else {
/*
		Result->Val = Std$String$new("Import not found");
		return MESSAGE;
*/
		return FAILURE;
	};
};

METHOD("name", TYP, T) {
//@module
//:Std$String$T
// Returns the name of <var>module</var>.
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	Result->Val = Std$String$new(Riva$Module$get_name(Module) ?: "<anonymous module>");
	return SUCCESS;
};

METHOD("path", TYP, T) {
//@module
//:Std$String$T
// Returns the path of <var>module</var>.
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	Result->Val = Std$String$new(Riva$Module$get_path(Module));
	return SUCCESS;
};

METHOD("version", TYP, T) {
//@module
//:Std$Integer$SmallT
// Returns the version of <var>module</var>.
	Riva$Module_t *Module = (Sys$Module_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Riva$Module$get_version(Module));
	return SUCCESS;
};

GLOBAL_FUNCTION(LoadSymbol, 1) {
	Result->Val = Std$Address$new(Riva$Module$load_symbol(Std$String$flatten(Args[0].Val)));
	return SUCCESS;
};


