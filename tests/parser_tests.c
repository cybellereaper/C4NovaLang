#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "nova/codegen.h"
#include "nova/ir.h"
#include "nova/parser.h"
#include "nova/semantic.h"

static const char *CORE_PROGRAM =
    "module demo.core\n"
    "type Option = Some(Number) | None\n"
    "fun identity(x: Number): Number = x\n"
    "fun wrap(): Option = Some(42)\n"
    "fun choose(v: Option): Number = match v { Some(value) -> value; None -> 0 }\n"
    "fun pipeline(): Number = 1 |> identity\n"
    "fun later(): Number = async { 42 }\n";

static bool token_matches(const NovaToken *token, const char *text) {
    size_t len = strlen(text);
    return token->length == len && strncmp(token->lexeme, text, len) == 0;
}

static void test_parser_and_semantics(void) {
    NovaParser parser;
    nova_parser_init(&parser, CORE_PROGRAM);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);
    assert(program->decl_count == 6);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);
    assert(ctx.diagnostics.count == 0);

    // ensure pipeline function infers Number type without effects
    const NovaFunDecl *pipeline = &program->decls[4].as.fun_decl;
    const NovaExprInfo *pipeline_info = nova_semantic_lookup_expr(&ctx, pipeline->body);
    assert(pipeline_info != NULL);
    assert(pipeline_info->type == ctx.type_number);
    assert((pipeline_info->effects & NOVA_EFFECT_ASYNC) == 0);

    // ensure async function records async effect
    const NovaFunDecl *later = &program->decls[5].as.fun_decl;
    const NovaExprInfo *later_info = nova_semantic_lookup_expr(&ctx, later->body);
    assert(later_info != NULL);
    assert((later_info->effects & NOVA_EFFECT_ASYNC) != 0);

    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

static void test_match_exhaustiveness_warning(void) {
    const char *source =
        "module demo.flags\n"
        "type Flag = Yes | No\n"
        "fun only_yes(f: Flag): Number = match f { Yes -> 1 }\n";

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);
    assert(ctx.diagnostics.count > 0);

    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

static void test_codegen_pipeline(void) {
    const char *source =
        "module demo.codegen\n"
        "fun main(): Number = if true { 42 } else { 0 }\n";

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);
    assert(ctx.diagnostics.count == 0);

    NovaIRProgram *ir = nova_ir_lower(program, &ctx);
    assert(ir != NULL);

    const char *object_path = "build/main.o";
    char error[256];
    bool ok = nova_codegen_emit_object(ir, &ctx, object_path, error, sizeof(error));
    assert(ok && "code generation failed");

    struct stat st;
    assert(stat(object_path, &st) == 0);

    remove(object_path);
    nova_ir_free(ir);
    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

static void test_ir_lowering_extensions(void) {
    const char *source =
        "module demo.ir\n"
        "fun identity(x: Number): Number = x\n"
        "fun double(x: Number): Number = x\n"
        "fun compute(): Number = 1 |> identity |> double\n"
        "fun conditional(flag: Bool): Number = if flag { 1 } else { 0 }\n";

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    assert(program != NULL);

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);

    NovaIRProgram *ir = nova_ir_lower(program, &ctx);
    assert(ir != NULL);

    const NovaIRFunction *compute_fn = NULL;
    const NovaIRFunction *conditional_fn = NULL;
    for (size_t i = 0; i < ir->function_count; ++i) {
        if (token_matches(&ir->functions[i].name, "compute")) {
            compute_fn = &ir->functions[i];
        } else if (token_matches(&ir->functions[i].name, "conditional")) {
            conditional_fn = &ir->functions[i];
        }
    }
    assert(compute_fn != NULL);
    assert(conditional_fn != NULL);

    assert(compute_fn->body != NULL);
    assert(compute_fn->body->kind == NOVA_IR_EXPR_CALL);
    assert(token_matches(&compute_fn->body->as.call.callee, "double"));
    assert(compute_fn->body->as.call.arg_count == 1);
    const NovaIRExpr *inner_call = compute_fn->body->as.call.args[0];
    assert(inner_call != NULL);
    assert(inner_call->kind == NOVA_IR_EXPR_CALL);
    assert(token_matches(&inner_call->as.call.callee, "identity"));
    assert(inner_call->as.call.arg_count == 1);
    const NovaIRExpr *literal = inner_call->as.call.args[0];
    assert(literal != NULL && literal->kind == NOVA_IR_EXPR_NUMBER);

    assert(conditional_fn->body != NULL);
    assert(conditional_fn->body->kind == NOVA_IR_EXPR_IF);
    assert(conditional_fn->body->as.if_expr.condition->kind == NOVA_IR_EXPR_IDENTIFIER);
    assert(conditional_fn->body->as.if_expr.then_branch->kind == NOVA_IR_EXPR_NUMBER);
    assert(conditional_fn->body->as.if_expr.else_branch->kind == NOVA_IR_EXPR_NUMBER);

    nova_ir_free(ir);
    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
}

int main(void) {
    test_parser_and_semantics();
    test_match_exhaustiveness_warning();
    test_codegen_pipeline();
    test_ir_lowering_extensions();
    printf("All tests passed.\n");
    return 0;
}

