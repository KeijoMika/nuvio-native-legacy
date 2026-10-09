#!/usr/bin/env bash
# Parte que roda NO ZimaOS (enviada por ssh pelo implantar.sh). Uso: implantar-remoto.sh BASE [deploy|limpar]
# Regra: nenhum container existente e apagado ou renomeado no deploy. Os novos tem nome versionado,
# o antigo so e parado (docker stop) e, se algo falhar, religado (docker start).
set -Eeuo pipefail
BASE="$1"; MODO="${2:-deploy}"
TS="${TS:-$(date +%Y%m%d%H%M%S)}"
RE_NGX='^nuvio-painel(-[0-9]+)?$'; RE_API='^nuvio-painel-api(-[0-9]+)?$'
UID_API=1000
NOVO_API="nuvio-painel-api-$TS"; NOVO_NGX="nuvio-painel-$TS"
BOM_NGX="$BASE/.nginx-bom"; BOM_API="$BASE/.api-bom"

nomes()   { docker ps -a --format '{{.Names}}' | grep -E "$1" | sort || true; }       # todos
rodando() { docker ps --format '{{.Names}}' | grep -E "$1" | sort | tail -1 || true; }
parados() { docker ps -a --filter status=exited --format '{{.Names}}' | grep -E "$1" | sort || true; }
existe()  { docker ps -a --format '{{.Names}}' | grep -x "$1" >/dev/null; }  # sem -q: com pipefail o SIGPIPE do docker daria falso negativo
servido() { curl -fsS --max-time 5 -o /dev/null http://127.0.0.1:8094/index.html; }
saude()   { curl -fsS --max-time 5 "$1" | grep -E '"ok": ?true' >/dev/null; }
api_pronta() { # container
  for _ in $(seq 20); do
    docker exec "$1" python -c 'import json,sys,urllib.request as u;sys.exit(0 if json.load(u.urlopen("http://127.0.0.1:8000/api/saude",timeout=4)).get("ok") else 1)' 2>/dev/null && return 0
    sleep 1
  done; return 1
}

# Se a 8094 nao responde (execucao anterior interrompida), religa o ultimo nginx conhecido como bom.
recupera() {
  servido && return 0
  local n a
  n="$(cat "$BOM_NGX" 2>/dev/null || echo nuvio-painel)"; a="$(cat "$BOM_API" 2>/dev/null || true)"
  [ -n "$a" ] && existe "$a" && docker start "$a" >/dev/null 2>&1 || true
  if existe "$n"; then docker start "$n" >/dev/null 2>&1 || true; echo "recuperado: $n religado"; fi
}

if [ "$MODO" = limpar ]; then
  servido || { echo "ERRO: 8094 nao esta servindo; rode o deploy antes de limpar" >&2; exit 1; }
  for n in $(parados "$RE_NGX") $(parados "$RE_API"); do
    [ "$n" = "$(cat "$BOM_NGX" 2>/dev/null)" ] || [ "$n" = "$(cat "$BOM_API" 2>/dev/null)" ] && continue
    docker rm "$n" >/dev/null && echo "removido: $n"
  done
  exit 0
fi

recupera
[ -s "$BASE/html/data.json" ] || { echo "ERRO: painel ainda nao publicado (rode publicar.sh antes)" >&2; exit 1; }

# migracao: o caminho antigo era $BASE/respostas.json; o antigo fica, a copia vai para respostas/
mkdir -p "$BASE/respostas"
if [ -f "$BASE/respostas.json" ] && [ ! -f "$BASE/respostas/respostas.json" ]; then
  cp -p "$BASE/respostas.json" "$BASE/respostas/respostas.json" && echo "migrado: respostas.json -> respostas/"
fi

docker build -t nuvio-painel-api "$BASE/api"          # falhou aqui: nada foi tocado
docker network inspect nuvio-painel-net >/dev/null 2>&1 || docker network create nuvio-painel-net >/dev/null
# dono fixo nao-root (ssh pode ser root): chown por container descartavel
docker run --rm --user 0 --entrypoint chown -v "$BASE/respostas":/data/respostas nuvio-painel-api -R "$UID_API:$UID_API" /data/respostas

ANT_NGX="$(rodando "$RE_NGX")"; ANT_API="$(rodando "$RE_API")"
if [ -n "$ANT_NGX" ] && servido; then echo "$ANT_NGX" > "$BOM_NGX"; [ -n "$ANT_API" ] && echo "$ANT_API" > "$BOM_API" || true; fi

CONCLUIDO=0
volta() {  # EXIT: so age se o deploy nao concluiu; remove SO os novos e religa os antigos
  local rc=$?
  [ "$CONCLUIDO" = 1 ] && return 0
  trap - EXIT ERR INT HUP TERM; set +e
  echo "FALHA (rc=$rc): voltando ao estado anterior" >&2
  docker rm -f "$NOVO_NGX" "$NOVO_API" >/dev/null 2>&1
  [ -n "$ANT_API" ] && docker start "$ANT_API" >/dev/null 2>&1
  [ -n "$ANT_NGX" ] && docker start "$ANT_NGX" >/dev/null 2>&1
  sleep 1
  if servido; then echo "restaurado: 8094 servindo ($ANT_NGX)" >&2; else echo "ATENCAO: 8094 NAO esta servindo apos a restauracao" >&2; fi
  exit "$rc"
}
trap volta EXIT
trap 'exit 130' INT HUP TERM

# 1) API nova (alias estavel para o upstream do nginx); readiness por exec, sem depender do alias
docker run -d --name "$NOVO_API" --restart unless-stopped --network nuvio-painel-net \
  --network-alias nuvio-painel-api --user "$UID_API:$UID_API" \
  -v "$BASE/html":/data/html:ro -v "$BASE/respostas":/data/respostas nuvio-painel-api >/dev/null
api_pronta "$NOVO_API"
[ -n "$ANT_API" ] && docker stop "$ANT_API" >/dev/null   # evita dois upstreams com o mesmo alias

# 2) nginx novo numa porta temporaria de loopback; o atual segue servindo
NGX_RUN=(-d --restart unless-stopped --network nuvio-painel-net
  -v "$BASE/html":/usr/share/nginx/html:ro -v "$BASE/nginx/default.conf":/etc/nginx/conf.d/default.conf:ro nginx:alpine)
docker run --name "$NOVO_NGX" -p 127.0.0.1::80 "${NGX_RUN[@]}" >/dev/null
tp="$(docker port "$NOVO_NGX" 80/tcp | head -1 | sed 's/.*://')"
for _ in $(seq 20); do saude "http://127.0.0.1:$tp/api/saude" && break; sleep 1; done
saude "http://127.0.0.1:$tp/api/saude"
curl -fsS --max-time 5 -o /dev/null "http://127.0.0.1:$tp/index.html"
docker rm -f "$NOVO_NGX" >/dev/null

# 3) troca: para o atual (mantido), sobe o definitivo na 8094 (docker run falha rapido se a porta estiver ocupada)
[ -n "$ANT_NGX" ] && docker stop "$ANT_NGX" >/dev/null
docker run --name "$NOVO_NGX" -p 8094:80 "${NGX_RUN[@]}" >/dev/null
for _ in $(seq 20); do saude "http://127.0.0.1:8094/api/saude" && break; sleep 1; done
saude "http://127.0.0.1:8094/api/saude"
servido

CONCLUIDO=1
echo "$NOVO_NGX" > "$BOM_NGX"; echo "$NOVO_API" > "$BOM_API"
echo "ok: $NOVO_NGX e $NOVO_API no ar; antigos parados (${ANT_NGX:--} ${ANT_API:--}); 'implantar.sh --limpar' remove parados"
