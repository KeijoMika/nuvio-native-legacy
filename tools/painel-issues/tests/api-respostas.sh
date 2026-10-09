#!/usr/bin/env bash
# Teste local da API de respostas e do mapa.py. Uso: tools/painel-issues/tests/api-respostas.sh
set -euo pipefail
AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$AQUI/../../.." && pwd)"
T="$(mktemp -d)"; PID=""
trap '[ -n "$PID" ] && kill "$PID" 2>/dev/null || true; rm -rf "$T"' EXIT
FALHAS=0
ok()  { echo "PASS $1"; }
bad() { echo "FAIL $1"; FALHAS=$((FALHAS+1)); }
espera() { # nome esperado obtido
  if [ "$2" = "$3" ]; then ok "$1 (rc/http=$3)"; else bad "$1 (esperado $2, veio $3)"; fi
}

PORTA="$(python3 -c 'import socket;s=socket.socket();s.bind(("127.0.0.1",0));print(s.getsockname()[1])')"
ORIGEM="http://192.168.1.20:8094"
cat > "$T/data.json" <<J
{"decisoes":[{"id":"dec-teste-um","pergunta":"?"},{"id":"dec-teste-dois","pergunta":"?"},{"id":"dec-teste-feita","pergunta":"?","aplicada_em":"2026-10-01"}]}
J
python3 "$AQUI/../api/servidor.py" --host 127.0.0.1 --porta "$PORTA" --data-json "$T/data.json" \
  --respostas "$T/respostas.json" --origens "$ORIGEM,http://100.77.116.81:8094" >"$T/srv.log" 2>&1 &
PID=$!
for _ in $(seq 50); do curl -fs "http://127.0.0.1:$PORTA/api/saude" >/dev/null 2>&1 && break; sleep 0.1; done
U="http://127.0.0.1:$PORTA/api/respostas"
post() { # corpo [headers extra...] -> http code
  local corpo="$1"; shift
  curl -s -o "$T/out" -w '%{http_code}' -X POST -H 'Content-Type: application/json' "$@" --data "$corpo" "$U"
}

espera "POST valido" 200 "$(post '{"id":"dec-teste-um","resposta":"sim","nota":"ok"}' -H "Origin: $ORIGEM")"
python3 - "$T/respostas.json" <<'P' && ok "gravado em respostas.json" || bad "gravado em respostas.json"
import json,sys
h=json.load(open(sys.argv[1]))["respostas"]["dec-teste-um"]["historico"]
assert h[-1]["resposta"]=="sim" and h[-1]["nota"]=="ok" and h[-1]["quando"]
P
espera "POST sem Origin (curl)" 200 "$(post '{"id":"dec-teste-um","resposta":"nao"}')"
espera "id inexistente" 404 "$(post '{"id":"dec-nao-existe","resposta":"sim"}')"
espera "id malformado" 400 "$(post '{"id":"../x","resposta":"sim"}')"
espera "resposta talvez" 400 "$(post '{"id":"dec-teste-um","resposta":"talvez"}')"
espera "nota 2000 chars" 200 "$(post "{\"id\":\"dec-teste-um\",\"resposta\":\"sim\",\"nota\":\"$(python3 -c 'print("a"*2000)')\"}")"
espera "nota 2001 chars" 400 "$(post "{\"id\":\"dec-teste-um\",\"resposta\":\"sim\",\"nota\":\"$(python3 -c 'print("a"*2001)')\"}")"
espera "corpo gigante" 413 "$(post "{\"id\":\"dec-teste-um\",\"resposta\":\"sim\",\"nota\":\"$(python3 -c 'print("a"*40000)')\"}")"
espera "Origin estranha" 403 "$(post '{"id":"dec-teste-um","resposta":"sim"}' -H 'Origin: http://evil.example')"
espera "decisao aplicada" 409 "$(post '{"id":"dec-teste-feita","resposta":"sim"}')"
espera "JSON invalido" 400 "$(post '{nao-json')"
espera "sem content-type json" 415 "$(curl -s -o /dev/null -w '%{http_code}' -X POST -H 'Content-Type: text/plain' --data '{}' "$U")"

# concorrencia: 20 POSTs em paralelo
for i in $(seq 20); do
  ( post "{\"id\":\"dec-teste-dois\",\"resposta\":\"sim\",\"nota\":\"c$i\"}" -o /dev/null >"$T/c$i" ) &
