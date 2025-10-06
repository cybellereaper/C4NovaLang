CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Iinclude
SRC=$(wildcard src/*.c)

all: build/tests

build/tests: $(SRC) tests/parser_tests.c | build
	$(CC) $(CFLAGS) $^ -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean
