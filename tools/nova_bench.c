#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nova/codegen.h"
#include "nova/ir.h"
#include "nova/parser.h"
#include "nova/semantic.h"

typedef struct {
    const char *name;
    const char *source;
    size_t iterations;
    double elapsed_ms;
} BenchCase;

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool run_compile_pipeline(const char *source) {
    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    if (!program) return false;

    NovaSemanticContext semantics;
    nova_semantic_context_init(&semantics);
    nova_semantic_analyze_program(&semantics, program);
    bool has_error = false;
    for (size_t i = 0; i < semantics.diagnostics.count; ++i) {
        if (semantics.diagnostics.items[i].severity == NOVA_DIAGNOSTIC_ERROR) {
            has_error = true;
            break;
        }
    }
    if (has_error) {
        nova_semantic_context_free(&semantics);
        nova_program_free(program);
        return false;
    }

    NovaIRProgram *ir = nova_ir_lower(program, &semantics);
    if (ir) {
        nova_ir_free(ir);
    }

    nova_semantic_context_free(&semantics);
    nova_program_free(program);
    return ir != NULL;
}

static bool run_benchmark(BenchCase *bench) {
    double start = now_ms();
    for (size_t i = 0; i < bench->iterations; ++i) {
        if (!run_compile_pipeline(bench->source)) {
            return false;
        }
    }
    bench->elapsed_ms = now_ms() - start;
    return true;
}

static void print_json(FILE *out, BenchCase *benches, size_t count, const char *git_commit) {
    fprintf(out, "{\n");
    fprintf(out, "  \"commit\": \"%s\",\n", git_commit ? git_commit : "unknown");
    fprintf(out, "  \"benchmarks\": {\n");
    for (size_t i = 0; i < count; ++i) {
        double ns_per_op = (benches[i].elapsed_ms * 1000000.0) / (double)benches[i].iterations;
        fprintf(out,
                "    \"%s\": { \"iterations\": %zu, \"elapsed_ms\": %.3f, \"ns_per_op\": %.1f }%s\n",
                benches[i].name,
                benches[i].iterations,
                benches[i].elapsed_ms,
                ns_per_op,
                (i + 1 == count) ? "" : ",");
    }
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

int main(int argc, char **argv) {
    const char *output_path = NULL;
    const char *commit = getenv("GIT_COMMIT");
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        }
    }

    static const char *startup_program =
        "module bench.startup\n"
        "fun main(): Number = 1\n";

    static const char *json_parse_program =
        "module bench.json\n"
        "type Json = Num(Number) | Str(String) | Arr(List[Json]) | Obj(List[String]) | Null\n"
        "fun parse_small(): Number = match Num(1) { Num(v) -> v; Str(_) -> 0; Arr(_) -> 0; Obj(_) -> 0; Null -> 0 }\n";

    static const char *hashmap_hotloop_program =
        "module bench.hashmap\n"
        "fun id(x: Number): Number = x\n"
        "fun hot(): Number = 1 |> id |> id |> id |> id |> id |> id |> id |> id |> id\n";

    static const char *numeric_loop_program =
        "module bench.numeric\n"
        "fun acc(v: Number): Number = if true { v } else { 0 }\n"
        "fun hot(): Number = acc(1) + acc(2) + acc(3)\n";

    static const char *concurrency_pingpong_program =
        "module bench.concurrent\n"
        "fun ping(): Number = async { 1 }\n"
        "fun pong(): Number = await ping()\n";

    BenchCase benches[] = {
        {.name = "startup", .source = startup_program, .iterations = 2500},
        {.name = "json_parse", .source = json_parse_program, .iterations = 1500},
        {.name = "hashmap_hotloop", .source = hashmap_hotloop_program, .iterations = 1500},
        {.name = "numeric_loop", .source = numeric_loop_program, .iterations = 2000},
        {.name = "concurrency_pingpong", .source = concurrency_pingpong_program, .iterations = 1500},
    };

    size_t bench_count = sizeof(benches) / sizeof(benches[0]);
    for (size_t i = 0; i < bench_count; ++i) {
        if (!run_benchmark(&benches[i])) {
            fprintf(stderr, "benchmark %s failed\n", benches[i].name);
            return 1;
        }
    }

    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "failed to open %s\n", output_path);
            return 1;
        }
    }

    print_json(out, benches, bench_count, commit);

    if (out != stdout) {
        fclose(out);
    }

    return 0;
}
