#!/bin/bash
# Offline 1920x1080 evidence for the rating card, tags, title and episode row.
set -eu
cd "$(dirname "$0")/.."
output=${1:-/Volumes/ExternalSSD/tmp/rev/avaliar21-png}
mkdir -p "$output"
bash tests/reacao_shot.sh "$output"
NV_AVALIAR_SHOTS=1 bash tests/detail_eps_shot.sh "$output"
