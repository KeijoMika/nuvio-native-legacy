// THE HISTORY IS NOT A FILTER ON "Continuar assistindo" (issue #244).
//
//   bash tests/cw_historico.sh
//
// "Mark as watched" writes a history mark and deletes the resume state
// (ctxmenu.c). The row reads the RESUME STATE only: "nenhuma fileira le dela"
// (ctxmenu.c) - the history is for the poster check and the detail eye (#212,
// catalogo.c), and after a reopen a record the servers still have is shown
// again on purpose (progresso.c: showing it "e a verdade").
//
// The trap this pins: filtering the row on cat_visto hides a REWATCHED movie
// forever, because the watched mark carries no timestamp to compare. What
// removes a card is the deletion of its records (tests/cw_remocao.sh) and the
// removal stamp (tests/cwremover.sh).
//
// Includes src/descoberta.c with the real catalogo.c and progresso.c; only the
// network is doubled. Doubles for the home state mean no valid snapshot, which is
// the first-launch case.
#include "../src/homeestado.h"
unsigned homeestado_geracao(void) { return 1; }
int homeestado_contexto_valido(void) { return 0; }
int homeestado_tem_fileira(const char *chave) { (void)chave; return 0; }
int homeestado_ordem_fileira(const char *chave) { (void)chave; return -1; }
int homeestado_salvar_se_geracao(const CatFileira *fils, int n, unsigned g) { (void)fils; (void)n; (void)g; return 1; }
int homeestado_identidade_geracao(unsigned g, char *dono, unsigned tamDono, int *perfil) {
  (void)g; if (dono && tamDono) dono[0] = 0; if (perfil) *perfil = 0; return 0; }
int arte_reserva_episodios(const char *imdb, const char *corpo) { (void)imdb; (void)corpo; return 0; }

#include "../src/descoberta.c"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

