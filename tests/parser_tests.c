#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nova/parser.h"
#include "nova/semantic.h"

static void test_parse_simple_program(void) {
    const char *source =
        "module demo.core\n"
        "import std.math\n"
        "let answer: Number = 42\n"
        "fun double(x: Number) : Number = x |> double\n";

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);
    assert(program->decl_count == 2);
    assert(program->decls[0].kind == NOVA_DECL_LET);
    assert(program->decls[1].kind == NOVA_DECL_FUN);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);
    assert(ctx.diagnostics.count == 0);

    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

static void test_semantic_duplicate_binding(void) {
    const char *source =
        "module demo.core\n"
        "let answer = 1\n"
        "let answer = 2\n";

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);
    assert(ctx.diagnostics.count >= 1);

    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

int main(void) {
    test_parse_simple_program();
    test_semantic_duplicate_binding();
    printf("All tests passed.\n");
    return 0;
}

