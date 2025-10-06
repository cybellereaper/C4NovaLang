#include "nova/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NovaToken peek_token(NovaParser *parser) {
    return parser->tokens.data[parser->current];
}

static NovaToken previous_token(NovaParser *parser) {
    return parser->tokens.data[parser->current - 1];
}

static bool is_at_end(NovaParser *parser) {
    return peek_token(parser).type == NOVA_TOKEN_EOF;
}

static bool check(NovaParser *parser, NovaTokenType type) {
    if (is_at_end(parser)) return type == NOVA_TOKEN_EOF;
    return peek_token(parser).type == type;
}

static NovaToken advance_token(NovaParser *parser) {
    if (!is_at_end(parser)) {
        parser->current++;
    }
    return previous_token(parser);
}

static bool match(NovaParser *parser, NovaTokenType type) {
    if (check(parser, type)) {
        advance_token(parser);
        return true;
    }
    return false;
}

static NovaToken consume(NovaParser *parser, NovaTokenType type, const char *message) {
    if (check(parser, type)) {
        return advance_token(parser);
    }
    fprintf(stderr, "Parse error: %s at line %zu\n", message, peek_token(parser).line);
    return (NovaToken){ .type = NOVA_TOKEN_ERROR, .lexeme = message };
}

static NovaExpr *nova_new_expr(NovaExprKind kind, NovaToken start) {
    NovaExpr *expr = (NovaExpr *)calloc(1, sizeof(NovaExpr));
    if (!expr) return NULL;
    expr->kind = kind;
    expr->start_token = start;
    return expr;
}

static NovaExpr *parse_expression(NovaParser *parser);
static NovaExpr *parse_block(NovaParser *parser);
static NovaExpr *parse_primary(NovaParser *parser);
static NovaExpr *parse_call_expr(NovaParser *parser);
static NovaExpr *parse_pipe_expr(NovaParser *parser);
static NovaExpr *parse_literal_expr(NovaParser *parser);

static NovaParamList parse_param_list(NovaParser *parser) {
    NovaParamList list;
    nova_param_list_init(&list);
    if (!check(parser, NOVA_TOKEN_RPAREN)) {
        while (true) {
            NovaToken name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected parameter name");
            NovaParam param = { .name = name, .has_type = false };
            if (match(parser, NOVA_TOKEN_COLON)) {
                param.has_type = true;
                param.type_name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected type name");
            }
            nova_param_list_push(&list, param);
            if (!match(parser, NOVA_TOKEN_COMMA)) {
                break;
            }
        }
    }
    return list;
}

static NovaExprList parse_argument_list(NovaParser *parser) {
    NovaExprList list;
    nova_expr_list_init(&list);
    if (!check(parser, NOVA_TOKEN_RPAREN)) {
        while (true) {
            NovaExpr *arg = parse_expression(parser);
            nova_expr_list_push(&list, arg);
            if (!match(parser, NOVA_TOKEN_COMMA)) {
                break;
            }
        }
    }
    return list;
}

static NovaExpr *parse_if_expr(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_IF, start);
    expr->as.if_expr.condition = parse_expression(parser);
    expr->as.if_expr.then_branch = parse_block(parser);
    if (match(parser, NOVA_TOKEN_ELSE)) {
        expr->as.if_expr.else_branch = parse_block(parser);
    }
    return expr;
}

static NovaExpr *parse_async_expr(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_ASYNC, start);
    expr->as.async_expr.expr = parse_block(parser);
    return expr;
}

static NovaExpr *parse_await_expr(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_AWAIT, start);
    expr->as.await_expr.expr = parse_expression(parser);
    return expr;
}

static NovaExpr *parse_effect_expr(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_EFFECT, start);
    expr->as.effect_expr.expr = parse_expression(parser);
    return expr;
}

