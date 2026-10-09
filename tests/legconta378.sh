#!/bin/bash
# #378: legenda "Da conta" vira "Nenhuma" com os ajustes locais protegidos.
# Ver o cabecalho de tests/legconta378.c (a parte do sync.c esta em
# tests/syncordem.sh, sessao "legconta").
#
#   bash tests/legconta378.sh
set -euo pipefail
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-legconta378.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc "${sources[@]}" tests/legconta378.c -Isrc -o "$work/t" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
NV_T_DIR="$work" "$work/t"
echo "legconta378.sh: ok"
