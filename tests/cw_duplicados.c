// The same work in a single card (issue #244, the row half).
//
//   bash tests/cw_duplicados.sh
//
// `/sync/playback` returns one record per paused resume and nothing guarantees one
// per work - the same episode paused twice yields two records with the same
// composite id ("tt123:1:14"), and each one became its own card.
//
// Covers the merge by work in `trakt_continuar`: N records of one work become one
// card (the newest `paused_at`); different works stay apart; and the edge cases of
// "same work" - same episode, same title with another episode, another season,
// another title, an id prefix, and non-IMDb addon ids such as "kitsu:41370:1",
// which idbase.h cuts at the SECOND ':' rather than the first.
//
// The other halves of #244 are in tests/cw_remocao.sh and tests/cw_historico.sh.
// No network: /sync/playback, /sync/history and the Cinemeta /meta are doubles.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/trakt.c"

// Six records of the SAME work (T1E14 in three of them, and the other two are
// other episodes of the same series), plus a movie from ANOTHER work.
static const char *PLAYBACK =
"[{\"id\":101,\"progress\":12.5,\"paused_at\":\"2026-10-01T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":1,\"number\":14,\"title\":\"E14\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":102,\"progress\":12.5,\"paused_at\":\"2026-10-02T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":1,\"number\":14,\"title\":\"E14\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":103,\"progress\":40,\"paused_at\":\"2026-10-03T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":1,\"number\":14,\"title\":\"E14\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":104,\"progress\":9,\"paused_at\":\"2026-09-20T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":1,\"number\":15,\"title\":\"E15\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":105,\"progress\":20,\"paused_at\":\"2026-09-01T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":2,\"number\":1,\"title\":\"S2E1\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":201,\"progress\":30,\"paused_at\":\"2026-10-02T12:00:00.000Z\","
 " \"type\":\"movie\","
 " \"movie\":{\"title\":\"Outro Filme\",\"year\":2021,"
 "   \"ids\":{\"trakt\":2,\"slug\":\"of\",\"imdb\":\"tt5555555\",\"tmdb\":2}}}]";

static const char *scene;    // which body /sync/playback returns

