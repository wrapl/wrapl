#include <Riva/Memory.h>
#include <Std.h>

#include <libtecla.h>

typedef struct tecla_t {
	Std$Type$t *Type;
	GetLine *Handle;
} tecla_t;

TYPE(T);

GLOBAL_FUNCTION(New, 2) {
	tecla_t *Tecla = new(tecla_t);
	Tecla->Type = T;
	Tecla->Handle = new_GetLine(((Std$Integer$smallt *)Args[0].Val)->Value, ((Std$Integer$smallt *)Args[1].Val)->Value);
	Result->Val = Tecla;
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$String$T) {
	tecla_t *Tecla = Args[0].Val;
	Result->Val = Std$String$copy(gl_get_line(Tecla->Handle, Std$String$flatten(Args[1].Val), 0, -1));
	return SUCCESS;
};

METHOD("read", TYP, T, TYP, Std$String$T, TYP, Std$String$T) {
	tecla_t *Tecla = Args[0].Val;
	Result->Val = Std$String$copy(gl_get_line(Tecla->Handle, Std$String$flatten(Args[1].Val), Std$String$flatten(Args[2].Val), -1));
	return SUCCESS;
};

