// Executa o Spotlight real; so a rede e a saida grafica sao controladas.
// Acesso aos estaticos evita acrescentar API de teste ao produto.
#define desc_buscar teste_buscar
#define desc_busca_n_alvos teste_alvos
#define desc_busca_alvo_n teste_n
#define desc_busca_alvo_item teste_item
#define desc_busca_n teste_total
#define desc_buscando teste_buscando
#define desc_busca_geracao teste_geracao
#define spotpessoa_pedir teste_pessoas_pedir
#define spotpessoa_atualizar teste_pessoas_atualizar
#define spotpessoa_n teste_pessoas_n
#define spotpessoa_geracao teste_pessoas_geracao
#define guia_preparar_busca teste_guia
#define gfx_recorte teste_recorte
#define tex_obter_larg teste_tex
#define tex_falhou teste_tex_falhou
#define gfx_esqueleto teste_esqueleto
#define gfx_cor teste_cor
#define gfx_sem_recorte teste_sem_recorte
#define txt_linha teste_linha
#define txt_linha_corta teste_linha_corta
#define txt_desenhar_alpha teste_texto
#define txt_tracking teste_tracking
#ifndef SPOTLIGHT_SOURCE
#define SPOTLIGHT_SOURCE "../src/spotlight.c"
#endif
#include SPOTLIGHT_SOURCE
#include "dados.h"
#include <assert.h>

