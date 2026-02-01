#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "nova/codegen.h"
#include "nova/ir.h"
#include "nova/parser.h"
#include "nova/semantic.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    size_t read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read != (size_t)size) {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    return buffer;
}

static void print_usage(const char *name) {
    fprintf(stderr, "Usage: %s [--strict] <file>...\n", name);
    fprintf(stderr, "  --strict  Treat semantic warnings as failures.\n");
}

static bool diagnostics_has_errors(const NovaDiagnosticList *list, size_t *warning_count, size_t *error_count) {
    bool has_error = false;
    size_t warnings = 0;
    size_t errors = 0;
    for (size_t i = 0; i < list->count; ++i) {
        if (list->items[i].severity == NOVA_DIAGNOSTIC_ERROR) {
            has_error = true;
            errors++;
        } else if (list->items[i].severity == NOVA_DIAGNOSTIC_WARNING) {
            warnings++;
        }
    }
    if (warning_count) *warning_count = warnings;
    if (error_count) *error_count = errors;
    return has_error;
}

static void ensure_build_dir(void) {
#ifdef _WIN32
    _mkdir("build");
#else
    mkdir("build", 0755);
#endif
}

static long nova_process_id(void) {
#ifdef _WIN32
    return (long)_getpid();
#else
    return (long)getpid();
#endif
}

static bool run_check(const char *path, bool strict) {
    char *source = read_file(path);
    if (!source) {
        fprintf(stderr, "nova-check: failed to read %s (%s)\n", path, strerror(errno));
        return false;
    }

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    if (!program || parser.had_error) {
        fprintf(stderr, "nova-check: parse failed for %s (%zu errors)\n", path, parser.diagnostics.count);
        nova_parser_free(&parser);
        free(source);
        return false;
    }

    NovaSemanticContext ctx;
    nova_semantic_context_init(&ctx);
    nova_semantic_analyze_program(&ctx, program);

    size_t warning_count = 0;
    size_t error_count = 0;
    bool has_error = diagnostics_has_errors(&ctx.diagnostics, &warning_count, &error_count);
    if (has_error || (strict && warning_count > 0)) {
        fprintf(stderr, "nova-check: semantic issues for %s (%zu errors, %zu warnings)\n", path, error_count, warning_count);
        nova_semantic_context_free(&ctx);
        nova_program_free(program);
        free(program);
        nova_parser_free(&parser);
        free(source);
        return false;
    }

    NovaIRProgram *ir = nova_ir_lower(program, &ctx);
    if (!ir) {
        fprintf(stderr, "nova-check: IR lowering failed for %s\n", path);
        nova_semantic_context_free(&ctx);
        nova_program_free(program);
        free(program);
        nova_parser_free(&parser);
        free(source);
        return false;
    }

    ensure_build_dir();
    char object_path[PATH_MAX];
    snprintf(object_path, sizeof(object_path), "build/nova_check_%ld.o", nova_process_id());
    char error[256];
    bool codegen_ok = nova_codegen_emit_object(ir, &ctx, object_path, error, sizeof(error));
    if (!codegen_ok) {
        fprintf(stderr, "nova-check: codegen failed for %s (%s)\n", path, error);
    }
    remove(object_path);

    nova_ir_free(ir);
    nova_semantic_context_free(&ctx);
    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
    free(source);

    if (!codegen_ok) {
        return false;
    }

    printf("nova-check: %s ok (warnings: %zu)\n", path, warning_count);
    return true;
}

int main(int argc, char **argv) {
    bool strict = false;
    int arg_start = 1;
    if (argc > 1 && strcmp(argv[1], "--strict") == 0) {
        strict = true;
        arg_start = 2;
    }
    if (argc <= arg_start) {
        print_usage(argv[0]);
        return 2;
    }

    bool all_ok = true;
    for (int i = arg_start; i < argc; ++i) {
        if (!run_check(argv[i], strict)) {
            all_ok = false;
        }
    }
    return all_ok ? 0 : 1;
}
