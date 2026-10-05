// Removing the WORK on Simkl (issue #244).
//
//   bash tests/simkl_remocao.sh
//
// `simkl_playback_remover` had a `break` on the first match keyed by the
// composite episode id - the same defect `trakt_playback_remover` had. The row
// is one card per work, and the card can name another episode than the record
// behind it (the progress pass re-points it): the match is the BASE id
// (idbase.h) and every record of the work is deleted.
//
// Includes src/simkl.c (the statics are the unit) and doubles only the network
// and the token. The threads are not exercised: `playWorkIds` and `fioApagar`
// run synchronously here; `simkl_playback_remover` just spawns one `fioApagar`
// per id it collects.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/catalogo.h"

// ---- doubles ---------------------------------------------------------------
static const char *tokenAtual = "TOKEN-TESTE";
const char *simklauth_token(void) { return tokenAtual; }
const char *nuvem_simkl_cliente(void) { return "cid-teste"; }
const char *nuvem_simkl_app(void) { return "nuvio"; }
void nuvem_url_escapar(const char *v, char *dst, unsigned tam) { snprintf(dst, tam, "%s", v); }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
int trakt_enfeitar_lote(CatItem *saida, int n) { (void)saida; return n; }
const char *cat_tipo_por_imdb(const char *i) { (void)i; return "movie"; }
char *rede_baixar(const char *u, int s) { (void)u; (void)s; return NULL; }
char *rede_baixar_com(const char *u, int s, const char *const *c)
{ (void)u; (void)s; (void)c; return NULL; }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st)
{ (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_postar_st(const char *u, int s, const char *const *c, const char *b, int *st)
{ (void)u; (void)s; (void)c; (void)b; if (st) *st = 200; return strdup(""); }
void rede_avisar_401(void (*cb)(const char *)) { (void)cb; }

static int nDelete, statusDelete = 200;
static char lastDelete[200];
char *rede_apagar(const char *u, int s, const char *const *c, int *st) {
  (void)s; (void)c;
  nDelete++;
  snprintf(lastDelete, sizeof lastDelete, "%s", u);
  if (st) *st = statusDelete;
  return strdup("");
}

#include "../src/simkl.c"

// ---- the fixture: play[] the way simkl_continuar fills it -------------------
static void fillPlay(void) {
  snprintf(play[0].chave, sizeof play[0].chave, "%s", "tt12042964:1:14"); play[0].id = 101;
  snprintf(play[1].chave, sizeof play[1].chave, "%s", "tt12042964:1:15"); play[1].id = 102;
  snprintf(play[2].chave, sizeof play[2].chave, "%s", "tt5555555");       play[2].id = 301;
  nPlay = 3;
}

int main(void) {
  long long ids[16];
  int n;

  fillPlay();

  // 1. The work matches by BASE id, all of its records - the card can name an
  //    episode no record has. The neighbour work stays apart.
  n = playWorkIds("tt12042964:1:9", ids, 16);
  assert(n == 2 && ids[0] == 101 && ids[1] == 102);
  n = playWorkIds("tt5555555", ids, 16);
  assert(n == 1 && ids[0] == 301);
  n = playWorkIds("tt9999999", ids, 16);
  assert(n == 0);
  puts("ok  1. the work matches every record, by BASE id");

  // 2. The delete forgets the id only on 2xx (the trakt_playback_remover rule).
  { long long *one = (long long *)malloc(sizeof *one);
    *one = 101;
    statusDelete = 500; nDelete = 0;
    fioApagar(one);
    assert(nDelete == 1 && strstr(lastDelete, "/sync/playback/101"));
    assert(playWorkIds("tt12042964", ids, 16) == 2);   // kept for the retry
    one = (long long *)malloc(sizeof *one);
    *one = 101;
    statusDelete = 200;
    fioApagar(one);
    assert(playWorkIds("tt12042964", ids, 16) == 1 && ids[0] == 102);
    puts("ok  2. a 5xx keeps the id; the 2xx forgets it"); }

  puts("simkl_remocao: all ok");
  return 0;
}