char *rede_baixar_com(const char *url, int seg, const char *const *cab) {
  (void)seg; (void)cab;
  if (strstr(url, "/sync/playback"))       return strdup(scene);
  if (strstr(url, "/sync/history"))        return strdup("[]");
  if (strstr(url, "/sync/watched/movies")) return strdup("[]");
  return NULL;
}
char *rede_baixar(const char *url, int seg) {
  (void)seg;
  if (!strstr(url, "/meta/")) return NULL;
  return strdup("{\"meta\":{\"imdbRating\":\"7.9\",\"releaseInfo\":\"2020\","
                "\"description\":\"d\",\"name\":\"n\","
                "\"videos\":[{\"id\":\"tt12042964:1:14\",\"released\":\"2020-05-01\",\"name\":\"E14\"},"
                            "{\"id\":\"tt12042964:1:15\",\"released\":\"2020-05-08\",\"name\":\"E15\"},"
                            "{\"id\":\"tt12042964:2:1\",\"released\":\"2021-05-01\",\"name\":\"S2E1\"}]}}");
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st)
{ (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_apagar(const char *url, int seg, const char *const *cab, int *st)
{ (void)url; (void)seg; (void)cab; if (st) *st = 200; return strdup(""); }
char *rede_postar_st(const char *u, int s, const char *const *c, const char *b,
                     int *st)
{ (void)u; (void)s; (void)c; (void)b; if (st) *st = 200; return strdup(""); }
void rede_avisar_401(void (*cb)(const char *)) { (void)cb; }

// --- link doubles ------------------------------------------------------------
const char *i18n(const char *s) { return s; }
unsigned long long cat_historico_geracao(void) { return 1; }
int cat_historico_definir_se_geracao(const char *a, const char *b, int c,
                                     unsigned long long d)
{ (void)a; (void)b; (void)c; (void)d; return 1; }
void cat_historico_definir_id(const char *a, const char *b, int c)
{ (void)a; (void)b; (void)c; }
int cat_historico_estado_id(const char *a, const char *b) { (void)a; (void)b; return -1; }
const char *cat_tipo_por_imdb(const char *i) { (void)i; return "movie"; }
int ajustes_tmdb_cw(void) { return 0; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }
const char *nuvem_trakt_cliente(void) { return ""; }
const char *scrobble_nome(int a) { (void)a; return "pause"; }
void scrobble_corpo(char *d, size_t n, const char *i, double p)
{ (void)i; (void)p; if (n) d[0] = 0; }
int scrobble_decidir(ScrobbleEstado *s, int e, const char *i, double p)
{ (void)s; (void)e; (void)i; (void)p; return 0; }
int cwo_conta_a_seguir(const char *i) { (void)i; return 0; }
void cwo_conta_trocar(const char *a, const char *b) { (void)a; (void)b; }
void cwo_marcar_estreia(const char *i, long long m) { (void)i; (void)m; }
int cwo_virada_aceita(long long a, long long b) { (void)a; (void)b; return 0; }
int simkl_e_a_seguir(const char *i) { (void)i; return 0; }
int simkl_playback_remover(const char *i) { (void)i; return 0; }
void simkl_esquecer(void) {}

// --- the merge by work, no network and without the rest of the app -----------
// idbase_equal is the criterion the row uses: the BASE id, the way idbase.h
// cuts it. Worth exercising directly, because it is what decides what
// "the same work" is - including for an id that is not IMDb.

int main(void) {
  CatItem v[64];
  int n;

  scene = PLAYBACK;
  trakt_definir("token", "client");     // without credentials trakt_continuar leaves early

  // --- A. five records of the same work in ONE card, the newest one ---------
  scene = PLAYBACK;
  n = trakt_continuar(v, 64);
  assert(n == 2);                                   // the series + the movie
  assert(!strcmp(v[0].imdb, "tt12042964:1:14"));    // the 10-03 record (id 103)
  assert(v[0].progresso == 40);                     // the one with the NEWEST paused_at
  assert(!strcmp(v[1].imdb, "tt5555555"));          // the movie stays
  puts("ok  A. 6 records -> 2 works; the series stays at the newest record");

  // --- B. different works stay different cards ------------------------------
  //     (covered by A: the series + the movie. Explicitly here, and with the
  //     neighbour of ANOTHER type, so the merge does not become a "dedup of
  //     everything")
  assert(strcmp(v[0].imdb, v[1].imdb) != 0);
  assert(!strcmp(v[0].tipo, "series") && !strcmp(v[1].tipo, "movie"));
  puts("ok  B. different works (series and movie) are not merged");

  // --- C. the edge cases of "the same work" --------------------------------
  assert(idbase_equal("tt12042964:1:14", "tt12042964:1:14") == 1);   // same episode
  assert(idbase_equal("tt12042964:1:14", "tt12042964:1:15") == 1);   // same title, other episode
  assert(idbase_equal("tt12042964:1:14", "tt12042964:2:1") == 1);    // other season
  assert(idbase_equal("tt12042964:1:14", "tt99999999:1:1") == 0);   // other title
  assert(idbase_equal("tt12042964:1:14", "tt1204296") == 0);        // prefix, not the same id
  // ADDON ID: "kitsu:41370:1" cuts at the SECOND ':' (idbase.h). A blind cut at
  // the first would give "kitsu" for every anime from the addon - the row would
  // merge different works, and so would the progress key.
  assert(idbase_equal("kitsu:41370:1:1", "kitsu:41370:1:2") == 1);
  assert(idbase_equal("kitsu:41370:1:1", "kitsu:99999:1:1") == 0);
  puts("ok  C. same id/episode/season merge; title and addon id do not");
  // (An addon id without "tt" never reaches the row: with no metahub art,
  //  `enfeitar` compacts it. That is the documented behaviour - the merge above
  //  is what matters here.)

  puts("cw_duplicados: all ok");
  return 0;
}
