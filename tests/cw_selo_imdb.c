// IMDb badge on the "Continuar assistindo" card (issue #243).
//
//   bash tests/cw_selo_imdb.sh
//
// The defect: `?extended=full` has no `imdbRating`, so an item arriving with art
// and synopsis from that block looked "ready" to the shortcut in
// trakt_enfeitar_lote, `enfeitar` never ran, and `d->nota` stayed 0 - the card
// shipped without the badge.
//
// This test intercepts only `rede_baixar` (the one network call on the path) and
// lets the real embellishing run, so it covers the correction end to end: the
// "complete" item is no longer skipped; a second refill does not touch the
// network; a title with no Cinemeta rating costs one GET per session, not per
// refill; an account switch forgets that; a failed GET does not count as an
// answer; and Trakt's `certification` replaced the hard-coded "14".
//
// All in process, no screen, no network.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/trakt.c"

// --- fake network: only the Cinemeta /meta/<type>/<tt>.json ------------------
static int calls;      // how many GETs the embellishing asked for
static int withRating = 1;   // does the Cinemeta have `imdbRating`?
static int netDown   = 0;   // is the network down?

char *rede_baixar(const char *url, int seg) {
  (void)seg;
  if (!strstr(url, "/meta/")) return NULL;
  calls++;
  if (netDown) return NULL;
  if (withRating)
    return strdup("{\"meta\":{\"imdbRating\":\"7.9\",\"releaseInfo\":\"2020\","
                  "\"description\":\"d\",\"name\":\"n\"}}");
  return strdup("{\"meta\":{\"releaseInfo\":\"2020\",\"description\":\"d\","
                "\"name\":\"n\"}}");
}

