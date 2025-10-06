#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<USAGE
Usage: $(basename "$0") [--target <platform>] [version]

Builds NovaLang binaries and packages them into an archive inside the dist/
folder. If no version is provided, the script uses the output of
`git describe --tags --always`. The supported targets are:

  * linux-x86_64 (default)
  * windows-x86_64
USAGE
}

TARGET="linux-x86_64"
VERSION=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -t|--target)
      if [[ $# -lt 2 ]]; then
        echo "error: --target requires a value" >&2
        exit 1
      fi
      TARGET="$2"
      shift 2
      ;;
    *)
      if [[ -n "$VERSION" ]]; then
        echo "error: multiple version arguments provided" >&2
        exit 1
      fi
      VERSION="$1"
      shift
      ;;
  esac
done

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT_DIR"

if ! command -v git >/dev/null 2>&1; then
  echo "git is required to determine the release version" >&2
  exit 1
fi

VERSION=${VERSION:-$(git describe --tags --always)}

case "$TARGET" in
  linux-x86_64)
    ARCHIVE_EXT="tar.gz"
    ;;
  windows-x86_64)
    ARCHIVE_EXT="zip"
    ;;
  *)
    echo "error: unsupported target '$TARGET'" >&2
    exit 1
    ;;
esac

DIST_ROOT="$ROOT_DIR/dist/$VERSION"
TARGET_DIR="$DIST_ROOT/$TARGET"
ARCHIVE_NAME="novalang-${VERSION}-${TARGET}.${ARCHIVE_EXT}"
ARCHIVE_PATH="$DIST_ROOT/$ARCHIVE_NAME"
CHECKSUM_PATH="$ARCHIVE_PATH.sha256"

rm -rf "$TARGET_DIR"
mkdir -p "$DIST_ROOT"
mkdir -p "$TARGET_DIR"
rm -f "$ARCHIVE_PATH" "$CHECKSUM_PATH"

make clean
make all

BIN_DIR="$TARGET_DIR/bin"
mkdir -p "$BIN_DIR"

copy_binary() {
  local src="$1"
  if [[ -f "$src" ]]; then
    :
  elif [[ -f "${src}.exe" ]]; then
    src="${src}.exe"
  else
    echo "warning: expected binary '$1' was not built" >&2
    return
  fi
  local dest="$BIN_DIR/$(basename "$src")"
  cp "$src" "$dest"
  if command -v strip >/dev/null 2>&1 && [[ "$TARGET" != windows-* ]]; then
    strip --strip-debug "$dest" || true
  fi
  chmod 755 "$dest" 2>/dev/null || true
}

copy_binary "build/nova-fmt"
copy_binary "build/nova-repl"
copy_binary "build/nova-lsp"
copy_binary "build/nova-new"
copy_binary "build/tests"

cp README.md "$TARGET_DIR/README.md"
cp nova.g4 "$TARGET_DIR/nova.g4"

cat > "$TARGET_DIR/RELEASE-NOTES.md" <<NOTES
# NovaLang ${VERSION}

This archive contains the precompiled NovaLang developer tooling for
${TARGET}. To install, extract the archive and add the \`bin/\` directory to
your PATH.

## Contents

- nova-fmt — source formatter
- nova-repl — interactive REPL with type reporting
- nova-lsp — minimal Language Server Protocol implementation
- nova-new — project scaffolding tool
- tests — parser and semantic regression test binary

Run \`./bin/tests\` to execute the end-to-end regression tests.
NOTES

if [[ "$TARGET" == "linux-x86_64" ]]; then
  tar -czf "$ARCHIVE_PATH" -C "$TARGET_DIR" .
else
  TARGET_DIR="$TARGET_DIR" ARCHIVE_PATH="$ARCHIVE_PATH" python - <<'PY'
import os
import pathlib
import zipfile

target_dir = pathlib.Path(os.environ["TARGET_DIR"]).resolve()
archive_path = pathlib.Path(os.environ["ARCHIVE_PATH"]).resolve()
archive_path.parent.mkdir(parents=True, exist_ok=True)

with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as zf:
    for path in target_dir.rglob('*'):
        zf.write(path, path.relative_to(target_dir))
PY
fi

if command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$ARCHIVE_PATH" > "$CHECKSUM_PATH"
elif command -v shasum >/dev/null 2>&1; then
  shasum -a 256 "$ARCHIVE_PATH" > "$CHECKSUM_PATH"
else
  ARCHIVE_PATH="$ARCHIVE_PATH" CHECKSUM_PATH="$CHECKSUM_PATH" python - <<'PY'
import hashlib
import os
from pathlib import Path

archive = Path(os.environ["ARCHIVE_PATH"]).resolve()
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
Path(os.environ["CHECKSUM_PATH"]).resolve().write_text(f"{digest}  {archive.name}\n")
PY
fi

echo "Created $ARCHIVE_PATH"
echo "Checksum written to $CHECKSUM_PATH"
