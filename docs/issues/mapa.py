#!/usr/bin/env python3
"""Gera docs/issues/MAPA.md a partir de docs/issues/mapa.json.
Uso: python3 docs/issues/mapa.py   (edite so o json; o md e derivado)"""
import json,os,re,collections,sys,subprocess
D=os.path.dirname(os.path.abspath(__file__))
J=json.load(open(os.path.join(D,'mapa.json')))
R=J['itens']
ALVOS=('2.0.3','2.0.4','2.0.5','2.1','2.2','futuro','nao vamos fazer','ja-lancada')
def valida():
    # falha alto: issue ABERTA sem alvo valido nao gera o mapa
    ruins=[f"#{r['numero']} ({r.get('alvo','<sem alvo>')})" for r in R if r['estado_github']=='aberta' and r.get('alvo') not in ALVOS]
    if ruins: sys.exit('ERRO: issue aberta sem alvo valido (use '+', '.join(ALVOS)+'): '+', '.join(ruins))
    nr={x['release'] for x in J['roadmap']}
    for a in ('2.0.3','2.0.4','2.0.5','2.1','2.2','futuro','nao vamos fazer'):
        if a not in nr: sys.exit('ERRO: roadmap sem a secao '+a)
    for r in R:
        if r['estado_github']=='aberta' and r['alvo']=='2.0.3' and r['status']=='aberta': sys.exit(f"ERRO: #{r['numero']} alvo 2.0.3 mas sem conserto (status aberta)")
        if r['estado_github']!='aberta' and 'alvo' in r: sys.exit(f"ERRO: #{r['numero']} fechada com alvo")
    por={x['numero']:x for x in R}
    fc=[x['numero'] for x in J.get('fechar_203',[])]; nf=[x['numero'] for x in J.get('nao_fechar_ainda',[])]
    for n in fc+nf:
        if n not in por: sys.exit(f"ERRO: #{n} nas listas de fechamento mas fora de itens")
    for n in fc:
        r=por[n]
        if r['estado_github']!='aberta' or r['status']!='lancada': sys.exit(f"ERRO: #{n} em 'fechar com a 2.0.3' precisa estar aberta no GitHub e com status lancada")
    if set(fc)&set(nf): sys.exit('ERRO: issue em fechar_203 e em nao_fechar_ainda: '+str(sorted(set(fc)&set(nf))))
RE_ID=re.compile(r'^dec-[a-z0-9]+(-[a-z0-9]+)*$')
def valida_decisoes():
    # id estavel por decisao: presente, bem formado, unico, nunca some nem e reusado
    ids=[]
    for k,d in enumerate(J['decisoes'],1):
        i=d.get('id')
        if not isinstance(i,str) or not RE_ID.match(i): sys.exit(f"ERRO: decisao {k} sem id valido (formato dec-<assunto>, minusculas/digitos/hifens): {i!r}")
        ids.append(i)
        a=d.get('aplicada_em')
        if a is not None and not re.match(r'^\d{4}-\d{2}-\d{2}$',str(a)): sys.exit(f"ERRO: {i}: aplicada_em deve ser AAAA-MM-DD")
    dup=sorted(i for i,c in collections.Counter(ids).items() if c>1)
    if dup: sys.exit('ERRO: ids de decisao repetidos: '+', '.join(dup))
    ret=J.get('decisoes_ids_retirados')
    if not isinstance(ret,list): sys.exit("ERRO: falta 'decisoes_ids_retirados' (lista, pode ser vazia)")
    if len(set(ret))!=len(ret): sys.exit('ERRO: decisoes_ids_retirados com repeticao')
    reus=sorted(set(ret)&set(ids))
    if reus: sys.exit('ERRO: id retirado foi reusado: '+', '.join(reus))
    try:
        ant=json.loads(subprocess.run(['git','-C',D,'show','HEAD:./mapa.json'],capture_output=True,text=True,check=True).stdout)
        antes={d['id'] for d in ant.get('decisoes',[]) if 'id' in d}|set(ant.get('decisoes_ids_retirados',[]))
    except Exception: antes=set()  # sem git/HEAD: so as checagens acima
    sumiu=sorted(antes-set(ids)-set(ret))
    if sumiu: sys.exit("ERRO: id de decisao sumiu sem ir para decisoes_ids_retirados: "+', '.join(sumiu))
