#!/usr/bin/env bash
# Implanta a API de respostas no ZimaOS. Sem --aplicar so mostra o plano e nao faz nada.
# Uso: implantar.sh [--aplicar]
#
# O que muda (somente o painel; nenhum outro container e tocado):
#   1. envia api/ para zimaos-lan:/DATA/AppData/nuvio-painel/api/ e constroi a imagem nuvio-painel-api
#   2. cria a rede docker nuvio-painel-net e o container nuvio-painel-api (sem porta publicada),
#      volume /DATA/AppData/nuvio-painel -> /data, restart unless-stopped
#   3. grava /DATA/AppData/nuvio-painel/nginx/default.conf (proxy de /api/)
#   4. RECRIA o container nuvio-painel (nginx:alpine, porta 8094, mesmo html) com o conf
#      montado e na rede nuvio-painel-net. Ele nao tem estado: o site vive em html/.
#      Indisponibilidade: alguns segundos.
set -euo pipefail

AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="zimaos-lan"
BASE="/DATA/AppData/nuvio-painel"
case "${1:-}" in
  --aplicar) ;;
  -h|--help) sed -n 2,12p "$0"; exit 0 ;;
  *) echo "[plano] nada foi executado. Com --aplicar:"; sed -n 5,12p "$0" | sed 's/^# */  /'; exit 0 ;;
esac

ssh "$HOST" "mkdir -p '$BASE/api' '$BASE/nginx'"
rsync -az "$AQUI/servidor.py" "$AQUI/Dockerfile" "$HOST:$BASE/api/"
rsync -az "$AQUI/nginx-default.conf" "$HOST:$BASE/nginx/default.conf"
ssh "$HOST" "chmod -R a+rX '$BASE/api' '$BASE/nginx'"

ssh "$HOST" bash -s -- "$BASE" <<'REMOTO'
set -euo pipefail
base="$1"
docker build -t nuvio-painel-api "$base/api"
docker network inspect nuvio-painel-net >/dev/null 2>&1 || docker network create nuvio-painel-net >/dev/null
docker rm -f nuvio-painel-api >/dev/null 2>&1 || true
docker run -d --name nuvio-painel-api --restart unless-stopped --network nuvio-painel-net \
  -v "$base":/data nuvio-painel-api >/dev/null
# recria so o nginx do painel, agora com o conf do proxy e na mesma rede
docker rm -f nuvio-painel >/dev/null 2>&1 || true
docker run -d --name nuvio-painel --restart unless-stopped --network nuvio-painel-net -p 8094:80 \
  -v "$base/html":/usr/share/nginx/html:ro \
  -v "$base/nginx/default.conf":/etc/nginx/conf.d/default.conf:ro nginx:alpine >/dev/null
sleep 2
curl -fsS http://127.0.0.1:8094/api/saude && echo
REMOTO
echo "pronto: http://192.168.1.20:8094 (rode publicar.sh para as decisoes ganharem ids no data.json)"
