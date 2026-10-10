# Entrega R2 — revisão 410b

Os três P2 foram corrigidos. Aproveitadas as alterações da tentativa anterior,
conferidas no commit `b8935ecf`; nenhuma correção de produção precisou ser descartada.
Durante a conferência, outro processo corrigiu o teste de símbolo e criou esse commit.
A validação abaixo foi executada em cópias isoladas, conferidas contra os arquivos finais.

| P2 | Correção | Prova antes → depois |
| --- | --- | --- |
| GPU fraca validava redução sem referência | Inicialização começa no nível 0 também em Mali-400. | `inicioFraco`: assert falha no código anterior; passa agora, rejeita 32→32 FPS e preserva o bloqueio no reinício. |
| LG não executava a reavaliação do legado | `main.c` chama `gpun_medir` também com `NV_WEBOS`. | Objeto de `main.c` compilado com `-DNV_WEBOS`: chamada ausente antes e presente depois; testes de migração/persistência passam. |
| Interrupção comparava cenas diferentes | Cancela candidato, volta ao nível anterior e limpa a referência, sem gravar nem bloquear. | `interrupcao`: assert falha antes; passa depois para saída da Home, perda de artes e suspensão nos dois degraus, incluindo nova referência 20→25 FPS. |

**Verificação executada:**
- `bash tests/gpunivel_r2.sh`: código anterior `c6b52408` com os testes novos falha nos três P2 (saída 1); `b8935ecf` passa (saída 0).
- `bash tests/gpunivel.sh`: passou.
- `bash tests/gpunivel_ajuda.sh`: passou; 13 opções, 30 idiomas, níveis automáticos e escolha manual.
- `bash tools/build-local.sh`: compilação e link completos do host passaram.
- `git diff --check HEAD`: passou.

Logs locais: `/tmp/nuvio-r2-validacao-9jhqs1kw/` (`antes.log`, `depois.log`,
`gpunivel.log`, `ajuda.log`, `build.log`). Testes com fontes reais e dublês de
GL/dados; sem instalação ou execução em TV física. Nenhum push realizado.

Commit de código: `b8935ecf` — `gpu: tres P2 do codex (#410)`, com o trailer solicitado.