valida_decisoes()
def carrega_respostas():
    p=os.path.join(D,'respostas-dono.json')
    if not os.path.exists(p): return {}
    try: r=json.load(open(p,encoding='utf-8'))['respostas']
    except Exception as e: sys.exit(f"ERRO: respostas-dono.json invalido: {e}")
    ok={d['id'] for d in J['decisoes']}|set(J['decisoes_ids_retirados'])
    out={}
    for i,v in r.items():
        h=v.get('historico') if isinstance(v,dict) else None
        if not h or not all(isinstance(x,dict) and x.get('resposta') in ('sim','nao') for x in h): sys.exit(f"ERRO: respostas-dono.json: historico invalido em {i}")
        if i not in ok: print(f"AVISO: resposta para id desconhecido {i} (ignorada)",file=sys.stderr); continue
        out[i]=h[-1]
    return out
RESP=carrega_respostas()
valida()
def vk(t): return [int(p) for p in re.findall(r'\d+',t)]
def grupo(r):
    s,rel=r['status'],r['release']
    if s!='lancada' and rel.startswith('2.0.3'): return '203'
    if rel.startswith('2.0.4'): return '204'
    if rel.startswith('2.0.5'): return '205'
    if rel.startswith(('2.1','2.2','futuro')) and s not in ('lancada','duplicada'): return 'fut'
    if s=='lancada': return 'ok'
    if r['estado_github']=='aberta': return 'sem'
    return 'fech'
G=collections.defaultdict(list)
for r in R: G[grupo(r)].append(r)
def resp(r):
    u=r['ultima_resposta']
    if not u['data']: return 'sem comentários'
    return ('nós ' if u['nos'] else 'autor ')+u['data'][5:]
def cell(s): return str(s).replace('|','/').replace('\n',' ')
def tab(rs):
    out=['| # | Título | Plat. | Tipo | Status | Release | Alvo | Conserto | Última resposta | Próximo passo |','|---|---|---|---|---|---|---|---|---|---|']
    for r in rs:
        fx=', '.join(h.split(' ')[0] for h in r['conserto'][:3]) if r['conserto'] and r['conserto'][0]!='sem commit' else ('sem commit' if r['conserto'] else '-')
        if r['status'] in ('aberta','precisa-log','respondida','por-desenho','fora-do-escopo','duplicada','fechada-sem-resposta') and not r['conserto']: fx='-'
        out.append(f"| [#{r['numero']}]({r['url']}) | {cell(r['titulo'])[:60]} | {cell(r['plataforma'])} | {r['tipo']} | {r['status']} | {cell(r['release'])} | {r.get('alvo','-')} | {cell(fx)} | {resp(r)} | {cell(r['proximo_passo'])[:90]} |")
    return '\n'.join(out)
def notas(rs):
    ls=[f"- **#{r['numero']}**: {r['notas']}" for r in rs if r['notas']]
    return ('\n\nNotas:\n\n'+'\n'.join(ls)) if ls else ''
