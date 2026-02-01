#pragma once

#include "nova/ast.h"
#include "nova/semantic.h"

struct NovaIRExpr;

typedef enum {
    NOVA_IR_EXPR_NUMBER,
    NOVA_IR_EXPR_STRING,
    NOVA_IR_EXPR_BOOL,
    NOVA_IR_EXPR_UNIT,
    NOVA_IR_EXPR_IDENTIFIER,
    NOVA_IR_EXPR_CALL,
    NOVA_IR_EXPR_SEQUENCE,
    NOVA_IR_EXPR_LIST,
    NOVA_IR_EXPR_IF,
    NOVA_IR_EXPR_WHILE,
    NOVA_IR_EXPR_MATCH,
} NovaIRExprKind;

typedef struct {
    NovaToken constructor;
    NovaToken *bindings;
    size_t binding_count;
    struct NovaIRExpr *body;
} NovaIRMatchArm;

typedef struct NovaIRExpr NovaIRExpr;

typedef struct {
    NovaToken name;
    NovaTypeId type;
} NovaIRParam;

struct NovaIRExpr {
    NovaIRExprKind kind;
    NovaTypeId type;
    union {
        double number_value;
        struct {
            char *text;
        } string_value;
        bool bool_value;
        struct {
            struct NovaIRExpr **elements;
            size_t count;
        } list;
        struct {
            struct NovaIRExpr **items;
            size_t count;
        } sequence;
        NovaToken identifier;
        struct {
            NovaToken callee;
            NovaIRExpr **args;
            size_t arg_count;
        } call;
        struct {
            NovaIRExpr *condition;
            NovaIRExpr *then_branch;
            NovaIRExpr *else_branch;
        } if_expr;
        struct {
            NovaIRExpr *condition;
            NovaIRExpr *body;
        } while_expr;
        struct {
            NovaIRExpr *scrutinee;
            NovaIRMatchArm *arms;
            size_t arm_count;
        } match_expr;
    } as;
};

typedef struct {
    NovaToken name;
    NovaIRParam *params;
    size_t param_count;
    NovaTypeId return_type;
    NovaEffectMask effects;
    NovaIRExpr *body;
} NovaIRFunction;

typedef struct {
    NovaIRFunction *functions;
    size_t function_count;
    size_t function_capacity;
} NovaIRProgram;

NovaIRProgram *nova_ir_lower(const NovaProgram *program, const NovaSemanticContext *semantics);
void nova_ir_free(NovaIRProgram *program);
