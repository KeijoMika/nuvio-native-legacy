#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
# tools/testa-tudo.sh discovers this contract through tests/*.sh.
python3 tools/cache-contrato.py
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
out=$(mktemp "${TMPDIR:-/tmp}/nuvio-cache-contract.XXXXXX")
trap 'rm -f "$out"' EXIT
cc "${sources[@]}" tests/cache_contrato.c -Isrc -o "$out" -O1 \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations
"$out"