static NovaExpr *parse_match_expr(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_MATCH, start);
    expr->as.pipe_expr.target = parse_expression(parser);
    NovaMatchArmList arms;
    nova_match_arm_list_init(&arms);
    consume(parser, NOVA_TOKEN_LBRACE, "expected '{' after match target");
    while (!check(parser, NOVA_TOKEN_RBRACE) && !is_at_end(parser)) {
        NovaToken pattern = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected pattern name");
        NovaParamList params;
        nova_param_list_init(&params);
        if (match(parser, NOVA_TOKEN_LPAREN)) {
            params = parse_param_list(parser);
            consume(parser, NOVA_TOKEN_RPAREN, "expected ')' after match parameters");
        }
        consume(parser, NOVA_TOKEN_ARROW, "expected '->' after match arm pattern");
        NovaExpr *body = parse_expression(parser);
        NovaMatchArm arm = { .pattern = pattern, .params = params, .body = body };
        nova_match_arm_list_push(&arms, arm);
    }
    consume(parser, NOVA_TOKEN_RBRACE, "expected '}' to close match");
    expr->as.pipe_expr.stages.items = (NovaExpr **)arms.items;
    expr->as.pipe_expr.stages.count = arms.count;
    expr->as.pipe_expr.stages.capacity = arms.capacity;
    return expr;
}

static NovaExpr *parse_expression(NovaParser *parser) {
    NovaToken start = peek_token(parser);
    if (match(parser, NOVA_TOKEN_IF)) {
        return parse_if_expr(parser, start);
    }
    if (match(parser, NOVA_TOKEN_MATCH)) {
        return parse_match_expr(parser, start);
    }
    if (match(parser, NOVA_TOKEN_ASYNC)) {
        return parse_async_expr(parser, start);
    }
    if (match(parser, NOVA_TOKEN_AWAIT)) {
        return parse_await_expr(parser, start);
    }
    if (match(parser, NOVA_TOKEN_BANG)) {
        return parse_effect_expr(parser, start);
    }
    return parse_pipe_expr(parser);
}

static NovaExpr *parse_pipe_expr(NovaParser *parser) {
    NovaExpr *left = parse_call_expr(parser);
    NovaToken start = left ? left->start_token : peek_token(parser);
    NovaExprList stages;
    nova_expr_list_init(&stages);
    while (match(parser, NOVA_TOKEN_PIPE_OPERATOR)) {
        NovaExpr *stage = parse_call_expr(parser);
        nova_expr_list_push(&stages, stage);
    }
    if (stages.count == 0) {
        return left;
    }
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_PIPE, start);
    expr->as.pipe_expr.target = left;
    expr->as.pipe_expr.stages = stages;
    return expr;
}

static NovaExpr *parse_call_expr(NovaParser *parser) {
    NovaExpr *primary = parse_primary(parser);
    while (match(parser, NOVA_TOKEN_LPAREN)) {
        NovaExpr *call = nova_new_expr(NOVA_EXPR_CALL, primary->start_token);
        call->as.call_expr.callee = primary->start_token;
        call->as.call_expr.args = parse_argument_list(parser);
        consume(parser, NOVA_TOKEN_RPAREN, "expected ')' after arguments");
        primary = call;
    }
    return primary;
}

static NovaExpr *parse_list_literal(NovaParser *parser, NovaToken start) {
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_LIST, start);
    nova_expr_list_init(&expr->as.block.expressions);
    if (!check(parser, NOVA_TOKEN_RBRACKET)) {
        while (true) {
            NovaExpr *item = parse_expression(parser);
            nova_expr_list_push(&expr->as.block.expressions, item);
            if (!match(parser, NOVA_TOKEN_COMMA)) {
                break;
            }
        }
    }
    consume(parser, NOVA_TOKEN_RBRACKET, "expected ']' after list literal");
    return expr;
}

static NovaExpr *parse_literal_expr(NovaParser *parser) {
    NovaToken token = advance_token(parser);
    NovaExpr *expr = nova_new_expr(NOVA_EXPR_LITERAL, token);
    if (!expr) return NULL;
    switch (token.type) {
    case NOVA_TOKEN_NUMBER:
        expr->as.literal.kind = NOVA_LITERAL_NUMBER;
        break;
    case NOVA_TOKEN_STRING:
        expr->as.literal.kind = NOVA_LITERAL_STRING;
        break;
    case NOVA_TOKEN_TRUE:
    case NOVA_TOKEN_FALSE:
        expr->as.literal.kind = NOVA_LITERAL_BOOL;
        break;
    default:
        break;
    }
    expr->as.literal.token = token;
    return expr;
}

