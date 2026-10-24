// Row 22 (mapa-erros-logs): o Cinemeta era consultado de novo a cada sync para
// os MESMOS episodios ausentes (329 linhas em 14 registros). O cache negativo
// de 24 h por serie tem de segurar o GET da segunda volta em diante.
// O item continua caindo (nao ha S9E19) — so nao se pergunta mais.
#include "trakt.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int SDL_GetTicks(void) { return 0; }
void marco(const char *m) { (void)m; }
// Mach-O: simkl_e_a_seguir entra como weak_import em trakt.c e um link
// estatico nao a resolve; o produtor forte (simkl.c) nao vem aqui.
int simkl_e_a_seguir(const char *id) { (void)id; return 0; }

static int nuvioGets, cineGets;
static long long estreiaMs;
static char estreiaId[48];

static char *ler(const char *nome) {
  char path[200]; FILE *f; char *b; long n;
  snprintf(path, sizeof path, "tests/fixtures/%s", nome);
  f = fopen(path, "rb"); assert(f);
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
  b = malloc((size_t)n + 1); assert(fread(b, 1, (size_t)n, f) == (size_t)n);
  b[n] = 0; fclose(f); return b;
}
char *rede_baixar(const char *url, int s) {
  (void)s;
  if (strstr(url, "catalog.nuvio.tv") && strstr(url, "tt7587890")) {
    nuvioGets++; return ler("nuvio_the_rookie.json");
  }
  if (strstr(url, "cinemeta") && strstr(url, "tt7587890")) {
    cineGets++; return ler("cinemeta_the_rookie.json");
  }
  return NULL;
}
char *rede_baixar_com(const char *u, int s, const char *const *c) { (void)u; (void)s; (void)c; return NULL; }
char *rede_apagar(const char *u, int s, const char *const *c, int *st) { (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
void rede_avisar_401(void (*cb)(const char *)) { (void)cb; }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)u; (void)s; (void)c; if (st) *st = 0; return NULL;
}
int cwo_conta_a_seguir(const char *id) { (void)id; return 1; }   // tudo aqui e "a seguir"
void cwo_conta_trocar(const char *a, const char *b) { (void)a; (void)b; }
void cwo_marcar_estreia(const char *id, long long ms) { snprintf(estreiaId, sizeof estreiaId, "%s", id); estreiaMs = ms; }
int cwo_virada_aceita(long long e, long long a) { (void)e; (void)a; return 1; }
int ajustes_tmdb_cw(void) { return 0; }
const char *ajustes_tmdb_idioma(void) { return "en-US"; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "en"; }
const char *i18n(const char *s) { return s; }
unsigned long long cat_historico_geracao(void) { return 1; }
int cat_historico_definir_se_geracao(const char *a, const char *b, int c, unsigned long long g) {
  (void)a; (void)b; (void)c; (void)g; return 1;
}

static int contarPadrao(const char *cam, const char *padrao) {
  FILE *f = fopen(cam, "r");
  int n = 0;
  char l[512];
  if (!f) return -1;
  while (fgets(l, sizeof l, f))
    if (strstr(l, padrao)) n++;
  fclose(f);
  return n;
}

// tt7587890 = The Rookie: o Nuvio E o Cinemeta listam so S9E1..E3.
// O item testado, S9E19, nao existe em nenhum dos dois.
static int um(const char *id, int t, int e, CatItem *o) {
  static CatItem v[1];
  memset(v, 0, sizeof v);
  snprintf(v[0].tipo, sizeof v[0].tipo, "series");
  snprintf(v[0].titulo, sizeof v[0].titulo, "Serie ficticia");
  snprintf(v[0].imdb, sizeof v[0].imdb, "%s:%d:%d", id, t, e);
  v[0].temporada = t; v[0].episodio = e;
  int n = trakt_enfeitar_lote(v, 1);
  *o = v[0];
  return n;
}

int main(void) {
  CatItem r;
  FILE *out = freopen("/tmp/trakt-cine-neg.log", "w", stdout);
  int linhas;
  assert(out);

  // CENARIO DO MAPA: a ficha do Nuvio nao cobre o S9E19 e o Cinemeta tambem
  // nao. A linha reimprimia a cada sync — 155 vezes num registro.
  cineGets = 0; nuvioGets = 0;
  assert(um("tt7587890", 9, 19, &r) == 0);
  assert(cineGets == 1);   // a primeira volta pergunta ao Cinemeta

  // Segunda volta (sync seguinte): o MESMO episodio ausente NAO repete.
  cineGets = 0;
  assert(um("tt7587890", 9, 19, &r) == 0);
  assert(cineGets == 0);
  fflush(stdout);
  linhas = contarPadrao("/tmp/trakt-cine-neg.log", "Cinemeta tambem nao tem");
  if (linhas != 1) {
    fprintf(stderr, "FAIL: %d linhas 'Cinemeta tambem nao tem' em 2 voltas (esperado 1)\n", linhas);
    return 1;  }

  // Episodio DIFERENTE da mesma serie continua sendo conferido (#356): o
  // negativo e por episodio ("tt:SxEy"), nao por serie.
  cineGets = 0;
  assert(um("tt7587890", 9, 2, &r) == 1);
  assert(!strcmp(r.imdb, "tt7587890:9:2"));
  assert(r.nomeEpisodio[0]);
  assert(cineGets == 0);   // a ficha positiva do Cinemeta ja estava em cache

  fprintf(stderr, "trakt_cine_neg: PASS\n");
  fclose(out);
  return 0;
}
