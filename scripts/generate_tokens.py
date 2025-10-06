#!/usr/bin/env python3
"""Generate token metadata from nova.g4 grammar."""
from __future__ import annotations
import pathlib
import re

def parse_tokens(grammar: str):
    token_pattern = re.compile(r"^(?P<name>[A-Z][A-Z0-9_]*)\s*:\s*(?P<value>.+?);\s*$")
    tokens = []
    for line in grammar.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        m = token_pattern.match(line)
        if m:
            tokens.append((m.group("name"), m.group("value")))
    return tokens

def write_header(tokens, output: pathlib.Path):
    with output.open("w", encoding="utf8") as f:
        f.write("// Auto-generated from nova.g4. Do not edit manually.\n")
        f.write("#pragma once\n\n")
        f.write("#define NOVA_TOKEN_KEYWORD_COUNT %d\n" % sum(1 for name, _ in tokens if name.isalpha() and name.isupper() and name not in {"NUMBER", "STRING", "ID", "COMMENT", "WS"}))
        f.write("static const char *const nova_token_names[] = {\n")
        for name, _ in tokens:
            f.write(f"    \"{name}\",\n")
        f.write("};\n\n")
        f.write("static const char *const nova_keyword_lexemes[] = {\n")
        for name, value in tokens:
            if name in {"NUMBER", "STRING", "ID", "COMMENT", "WS"}:
                continue
            if value.startswith("'"):
                literal = value.strip("'")
                if literal and not any(c in literal for c in "[](){}+*?\\"):
                    f.write(f"    \"{literal}\",\n")
        f.write("};\n")

def main():
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    grammar_path = repo_root / "nova.g4"
    output_path = repo_root / "include" / "nova" / "generated_tokens.h"
    tokens = parse_tokens(grammar_path.read_text(encoding="utf8"))
    write_header(tokens, output_path)

if __name__ == "__main__":
    main()
