CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Iinclude
SRC=$(wildcard src/*.c)
TOOLS=build/nova-fmt build/nova-repl build/nova-lsp build/nova-new

all: build/tests $(TOOLS)

build/tests: $(SRC) tests/parser_tests.c | build
	$(CC) $(CFLAGS) $^ -o $@

build/nova-fmt: $(SRC) tools/nova_fmt.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_fmt.c -o $@

build/nova-repl: $(SRC) tools/nova_repl.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_repl.c -o $@

build/nova-lsp: $(SRC) tools/nova_lsp.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_lsp.c -o $@

build/nova-new: $(SRC) tools/nova_new.c | build
	$(CC) $(CFLAGS) $(SRC) tools/nova_new.c -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean
