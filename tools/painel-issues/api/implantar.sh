#!/usr/bin/env bash
# Implanta a API de respostas no ZimaOS. Sem --aplicar so mostra o plano e nao faz nada.
# Uso: implantar.sh [--aplicar]
#
# O que muda (somente o painel; nenhum outro container e tocado):
#   0. pre-requisito: o painel ja publicado com ids (publicar.sh), senao /api/saude fica 503 e aborta
#   1. envia api/ para zimaos-lan:/DATA/AppData/nuvio-painel/api/ e constroi a imagem nuvio-painel-api
#   2. cria a rede nuvio-painel-net e o container nuvio-painel-api (sem porta publicada, sem root):
#      html/ -> /data/html (so leitura), respostas/ -> /data/respostas (unica pasta gravavel)
#   3. grava /DATA/AppData/nuvio-painel/nginx/default.conf (proxy de /api/)
#   4. valida um nginx NOVO (nome temporario, porta aleatoria em 127.0.0.1) antes de tocar no atual;
#      so entao troca: o atual vira nuvio-painel-old (parado), o novo sobe em 8094, o health check
#      confere o corpo "ok"; qualquer falha restaura o container antigo (trap). Rodar de novo e seguro.
#      Indisponibilidade: so o tempo da troca (segundos).
set -Eeuo pipefail

AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="zimaos-lan"
BASE="/DATA/AppData/nuvio-painel"
case "${1:-}" in
  --aplicar) ;;
  -h|--help) sed -n 2,15p "$0"; exit 0 ;;
  *) echo "[plano] nada foi executado. Com --aplicar:"; sed -n 5,15p "$0" | sed 's/^# */  /'; exit 0 ;;
esac

ssh "$HOST" "mkdir -p '$BASE/api' '$BASE/nginx' '$BASE/respostas'"
rsync -az "$AQUI/servidor.py" "$AQUI/Dockerfile" "$HOST:$BASE/api/"
rsync -az "$AQUI/nginx-default.conf" "$HOST:$BASE/nginx/default.conf"
ssh "$HOST" "chmod -R a+rX '$BASE/api' '$BASE/nginx' && chmod 755 '$BASE/respostas'"

ssh "$HOST" bash -s -- "$BASE" <<'REMOTO'
set -Eeuo pipefail
base="$1"
API=nuvio-painel-api; NGX=nuvio-painel
OK_RE='"ok": ?true'
existe() { docker ps -a --format '{{.Names}}' | grep -qx "$1"; }
saude() { curl -fsS --max-time 5 "$1" | grep -Eq "$OK_RE"; }
espera() { # url: ate 20 tentativas
  for _ in $(seq 20); do saude "$1" && return 0; sleep 1; done; return 1; }

API_OLD=0; NGX_OLD=0
rollback() {
  set +e; trap - ERR
  echo "ERRO: revertendo para o estado anterior" >&2
  docker rm -f "$NGX-novo" >/dev/null 2>&1
  if [ "$NGX_OLD" = 1 ]; then docker rm -f "$NGX" >/dev/null 2>&1; docker rename "$NGX-old" "$NGX"; docker start "$NGX" >/dev/null; fi
  if [ "$API_OLD" = 1 ]; then docker rm -f "$API" >/dev/null 2>&1; docker rename "$API-old" "$API"; docker start "$API" >/dev/null
  else docker rm -f "$API" >/dev/null 2>&1; fi
  exit 1
}
trap rollback ERR

[ -s "$base/html/data.json" ] || { echo "ERRO: painel ainda nao publicado (rode publicar.sh antes)" >&2; exit 1; }
# restos de uma execucao anterior que terminou limpa so deixam -old/-novo parados
docker rm -f "$NGX-novo" "$API-old" "$NGX-old" >/dev/null 2>&1 || true

docker build -t nuvio-painel-api "$base/api"
docker network inspect nuvio-painel-net >/dev/null 2>&1 || docker network create nuvio-painel-net >/dev/null
mkdir -p "$base/respostas"

# 1) API: o antigo (se houver) sai do caminho, o novo precisa ficar pronto
if existe "$API"; then docker stop "$API" >/dev/null; docker rename "$API" "$API-old"; API_OLD=1; fi
docker run -d --name "$API" --restart unless-stopped --network nuvio-painel-net \
  --user "$(id -u):$(id -g)" \
  -v "$base/html":/data/html:ro -v "$base/respostas":/data/respostas nuvio-painel-api >/dev/null
for _ in $(seq 20); do
  docker exec "$API" python -c 'import json,sys,urllib.request as u;sys.exit(0 if json.load(u.urlopen("http://127.0.0.1:8000/api/saude",timeout=4)).get("ok") else 1)' 2>/dev/null && break
  sleep 1
done
docker exec "$API" python -c 'import json,sys,urllib.request as u;sys.exit(0 if json.load(u.urlopen("http://127.0.0.1:8000/api/saude",timeout=4)).get("ok") else 1)'

# 2) nginx novo sob nome temporario, porta aleatoria so em loopback; nada do atual e tocado ainda
docker run -d --name "$NGX-novo" --network nuvio-painel-net -p 127.0.0.1::80 \
  -v "$base/html":/usr/share/nginx/html:ro \
  -v "$base/nginx/default.conf":/etc/nginx/conf.d/default.conf:ro nginx:alpine >/dev/null
tp="$(docker port "$NGX-novo" 80/tcp | head -1 | sed 's/.*://')"
espera "http://127.0.0.1:$tp/api/saude"
curl -fsS --max-time 5 -o /dev/null "http://127.0.0.1:$tp/index.html"
docker rm -f "$NGX-novo" >/dev/null

# 3) troca: o atual vira -old (parado), o definitivo sobe em 8094
if existe "$NGX"; then docker stop "$NGX" >/dev/null; docker rename "$NGX" "$NGX-old"; NGX_OLD=1; fi
docker run -d --name "$NGX" --restart unless-stopped --network nuvio-painel-net -p 8094:80 \
  -v "$base/html":/usr/share/nginx/html:ro \
  -v "$base/nginx/default.conf":/etc/nginx/conf.d/default.conf:ro nginx:alpine >/dev/null
espera "http://127.0.0.1:8094/api/saude"
curl -fsS --max-time 5 -o /dev/null "http://127.0.0.1:8094/index.html"

# sucesso: descarta os antigos
trap - ERR
docker rm -f "$NGX-old" "$API-old" >/dev/null 2>&1 || true
echo "ok: $(curl -fsS --max-time 5 http://127.0.0.1:8094/api/saude)"
REMOTO
echo "pronto: http://192.168.1.20:8094"