L=[]
L.append(f"# Mapa vivo das issues\n\nBase: `{J['base']}` (integracao/2.0.3.1, que sai como 2.0.4; a 2.0.3 está na tag v2.0.3, 8ed4517c). Atualizado em {J['atualizado_em']}. {len(R)} issues (abertas e fechadas) de iqui27/nuvio-native-legacy.\n")
L.append("""## Como atualizar

1. Chegou issue nova, ou um conserto entrou numa branch/tag: edite **uma** entrada em `docs/issues/mapa.json` (procure por `"numero": N`) e rode `python3 docs/issues/mapa.py`, que valida e reescreve este arquivo. Sem o script, edite a linha equivalente aqui.
2. Campos: `numero`, `titulo`, `plataforma` (LG, Samsung .tpk, Samsung .wgt, Android, all, `?`), `tipo` (bug, feature, question, meta), `status`, `release` (2.0.2 ou tag antiga, 2.0.3 = integracao/2.0.3, `2.0.4` = hotfix em integracao/2.0.3.1, `2.0.5 (branch)`, 2.1/2.2/futuro, `-`), `conserto` (hashes curtos com a ref entre parênteses), `ultima_resposta` (`nos` = o último comentário é nosso, `data`, `ultimo_comentario_por`, `ultima_nossa`), `proximo_passo`, `notas`, `alvo` (só issues ABERTAS, obrigatório: `2.0.3`, `2.0.4`, `2.0.5`, `2.1`, `2.2`, `futuro`, `nao vamos fazer`, ou `ja-lancada` quando já saiu e só falta fechar). `python3 docs/issues/mapa.py --check` valida e confere se o MAPA.md está em dia, sem escrever; o gerador sai com erro se uma issue aberta não tiver alvo. O roadmap e as decisões ficam em `roadmap` e `decisoes` no json.
3. Vocabulário de `status`: `aberta`, `respondida`, `consertada-nao-lancada`, `lancada`, `por-desenho`, `fora-do-escopo`, `precisa-log`, `duplicada`. Extra: `fechada-sem-resposta` (issue fechada sem nenhum comentário).
4. Regra de honestidade: só vale `consertada-nao-lancada`/`lancada` com commit na ref. "Lançado" = commit contido numa tag `v*`. Sem commit, escreva "suspeita" ou "sem commit" em `conserto`/`notas`. "Lançada" em issue antiga sem commit com `#N` quer dizer: a nossa resposta cita uma versão que existe como tag (ver nota na linha).
5. Atenção: mensagens de commit com `(#203)` / `(#204)` falam da VERSÃO 2.0.3 / 2.0.4 (o plano que hoje é a 2.0.5; a linha 2.0.3.1 é que saiu como 2.0.4), não das issues #203/#204. Esses dois números foram ignorados na busca por commits.
6. Próximo passo "postar correção": há rascunhos em `/Volumes/ExternalSSD/tmp/203-respostas-correcao.md` (#302 #350 #286 #312 #368 #360); o dono decide. Correção conhecida: "Dolby Vision in MKV" é DESLIGADO por padrão (a resposta do #312 disse ligado).
""")
L.append("## Roadmap\n\nAlvo de cada issue aberta e os recursos por versão. Alvos são decisão de planejamento, não promessa pública; os pontos marcados SUSPEITA não têm confirmação.\n")
for x in J['roadmap']:
    L.append(f"### {x['release']} - {x['situacao']}\n\n{x['descricao']}\n")
    for i in x['itens']:
        iss=(' ('+', '.join('#'+str(n) for n in i['issues'])+')') if i['issues'] else ''
        og=f" Origem: {i['origem']}." if i.get('origem') else ''
        br=(' Branches: '+', '.join('`'+b+'`' for b in i['branches'])+'.') if i.get('branches') else ''
        L.append(f"- **{i['nome']}**{iss}: {i['porque']}{og}{br}")
    L.append('')
L.append("Plano de refatoração: `docs/plans/refatoracao-geral.md` (branch `agente/refatoracao-plano`, ainda não integrado).\n")
L.append("### Issues abertas por alvo\n")
L.append("| Alvo | Qtd | Issues |\n|---|---|---|")
for a in ALVOS:
    ns=sorted(r['numero'] for r in R if r['estado_github']=='aberta' and r['alvo']==a)
    L.append(f"| {a} | {len(ns)} | "+', '.join('#'+str(n) for n in ns)+" |")
L.append("\n## Fechar com a 2.0.3\n\nIssues ABERTAS no GitHub cujo conserto saiu na v2.0.3 (commits contidos na tag). Cada uma tem uma resposta curta em inglês para colar; \"autor confirmou\" diz se quem abriu já testou. O dono decide quando fechar.\n")
L.append("| # | Título | Autor confirmou? | Resposta curta (EN) |\n|---|---|---|---|")
for x in J.get('fechar_203',[]):
    r=next(i for i in R if i['numero']==x['numero'])
    L.append(f"| [#{r['numero']}]({r['url']}) | {cell(r['titulo'])[:60]} | "+('SIM: ' if x['confirmado'] else 'NÃO: ')+f"{cell(x['confirmacao'])} | {cell(x['resposta'])} |")
L.append("\n### Não fechar ainda\n")
L.append("| # | Título | Motivo |\n|---|---|---|")
for x in J.get('nao_fechar_ainda',[]):
    r=next(i for i in R if i['numero']==x['numero'])
    L.append(f"| [#{r['numero']}]({r['url']}) | {cell(r['titulo'])[:60]} | {cell(x['motivo'])} |")
L.append("\n## Decisões para o dono\n")
def estado_dec(d):
    if d.get('aplicada_em'): return 'aplicada'
    return 'respondida, a aplicar' if d['id'] in RESP else 'pendente'
