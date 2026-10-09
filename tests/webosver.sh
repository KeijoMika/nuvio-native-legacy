#!/bin/sh
set -e
cd "$(dirname "$0")/.."
d=$(mktemp -d "${TMPDIR:-/tmp}/webosver.XXXXXX")
trap 'rm -rf "$d"' EXIT
cc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -Isrc tests/webosver.c src/webosver.c -pthread -o "$d/t"
"$d/t" "$d"
