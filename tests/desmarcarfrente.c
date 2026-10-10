// Menu real e caso do dono (TCL, 10/10): voltar de T3 para T2E6.
// So o envio externo e capturado; mapa, lapides, grafico e eventos sao reais.
#define visto_episodios capturarEnvio
#define visto_destinos destinosTeste
#include "../src/episodios.c"
#undef visto_episodios
#undef visto_destinos
#include "vistonao.h"
#include "temporadas_grafico.h"
#include <assert.h>

#define SILO "tt14688458"
static int falhas, enviados, sentido, chamadas;
static VistoPar paresEnviados[256];
#define CHECK(c) do { if (!(c)) { printf("FALHOU %d: %s\n", __LINE__, #c); falhas++; } } while (0)
int destinosTeste(void) { return VISTO_TRAKT | VISTO_SIMKL | VISTO_CONTA; }
int capturarEnvio(const char *id, const char *tipo, const VistoPar *p, int n, int v, int d) {
  CHECK(!strcmp(id, SILO) && !strcmp(tipo, "series"));
  CHECK(d == destinosTeste());
  memcpy(paresEnviados, p, n * sizeof *p);
  enviados = n; sentido = v; chamadas++;
  return 1;
}
static void teclaTeste(SDL_Keycode k) {
  SDL_Event ev = {0}; ev.type = SDL_KEYDOWN; ev.key.keysym.sym = k;
  episodios_menu_evento(&ev);
}
static int opcaoFrente(void) {
  for (int i = 0; i < vmOpcoes(); i++)
    if (!strcmp(vmLin[i].rot, i18n("Desmarcar daqui em diante"))) return i;
  return -1;
}
static void abrir(void) { episodios_menu_visto(0, 2, 6, "T2E6"); }
int main(void) {
  CatItem c = {0}; CatEp eps[30] = {0}; TgEp graf[30]; TgDados d;
  int i, pos, vistos, exibidos, t = 0, e = 0;
  snprintf(c.imdb, sizeof c.imdb, "%s", SILO);
  snprintf(c.tipo, sizeof c.tipo, "series");
  c.nTemporadas = 3;
  for (i = 0; i < 3; i++) c.temporadas[i] = i + 1;
  cat_definir(&c, 1);
  for (i = 0; i < 30; i++) {
    eps[i].temporada = i / 10 + 1; eps[i].episodio = i % 10 + 1;
    vistoep_definir(SILO, eps[i].temporada, eps[i].episodio, i < 29);
  }
  cat_definir_episodios(0, eps, 30);
  extras_teste_progresso(SILO, 29, 30, 3, 10);
  vistoep_lapides(vistonao_barra, vistonao_gesto);
  unsigned rev = vistoep_revisao();
  abrir(); pos = opcaoFrente(); CHECK(pos >= 0);
  // Antes da mudanca a opcao falta e a linha 2 e "Temporada inteira":
  // executa-la tambem demonstra por que o resultado nao atende ao pedido.
  if (pos < 0) pos = 2;
  for (i = 0; i < pos; i++) teclaTeste(SDLK_DOWN);
  teclaTeste(SDLK_RETURN);
  CHECK(vmFeito && vmFeitoN == 14 && vmFeitoVisto == 0);
  CHECK(chamadas == 1 && enviados == 14 && sentido == 0);
  for (i = 0; i < enviados; i++)
    CHECK(paresEnviados[i].temporada > 2 ||
          (paresEnviados[i].temporada == 2 && paresEnviados[i].episodio >= 6));
  CHECK(vistoep_contar(SILO) == 15 && vistoep_revisao() > rev);
  for (i = 0; i < 30; i++) {
    int tt = eps[i].temporada, ee = eps[i].episodio;
    CHECK(vistoep_estado(SILO, tt, ee) == (i < 15));
    CHECK(vistonao_barra(SILO, tt, ee, 0) == (i >= 15));
    graf[i] = (TgEp){.temporada = tt, .episodio = ee,
                    .visto = vistoep_estado(SILO, tt, ee)};
  }
  memset(&d, 0, sizeof d);
  tgraf_montar(&d, graf, 30, 1, 0, 0, 0, NULL);
  CHECK(d.t[tgraf_coluna(&d, 1)].vistos == 10);
  CHECK(d.t[tgraf_coluna(&d, 2)].vistos == 5);
  CHECK(d.t[tgraf_coluna(&d, 3)].vistos == 0);
  CHECK(extras_progresso_serie(&vistos, &exibidos) && vistos == 15 && exibidos == 30);
  CHECK(extras_proximo_episodio(&t, &e) && t == 2 && e == 6);
  abrir(); CHECK(opcaoFrente() < 0); // so anteriores vistos: nao oferece
  vistoep_definir(SILO, 3, 9, 1);
  abrir(); pos = opcaoFrente(); CHECK(pos >= 0 && vmVisto == 1);
  if (pos >= 0) {
    for (i = 0; i < pos; i++) teclaTeste(SDLK_DOWN);
    teclaTeste(SDLK_RETURN);
    CHECK(chamadas == 2 && enviados == 1 && sentido == 0);
    CHECK(vistoep_contar(SILO) == 15); // foco nao visto: continua DESMARCANDO
  }
  // Fonte antiga nao ressuscita o que foi desmarcado.
  vistoep_fonte(SILO, 2, 6, 1, 0, NULL);
  CHECK(vistoep_estado(SILO, 2, 6) == 0);
  // A ausencia da opcao nao desloca "Fontes deste episodio".
  abrir(); for (i = 0; i < 10; i++) teclaTeste(SDLK_DOWN);
  teclaTeste(SDLK_RETURN); CHECK(episodios_menu_pediu_fontes());
  printf("desmarcarfrente: %s (%d falhas)\n", falhas ? "FALHOU" : "PASS", falhas);
  return falhas != 0;
}
