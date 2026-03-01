# Nova Performance Log

## Current execution model summary

- Target platform: Linux x86_64 (CI + local baseline)
- Execution model: AOT compiler path (C or LLVM codegen)
- Type system: static with semantic analysis annotations
- Memory model: manual memory management + optional GC support module

## Bottleneck ranking (current)

1. **Lexer token scanning** for long identifiers/numeric literals (tight per-character loop).
2. **Parser + semantic pipeline throughput** on generated workloads with many declarations.
3. **IR lowering throughput** on call-heavy and control-flow-heavy modules.

## Optimization shipped in this change

- Reworked identifier and numeric lexing loops to use direct indexed scanning instead of repeated `peek`/`advance` calls.
- This reduces per-character function-call overhead in the lexer hot path.

## Tradeoffs

- Slightly more duplicated position/column update logic inside hot loops.
- No behavior changes expected; covered by existing parser/codegen tests.

## Follow-up roadmap

1. Add stage-level tracing (`parse`, `semantic`, `ir`) in benchmark output JSON.
2. Introduce allocation profiling counters in parser + IR builders.
3. Add a nightly `perf` sampling workflow and commit profile artifacts.
4. Tighten regression threshold from 25% to 10% after gathering stable machine baselines.
