// A CARD'S ID AND ITS EPISODE FIELDS HAVE TO NAME THE SAME EPISODE.
//
// Measured on the TV, 2026-10-05: the Continue Watching card showed S1E10 with no
// progress bar and no remove option, while its own id said tt12042964:1:13.
//
// cat_apontar_episodio is the only writer of an item's season/episode fields, and
// it never touched the id. The pass that applies saved progress matches records to
// items BY TITLE ONLY, so a record for E10 landed on the E13 card and left the id
// behind.
//
//   bash tests/cw_episode_id.sh
#include "../src/catalogo.h"
#include "../src/progresso.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int         ajustes_idioma_ingles(void) { return 0; }
int         ajustes_idioma(void) { return 0; }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
const char *dados_dir(void) { return ""; }
const char *sessao_usuario(void) { return ""; }
int         perfis_ativo(void) { return 1; }
const char *desc_genero_pt(const char *g) { return g; }
int prog_ler(ProgRegistro *saida, int max) { (void)saida; (void)max; return 0; }
int prog_gravar_local(const char *imdb, int t, int e, double p, double d) {
  (void)imdb; (void)t; (void)e; (void)p; (void)d; return 0;
}

static CatItem lote[3];

static void publicar(void) {
  CatFileira f;
  memset(lote, 0, sizeof lote);
  // 0: Continue Watching card, id WITH an episode.
  snprintf(lote[0].imdb, sizeof lote[0].imdb, "tt12042964:1:13");
  snprintf(lote[0].titulo, sizeof lote[0].titulo, "Serie");
  lote[0].temporada = 1; lote[0].episodio = 13; lote[0].progresso = 37;
  // 1: catalog poster for the SAME series, id WITHOUT an episode.
  snprintf(lote[1].imdb, sizeof lote[1].imdb, "tt12042964");
  snprintf(lote[1].titulo, sizeof lote[1].titulo, "Serie");
  // 2: a movie, no episode at all.
  snprintf(lote[2].imdb, sizeof lote[2].imdb, "tt7777777");
  snprintf(lote[2].titulo, sizeof lote[2].titulo, "Filme");
  memset(&f, 0, sizeof f);
  snprintf(f.chave, sizeof f.chave, "fileira");
  f.ini = 0; f.n = 3;
  cat_definir_tudo(lote, 3, &f, 1);
  cat_quadro();
}

int main(void) {
  const CatItem *c;

  publicar();

  // 1. Id with an episode: the id follows the fields.
  cat_apontar_episodio(0, 1, 10);
  c = cat_item(0);
  assert(c && !strcmp(c->imdb, "tt12042964:1:10"));
  assert(c->temporada == 1 && c->episodio == 10);
  puts("ok  1. id with an episode follows the fields");

  // 2. Id without an episode: the id does NOT change. That is the catalog poster,
  //    where the season/episode fields are what its caption shows.
  cat_apontar_episodio(1, 2, 4);
  c = cat_item(1);
  assert(c && !strcmp(c->imdb, "tt12042964"));
  assert(c->temporada == 2 && c->episodio == 4);
  puts("ok  2. id without an episode does not change");

  // 3. Movie: nothing to point at, id intact.
  cat_apontar_episodio(2, 0, 0);
  c = cat_item(2);
  assert(c && !strcmp(c->imdb, "tt7777777"));
  puts("ok  3. movie id intact");

  publicar();
  cat_quadro();
  puts("cw_episode_id: all ok");
  return 0;
}
