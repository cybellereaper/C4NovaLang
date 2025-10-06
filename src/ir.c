#include "nova/ir.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static char *copy_token_text(const NovaToken *token) {
    char *text = malloc(token->length + 1);
    if (!text) return NULL;
    memcpy(text, token->lexeme, token->length);
    text[token->length] = '\0';
    return text;
}

static bool token_equals_cstr(const NovaToken *token, const char *text) {
    size_t len = strlen(text);
    return token && token->length == len && strncmp(token->lexeme, text, len) == 0;
}

static NovaIRExpr *nova_ir_expr_new(NovaIRExprKind kind, NovaTypeId type) {
    NovaIRExpr *expr = calloc(1, sizeof(NovaIRExpr));
    if (!expr) return NULL;
    expr->kind = kind;
    expr->type = type;
    return expr;
}

static NovaIRExpr *lower_expr(const NovaExpr *expr, const NovaSemanticContext *semantics);

static NovaTypeId infer_type_from_token(const NovaSemanticContext *semantics, const NovaToken *token) {
    if (!token) return semantics->type_unknown;
    if (token_equals_cstr(token, "Number")) return semantics->type_number;
    if (token_equals_cstr(token, "String")) return semantics->type_string;
    if (token_equals_cstr(token, "Bool")) return semantics->type_bool;
    if (token_equals_cstr(token, "Unit")) return semantics->type_unit;
    const NovaTypeRecord *record = nova_semantic_find_type(semantics, token);
    if (record) return record->type_id;
    return semantics->type_unknown;
}

static NovaIRExpr *lower_literal(const NovaExpr *expr, const NovaSemanticContext *semantics) {
    const NovaExprInfo *info = nova_semantic_lookup_expr(semantics, expr);
    NovaTypeId type = info ? info->type : 0;
    NovaIRExpr *ir = NULL;
    switch (expr->as.literal.kind) {
    case NOVA_LITERAL_NUMBER: {
        char buffer[64];
        size_t len = expr->as.literal.token.length;
        len = len < sizeof(buffer) - 1 ? len : sizeof(buffer) - 1;
        memcpy(buffer, expr->as.literal.token.lexeme, len);
        buffer[len] = '\0';
        ir = nova_ir_expr_new(NOVA_IR_EXPR_NUMBER, type);
        if (ir) {
            ir->as.number_value = strtod(buffer, NULL);
        }
        break;
    }
    case NOVA_LITERAL_STRING: {
        ir = nova_ir_expr_new(NOVA_IR_EXPR_STRING, type);
        if (ir) {
            ir->as.string_value.text = copy_token_text(&expr->as.literal.token);
        }
        break;
    }
    case NOVA_LITERAL_BOOL: {
        ir = nova_ir_expr_new(NOVA_IR_EXPR_BOOL, type);
        if (ir) {
            ir->as.bool_value = token_equals_cstr(&expr->as.literal.token, "true");
        }
        break;
    }
    case NOVA_LITERAL_UNIT:
        ir = nova_ir_expr_new(NOVA_IR_EXPR_NUMBER, type);
        if (ir) {
            ir->as.number_value = 0.0;
        }
        break;
    case NOVA_LITERAL_LIST:
        break;
    }
    return ir;
}

static NovaIRExpr *lower_call(const NovaExpr *expr, const NovaSemanticContext *semantics) {
    const NovaExprInfo *info = nova_semantic_lookup_expr(semantics, expr);
    NovaIRExpr *ir = nova_ir_expr_new(NOVA_IR_EXPR_CALL, info ? info->type : 0);
    if (!ir) return NULL;
    NovaExpr *callee_expr = expr->as.call.callee;
    if (callee_expr->kind != NOVA_EXPR_IDENTIFIER) {
        free(ir);
        return NULL;
    }
    ir->as.call.callee = callee_expr->as.identifier.name;
    ir->as.call.arg_count = expr->as.call.args.count;
    if (ir->as.call.arg_count > 0) {
        ir->as.call.args = calloc(ir->as.call.arg_count, sizeof(NovaIRExpr *));
        if (!ir->as.call.args) {
            free(ir);
            return NULL;
        }
        for (size_t i = 0; i < expr->as.call.args.count; ++i) {
            ir->as.call.args[i] = lower_expr(expr->as.call.args.items[i].value, semantics);
        }
    }
    return ir;
}

