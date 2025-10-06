#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "nova/token.h"

struct NovaExpr;
struct NovaTypeRef;

typedef struct NovaExpr NovaExpr;
typedef struct NovaBlock NovaBlock;

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
    bool has_label;
    NovaToken label;
    NovaExpr *value;
} NovaArg;

typedef struct {
    NovaArg *items;
    size_t count;
    size_t capacity;
} NovaArgList;

typedef struct {
    NovaExpr **items;
    size_t count;
    size_t capacity;
} NovaExprList;

typedef enum {
    NOVA_LITERAL_NUMBER,
    NOVA_LITERAL_STRING,
    NOVA_LITERAL_BOOL,
    NOVA_LITERAL_UNIT,
    NOVA_LITERAL_LIST,
} NovaLiteralKind;

typedef struct {
    NovaLiteralKind kind;
    NovaToken token;
    NovaExprList elements;
} NovaLiteral;

typedef struct {
    NovaToken name;
} NovaIdentifier;

typedef struct {
    NovaExpr *condition;
    NovaExpr *then_branch;
    NovaExpr *else_branch; // nullable
} NovaIfExpr;

typedef struct {
    NovaExpr *value;
} NovaUnaryExpr;

typedef struct {
    NovaExpr *target;
    NovaExprList stages;
} NovaPipeExpr;

typedef struct {
    NovaExpr *callee;
    NovaArgList args;
} NovaCallExpr;

typedef struct {
    NovaParamList params;
    NovaExpr *body;
    bool body_is_block;
} NovaLambdaExpr;

struct NovaBlock {
    NovaExprList expressions;
};

typedef struct {
    NovaToken name;
    NovaParamList bindings;
    NovaExpr *body;
} NovaMatchArm;

typedef struct {
    NovaMatchArm *items;
    size_t count;
    size_t capacity;
} NovaMatchArmList;

typedef struct {
    NovaExpr *scrutinee;
    NovaMatchArmList arms;
} NovaMatchExpr;

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
    NOVA_EXPR_BLOCK,
    NOVA_EXPR_LIST_LITERAL,
    NOVA_EXPR_PAREN,
} NovaExprKind;

struct NovaExpr {
    NovaExprKind kind;
    NovaToken start_token;
    union {
        NovaIfExpr if_expr;
        NovaMatchExpr match_expr;
        NovaUnaryExpr unary;
        NovaPipeExpr pipe;
        NovaCallExpr call;
        NovaIdentifier identifier;
        NovaLiteral literal;
        NovaLambdaExpr lambda;
        NovaBlock block;
        NovaExpr *inner;
    } as;
};

typedef struct {
    NovaToken name;
    bool has_type;
    NovaToken type_name;
    NovaExpr *value;
} NovaLetDecl;

typedef struct {
    NovaToken name;
    NovaParamList params;
    bool has_return_type;
    NovaToken return_type;
    NovaExpr *body;
} NovaFunDecl;

typedef struct {
    NovaToken name;
    NovaParamList payload;
} NovaVariantDecl;

typedef struct {
    NovaVariantDecl *items;
    size_t count;
    size_t capacity;
} NovaVariantList;

typedef enum {
    NOVA_TYPE_DECL_SUM,
    NOVA_TYPE_DECL_TUPLE,
} NovaTypeDeclKind;

typedef struct {
    NovaToken name;
    NovaTypeDeclKind kind;
    NovaVariantList variants; // for sum types
    NovaParamList tuple_fields; // for tuple types
} NovaTypeDecl;

typedef enum {
    NOVA_DECL_LET,
    NOVA_DECL_FUN,
    NOVA_DECL_TYPE,
} NovaDeclKind;

typedef struct {
    NovaDeclKind kind;
    union {
        NovaLetDecl let_decl;
        NovaFunDecl fun_decl;
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

void nova_arg_list_init(NovaArgList *list);
void nova_arg_list_push(NovaArgList *list, NovaArg arg);
void nova_arg_list_free(NovaArgList *list);

void nova_expr_list_init(NovaExprList *list);
void nova_expr_list_push(NovaExprList *list, NovaExpr *expr);
void nova_expr_list_free(NovaExprList *list);

void nova_match_arm_list_init(NovaMatchArmList *list);
void nova_match_arm_list_push(NovaMatchArmList *list, NovaMatchArm arm);
void nova_match_arm_list_free(NovaMatchArmList *list);

void nova_variant_list_init(NovaVariantList *list);
void nova_variant_list_push(NovaVariantList *list, NovaVariantDecl variant);
void nova_variant_list_free(NovaVariantList *list);

void nova_module_path_init(NovaModulePath *path);
void nova_module_path_push(NovaModulePath *path, NovaToken segment);
void nova_module_path_free(NovaModulePath *path);

void nova_program_init(NovaProgram *program);
void nova_program_add_import(NovaProgram *program, NovaImportDecl import);
void nova_program_add_decl(NovaProgram *program, NovaDecl decl);
void nova_program_free(NovaProgram *program);

