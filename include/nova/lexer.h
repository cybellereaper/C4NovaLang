#pragma once

#include "nova/token.h"

typedef struct {
    const char *source;
    size_t length;
    size_t position;
    size_t line;
    size_t column;
} NovaLexer;

void nova_lexer_init(NovaLexer *lexer, const char *source, size_t length);
NovaToken nova_lexer_next(NovaLexer *lexer);
NovaTokenArray nova_lexer_tokenize(const char *source);

