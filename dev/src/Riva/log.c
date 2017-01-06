#include "libriva.h"
#include <stdio.h>

extern void log_nolog(void);

__log_writes_fn __log_Writes = (__log_writes_fn)log_nolog;
__log_writen_fn __log_Writen = (__log_writen_fn)log_nolog;
__log_writef_fn __log_Writef = (__log_writef_fn)log_nolog;

static void log_default_writes(const char *String) {
	fputs(String, stdout);
};

static void log_default_writen(const char *String, unsigned long Length) {
	fwrite(String, 1, Length, stdout);
};

__log_writes_fn __log_Errors = (__log_writes_fn)log_default_writes;
__log_writen_fn __log_Errorn = (__log_writen_fn)log_default_writen;
__log_writef_fn __log_Errorf = (__log_writef_fn)printf;

void log_enable(void) {
	__log_Writes = __log_Errors;
	__log_Writen = __log_Errorn;
	__log_Writef = __log_Errorf;
};

void log_disable(void) {
	__log_Writes = (__log_writes_fn)log_nolog;
	__log_Writen = (__log_writen_fn)log_nolog;
	__log_Writef = (__log_writef_fn)log_nolog;
};

static void log_redirect(__log_writes_fn Writes, __log_writen_fn Writen, __log_writef_fn Writef) {
	__log_Errors = Writes;
	__log_Errorn = Writen;
	__log_Errorf = Writef;
	__log_Writes = Writes;
	__log_Writen = Writen;
	__log_Writef = Writef;
};

void log_init(void) {
	module_t *Module = module_new("Riva/Log");
	module_add_alias(Module, "library:/Riva/Log");
	module_export(Module, "_writes", 0, log_writes);
	module_export(Module, "_writen", 0, log_writen);
	module_export(Module, "_writef", 0, log_writef);
	module_export(Module, "_errors", 0, log_errors);
	module_export(Module, "_errorn", 0, log_errorn);
	module_export(Module, "_errorf", 0, log_errorf);
	module_export(Module, "_enable", 0, log_enable);
	module_export(Module, "_disable", 0, log_disable);
	module_export(Module, "_redirect", 0, log_redirect);
};
