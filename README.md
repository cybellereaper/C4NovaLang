# NovaLang Toolchain Bootstrap

This repository contains an ANTLR4 grammar (`nova.g4`) for the NovaLang surface
syntax and now includes the first steps toward a C-based reference
implementation. The initial milestone focuses on getting source text tokenised,
parsed into an abstract syntax tree (AST), and analysed by an embryonic
semantic layer.

## What Exists Today

* `nova.g4` — source grammar that remains the authoritative description of the
  language syntax.
* `scripts/generate_tokens.py` — lightweight generator that extracts token
  metadata from `nova.g4` and emits `include/nova/generated_tokens.h`, ensuring
  the handwritten lexer stays aligned with the grammar.
* `include/` & `src/` — C headers and implementations for:
  * Token infrastructure (`nova/token.h`, `src/token.c`).
  * Lexer (`nova/lexer.h`, `src/lexer.c`) translating the ANTLR tokens into a
    stream of `NovaToken` structures.
  * AST data structures (`nova/ast.h`, `src/ast.c`).
  * Recursive-descent parser (`nova/parser.h`, `src/parser.c`) mirroring the
    grammar rules and producing a `NovaProgram` tree.
  * Early semantic analysis (`nova/semantic.h`, `src/semantic.c`) that performs
    scope tracking, literal typing, and simple diagnostics for duplicate or
    unknown identifiers.
* `tests/parser_tests.c` — smoke tests that exercise the lexer, parser, and
  semantic pass, validating duplicate-definition detection and a well-formed
  sample program.
* `Makefile` — builds the test binary with `gcc`.

## Running the Tests

```
make
./build/tests
```

The test suite constructs small NovaLang snippets, parses them, and runs the
semantic analyser to ensure basic invariants hold.

## Next Steps

1. Flesh out the AST to capture the full richness of `nova.g4`, including proper
   variant/type declarations and match arms.
2. Improve the parser with richer error recovery and correct handling for all
   expression forms (especially pipelines, async/await, and blocks) per the
   grammar.
3. Expand semantic analysis with real type inference, effect tracking, and
   exhaustiveness checking for pattern matches.
4. Introduce a typed intermediate representation and code generation path to
   native object files.
5. Layer on developer tooling (formatter, REPL, eventually an LSP server) once
   the core compiler pipeline is stable.

These steps continue the march toward the full NovaLang toolchain described in
the original request.
