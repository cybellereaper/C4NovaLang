#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nova/lexer.h"
#include "nova/parser.h"

static char *read_all(FILE *in) {
    size_t capacity = 1024;
    size_t length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) return NULL;
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        buffer[length++] = (char)c;
    }
    buffer[length] = '\0';
    return buffer;
}

static void write_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; ++i) {
        fputs("    ", out);
    }
}

static void format_tokens(const NovaTokenArray *tokens, FILE *out) {
    int indent = 0;
    bool new_line = true;
    for (size_t i = 0; i < tokens->size; ++i) {
        NovaToken token = tokens->data[i];
        if (token.type == NOVA_TOKEN_EOF) break;
        switch (token.type) {
        case NOVA_TOKEN_RBRACE:
            indent = indent > 0 ? indent - 1 : 0;
            if (!new_line) {
                fputc('\n', out);
            }
            write_indent(out, indent);
            fwrite(token.lexeme, 1, token.length, out);
            new_line = true;
            fputc('\n', out);
            break;
        case NOVA_TOKEN_LBRACE:
            if (!new_line) {
                fputc('\n', out);
            }
            write_indent(out, indent);
            fwrite(token.lexeme, 1, token.length, out);
            fputc('\n', out);
            indent++;
            new_line = true;
            break;
        case NOVA_TOKEN_SEMICOLON:
            fwrite(token.lexeme, 1, token.length, out);
            fputc('\n', out);
            new_line = true;
            break;
        default:
            if (new_line) {
                write_indent(out, indent);
                new_line = false;
            } else {
                fputc(' ', out);
            }
            fwrite(token.lexeme, 1, token.length, out);
            break;
        }
    }
    if (!new_line) {
        fputc('\n', out);
    }
}

int main(int argc, char **argv) {
    FILE *in = stdin;
    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (!in) {
            fprintf(stderr, "nova-fmt: failed to open %s\n", argv[1]);
            return 1;
        }
    }
    char *source = read_all(in);
    if (in != stdin) fclose(in);
    if (!source) {
        fprintf(stderr, "nova-fmt: failed to read input\n");
        return 1;
    }

    NovaParser parser;
    nova_parser_init(&parser, source);
    NovaProgram *program = nova_parser_parse(&parser);
    if (!program || parser.had_error) {
        fprintf(stderr, "nova-fmt: parse failed with %zu errors\n", parser.diagnostics.count);
        free(source);
        nova_parser_free(&parser);
        return 1;
    }

    format_tokens(&parser.tokens, stdout);

    nova_program_free(program);
    free(program);
    nova_parser_free(&parser);
    free(source);
    return 0;
}

