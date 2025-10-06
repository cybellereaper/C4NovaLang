#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "nova/token.h"

typedef enum {
    NOVA_EXPR_IF,
    NOVA_EXPR_MATCH,
    NOVA_EXPR_ASYNC,
    NOVA_EXPR_AWAIT,
    NOVA_EXPR_EFFECT,
    NOVA_EXPR_PIPE,
    NOVA_EXPR_CALL,
    NOVA_EXPR_IDENTIFIER,
    NOVA_EXPR_LITERAL,
    NOVA_EXPR_LAMBDA,
    NOVA_EXPR_PAREN,
    NOVA_EXPR_BLOCK,
    NOVA_EXPR_LIST
} NovaExprKind;

typedef enum {
    NOVA_LITERAL_NUMBER,
    NOVA_LITERAL_STRING,
    NOVA_LITERAL_BOOL,
    NOVA_LITERAL_UNIT
} NovaLiteralKind;

typedef struct NovaExpr NovaExpr;

typedef struct {
    NovaLiteralKind kind;
    NovaToken token;
} NovaLiteral;

typedef struct {
    NovaExpr **items;
    size_t count;
    size_t capacity;
} NovaExprList;

typedef struct {
    NovaToken name;
    bool has_type;
    NovaToken type_name;
} NovaParam;

typedef struct {
    NovaParam *items;
    size_t count;
    size_t capacity;
} NovaParamList;

typedef struct {
    NovaExpr *condition;
    NovaExpr *then_branch;
    NovaExpr *else_branch;
} NovaIfExpr;

typedef struct {
    NovaToken pattern;
    NovaParamList params;
    NovaExpr *body;
} NovaMatchArm;

typedef struct {
    NovaMatchArm *items;
    size_t count;
    size_t capacity;
} NovaMatchArmList;

typedef struct {
    NovaExpr *target;
    NovaExprList stages;
} NovaPipeExpr;

typedef struct {
    NovaToken callee;
    NovaExprList args;
} NovaCallExpr;

typedef struct {
    NovaParamList params;
    NovaExpr *body;
} NovaLambdaExpr;

typedef struct NovaBlock {
    NovaExprList expressions;
} NovaBlock;

struct NovaExpr {
    NovaExprKind kind;
    NovaToken start_token;
    union {
        NovaIfExpr if_expr;
        struct { NovaExpr *expr; } await_expr;
        struct { NovaExpr *expr; } effect_expr;
        struct { NovaExpr *expr; } async_expr;
        NovaPipeExpr pipe_expr;
        NovaCallExpr call_expr;
        NovaLiteral literal;
        NovaLambdaExpr lambda;
        NovaBlock block;
        NovaExpr *inner;
    } as;
};

typedef struct {
    NovaToken name;
    NovaParamList params;
    bool has_return_type;
    NovaToken return_type;
    NovaExpr *body;
} NovaFunDecl;

typedef struct {
    NovaToken name;
    bool has_type;
    NovaToken type_name;
    NovaExpr *value;
} NovaLetDecl;

typedef struct {
    NovaToken name;
    bool has_body;
} NovaTypeDecl;

typedef enum {
    NOVA_DECL_FUN,
    NOVA_DECL_LET,
    NOVA_DECL_TYPE
} NovaDeclKind;

typedef struct {
    NovaDeclKind kind;
    union {
        NovaFunDecl fun_decl;
        NovaLetDecl let_decl;
        NovaTypeDecl type_decl;
    } as;
} NovaDecl;

typedef struct {
    NovaToken *segments;
    size_t count;
    size_t capacity;
} NovaModulePath;

typedef struct {
    NovaModulePath path;
} NovaModuleDecl;

typedef struct {
    NovaModulePath path;
    NovaToken *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
} NovaImportDecl;

typedef struct {
    NovaModuleDecl module_decl;
    NovaImportDecl *imports;
    size_t import_count;
    size_t import_capacity;
    NovaDecl *decls;
    size_t decl_count;
    size_t decl_capacity;
} NovaProgram;

void nova_param_list_init(NovaParamList *list);
void nova_param_list_push(NovaParamList *list, NovaParam param);
void nova_param_list_free(NovaParamList *list);

void nova_expr_list_init(NovaExprList *list);
void nova_expr_list_push(NovaExprList *list, NovaExpr *expr);
void nova_expr_list_free(NovaExprList *list);

void nova_match_arm_list_init(NovaMatchArmList *list);
void nova_match_arm_list_push(NovaMatchArmList *list, NovaMatchArm arm);
void nova_match_arm_list_free(NovaMatchArmList *list);

void nova_module_path_init(NovaModulePath *path);
void nova_module_path_push(NovaModulePath *path, NovaToken segment);
void nova_module_path_free(NovaModulePath *path);

void nova_program_init(NovaProgram *program);
void nova_program_add_import(NovaProgram *program, NovaImportDecl import);
void nova_program_add_decl(NovaProgram *program, NovaDecl decl);
void nova_program_free(NovaProgram *program);