done
for i in $(seq 20); do while ! kill -0 "$PID" 2>/dev/null || [ ! -s "$T/c$i" ]; do sleep 0.05; done; done
[ "$(cat "$T"/c[0-9]* | tr -d '\n')" = "$(printf '200%.0s' $(seq 20))" ] && ok "20 POSTs concorrentes todos 200" || bad "algum POST concorrente nao deu 200"
python3 - "$T/respostas.json" <<'P' && ok "20 escritas concorrentes, JSON valido e completo" || bad "concorrencia"
import json,sys
d=json.load(open(sys.argv[1]))
notas=sorted(x["nota"] for x in d["respostas"]["dec-teste-dois"]["historico"])
assert notas==sorted("c%d"%i for i in range(1,21)), notas
assert len(d["respostas"]["dec-teste-um"]["historico"])==3
P
ls "$T" | grep -q '\.tmp$' && bad "sobrou arquivo tmp" || ok "sem arquivo tmp sobrando"
curl -s "$U" | python3 -c 'import json,sys;d=json.load(sys.stdin);assert len(d["respostas"]["dec-teste-dois"]["historico"])==20' \
  && ok "GET devolve tudo" || bad "GET"

# mapa.py: copia isolada com um respostas-dono.json de exemplo
M="$T/mapa"; mkdir "$M"; cp "$REPO/docs/issues/"{mapa.py,mapa.json,MAPA.md} "$M/"
rc=0; python3 "$M/mapa.py" --check >/dev/null 2>&1 || rc=$?; espera "mapa.py --check com ids" 0 "$rc"
ID="$(python3 -c "import json;print(json.load(open('$M/mapa.json'))['decisoes'][0]['id'])")"
cat > "$M/respostas-dono.json" <<J
{"versao":1,"respostas":{"$ID":{"historico":[{"resposta":"nao","nota":"so depois da 2.0.5","quando":"2026-10-09T12:00:00-03:00"}]}}}
J
python3 "$M/mapa.py" >/dev/null && grep -q "Resposta do dono: NÃO\*\* Nota: so depois da 2.0.5" "$M/MAPA.md" && grep -q "respondida, a aplicar" "$M/MAPA.md" \
  && ok "mapa.py mostra a resposta" || bad "mapa.py mostra a resposta"
# id que some sem ir para decisoes_ids_retirados => erro (compara com o HEAD de um git temporario)
cp "$REPO/docs/issues/mapa.json" "$M/mapa.json"; rm -f "$M/respostas-dono.json"
git -C "$M" init -q && git -C "$M" add mapa.json && git -C "$M" -c user.name=t -c user.email=t@t commit -qm base 2>/dev/null
# HEAD:./mapa.json precisa existir no repo temporario: mapa.py usa o git do proprio diretorio
python3 - "$M/mapa.json" <<'P'
import sys
p=sys.argv[1]; t=open(p).read(); t=t.replace('"id": "dec-135-vidaa-rtl",','"id": "dec-135-trocado",',1); open(p,'w').write(t)
P
rc=0; python3 "$M/mapa.py" >/dev/null 2>"$T/err" || rc=$?; espera "id sumido sem retirar falha" 1 "$rc"
grep -q "sumiu" "$T/err" && ok "mensagem diz que o id sumiu" || bad "mensagem do id sumido"
python3 - "$M/mapa.json" <<'P'
import sys
p=sys.argv[1]; t=open(p).read()
t=t.replace('"decisoes_ids_retirados": [],','"decisoes_ids_retirados": ["dec-135-vidaa-rtl"],',1); open(p,'w').write(t)
P
rc=0; python3 "$M/mapa.py" >/dev/null 2>&1 || rc=$?; espera "id sumido mas retirado passa" 0 "$rc"
python3 - "$M/mapa.json" <<'P'
import sys
p=sys.argv[1]; t=open(p).read()
t=t.replace('"id": "dec-135-trocado",','"id": "dec-135-vidaa-rtl",',1); open(p,'w').write(t)
P
rc=0; python3 "$M/mapa.py" >/dev/null 2>&1 || rc=$?; espera "id retirado reusado falha" 1 "$rc"
[ "$FALHAS" = 0 ] && { echo "TUDO OK"; exit 0; } || { echo "$FALHAS falha(s)"; exit 1; }
