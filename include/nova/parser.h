#pragma once

#include "nova/ast.h"
#include "nova/lexer.h"

typedef struct {
    NovaTokenArray tokens;
    size_t current;
    const char *source;
} NovaParser;

typedef struct {
    bool ok;
    const char *message;
    NovaToken token;
} NovaParseResult;

void nova_parser_init(NovaParser *parser, const char *source);
NovaProgram *nova_parser_parse(NovaParser *parser);
void nova_parser_free(NovaParser *parser);

