#!/bin/bash
# "Continuar assistindo": the same work in a SINGLE card (issue #244).
#
#   bash tests/cw_duplicados.sh
#   SANITIZE=1 bash tests/cw_duplicados.sh
#
# One binary, no SDL: tests/cw_duplicados.c includes src/trakt.c and doubles
# /sync/playback (six records for TWO works), /sync/history and the Cinemeta
# /meta. See the header of the .c for the cases.
#
# The other halves of #244 are in tests/cw_remocao.sh.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cwdup.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
cc ${flags[@]+"${flags[@]}"} -std=gnu11 ${NV_CFLAGS:-} -Isrc \
  tests/cw_duplicados.c src/js.c src/jsw.c \
  -o "$dir/teste" -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -Wno-format-truncation -Wno-misleading-indentation \
  -lpthread
"$dir/teste"
