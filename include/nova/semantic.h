#pragma once

#include "nova/ast.h"

typedef enum {
    NOVA_TYPE_UNKNOWN,
    NOVA_TYPE_NUMBER,
    NOVA_TYPE_STRING,
    NOVA_TYPE_BOOL,
    NOVA_TYPE_FUNCTION,
    NOVA_TYPE_UNIT,
    NOVA_TYPE_LIST
} NovaTypeKind;

typedef struct {
    NovaTypeKind kind;
} NovaType;

typedef struct {
    NovaToken token;
    const char *message;
} NovaDiagnostic;

typedef struct {
    NovaDiagnostic *items;
    size_t count;
    size_t capacity;
} NovaDiagnosticList;

typedef struct NovaScopeEntry {
    NovaToken name;
    NovaType type;
    struct NovaScopeEntry *next;
} NovaScopeEntry;

typedef struct NovaScope {
    NovaScopeEntry *entries;
    struct NovaScope *parent;
} NovaScope;

typedef struct {
    NovaScope *scope;
    NovaDiagnosticList diagnostics;
} NovaSemanticContext;

void nova_semantic_context_init(NovaSemanticContext *ctx);
void nova_semantic_context_free(NovaSemanticContext *ctx);
void nova_semantic_analyze_program(NovaSemanticContext *ctx, const NovaProgram *program);

