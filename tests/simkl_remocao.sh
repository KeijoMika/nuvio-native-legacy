#!/bin/bash
# Removing the WORK on Simkl (issue #244).
#
#   bash tests/simkl_remocao.sh
#   SANITIZE=1 bash tests/simkl_remocao.sh
#
# One binary, no SDL: tests/simkl_remocao.c includes src/simkl.c and doubles
# only the network and the token. See the header of the .c for the cases.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-smkrem.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
cc ${flags[@]+"${flags[@]}"} -std=gnu11 ${NV_CFLAGS:-} -Isrc \
  tests/simkl_remocao.c src/js.c src/jsw.c \
  -o "$dir/teste" -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -lpthread
"$dir/teste"