static CatItem rem[2][4];
static int quant[2], falhas, cortadas, artes;
static float fimRecorte;
static char termo[96];
static char generoDesenhado[160];
void teste_buscar(const char *t) { snprintf(termo, sizeof termo, "%s", t); }
int teste_alvos(void) { return 2; }
int teste_n(int a, const char *t) { return !strcmp(t, termo) ? quant[a] : 0; }
int teste_item(int a, int i, CatItem *it) { *it = rem[a][i]; return 1; }
int teste_total(const char *t) { return teste_n(0, t) + teste_n(1, t); }
int teste_buscando(void) { return 0; }
int teste_geracao(void) { return 1; }
void teste_pessoas_pedir(const char *t, unsigned a) { (void)t; (void)a; }
void teste_pessoas_atualizar(unsigned a) { (void)a; }
int teste_pessoas_n(const char *t) { (void)t; return 0; }
unsigned teste_pessoas_geracao(void) { return 0; }
void teste_guia(void) {}
void teste_sem_recorte(void) {}
void teste_cor(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)r; (void)raio; (void)cr; (void)cg; (void)cb; (void)a;
}
TxtLinha teste_linha(TxtEstilo e, const char *s, int r, int g, int b, int a) {
  (void)e; (void)s; (void)r; (void)g; (void)b; (void)a;
  return (TxtLinha){0, 100, 20, 0, 0};
}
TxtLinha teste_linha_corta(TxtEstilo e, const char *s, int r, int g, int b, int a, float w) {
  if (e == TXT_ILHA_GENERO) snprintf(generoDesenhado, sizeof generoDesenhado, "%s", s);
  (void)w; return teste_linha(e, s, r, g, b, a);
}
void teste_texto(TxtLinha l, float x, float y, float a) { (void)l; (void)x; (void)y; (void)a; }
float teste_tracking(TxtEstilo e, const char *s, int r, int g, int b,
                     float x, float y, float a, float tracking) {
  (void)e; (void)s; (void)r; (void)g; (void)b; (void)x; (void)y; (void)a; (void)tracking;
  return 100;
}
void teste_recorte(float x, float y, float w, float h) {
  (void)x; (void)w; fimRecorte = y + h;
}
GLuint teste_tex(const char *url, float w) { (void)url; (void)w; return 0; }
int teste_tex_falhou(const char *url) { (void)url; return 0; }
void teste_esqueleto(GfxRect r, float raio, float cr, float cg, float cb, float a) {
  (void)raio; (void)cr; (void)cg; (void)cb; (void)a;
  artes++;
  if (r.y + r.h > fimRecorte + 0.1f) cortadas++;
}
static void check(int ok, const char *msg) {
  printf("%s %s\n", ok ? "PASS" : "FAIL", msg);
  if (!ok) falhas++;
}
static CatItem item(const char *id, const char *tipo, const char *ano, int nota) {
  CatItem c = {0};
  snprintf(c.imdb, sizeof c.imdb, "%s", id);
  snprintf(c.tipo, sizeof c.tipo, "%s", tipo);
  snprintf(c.titulo, sizeof c.titulo, "Silo");
  snprintf(c.meta, sizeof c.meta, "%s", ano);
  snprintf(c.poster, sizeof c.poster, "poster-%s", id);
  snprintf(c.backdrop, sizeof c.backdrop, "fundo-%s", id);
  c.nota = nota;
  return c;
}
static void montar(void) {
  spot_abrir(0);
  snprintf(consulta, sizeof consulta, "silo"); nConsulta = 4;
  remontar();
}
static void assentar(void) {
  for (int i = 0; i < 180; i++) spot_atualizar(1.0f / 60, (unsigned)i);
}
static const char *topoId(void) { return cat_item(lin[1].ref)->imdb; }
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS) == 0);
  dados_iniciar(getenv("NUVIO_DADOS")); ajustes_iniciar();
  cat_quadro();
  rem[0][0] = item("tt2015", "movie", "2015", 89);
  rem[0][1] = item("tt2021", "movie", "2021", 60);
  rem[0][2] = item("tt2014", "movie", "2014", 72);
  rem[1][0] = item("tt2023", "series", "2023", 81);
  quant[0] = 3; quant[1] = 1;
  CatItem serie = rem[1][0];
  CatFileira cw = {0};
  strcpy(cw.chave, "continue_watching"); cw.n = 1;
  cat_definir_tudo(&serie, 1, &cw, 1);
  montar();
  check(!strcmp(topoId(), serie.imdb), "Continuar assistindo vence homonimo do primeiro alvo");

  serie.naLista = 1;
  cat_definir_tudo(&serie, 1, NULL, 0); montar();
  check(!strcmp(topoId(), serie.imdb), "biblioteca fora das fileiras vence nota 8.9");
  // Ausente da resposta remota tambem precisa aparecer.
  quant[1] = 0; cat_definir_tudo(&serie, 1, NULL, 0); montar();
  check(!strcmp(topoId(), serie.imdb), "biblioteca local sem resposta remota");
  quant[1] = 1;

  strcpy(serie.titulo, "Silo extra");
  strcpy(rem[1][0].titulo, "Silo extra");
  cat_definir_tudo(&serie, 1, &cw, 1); montar();
  check(!strcmp(topoId(), "tt2015"), "titulo exato precede titulo pessoal parcial");
  strcpy(rem[1][0].titulo, "Silo");

  // Sem dado pessoal: ordem da fonte, sem usar nota como popularidade.
  CatItem base = item("ttBase", "movie", "2000", 0);
  strcpy(base.titulo, "Outro");
  cat_definir_tudo(&base, 1, NULL, 0); montar();
  check(!strcmp(topoId(), "tt2015"), "ordem da fonte preservada sem popularidade conhecida");
  for (int i = 0; i < nLin; i++) if (lin[i].tipo == L_TOPO || lin[i].tipo == L_TITULO) {
    const CatItem *c = cat_item(lin[i].ref);
    check(!strcmp(lin[i].arte, lin[i].tipo == L_TOPO ? c->backdrop : c->poster),
          "arte copiada do mesmo titulo depois de ordenar");
  }
  painel = P_LISTA; focoL = 1; assentar();
  cortadas = artes = 0; desenhaLista(0, 1);
  check(artes > 0 && cortadas == 0, "nenhum poster parcial invade o rodape a 130%");
  focoL = nLin - 1; assentar();
  cortadas = artes = 0; desenhaLista(0, 1);
  check(artes > 0 && cortadas == 0 &&
        lin[focoL].y + lin[focoL].h - scrollY <= listaVisivel() + .1f,
        "ultima linha inteira ao navegar para baixo");

  // Sem rede: publicacao do catalogo desloca o indice e muda a arte.
  quant[0] = quant[1] = 0;
  CatItem lote[2] = { item("ttA", "series", "2023", 81), item("ttB", "movie", "2015", 89) };
  strcpy(lote[0].genero, "Drama");
  cw.n = 1; cat_definir_tudo(lote, 1, &cw, 1); montar();
  lote[1] = lote[0]; lote[0] = rem[0][0]; cw.ini = 1;
  strcpy(lote[0].genero, "Documentary");
  strcpy(lote[1].backdrop, "fundo-novo-A");
  cat_definir_tudo(lote, 2, &cw, 1);
  desenhaLinha(1, 0, 0, 1);
  check(!strcmp(generoDesenhado, "Drama"), "publicacao entre update e desenho nao mistura titulo e genero");
  spot_atualizar(.016f, 1);
  check(lin[1].ref == 1 && !strcmp(lin[1].arte, "fundo-novo-A"),
        "revisao do catalogo atualiza indice e arte sem nova consulta");
  // OK chega antes do proximo atualizar (ordem dos eventos no app).
  cw.ini = 0; cat_definir_tudo(&lote[1], 1, &cw, 1); montar();
  cw.ini = 1; cat_definir_tudo(lote, 2, &cw, 1);
  acionar(1);
  SpotPedido p;
  check(spot_pediu(&p) && p.indice == 1, "OK entre publicacao e quadro abre a identidade escolhida");
  printf("RESULTADO: %d falhas\n", falhas);
  return falhas ? 1 : 0;
}
