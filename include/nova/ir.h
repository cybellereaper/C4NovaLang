#pragma once

#include "nova/ast.h"
#include "nova/semantic.h"

typedef enum {
    NOVA_IR_EXPR_NUMBER,
    NOVA_IR_EXPR_STRING,
    NOVA_IR_EXPR_BOOL,
    NOVA_IR_EXPR_IDENTIFIER,
    NOVA_IR_EXPR_CALL,
} NovaIRExprKind;

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
        NovaToken identifier;
        struct {
            NovaToken callee;
            NovaIRExpr **args;
            size_t arg_count;
        } call;
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