static NovaExpr *parse_primary(NovaParser *parser) {
    if (match(parser, NOVA_TOKEN_LBRACE)) {
        return parse_block(parser);
    }
    if (match(parser, NOVA_TOKEN_LPAREN)) {
        if (check(parser, NOVA_TOKEN_RPAREN)) {
            advance_token(parser);
            NovaExpr *expr = nova_new_expr(NOVA_EXPR_LITERAL, previous_token(parser));
            expr->as.literal.kind = NOVA_LITERAL_UNIT;
            expr->as.literal.token = previous_token(parser);
            return expr;
        }
        size_t save = parser->current;
        NovaParamList params = parse_param_list(parser);
        if (match(parser, NOVA_TOKEN_RPAREN) && match(parser, NOVA_TOKEN_ARROW)) {
            NovaExpr *lambda = nova_new_expr(NOVA_EXPR_LAMBDA, previous_token(parser));
            lambda->as.lambda.params = params;
            lambda->as.lambda.body = parse_expression(parser);
            return lambda;
        }
        parser->current = save;
        NovaExpr *expr = parse_expression(parser);
        consume(parser, NOVA_TOKEN_RPAREN, "expected ')' after expression");
        return expr;
    }
    if (match(parser, NOVA_TOKEN_LBRACKET)) {
        return parse_list_literal(parser, previous_token(parser));
    }
    if (check(parser, NOVA_TOKEN_NUMBER) || check(parser, NOVA_TOKEN_STRING) || check(parser, NOVA_TOKEN_TRUE) || check(parser, NOVA_TOKEN_FALSE)) {
        return parse_literal_expr(parser);
    }
    if (match(parser, NOVA_TOKEN_IDENTIFIER)) {
        NovaToken name = previous_token(parser);
        NovaExpr *expr = nova_new_expr(NOVA_EXPR_IDENTIFIER, name);
        expr->as.literal.token = name;
        return expr;
    }
    return NULL;
}

static NovaExpr *parse_block(NovaParser *parser) {
    NovaToken start = previous_token(parser);
    NovaExpr *block = nova_new_expr(NOVA_EXPR_BLOCK, start);
    nova_expr_list_init(&block->as.block.expressions);
    if (!check(parser, NOVA_TOKEN_RBRACE)) {
        do {
            NovaExpr *expr = parse_expression(parser);
            nova_expr_list_push(&block->as.block.expressions, expr);
        } while (match(parser, NOVA_TOKEN_SEMICOLON));
    }
    consume(parser, NOVA_TOKEN_RBRACE, "expected '}' after block");
    return block;
}

static NovaModulePath parse_module_path(NovaParser *parser) {
    NovaModulePath path;
    nova_module_path_init(&path);
    NovaToken segment = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected identifier");
    nova_module_path_push(&path, segment);
    while (match(parser, NOVA_TOKEN_DOT)) {
        NovaToken seg = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected identifier after '.'");
        nova_module_path_push(&path, seg);
    }
    return path;
}

static NovaDecl parse_fun_decl(NovaParser *parser, NovaToken name) {
    NovaDecl decl;
    decl.kind = NOVA_DECL_FUN;
    decl.as.fun_decl.name = name;
    decl.as.fun_decl.params = parse_param_list(parser);
    consume(parser, NOVA_TOKEN_RPAREN, "expected ')' after parameters");
    decl.as.fun_decl.has_return_type = false;
    if (match(parser, NOVA_TOKEN_COLON)) {
        decl.as.fun_decl.has_return_type = true;
        decl.as.fun_decl.return_type = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected return type");
    }
    consume(parser, NOVA_TOKEN_EQUAL, "expected '=' before function body");
    decl.as.fun_decl.body = parse_expression(parser);
    return decl;
}

static NovaDecl parse_let_decl(NovaParser *parser, NovaToken name) {
    NovaDecl decl;
    decl.kind = NOVA_DECL_LET;
    decl.as.let_decl.name = name;
    decl.as.let_decl.has_type = false;
    if (match(parser, NOVA_TOKEN_COLON)) {
        decl.as.let_decl.has_type = true;
        decl.as.let_decl.type_name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected type name");
    }
    consume(parser, NOVA_TOKEN_EQUAL, "expected '=' in let declaration");
    decl.as.let_decl.value = parse_expression(parser);
    return decl;
}

