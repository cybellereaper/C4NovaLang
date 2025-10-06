#include "nova/semantic.h"

#include <stdlib.h>
#include <string.h>

static void diagnostics_init(NovaDiagnosticList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void diagnostics_push(NovaDiagnosticList *list, NovaToken token, const char *message) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        NovaDiagnostic *new_items = realloc(list->items, new_capacity * sizeof(NovaDiagnostic));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = (NovaDiagnostic){ .token = token, .message = message };
}

static NovaScope *scope_push(NovaScope *parent) {
    NovaScope *scope = (NovaScope *)calloc(1, sizeof(NovaScope));
    scope->entries = NULL;
    scope->parent = parent;
    return scope;
}

static void scope_pop(NovaScope *scope) {
    NovaScopeEntry *entry = scope->entries;
    while (entry) {
        NovaScopeEntry *next = entry->next;
        free(entry);
        entry = next;
    }
    free(scope);
}

static NovaScopeEntry *scope_lookup(NovaScope *scope, NovaToken name) {
    for (NovaScope *s = scope; s != NULL; s = s->parent) {
        for (NovaScopeEntry *entry = s->entries; entry != NULL; entry = entry->next) {
            if (entry->name.length == name.length && strncmp(entry->name.lexeme, name.lexeme, name.length) == 0) {
                return entry;
            }
        }
    }
    return NULL;
}

static void scope_define(NovaScope *scope, NovaToken name, NovaType type, NovaDiagnosticList *diagnostics) {
    if (scope_lookup(scope, name) != NULL) {
        diagnostics_push(diagnostics, name, "symbol already defined");
        return;
    }
    NovaScopeEntry *entry = (NovaScopeEntry *)calloc(1, sizeof(NovaScopeEntry));
    entry->name = name;
    entry->type = type;
    entry->next = scope->entries;
    scope->entries = entry;
}

static NovaType nova_type_from_literal(const NovaExpr *expr) {
    switch (expr->as.literal.kind) {
    case NOVA_LITERAL_NUMBER: return (NovaType){ .kind = NOVA_TYPE_NUMBER };
    case NOVA_LITERAL_STRING: return (NovaType){ .kind = NOVA_TYPE_STRING };
    case NOVA_LITERAL_BOOL: return (NovaType){ .kind = NOVA_TYPE_BOOL };
    case NOVA_LITERAL_UNIT: return (NovaType){ .kind = NOVA_TYPE_UNIT };
    }
    return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
}

static NovaType analyze_expr(NovaSemanticContext *ctx, NovaScope *scope, const NovaExpr *expr);

static NovaType analyze_block(NovaSemanticContext *ctx, NovaScope *scope, const NovaBlock *block) {
    NovaScope *inner = scope_push(scope);
    NovaType result = { .kind = NOVA_TYPE_UNIT };
    for (size_t i = 0; i < block->expressions.count; ++i) {
        result = analyze_expr(ctx, inner, block->expressions.items[i]);
    }
    scope_pop(inner);
    return result;
}

static NovaType analyze_expr(NovaSemanticContext *ctx, NovaScope *scope, const NovaExpr *expr) {
    if (!expr) return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
    switch (expr->kind) {
    case NOVA_EXPR_LITERAL:
        return nova_type_from_literal(expr);
    case NOVA_EXPR_IDENTIFIER: {
        NovaScopeEntry *entry = scope_lookup(scope, expr->as.literal.token);
        if (!entry) {
            diagnostics_push(&ctx->diagnostics, expr->start_token, "undefined identifier");
            return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
        }
        return entry->type;
    }
    case NOVA_EXPR_BLOCK:
        return analyze_block(ctx, scope, &expr->as.block);
    case NOVA_EXPR_PIPE:
        return analyze_expr(ctx, scope, expr->as.pipe_expr.stages.items[expr->as.pipe_expr.stages.count - 1]);
    case NOVA_EXPR_CALL:
        return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
    case NOVA_EXPR_IF: {
        analyze_expr(ctx, scope, expr->as.if_expr.condition);
        NovaType then_type = analyze_expr(ctx, scope, expr->as.if_expr.then_branch);
        NovaType else_type = { .kind = NOVA_TYPE_UNIT };
        if (expr->as.if_expr.else_branch) {
            else_type = analyze_expr(ctx, scope, expr->as.if_expr.else_branch);
        }
        if (then_type.kind != else_type.kind) {
            diagnostics_push(&ctx->diagnostics, expr->start_token, "if branches have mismatched types");
        }
        return then_type;
    }
    case NOVA_EXPR_ASYNC:
    case NOVA_EXPR_AWAIT:
    case NOVA_EXPR_EFFECT:
    case NOVA_EXPR_MATCH:
    case NOVA_EXPR_LAMBDA:
    case NOVA_EXPR_LIST:
    case NOVA_EXPR_PAREN:
        return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
    }
    return (NovaType){ .kind = NOVA_TYPE_UNKNOWN };
}

static void analyze_decl(NovaSemanticContext *ctx, NovaScope *scope, const NovaDecl *decl) {
    switch (decl->kind) {
    case NOVA_DECL_LET: {
        NovaType value_type = analyze_expr(ctx, scope, decl->as.let_decl.value);
        if (decl->as.let_decl.has_type && decl->as.let_decl.type_name.length > 0) {
            // For now, ensure annotated type matches literal type for simple literals
            if (decl->as.let_decl.type_name.length == 6 && strncmp(decl->as.let_decl.type_name.lexeme, "String", 6) == 0 && value_type.kind != NOVA_TYPE_STRING) {
                diagnostics_push(&ctx->diagnostics, decl->as.let_decl.type_name, "type mismatch");
            }
        }
        scope_define(scope, decl->as.let_decl.name, value_type, &ctx->diagnostics);
        break;
    }
    case NOVA_DECL_FUN: {
        NovaType fn_type = { .kind = NOVA_TYPE_FUNCTION };
        scope_define(scope, decl->as.fun_decl.name, fn_type, &ctx->diagnostics);
        NovaScope *inner = scope_push(scope);
        for (size_t i = 0; i < decl->as.fun_decl.params.count; ++i) {
            NovaParam param = decl->as.fun_decl.params.items[i];
            scope_define(inner, param.name, (NovaType){ .kind = NOVA_TYPE_UNKNOWN }, &ctx->diagnostics);
        }
        analyze_expr(ctx, inner, decl->as.fun_decl.body);
        scope_pop(inner);
        break;
    }
    case NOVA_DECL_TYPE:
        scope_define(scope, decl->as.type_decl.name, (NovaType){ .kind = NOVA_TYPE_UNKNOWN }, &ctx->diagnostics);
        break;
    }
}

void nova_semantic_context_init(NovaSemanticContext *ctx) {
    ctx->scope = scope_push(NULL);
    diagnostics_init(&ctx->diagnostics);
}

void nova_semantic_context_free(NovaSemanticContext *ctx) {
    scope_pop(ctx->scope);
    free(ctx->diagnostics.items);
}

void nova_semantic_analyze_program(NovaSemanticContext *ctx, const NovaProgram *program) {
    for (size_t i = 0; i < program->decl_count; ++i) {
        analyze_decl(ctx, ctx->scope, &program->decls[i]);
    }
}

