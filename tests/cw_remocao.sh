#!/bin/bash
# Removing from "Continuar assistindo" has to win (issue #244).
#
#   bash tests/cw_remocao.sh
#   SANITIZE=1 bash tests/cw_remocao.sh
#
# One binary, no SDL: tests/cw_remocao.c includes src/trakt.c and doubles
# /sync/playback (three records of the SAME work), /sync/history, the Cinemeta
# /meta and the playback DELETE. See the header of the .c for the three defences.
#
# The merge by work (which turns the three records into one card) is in
# tests/cw_duplicados.sh; the history guard is in tests/cwremover.sh, which pulls
# in descoberta.c and catalogo.c for real.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cwrem.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
cc ${flags[@]+"${flags[@]}"} -std=gnu11 ${NV_CFLAGS:-} -Isrc \
  tests/cw_remocao.c src/js.c src/jsw.c \
  -o "$dir/teste" -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -Wno-format-truncation -Wno-misleading-indentation \
  -lpthread
"$dir/teste"