static NovaDecl parse_type_decl(NovaParser *parser, NovaToken name) {
    NovaDecl decl;
    decl.kind = NOVA_DECL_TYPE;
    decl.as.type_decl.name = name;
    decl.as.type_decl.has_body = false;
    if (match(parser, NOVA_TOKEN_EQUAL)) {
        decl.as.type_decl.has_body = true;
        // Skip until newline or semicolon for now (placeholder)
        while (!check(parser, NOVA_TOKEN_EOF) && !check(parser, NOVA_TOKEN_SEMICOLON) && !check(parser, NOVA_TOKEN_RBRACE)) {
            if (match(parser, NOVA_TOKEN_PIPE)) {
                continue;
            }
            advance_token(parser);
        }
    }
    return decl;
}

static void parse_declaration(NovaParser *parser, NovaProgram *program) {
    if (match(parser, NOVA_TOKEN_TYPE)) {
        NovaToken name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected type name");
        NovaDecl decl = parse_type_decl(parser, name);
        nova_program_add_decl(program, decl);
    } else if (match(parser, NOVA_TOKEN_FUN)) {
        NovaToken name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected function name");
        consume(parser, NOVA_TOKEN_LPAREN, "expected '(' after function name");
        NovaDecl decl = parse_fun_decl(parser, name);
        nova_program_add_decl(program, decl);
    } else if (match(parser, NOVA_TOKEN_LET)) {
        NovaToken name = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected binding name");
        NovaDecl decl = parse_let_decl(parser, name);
        nova_program_add_decl(program, decl);
    } else {
        advance_token(parser);
    }
}

static void parse_import(NovaParser *parser, NovaProgram *program) {
    consume(parser, NOVA_TOKEN_IMPORT, "expected 'import'");
    NovaModulePath path = parse_module_path(parser);
    NovaImportDecl import_decl;
    import_decl.path = path;
    import_decl.symbols = NULL;
    import_decl.symbol_count = import_decl.symbol_capacity = 0;
    if (match(parser, NOVA_TOKEN_LBRACE)) {
        while (!check(parser, NOVA_TOKEN_RBRACE)) {
            if (import_decl.symbol_count == import_decl.symbol_capacity) {
                size_t new_capacity = import_decl.symbol_capacity == 0 ? 4 : import_decl.symbol_capacity * 2;
                NovaToken *symbols = realloc(import_decl.symbols, new_capacity * sizeof(NovaToken));
                if (!symbols) break;
                import_decl.symbols = symbols;
                import_decl.symbol_capacity = new_capacity;
            }
            NovaToken symbol = consume(parser, NOVA_TOKEN_IDENTIFIER, "expected imported symbol name");
            import_decl.symbols[import_decl.symbol_count++] = symbol;
            if (!match(parser, NOVA_TOKEN_COMMA)) {
                break;
            }
        }
        consume(parser, NOVA_TOKEN_RBRACE, "expected '}' after import list");
    }
    nova_program_add_import(program, import_decl);
}

static void parse_module_decl(NovaParser *parser, NovaProgram *program) {
    consume(parser, NOVA_TOKEN_MODULE, "expected 'module'");
    program->module_decl.path = parse_module_path(parser);
}

void nova_parser_init(NovaParser *parser, const char *source) {
    parser->tokens = nova_lexer_tokenize(source);
    parser->current = 0;
    parser->source = source;
}

NovaProgram *nova_parser_parse(NovaParser *parser) {
    NovaProgram *program = (NovaProgram *)calloc(1, sizeof(NovaProgram));
    if (!program) return NULL;
    nova_program_init(program);
    parse_module_decl(parser, program);
    while (match(parser, NOVA_TOKEN_IMPORT)) {
        parser->current--; // rewind because parse_import consumes IMPORT
        parse_import(parser, program);
    }
    while (!is_at_end(parser)) {
        parse_declaration(parser, program);
        if (check(parser, NOVA_TOKEN_EOF)) break;
    }
    return program;
}

void nova_parser_free(NovaParser *parser) {
    nova_token_array_free(&parser->tokens);
}

