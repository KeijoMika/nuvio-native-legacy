#!/bin/bash
# A card's id and its episode fields have to name the same episode.
#
#   bash tests/cw_episode_id.sh
#   SANITIZE=1 bash tests/cw_episode_id.sh
#
# One binary, no SDL: compiles src/catalogo.c with the doubles it needs to link.
# See the header of the .c for the case measured on the TV.
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/catalogo.c tests/cw_episode_id.c \
  -Isrc -o /tmp/nuvio-cw-episode-id -O1 -g \
  -Wall -Wno-deprecated-declarations -Wno-format-truncation
/tmp/nuvio-cw-episode-id
