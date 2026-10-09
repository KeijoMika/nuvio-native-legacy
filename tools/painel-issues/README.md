# Painel de issues e roadmap

Site estatico (HTML + JS puro, sem CDN, funciona offline na LAN) que mostra
`docs/issues/mapa.json`: KPIs, barras por alvo e status, roadmap por release,
decisoes pendentes, "fechar com a 2.0.3", "nao fechar ainda" e a tabela de issues.

## Gerar e ver localmente

    python3 tools/painel-issues/gerar.py [--repo <worktree>] [--out <dir>]
    python3 -m http.server -d tools/painel-issues/site

Sem argumentos le o mapa do proprio repo e escreve em `tools/painel-issues/site/`
(ignorado pelo git). Falha com codigo != 0 se o `mapa.json` faltar ou for invalido.

## Publicar

    tools/painel-issues/publicar.sh [--dry-run] [worktree]

Gera o site, envia por rsync/ssh para `zimaos-lan:/DATA/AppData/nuvio-painel/html/`
e garante o container `nuvio-painel` (nginx:alpine, porta 8094). Nunca toca outros
containers; aborta se a 8094 estiver ocupada por outra coisa. Padrao de worktree:
`/Volumes/ExternalSSD/nv-2031-int`.

## Acesso

Somente LAN (`http://192.168.1.20:8094`) ou Tailscale. Nao expor publicamente:
as notas contem detalhes internos.

## Respostas do dono (Sim/Nao + informacao adicional)

Cada decisao em `docs/issues/mapa.json` tem um `id` estavel (`dec-...`); o painel mostra
Sim/Nao, texto (2000 caracteres) e Enviar, e permite alterar ate a decisao ser marcada
`aplicada_em` (AAAA-MM-DD, preenchido pela coordenacao). Sem a API, o painel fica so leitura.

- API: `api/servidor.py` (stdlib), `POST/GET /api/respostas`, grava `/data/respostas/respostas.json` (no ZimaOS: `/DATA/AppData/nuvio-painel/respostas/`; o `html/` entra no container so leitura, e a API roda sem root)
  (historico por id, ultima = atual). **Sem autenticacao**: so LAN/Tailscale, como o painel;
  a defesa e o cabecalho Origin (origens do painel) + Content-Type JSON.
- Implantar (precisa de OK do dono, recria o container nginx do painel):
  `tools/painel-issues/api/implantar.sh` mostra o plano; `--aplicar` executa.
- Ler as respostas: `tools/painel-issues/respostas.sh baixar` -> `docs/issues/respostas-dono.json`;
  depois `python3 docs/issues/mapa.py` mostra "resposta do dono" no MAPA.md. `publicar.sh` ja baixa antes de gerar.
- Teste local: `tools/painel-issues/tests/api-respostas.sh`.
