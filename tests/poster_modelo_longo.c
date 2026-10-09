// #390: o "Modelo de URL dos posteres" colado do celular (ou digitado) com
// mais de 299 caracteres, chave com maiusculas e {imdb} no fim nao era salvo:
// a modal cortava em 299 (PP_MODELO_MAX - 1), o alfabeto do campo baixava a
// caixa da chave, e o validador recusava o que sobrava sem gravar nada.
// Caminho de verdade: modal do campo -> texto do celular -> pstDefinir ->
// posteres.txt -> leitura de volta -> URL montada. A chave e de mentira.
#include "dados.h"
#include "posterprov.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ajustes_teste_poster_modelo(const char *colado, char *lido, size_t n);

static int falhas;
#define OK(c, m) do { if (c) printf("ok   %s\n", m); else { printf("FALHA %s\n", m); falhas++; } } while (0)

int main(void) {
  char modelo[600], lido[1024], u[PP_URL_MAX];
  int aviso, k;
  dados_iniciar(NULL);   // NUVIO_DADOS: diretorio temporario do .sh
  // ~380 caracteres: chave de mentira com maiusculas, config longa, {imdb} no fim.
  k = snprintf(modelo, sizeof modelo,
               "https://posters.exemplo.invalid/FAKEKEY_ABCdef0123456789XYZ/config=");
  while (k < 360) k += snprintf(modelo + k, sizeof modelo - (size_t)k, "Ab1-Cd2_Ef3~");
  k += snprintf(modelo + k, sizeof modelo - (size_t)k, "/poster/{type}/{imdb}.jpg");
  printf("modelo de %d caracteres\n", k);
  assert(k > 300 && k < 400);

  aviso = ajustes_teste_poster_modelo(modelo, lido, sizeof lido);
  printf("aviso longo=%d\n", aviso);
  OK(aviso == 0, "o modelo longo e aceito (sem aviso de invalido)");
  OK(!strcmp(lido, modelo), "posteres.txt devolve o modelo inteiro, na mesma caixa");
  OK(posterprov_cfg()->prov == PP_MODELO && !strcmp(posterprov_cfg()->modelo, modelo),
     "o provedor ativo recebe o modelo inteiro");
  OK(posterprov_montar_url(posterprov_cfg(), "tt0111161", 0, "movie", u, sizeof u) &&
     strstr(u, "FAKEKEY_ABCdef") && strstr(u, "/poster/movie/tt0111161.jpg"),
     "a URL do cartaz sai com a chave intacta e o {imdb} trocado");

  // O curto de sempre continua igual.
  aviso = ajustes_teste_poster_modelo("https://meu.servidor.invalid/{type}/{imdb}.jpg", lido, sizeof lido);
  printf("aviso curto=%d lido=%s\n", aviso, lido);
  OK(aviso == 0 && !strcmp(lido, "https://meu.servidor.invalid/{type}/{imdb}.jpg"), "modelo curto inalterado");
  // Sem marcador continua recusado.
  aviso = ajustes_teste_poster_modelo("https://meu.servidor.invalid/x.jpg", lido, sizeof lido);
  OK(aviso != 0, "modelo sem marcador continua recusado");

  if (falhas) { printf("%d falha(s)\n", falhas); return 1; }
  printf("tudo ok\n");
  return 0;
}
