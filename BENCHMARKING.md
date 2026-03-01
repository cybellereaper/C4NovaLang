# Benchmarking Nova

This repository includes a reproducible benchmark harness focused on compiler pipeline speed.

## Benchmarks

`make bench` runs the following versioned workloads:

- `startup`
- `json_parse`
- `hashmap_hotloop`
- `numeric_loop`
- `concurrency_pingpong`

Each benchmark executes parse + semantic + IR lowering and emits `bench/results/latest.json`.

## Reproduction checklist

1. Use a dedicated machine and close background workloads.
2. Pin CPU frequency/governor (Linux example):
   - `sudo cpupower frequency-set --governor performance`
3. Pin benchmark process to one core:
   - `taskset -c 2 make bench`
4. Run at least 3 times and compare medians.

## Baseline metadata

When updating `bench/baseline.json`, include:

- Commit hash
- CPU model / core count
- OS and kernel version
- Compiler version and flags

Current benchmark flags inherit `CFLAGS` from `Makefile`.

## Regression gate

Use:

- `make bench-verify`

This compares `bench/results/latest.json` against `bench/baseline.json` using
`BENCH_THRESHOLD_PERCENT` (default `25`).
