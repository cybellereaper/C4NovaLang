#include "nova/ast.h"

#include <stdlib.h>
#include <string.h>

static void *nova_realloc(void *ptr, size_t new_count, size_t element_size) {
    return realloc(ptr, new_count * element_size);
}

void nova_param_list_init(NovaParamList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void nova_param_list_push(NovaParamList *list, NovaParam param) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        NovaParam *new_items = nova_realloc(list->items, new_capacity, sizeof(NovaParam));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = param;
}

void nova_param_list_free(NovaParamList *list) {
    free(list->items);
    list->items = NULL;
    list->count = list->capacity = 0;
}

void nova_expr_list_init(NovaExprList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void nova_expr_list_push(NovaExprList *list, NovaExpr *expr) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        NovaExpr **new_items = nova_realloc(list->items, new_capacity, sizeof(NovaExpr *));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = expr;
}

void nova_expr_list_free(NovaExprList *list) {
    free(list->items);
    list->items = NULL;
    list->count = list->capacity = 0;
}

void nova_match_arm_list_init(NovaMatchArmList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void nova_match_arm_list_push(NovaMatchArmList *list, NovaMatchArm arm) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 2 : list->capacity * 2;
        NovaMatchArm *new_items = nova_realloc(list->items, new_capacity, sizeof(NovaMatchArm));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = arm;
}

void nova_match_arm_list_free(NovaMatchArmList *list) {
    for (size_t i = 0; i < list->count; ++i) {
        nova_param_list_free(&list->items[i].params);
    }
    free(list->items);
    list->items = NULL;
    list->count = list->capacity = 0;
}

void nova_module_path_init(NovaModulePath *path) {
    path->segments = NULL;
    path->count = 0;
    path->capacity = 0;
}

void nova_module_path_push(NovaModulePath *path, NovaToken segment) {
    if (path->count == path->capacity) {
        size_t new_capacity = path->capacity == 0 ? 4 : path->capacity * 2;
        NovaToken *new_segments = nova_realloc(path->segments, new_capacity, sizeof(NovaToken));
        if (!new_segments) return;
        path->segments = new_segments;
        path->capacity = new_capacity;
    }
    path->segments[path->count++] = segment;
}

void nova_module_path_free(NovaModulePath *path) {
    free(path->segments);
    path->segments = NULL;
    path->count = path->capacity = 0;
}

void nova_program_init(NovaProgram *program) {
    nova_module_path_init(&program->module_decl.path);
    program->imports = NULL;
    program->import_count = 0;
    program->import_capacity = 0;
    program->decls = NULL;
    program->decl_count = 0;
    program->decl_capacity = 0;
}

void nova_program_add_import(NovaProgram *program, NovaImportDecl import) {
    if (program->import_count == program->import_capacity) {
        size_t new_capacity = program->import_capacity == 0 ? 2 : program->import_capacity * 2;
        NovaImportDecl *new_imports = nova_realloc(program->imports, new_capacity, sizeof(NovaImportDecl));
        if (!new_imports) return;
        program->imports = new_imports;
        program->import_capacity = new_capacity;
    }
    program->imports[program->import_count++] = import;
}

void nova_program_add_decl(NovaProgram *program, NovaDecl decl) {
    if (program->decl_count == program->decl_capacity) {
        size_t new_capacity = program->decl_capacity == 0 ? 4 : program->decl_capacity * 2;
        NovaDecl *new_decls = nova_realloc(program->decls, new_capacity, sizeof(NovaDecl));
        if (!new_decls) return;
        program->decls = new_decls;
        program->decl_capacity = new_capacity;
    }
    program->decls[program->decl_count++] = decl;
}

void nova_program_free(NovaProgram *program) {
    for (size_t i = 0; i < program->import_count; ++i) {
        nova_module_path_free(&program->imports[i].path);
        free(program->imports[i].symbols);
    }
    free(program->imports);
    for (size_t i = 0; i < program->decl_count; ++i) {
        NovaDecl *decl = &program->decls[i];
        switch (decl->kind) {
        case NOVA_DECL_FUN:
            nova_param_list_free(&decl->as.fun_decl.params);
            break;
        case NOVA_DECL_LET:
            break;
        case NOVA_DECL_TYPE:
            break;
        }
    }
    free(program->decls);
    nova_module_path_free(&program->module_decl.path);
    program->imports = NULL;
    program->decls = NULL;
    program->import_count = program->import_capacity = 0;
    program->decl_count = program->decl_capacity = 0;
}

