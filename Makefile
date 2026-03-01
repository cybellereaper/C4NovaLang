CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Iinclude
SRC=$(wildcard src/*.c)
TOOLS=build/nova-fmt build/nova-repl build/nova-lsp build/nova-new build/nova-check
BENCH=build/nova-bench
VERSION?=$(shell git describe --tags --always)
RELEASE_TARGET?=linux-x86_64

all: build/tests $(TOOLS)

build/tests: $(SRC) tests/parser_tests.c | build build/nova-check
	$(CC) $(CFLAGS) $^ -o $@

build/nova-fmt: $(SRC) tools/nova_fmt.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_fmt.c -o $@

build/nova-repl: $(SRC) tools/nova_repl.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_repl.c -o $@

build/nova-lsp: $(SRC) tools/nova_lsp.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_lsp.c -o $@

build/nova-new: $(SRC) tools/nova_new.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_new.c -o $@

build/nova-check: $(SRC) tools/nova_check.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_check.c -o $@

$(BENCH): $(SRC) tools/nova_bench.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_bench.c -o $@

bench: $(BENCH)
	@mkdir -p bench/results
	@GIT_COMMIT=$$(git rev-parse --short HEAD 2>/dev/null || echo unknown) ./$(BENCH) --output bench/results/latest.json
	@cat bench/results/latest.json

bench-verify: bench
	@python3 scripts/check_bench_regression.py bench/baseline.json bench/results/latest.json $${BENCH_THRESHOLD_PERCENT:-25}

build:
	mkdir -p build

release:
	./scripts/build_release.sh --target $(RELEASE_TARGET) $(VERSION)

clean:
	rm -rf build

.PHONY: all clean bench bench-verify
