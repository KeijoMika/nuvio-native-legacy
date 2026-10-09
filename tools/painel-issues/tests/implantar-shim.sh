#!/usr/bin/env bash
# Simula api/implantar-remoto.sh com um "docker" e um "curl" falsos (sem docker, sem rede, sem ssh).
set -euo pipefail
AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REMOTO="$AQUI/../api/implantar-remoto.sh"
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
FALHAS=0
ok()  { echo "PASS $1"; }
bad() { echo "FAIL $1"; FALHAS=$((FALHAS+1)); }
SH="$W/bin"; mkdir -p "$SH"
export ST="$W/st"

cat > "$SH/sleep" <<'S'
#!/bin/sh
exit 0
S
cat > "$SH/docker" <<'S'
#!/usr/bin/env bash
# estado: $ST/c/<nome> com "status porta"
C="$ST/c"; mkdir -p "$C"
st()   { cut -d' ' -f1 "$C/$1"; }
porta(){ cut -d' ' -f2 "$C/$1"; }
cmd="$1"; shift
case "$cmd" in
  build) [ -z "${FALHA_BUILD:-}" ] ;;
  network) [ "$1" = create ] && touch "$ST/net" && exit 0; [ -f "$ST/net" ] ;;
  ps) todos=0; ex=0
      for a in "$@"; do [ "$a" = -a ] && todos=1; [ "$a" = status=exited ] && ex=1; done
      for f in "$C"/*; do [ -e "$f" ] || continue; n="$(basename "$f")"
        if [ $ex = 1 ]; then [ "$(st "$n")" = exited ] && echo "$n"
        elif [ $todos = 1 ]; then echo "$n"
        else [ "$(st "$n")" = running ] && echo "$n"; fi
      done ;;
  run) nome=""; p=""; rm=0
       while [ $# -gt 0 ]; do case "$1" in --name) nome="$2"; shift;; -p) p="$2"; shift;; --rm) rm=1;; esac; shift; done
       [ $rm = 1 ] && exit 0
       porta8094=""; case "$p" in 8094:*) porta8094=1;; esac
       if [ -n "$porta8094" ]; then
         [ -n "${FALHA_PORTA:-}" ] && exit 125
         for f in "$C"/*; do [ -e "$f" ] && [ "$(st "$(basename "$f")")" = running ] && [ "$(porta "$(basename "$f")")" = 8094 ] && exit 125; done
         case "${FALHA_MATA:-}" in
           term) echo "created -" > "$C/$nome"; kill -TERM "$PPID"; exit 0 ;;
           kill) echo "created -" > "$C/$nome"; kill -KILL "$PPID"; exit 0 ;;
         esac
         echo "running 8094" > "$C/$nome"
       elif [ -n "$p" ]; then echo "running tmp" > "$C/$nome"
       else echo "running -" > "$C/$nome"; fi ;;
  stop)  echo "exited $(porta "$1")" > "$C/$1" ;;
  start) if [ "$(porta "$1")" = 8094 ]; then
           for f in "$C"/*; do n="$(basename "$f")"; [ "$n" != "$1" ] && [ "$(st "$n")" = running ] && [ "$(porta "$n")" = 8094 ] && exit 1; done
         fi; echo "running $(porta "$1")" > "$C/$1" ;;
  rm) for a in "$@"; do case "$a" in -f) ;; *) rm -f "$C/$a";; esac; done ;;
  exec) [ -z "${FALHA_API:-}" ] ;;
  port) echo "127.0.0.1:45678" ;;
  *) echo "docker shim: $cmd?" >&2; exit 99 ;;
esac
S
cat > "$SH/curl" <<'S'
#!/usr/bin/env bash
url="${@: -1}"; C="$ST/c"
case "$url" in
  *:45678/*) [ -z "${FALHA_TEMP:-}" ] || exit 22 ;;
  *:8094/*)  achou=0
    for f in "$C"/*; do [ -e "$f" ] || continue; n="$(basename "$f")"
      [ "$(cut -d' ' -f1 "$f")" = running ] && [ "$(cut -d' ' -f2 "$f")" = 8094 ] || continue
      [ -n "${FALHA_POS:-}" ] && case "$n" in *"$TS"*) continue;; esac
      achou=1; done
    [ $achou = 1 ] || exit 22 ;;
  *) exit 22 ;;
esac
case "$url" in */api/saude) echo '{"ok": true}';; esac
S
chmod +x "$SH"/*

novo_cenario() { # recria base e estado: nginx legado servindo na 8094
  rm -rf "$W/base" "$ST"; mkdir -p "$W/base/html" "$W/base/api" "$ST/c"
  echo '{"decisoes":[{"id":"dec-a"}]}' > "$W/base/html/data.json"
  echo '{"versao":1,"respostas":{"dec-a":{"historico":[]}}}' > "$W/base/respostas.json"
  echo "running 8094" > "$ST/c/nuvio-painel"
}
roda() { # TS [VAR=val...] -> rc
  local ts="$1"; shift
  ( export PATH="$SH:$PATH" TS="$ts"; for kv in "$@"; do export "$kv"; done
    bash "$REMOTO" "$W/base" ) >"$W/out" 2>&1 && return 0 || return $?
}
estado() { cat "$ST/c/$1" 2>/dev/null | cut -d' ' -f1 || true; }
tem() { [ -e "$ST/c/$1" ]; }
espera() { if [ "$2" = "$3" ]; then ok "$1"; else bad "$1 (esperado $2, veio $3)"; fi; }
sem_novos() { ! ls "$ST/c" | grep -q -- "-$1$"; }

echo "== sucesso (com migracao de respostas.json)"
novo_cenario; rc=0; roda 20260101000001 || rc=$?
espera "deploy ok rc" 0 "$rc"
espera "novo nginx rodando" running "$(estado nuvio-painel-20260101000001)"
espera "antigo parado, nao removido" exited "$(estado nuvio-painel)"
[ -f "$W/base/respostas/respostas.json" ] && [ -f "$W/base/respostas.json" ] && ok "respostas.json copiado e antigo mantido" || bad "migracao"
[ "$(cat "$W/base/.nginx-bom")" = nuvio-painel-20260101000001 ] && ok "marcador do ultimo bom" || bad "marcador"
echo "== segundo deploy: antigos versionados so parados"
rc=0; roda 20260101000002 || rc=$?
espera "2o deploy rc" 0 "$rc"
espera "1a versao parada" exited "$(estado nuvio-painel-20260101000001)"
espera "1a API parada" exited "$(estado nuvio-painel-api-20260101000001)"
espera "2a versao rodando" running "$(estado nuvio-painel-20260101000002)"
echo "== --limpar remove so os parados que nao sao o ultimo bom"
rc=0; ( export PATH="$SH:$PATH" TS=x; bash "$REMOTO" "$W/base" limpar ) >"$W/out" 2>&1 || rc=$?
espera "limpar rc" 0 "$rc"
tem nuvio-painel && bad "legado parado deveria sair" || ok "legado removido"
tem nuvio-painel-20260101000001 && bad "1a versao parada deveria sair" || ok "versoes antigas removidas"
espera "atual intacto" running "$(estado nuvio-painel-20260101000002)"

echo "== falha no build: nada tocado"
novo_cenario; rc=0; roda 20260101000003 FALHA_BUILD=1 || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "build falhou mas rc=0"
espera "antigo segue rodando" running "$(estado nuvio-painel)"
sem_novos 20260101000003 && ok "nenhum container novo" || bad "sobrou container novo"

echo "== API nao fica pronta"
novo_cenario; rc=0; roda 20260101000004 FALHA_API=1 || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo segue rodando" running "$(estado nuvio-painel)"
sem_novos 20260101000004 && ok "novos removidos" || bad "novos sobraram"

echo "== nginx falha na porta temporaria"
novo_cenario; rc=0; roda 20260101000005 FALHA_TEMP=1 || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo nunca parou" running "$(estado nuvio-painel)"
sem_novos 20260101000005 && ok "novos removidos" || bad "novos sobraram"

echo "== porta 8094 ocupada na troca"
novo_cenario; rc=0; roda 20260101000006 FALHA_PORTA=1 || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo religado" running "$(estado nuvio-painel)"
sem_novos 20260101000006 && ok "novos removidos" || bad "novos sobraram"

echo "== falha depois da troca (health check)"
novo_cenario; rc=0; roda 20260101000007 FALHA_POS=1 || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo religado" running "$(estado nuvio-painel)"
sem_novos 20260101000007 && ok "novos removidos" || bad "novos sobraram"
grep -q "restaurado: 8094 servindo" "$W/out" && ok "imprime o estado restaurado" || bad "sem mensagem de restauracao"

echo "== interrompido (SIGTERM) logo apos parar o antigo"
novo_cenario; rc=0; roda 20260101000008 FALHA_MATA=term || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo religado pelo trap" running "$(estado nuvio-painel)"
sem_novos 20260101000008 && ok "novos removidos" || bad "novos sobraram"

echo "== morto sem trap (SIGKILL) e reexecucao"
novo_cenario; rc=0; roda 20260101000009 FALHA_MATA=kill || rc=$?
[ "$rc" != 0 ] && ok "rc != 0 ($rc)" || bad "rc=0"
espera "antigo ficou parado" exited "$(estado nuvio-painel)"
rc=0; roda 20260101000010 || rc=$?
espera "reexecucao rc" 0 "$rc"
grep -q "recuperado: nuvio-painel religado" "$W/out" && ok "religou o ultimo bom antes de tudo" || bad "nao recuperou primeiro"
espera "novo servindo" running "$(estado nuvio-painel-20260101000010)"

[ "$FALHAS" = 0 ] && { echo "TUDO OK"; exit 0; } || { echo "$FALHAS falha(s)"; exit 1; }