def linha_resp(d):
    r=RESP.get(d['id'])
    if not r: return ''
    nota=(' Nota: '+cell(r['nota'])+'.') if r.get('nota') else ''
    return f" **Resposta do dono: {'SIM' if r['resposta']=='sim' else 'NÃO'}**{nota} ({r.get('quando','?')[:10]})."
pend=[d for d in J['decisoes'] if not d.get('aplicada_em')]
for k,d in enumerate(pend,1):
    L.append(f"{k}. `{d['id']}` [{estado_dec(d)}] {d['pergunta']} Recomendação: {d['recomendacao']}{linha_resp(d)}")
apl=[d for d in J['decisoes'] if d.get('aplicada_em')]
if apl:
    L.append('\n### Decisões aplicadas\n')
    for d in apl: L.append(f"- `{d['id']}` aplicada em {d['aplicada_em']}: {d['pergunta']}{linha_resp(d)}")
L.append('')
c=collections.Counter(r['status'] for r in R)
L.append("## Resumo\n\nPor status:\n\n| Status | Qtd |\n|---|---|")
for k,v in c.most_common(): L.append(f"| {k} | {v} |")
rc=collections.Counter()
for r in R:
    rel=r['release']
    if r['status']=='lancada': rc['lançadas em tag v* (qualquer versão)']+=1
    elif rel.startswith('2.0.3'): rc['2.0.3 lançada, com pendência']+=1
    elif rel.startswith('2.0.4'): rc['2.0.4 (hotfix, sem tag)']+=1
    elif rel.startswith('2.0.5'): rc['2.0.5 (branches)']+=1
    elif rel.startswith(('2.1','2.2','futuro')): rc['futuro (2.1/2.2)']+=1
    else: rc['sem release']+=1
L.append("\nPor release (grupo de planejamento):\n\n| Grupo | Qtd |\n|---|---|")
for k,v in rc.most_common(): L.append(f"| {k} | {v} |")
rl=collections.Counter(r['release'] for r in R if r['status']=='lancada')
L.append("\nLançadas por versão: "+', '.join(f"{k}: {v}" for k,v in sorted(rl.items(),key=lambda kv:vk(kv[0])))+".")
ab=sum(1 for r in R if r['estado_github']=='aberta'); L.append(f"\nAbertas no GitHub: {ab}. Fechadas: {len(R)-ab}.")
L.append(f"Abertas sem nenhum comentário nosso: {sum(1 for r in R if r['estado_github']=='aberta' and not r['ultima_resposta']['ultima_nossa'])}.\n")
sec=[('203','## 2.0.3 lançada com pendência (precisa-log, respondida ou conserto parcial)'),('204','## 2.0.4 (hotfix em integracao/2.0.3.1, sem tag)'),('205','## Planejado na 2.0.5 (com branch)'),('fut','## Futuro (2.1, 2.2, depois)'),('sem','## Aberta sem plano'),('ok','## Já lançado'),('fech','## Fechado sem conserto (por-desenho, fora-do-escopo, duplicada, respondida, sem resposta)')]
for k,t in sec:
    rs=G.get(k,[])
    L.append(f"{t}\n\n{len(rs)} issues.\n")
    if k=='ok':
        a=[r for r in rs if r['estado_github']=='aberta']; f=[r for r in rs if r['estado_github']!='aberta']
        L.append(f"### Lançadas e ainda abertas no GitHub ({len(a)})\n\n"+tab(a)+notas(a)+f"\n\n### Lançadas e fechadas ({len(f)})\n\n"+tab(f)+notas([r for r in f if r['notas']])+"\n")
    else:
        L.append(tab(rs)+notas(rs)+"\n")
L.append("## Fora do GitHub (Reddit)\n")
for o in J['fora_do_github']:
    L.append(f"### {o['titulo']}\n\n- Plataforma: {o['plataforma']}\n- Status: {o['status']}\n- Release: {o['release']}\n- Conserto: "+('; '.join(o['conserto']) if o['conserto'] else 'nenhum encontrado')+f"\n- Próximo passo: {o['proximo_passo']}\n- Notas: {o['notas']}\n")
out='\n'.join(L)
dest=os.path.join(D,'MAPA.md')
if '--check' in sys.argv:
    if open(dest).read()!=out: sys.exit('ERRO: MAPA.md fora de data; rode python3 docs/issues/mapa.py')
    print('ok (check)',len(R)); sys.exit(0)
open(dest,'w').write(out)
print('ok',len(R))
