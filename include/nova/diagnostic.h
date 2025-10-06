#pragma once

#include "nova/token.h"

typedef enum {
    NOVA_DIAGNOSTIC_ERROR,
    NOVA_DIAGNOSTIC_WARNING,
} NovaDiagnosticSeverity;

typedef struct {
    NovaToken token;
    const char *message;
    NovaDiagnosticSeverity severity;
} NovaDiagnostic;

typedef struct {
    NovaDiagnostic *items;
    size_t count;
    size_t capacity;
} NovaDiagnosticList;

void nova_diagnostic_list_init(NovaDiagnosticList *list);
void nova_diagnostic_list_push(NovaDiagnosticList *list, NovaDiagnostic diagnostic);
void nova_diagnostic_list_free(NovaDiagnosticList *list);

