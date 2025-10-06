#pragma once

#include "nova/ast.h"
#include "nova/diagnostic.h"
#include "nova/lexer.h"

typedef struct {
    NovaTokenArray tokens;
    size_t current;
    const char *source;
    NovaDiagnosticList diagnostics;
    bool panic_mode;
    bool had_error;
} NovaParser;

void nova_parser_init(NovaParser *parser, const char *source);
NovaProgram *nova_parser_parse(NovaParser *parser);
void nova_parser_free(NovaParser *parser);