// --- DOUBLES: the same ones from tests/cwordem_desc.c (the set of
//     tests/cateps.c), which exist only so descoberta.c links without the rest
//     of the app -------------------------------------------------------------
int         ajustes_idioma_ingles(void) { return 0; }
int ajustes_idioma(void) { return 0; }
const char *i18n(const char *s)         { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
const char *dados_dir(void)             { return ""; }
const char *sessao_usuario(void)        { return ""; }
int         perfis_ativo(void)          { return 1; }
char *dados_ler(const char *nome)                 { (void)nome; return NULL; }
int   dados_gravar(const char *nome, const char *c) { (void)nome; (void)c; return 1; }
int   dados_apagar(const char *nome)              { (void)nome; return 1; }
void  SDL_Delay(Uint32 ms)                 { usleep(ms * 1000); }
static int testSource;          // 0 = AJ_CWF_AMBAS, 2 = Trakt only
int   ajustes_cw_fonte(void)               { return testSource; }
int   ajustes_tmdb_ligado(void)            { return 0; }
int   ajustes_tmdb_basico(void)            { return 0; }
int   ajustes_meta_externo(void)           { return 0; }
int   ajustes_meta_so_cinemeta(void)        { return 0; }
int   ajustes_fundo_addon(void)            { return 0; }
int   ajustes_logo_addon(void)             { return 0; }
int   addons_aceita_id(int i, const char *t, const char *id) { (void)i; (void)t; (void)id; return -1; }
int   ajustes_tmdb_arte(void)              { return 0; }
int   ajustes_tmdb_elenco(void)            { return 0; }
int   ajustes_tmdb_cw(void)                { return 0; }
const char *ajustes_tmdb_idioma(void)      { return "pt-BR"; }
const char *ajustes_tmdb_chave(void)       { return ""; }
void  fil_gravar_registro(void)            { }
int   fil_podar_catalogos(const char *const *ids, const char *const *bases, int n,
                          int perfilDaLista) {
  (void)ids; (void)bases; (void)n; (void)perfilDaLista; return 0; }
int   fil_addon_novo(const char *id, const char *base) { (void)id; (void)base; return 0; }
int   addons_perfil_da_lista(void)         { return 0; }
int   addons_ativo(int i)                  { (void)i; return 1; }
int   addons_fornece(int i, int oque)     { (void)i; (void)oque; return 0; }
int   addons_sondado(int i)              { (void)i; return 0; }
int   fil_limite(void)                     { return 16; }
int   fil_oculta(const char *c)            { (void)c; return 0; }
// Doubles for the quota choice (#126): nothing chosen on the TV, and the record
// of catalogs outside the quota does not interest this test.
int fil_escolhida(const char *c) { (void)c; return -1; }
int fil_migrar_197(const char *const *c, int n) { (void)c; (void)n; return 0; }
void fil_registrar_se_couber(const char *c, const char *t, const char *a,
                             const char *tp) { (void)c; (void)t; (void)a; (void)tp; }
void  fil_registrar(const char *c, const char *t, const char *a,
                    const char *tp, int itens) {
  (void)c; (void)t; (void)a; (void)tp; (void)itens;
}
// Context in parts (homeestado.h, 1.4.5): constant here, so nothing changes in
// the middle of the assembly and its end follows the usual path.
void homeestado_contexto(HomeContexto *c) { *c = (HomeContexto){0}; c->perfil = 1; }
int homeestado_mudancas(const HomeContexto *a, const HomeContexto *b) { (void)a; (void)b; return 0; }
const char *homeestado_mudancas_texto(int m, char *b, unsigned t) { (void)m; if (b && t) b[0] = 0; return b; }
int   fil_tem_ordem(void)                  { return 0; }
int   fil_unir(const char *const *c, int n, int *s, int m) {
  int i; (void)c; for (i = 0; i < n && i < m; i++) s[i] = i; return i;
}
void  marco(const char *n)                 { (void)n; }
int   simkl_ativo(void)                    { return 0; }
int   simkl_continuar(CatItem *s, int m)   { (void)s; (void)m; return 0; }
int   simkl_plantowatch(CatItem *s, int m) { (void)s; (void)m; return 0; }
int   ajustes_salvos_no_simkl(void)        { return 0; }
// The fake embellishing: for the ACCOUNT's "next up" (#199) it does what trakt.c
// does - notes the premiere and discards the series that ended (see CONTA below).
int   trakt_enfeitar_lote(CatItem *s, int n);
int   trakt_lista(const char *q, CatItem *s, int m) { (void)q; (void)s; (void)m; return 0; }
// The app's own social service (recomenda.c) is out of this test: the union is
// only what Trakt brought.
int   recomenda_social_mesclar(CatItem *i, int nTrakt, int max) { (void)i; (void)max; return nTrakt; }
int   trakt_social(CatItem *s, int m)      { (void)s; (void)m; return 0; }
const char *nuvem_trakt_cliente(void)      { return ""; }
int   addons_n(void)                       { return 0; }
const char *addons_base(int i)             { (void)i; return ""; }
const char *addons_id_manifesto(int i)     { (void)i; return ""; }
const char *addons_nome(int i)             { (void)i; return "addon"; }
unsigned addons_versao(void)               { return 1; }
const char *addons_base_por_id(const char *id) { (void)id; return ""; }
void  addons_manifesto_lido(int i, const char *corpo) { (void)i; (void)corpo; }
int   arte_reserva_registrar(const char *url, const char *imdb, int poster) {
  (void)url; (void)imdb; (void)poster; return 1; }

static int testMode = CWO_PADRAO, hiddenTest = 1;
int ajustes_cw_ordem(void)                { return testMode; }typedef struct { const char *id; int prog; long long when; } Fake;
static const Fake FAKE[] = {
  { "tt12042964:1:15", 20, 900500 },   // S1E15 IN PROGRESS (series)
  { "tt5555555",       30, 900400 },   // a movie in progress
};
#define NFAKE (int)(sizeof FAKE / sizeof *FAKE)
int trakt_e_a_seguir(const char *id) { (void)id; return 0; }
int simkl_e_a_seguir(const char *id) { (void)id; return 0; }
int trakt_continuar_falhou(void) { return 0; }
int trakt_continuar(CatItem *s, int m) {
  int i;
  for (i = 0; i < NFAKE && i < m; i++) {
    memset(&s[i], 0, sizeof s[i]);
    snprintf(s[i].imdb, sizeof s[i].imdb, "%s", FAKE[i].id);
    { const char *dp = strchr(FAKE[i].id, ':');
      if (dp) {
        snprintf(s[i].tipo, sizeof s[i].tipo, "series");
        sscanf(dp + 1, "%d:%d", &s[i].temporada, &s[i].episodio);
      } else {
        snprintf(s[i].tipo, sizeof s[i].tipo, "movie");
      } }
    s[i].progresso = FAKE[i].prog;
    s[i].retomadoMs = FAKE[i].when;
  }
  return i;
}
static const char *BODY_META =
  "{\"meta\":{\"imdbRating\":\"7.9\",\"releaseInfo\":\"2020\",\"description\":\"d\",\"name\":\"n\","
  "\"videos\":[{\"id\":\"tt12042964:1:14\",\"released\":\"2020-05-01\",\"name\":\"E14\"},"
              "{\"id\":\"tt12042964:1:15\",\"released\":\"2020-05-08\",\"name\":\"E15\"}]}}";
char *rede_baixar(const char *url, int seg) {
  (void)seg;
  if (!strstr(url, "/meta/")) return NULL;
  return strdup(BODY_META);
}
char *rede_baixar_com(const char *u, int s, const char *const *c) { (void)u;(void)s;(void)c; return NULL; }
char *rede_baixar_st(const char *u,int s,const char *const *c,int *st){(void)u;(void)s;(void)c;if(st)*st=0;return NULL;}
char *rede_apagar(const char *u,int s,const char *const *c,int *st){(void)u;(void)s;(void)c;if(st)*st=200;return strdup("");}
char *rede_postar_st(const char *u,int s,const char *const *c,const char *b,int *st){(void)u;(void)s;(void)c;(void)b;if(st)*st=200;return strdup("");}
void rede_avisar_401(void (*cb)(const char *)) { (void)cb; }
int trakt_ativo(void) { return 1; }
int ajustes_cw_do_episodio_mais_alto(void) { return 1; }
/* missing in the head: the embellishing and the clock (the head of cwordem_desc
   brings them later) */
int contalib_sementes_a_seguir(ContaSemente *s, int m, int a) { (void)s;(void)m;(void)a; return 0; }
int trakt_enfeitar_lote(CatItem *v, int n) { (void)v; return n; }
Uint32 SDL_GetTicks(void) { return 0; }
int ajustes_cw_concluido(void) { return 90; }
int ajustes_cw_mostrar_nao_exibidos(void) { return 1; }
int ajustes_itens_fileira(void) { return 12; }
// The cases below, per the header.
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int inRow(const CatItem *l, int n, const char *id) {
  int i; for (i = 0; i < n; i++) if (!strcmp(l[i].imdb, id)) return 1; return 0;
}

int main(void) {
  CatItem batch[CONT_MAX];
  int n;

  // Baseline: clean history, the episode in progress appears.
  n = montarContinuar(batch, CONT_MAX);
  printf("clean history:           n=%d  S1E15 in the row: %s\n", n, inRow(batch,n,"tt12042964:1:15")?"YES":"NO");
  assert(inRow(batch, n, "tt12042964:1:15"));

  // A MOVIE marked as watched keeps its paused record in the row: the mark is
  // not a filter. (A cat_visto filter here is what hid rewatched movies.)
  cat_historico_definir_id("tt5555555", "movie", 1);
  n = montarContinuar(batch, CONT_MAX);          // REBUILDS with the new history
  printf("movie marked watched:    n=%d  movie in the row: %s\n",
         n, inRow(batch,n,"tt5555555")?"YES":"NO");
  assert(inRow(batch, n, "tt5555555"));

  // A SERIES marked as watched keeps its episode in progress too - both marks
  // the menu records: the composite id and the title id.
  cat_historico_definir_id("tt12042964:1:15", "series", 1);
  cat_historico_definir_id("tt12042964", "series", 1);
  n = montarContinuar(batch, CONT_MAX);
  printf("series marked watched:   n=%d  S1E15 in the row: %s\n", n, inRow(batch,n,"tt12042964:1:15")?"YES":"NO");
  assert(inRow(batch, n, "tt12042964:1:15"));

  puts("ok: the history marks move no card; the resume state decides");
  return 0;
}
