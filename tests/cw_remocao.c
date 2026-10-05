// Removing from "Continuar assistindo" wins for real (issue #244).
//
//   bash tests/cw_remocao.sh
//
// Marking as watched left every card in place with only the progress bar erased,
// and "Tirar de Continuar assistindo" brought the work back. Two defences, of
// which this covers one in process:
//
//   1. delete EVERY record of the WORK (trakt_playback_remover - there used to be
//      a `break` on the first match, and the match itself is by BASE id because
//      the card can name another episode than the record).
//
// (2), the stamp when marking as watched, is one line mirroring the already
// tested path of `desc_tirar_continuar`; the stamp itself is exercised by
// tests/cwremover.sh. That the history is NOT a row filter is pinned by
// tests/cw_historico.sh.
//
// No network: /sync/playback, /sync/history and the Cinemeta /meta are doubles.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/trakt.c"

// Three records of the same episode (ids 101..103): what the `break` left
// behind. One of ANOTHER episode of the same work (201, E15): the card is one
// per work, so a press has to reach it too. One of another work (301): it must
// NOT be touched.
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
 "{\"id\":201,\"progress\":20,\"paused_at\":\"2026-10-04T10:00:00.000Z\","
 " \"type\":\"episode\",\"episode\":{\"season\":1,\"number\":15,\"title\":\"E15\"},"
 " \"show\":{\"title\":\"The World of the Married\",\"year\":2020,"
 "   \"ids\":{\"trakt\":1,\"slug\":\"wotm\",\"imdb\":\"tt12042964\",\"tmdb\":1}}},"
 "{\"id\":301,\"progress\":30,\"paused_at\":\"2026-10-02T12:00:00.000Z\","
 " \"type\":\"movie\","
 " \"movie\":{\"title\":\"Outro Filme\",\"year\":2021,"
 "   \"ids\":{\"trakt\":2,\"slug\":\"of\",\"imdb\":\"tt5555555\",\"tmdb\":2}}}]";

static int deleteCalls;         // how many DELETEs went out
static int deleteUrls;       // how many distinct URLs were requested
static int fail5xx;         // is the server answering 500?

char *rede_baixar_com(const char *url, int seg, const char *const *cab) {
  (void)seg; (void)cab;
  if (strstr(url, "/sync/playback"))       return strdup(PLAYBACK);
  if (strstr(url, "/sync/history"))        return strdup("[]");
  if (strstr(url, "/sync/watched/movies")) return strdup("[]");
  return NULL;
}
char *rede_baixar(const char *url, int seg) {
  (void)seg;
  if (!strstr(url, "/meta/")) return NULL;
  return strdup("{\"meta\":{\"imdbRating\":\"7.9\",\"releaseInfo\":\"2020\","
                "\"description\":\"d\",\"name\":\"n\","
                "\"videos\":[{\"id\":\"tt12042964:1:14\",\"released\":\"2020-05-01\",\"name\":\"E14\"}]}}");
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st)
{ (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_apagar(const char *url, int seg, const char *const *cab, int *st) {
  (void)seg; (void)cab;
  if (strstr(url, "/sync/playback/")) { deleteCalls++; deleteUrls++; }
  if (st) *st = fail5xx ? 500 : 200;
  return strdup("");
}
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

int main(void) {
  CatItem v[64];
  int n, i, found;

  trakt_definir("token", "client");

  // 1. play[] knows every record: three of episode E14 (what the `break` left
  //     behind), one of E15 - the same WORK, another episode - and one of
  //     another work.
  n = trakt_continuar(v, 64);
  assert(n >= 1);      // how many cards come out is tests/cw_duplicados.sh;
                       // what matters here is that the records were kept
  found = 0;
  for (i = 0; i < nPlay; i++)
    if (idbase_equal(play[i].chave, "tt12042964")) found++;
  assert(found == 4);
  puts("ok  1. play[] knows the 4 records of the work (E14 x3, E15)");

  // 2. One press removes the WORK. The card can name an episode no record has
  //    (the progress pass re-points it to the newest local episode): the match
  //    is by BASE id, so all four go and the other work keeps its record.
  deleteCalls = 0; deleteUrls = 0;
  assert(trakt_playback_remover("tt12042964:1:9") == 1);
  assert(deleteCalls == 4);                // it was 1 with the `break`
  assert(deleteUrls == 4);
  found = 0;
  for (i = 0; i < nPlay; i++)
    if (idbase_equal(play[i].chave, "tt5555555")) found++;
  assert(found == 1);
  puts("ok  2. one press deletes the 4 records of the work; the other work is untouched");

  // 3. Nothing left to delete.
  deleteCalls = 0;
  assert(trakt_playback_remover("tt12042964:1:9") == 0);
  assert(deleteCalls == 0);
  puts("ok  3. the second attempt asks for no DELETE at all");

  // 4. The Salvos panel passes the bare TITLE id ("O ID DO TITULO, nunca do
  //    episodio", ctxmenu.c): it has to reach the records too.
  trakt_continuar(v, 64);
  deleteCalls = 0;
  assert(trakt_playback_remover("tt12042964") == 1);
  assert(deleteCalls == 4);
  puts("ok  4. the bare title id reaches every record of the work");

  // 5. A 5xx must not erase the table rows, or the item can never be removed
  //    again without reopening.
  trakt_continuar(v, 64);
  fail5xx = 1;
  deleteCalls = 0;
  assert(trakt_playback_remover("tt12042964:1:14") == 0);
  assert(deleteCalls == 4);                // the requests went out...
  fail5xx = 0;
  deleteCalls = 0;
  assert(trakt_playback_remover("tt12042964:1:14") == 1);
  assert(deleteCalls == 4);                // ...and the ids survived for the retry
  puts("ok  5. a 5xx keeps the ids; the retry deletes");

  puts("cw_remocao: all ok (the stamp: tests/cwremover.sh)");
  return 0;
}
