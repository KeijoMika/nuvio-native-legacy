#!/usr/bin/env bash
# Traz as respostas do dono (ZimaOS) para docs/issues/respostas-dono.json (versionado).
# Uso: respostas.sh baixar [worktree]
set -euo pipefail
AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="${PAINEL_HOST_SSH:-zimaos-lan}"
REMOTO="${PAINEL_RESPOSTAS_REMOTO:-/DATA/AppData/nuvio-painel/respostas.json}"
cmd="${1:-}"; REPO="${2:-$(cd "$AQUI/../.." && pwd)}"
[ "$cmd" = baixar ] || { echo "uso: $0 baixar [worktree]"; exit 2; }
DEST="$REPO/docs/issues/respostas-dono.json"
TMP="$(mktemp)"; trap 'rm -f "$TMP"' EXIT
if [ -n "${PAINEL_RESPOSTAS_ARQUIVO:-}" ]; then cp "$PAINEL_RESPOSTAS_ARQUIVO" "$TMP"   # teste local
else ssh -o ConnectTimeout=8 -o BatchMode=yes "$HOST" cat "$REMOTO" > "$TMP"; fi
python3 - "$TMP" "$DEST" <<'PY'
import json, sys
d = json.load(open(sys.argv[1], encoding="utf-8"))
assert isinstance(d.get("respostas"), dict), "formato inesperado"
with open(sys.argv[2], "w", encoding="utf-8") as f:
    json.dump(d, f, ensure_ascii=False, indent=1, sort_keys=True); f.write("\n")
print("ok: %d decisoes com resposta -> %s" % (len(d["respostas"]), sys.argv[2]))
PY
