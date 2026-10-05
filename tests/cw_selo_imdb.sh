#!/bin/bash
# IMDb badge on the "Continuar assistindo" card (issue #243).
#
#   bash tests/cw_selo_imdb.sh
#   SANITIZE=1 bash tests/cw_selo_imdb.sh     # ASan + UBSan
#
# A single binary, and no SDL: tests/cw_selo_imdb.c includes src/trakt.c and
# intercepts `rede_baixar` (the only network call on the path), with the doubles
# needed to link. See the header of the .c for the list of what it proves.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cwselo.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
cc ${flags[@]+"${flags[@]}"} -std=gnu11 ${NV_CFLAGS:-} -Isrc \
  tests/cw_selo_imdb.c src/js.c src/jsw.c \
  -o "$dir/teste" -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -Wno-format-truncation -Wno-misleading-indentation \
  -lpthread
"$dir/teste"
