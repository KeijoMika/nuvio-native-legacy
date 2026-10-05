#!/bin/bash
# The history is NOT a filter on "Continuar assistindo" (#244).
#
#   bash tests/cw_historico.sh
#   SANITIZE=1 bash tests/cw_historico.sh
#
# One binary, no SDL: tests/cw_historico.c includes src/descoberta.c with the REAL
# catalogo.c and progresso.c, and doubles only the network (the "Trakt",
# /sync/history, the Cinemeta /meta). See the header of the .c for the rule it
# pins.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cwhist.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
cc ${flags[@]+"${flags[@]}"} -std=gnu11 ${NV_CFLAGS:-} -Isrc \
  tests/cw_historico.c src/catalogo.c src/progresso.c src/cwordem.c src/cotacat.c \
  src/js.c src/colecoes.c src/redeurl.c src/catordem.c \
  -o "$dir/teste" -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -Wno-format-truncation -Wno-misleading-indentation \
  -lpthread
"$dir/teste"