static NovaIRExpr *lower_expr(const NovaExpr *expr, const NovaSemanticContext *semantics) {
    if (!expr) return NULL;
    switch (expr->kind) {
    case NOVA_EXPR_LITERAL:
    case NOVA_EXPR_LIST_LITERAL:
        return lower_literal(expr, semantics);
    case NOVA_EXPR_IDENTIFIER: {
        const NovaExprInfo *info = nova_semantic_lookup_expr(semantics, expr);
        NovaIRExpr *ir = nova_ir_expr_new(NOVA_IR_EXPR_IDENTIFIER, info ? info->type : 0);
        if (ir) {
            ir->as.identifier = expr->as.identifier.name;
        }
        return ir;
    }
    case NOVA_EXPR_CALL:
        return lower_call(expr, semantics);
    case NOVA_EXPR_BLOCK:
        if (expr->as.block.expressions.count == 0) {
            return nova_ir_expr_new(NOVA_IR_EXPR_NUMBER, semantics->type_unit);
        }
        return lower_expr(expr->as.block.expressions.items[expr->as.block.expressions.count - 1], semantics);
    case NOVA_EXPR_PAREN:
        return lower_expr(expr->as.inner, semantics);
    default:
        return NULL;
    }
}

static void nova_ir_function_init(NovaIRFunction *fn) {
    fn->params = NULL;
    fn->param_count = 0;
    fn->body = NULL;
}

static void nova_ir_expr_free(NovaIRExpr *expr) {
    if (!expr) return;
    switch (expr->kind) {
    case NOVA_IR_EXPR_STRING:
        free(expr->as.string_value.text);
        break;
    case NOVA_IR_EXPR_CALL:
        if (expr->as.call.args) {
            for (size_t i = 0; i < expr->as.call.arg_count; ++i) {
                nova_ir_expr_free(expr->as.call.args[i]);
            }
            free(expr->as.call.args);
        }
        break;
    default:
        break;
    }
    free(expr);
}

static void nova_ir_function_free(NovaIRFunction *fn) {
    free(fn->params);
    nova_ir_expr_free(fn->body);
}

NovaIRProgram *nova_ir_lower(const NovaProgram *program, const NovaSemanticContext *semantics) {
    NovaIRProgram *ir = calloc(1, sizeof(NovaIRProgram));
    if (!ir) return NULL;
    for (size_t i = 0; i < program->decl_count; ++i) {
        const NovaDecl *decl = &program->decls[i];
        if (decl->kind != NOVA_DECL_FUN) continue;
        if (ir->function_count == ir->function_capacity) {
            size_t new_capacity = ir->function_capacity == 0 ? 4 : ir->function_capacity * 2;
            NovaIRFunction *functions = realloc(ir->functions, new_capacity * sizeof(NovaIRFunction));
            if (!functions) {
                continue;
            }
            ir->functions = functions;
            ir->function_capacity = new_capacity;
        }
        NovaIRFunction *fn = &ir->functions[ir->function_count++];
        nova_ir_function_init(fn);
        fn->name = decl->as.fun_decl.name;
        fn->param_count = decl->as.fun_decl.params.count;
        if (fn->param_count > 0) {
            fn->params = calloc(fn->param_count, sizeof(NovaIRParam));
            for (size_t p = 0; p < fn->param_count; ++p) {
                fn->params[p].name = decl->as.fun_decl.params.items[p].name;
                if (decl->as.fun_decl.params.items[p].has_type) {
                    fn->params[p].type = infer_type_from_token(semantics, &decl->as.fun_decl.params.items[p].type_name);
                } else {
                    fn->params[p].type = semantics->type_unknown;
                }
            }
        }
        const NovaExprInfo *body_info = nova_semantic_lookup_expr(semantics, decl->as.fun_decl.body);
        fn->return_type = body_info ? body_info->type : semantics->type_unknown;
        fn->effects = body_info ? body_info->effects : NOVA_EFFECT_NONE;
        fn->body = lower_expr(decl->as.fun_decl.body, semantics);
    }
    return ir;
}

void nova_ir_free(NovaIRProgram *program) {
    if (!program) return;
    for (size_t i = 0; i < program->function_count; ++i) {
        nova_ir_function_free(&program->functions[i]);
    }
    free(program->functions);
    free(program);
}