// --- doubles that exist only for the linker (none of this is exercised) -----
const char *i18n(const char *s) { return s; }
unsigned long long cat_historico_geracao(void) { return 1; }
int cat_historico_definir_se_geracao(const char *a, const char *b, int c,
                                     unsigned long long d)
{ (void)a; (void)b; (void)c; (void)d; return 1; }
const char *cat_tipo_por_imdb(const char *i) { (void)i; return "movie"; }
int ajustes_tmdb_cw(void) { return 0; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }
const char *nuvem_trakt_cliente(void) { return ""; }
char *rede_baixar_com(const char *u, int s, const char *const *c)
{ (void)u; (void)s; (void)c; return NULL; }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st)
{ (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_apagar(const char *u, int s, const char *const *c, int *st)
{ (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_postar_st(const char *u, int s, const char *const *c, const char *b,
                     int *st)
{ (void)u; (void)s; (void)c; (void)b; if (st) *st = 0; return NULL; }
void rede_avisar_401(void (*cb)(const char *)) { (void)cb; }
const char *scrobble_nome(int a) { (void)a; return "pause"; }
void scrobble_corpo(char *d, size_t n, const char *i, double p)
{ (void)i; (void)p; if (n) d[0] = 0; }
int scrobble_decidir(ScrobbleEstado *s, int evento, const char *id, double pct)
{ (void)s; (void)evento; (void)id; (void)pct; return 0; }
int cwo_conta_a_seguir(const char *i) { (void)i; return 0; }
void cwo_conta_trocar(const char *a, const char *b) { (void)a; (void)b; }
void cwo_marcar_estreia(const char *i, long long m) { (void)i; (void)m; }
int cwo_virada_aceita(long long a, long long b) { (void)a; (void)b; return 0; }

// --- an item in the state Trakt delivers it ------------------------------
// Art and synopsis present (which is what `doBlocoTrakt` does), rating ABSENT
// (the `extended=full` has no `imdbRating`). This was the item that escaped.
static void makeTraktItem(CatItem *d, const char *imdb) {
  memset(d, 0, sizeof *d);
  snprintf(d->imdb, sizeof d->imdb, "%s", imdb);
  snprintf(d->tipo, sizeof d->tipo, "series");
  snprintf(d->poster, sizeof d->poster, "p.jpg");
  snprintf(d->backdrop, sizeof d->backdrop, "b.jpg");
  snprintf(d->sinopse, sizeof d->sinopse, "synopsis");
  snprintf(d->titulo, sizeof d->titulo, "The World of the Married");
}

int main(void) {
  CatItem d[1];

  // 1. THE DEFECT. `GET=0 nota=0` was the old output for exactly this item.
  makeTraktItem(&d[0], "tt12042964:1:14");
  calls = 0;
  assert(trakt_enfeitar_lote(d, 1) == 1);
  assert(calls == 1);
  assert(d[0].nota == 79);            // 7.9 -> 79 (x10, like the catalog)
  puts("ok  1. Trakt item without a rating: embellishing runs and the IMDb badge enters");

  // 2. With the rating read, the next refill does not go back to the network.
  //    A REFILL REBUILDS the item from scratch (montarContinuar) - simulate it.
  makeTraktItem(&d[0], "tt12042964:1:14");
  calls = 0;
  assert(trakt_enfeitar_lote(d, 1) == 1);
  assert(calls == 0);
  assert(d[0].nota == 79);
  puts("ok  2. rating present: no new GET");

  // 3. Title WITHOUT a rating in the Cinemeta: one GET per session, not per
  //    refill. Also covers another EPISODE of the same work - the question
  //    belongs to the TITLE.
  ratingForget();
  withRating = 0;
  { CatItem b[1], c[1], e[1];
    makeTraktItem(&b[0], "tt9999999:1:1"); calls = 0;
    assert(trakt_enfeitar_lote(b, 1) == 1);
    assert(calls == 1);
    assert(b[0].nota == 0);           // no rating: no badge - and never a "14"
    makeTraktItem(&c[0], "tt9999999:1:1");  // same work, next refill
    assert(trakt_enfeitar_lote(c, 1) == 1);
    assert(calls == 1);
    makeTraktItem(&e[0], "tt9999999:1:7");  // another episode, same question
    assert(trakt_enfeitar_lote(e, 1) == 1);
    assert(calls == 1);
    puts("ok  3. no rating in the Cinemeta: one GET per session, not per refill"); }

  // 4. The memory belongs to the CREDENTIAL SESSION. The switch goes through
  //    trakt_esquecer - the wiring is the claim here, not the reset function.
  trakt_esquecer();
  { CatItem f[1];
    makeTraktItem(&f[0], "tt9999999:1:1");
    assert(trakt_enfeitar_lote(f, 1) == 1);
    assert(calls == 2);
    puts("ok  4. a credential switch forgets the memory and asks again"); }

  // 5. A network failure does NOT mark the question: the title tries again and
  //    succeeds.
  ratingForget();
  { CatItem g[1], h[1];
    netDown = 1; withRating = 1; calls = 0;
    makeTraktItem(&g[0], "tt12042964:1:14");
    assert(trakt_enfeitar_lote(g, 1) == 1);
    netDown = 0;
    makeTraktItem(&h[0], "tt12042964:1:14");
    assert(trakt_enfeitar_lote(h, 1) == 1);
    assert(calls == 2);
    assert(h[0].nota == 79);
    puts("ok  5. a network failure does not mark the question"); }

  // 6. The hard-coded `"14"` is gone; Trakt's `certification` takes its place.
  //    Without `certification` the field stays EMPTY - the drawing side is
  //    already guarded by `classificacao[0]`, so the badge simply does not
  //    appear.
  { CatItem x[2];
    memset(x, 0, sizeof x);
    doBlocoTrakt(&x[0], "{\"year\":2020,\"certification\":\"TV-MA\"}", NULL, "series");
    assert(!strcmp(x[0].classificacao, "TV-MA"));
    memset(&x[1], 0, sizeof x[1]);
    doBlocoTrakt(&x[1], "{\"year\":2020}", NULL, "series");
    assert(x[1].classificacao[0] == 0);
    puts("ok  6. Trakt certification enters; without it the field stays empty"); }

  puts("cw_selo_imdb: all ok");
  return 0;
}
