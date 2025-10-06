#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<USAGE
Usage: $(basename "$0") [version]

Builds NovaLang binaries and packages them into a tarball inside the dist/
folder. If no version is provided, the script uses the output of
`git describe --tags --always`.
USAGE
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT_DIR"

if ! command -v git >/dev/null 2>&1; then
  echo "git is required to determine the release version" >&2
  exit 1
fi

VERSION=${1:-$(git describe --tags --always)}
TARGET="linux-x86_64"
DIST_DIR="$ROOT_DIR/dist/$VERSION"
ARCHIVE_NAME="novalang-${VERSION}-${TARGET}.tar.gz"
ARCHIVE_PATH="$DIST_DIR/$ARCHIVE_NAME"
CHECKSUM_PATH="$ARCHIVE_PATH.sha256"

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

make clean
make all

BIN_DIR="$DIST_DIR/bin"
mkdir -p "$BIN_DIR"

copy_binary() {
  local src=$1
  if [[ -f "$src" ]]; then
    local dest="$BIN_DIR/$(basename "$src")"
    cp "$src" "$dest"
    if command -v strip >/dev/null 2>&1; then
      strip --strip-debug "$dest" || true
    fi
    chmod 755 "$dest"
  else
    echo "warning: expected binary '$src' was not built" >&2
  fi
}

copy_binary "build/nova-fmt"
copy_binary "build/nova-repl"
copy_binary "build/nova-lsp"
copy_binary "build/nova-new"
copy_binary "build/tests"

cp README.md "$DIST_DIR/README.md"
cp nova.g4 "$DIST_DIR/nova.g4"

cat > "$DIST_DIR/RELEASE-NOTES.md" <<NOTES
# NovaLang ${VERSION}

This archive contains the precompiled NovaLang developer tooling for
${TARGET}. To install, extract the archive and add the `bin/` directory to
your PATH.

## Contents

- nova-fmt — source formatter
- nova-repl — interactive REPL with type reporting
- nova-lsp — minimal Language Server Protocol implementation
- nova-new — project scaffolding tool
- tests — parser and semantic regression test binary

Run `./bin/tests` to execute the end-to-end regression tests.
NOTES

(
  cd "$DIST_DIR"
  tar -czf "$ARCHIVE_NAME" README.md nova.g4 RELEASE-NOTES.md bin
)

if command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$ARCHIVE_PATH" > "$CHECKSUM_PATH"
else
  openssl dgst -sha256 "$ARCHIVE_PATH" > "$CHECKSUM_PATH"
fi

echo "Created $ARCHIVE_PATH"
echo "Checksum written to $CHECKSUM_PATH"
