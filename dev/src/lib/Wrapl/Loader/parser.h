#ifndef PARSER_H
#define PARSER_H

#include <Riva/Module.h>

#include "scanner.h"
#include "compiler.h"

extern expr_t *accept_expr(scanner_t *Scanner);
extern module_expr_t *accept_module(scanner_t *Scanner, Riva$Module$provider_t *Provider);
extern module_expr_t *parse_module(scanner_t *Scanner, Riva$Module$provider_t *Provider);
extern command_expr_t *accept_command(scanner_t *Scanner);

#endif
