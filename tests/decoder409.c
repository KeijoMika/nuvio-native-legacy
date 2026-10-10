// #409: selecao real, sem rede nem TV. Stubs fracos permitem provar o FAIL anterior.
#include "streams.h"
#include "badges.h"
#include "ajustes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
__attribute__((weak)) void stream_definir_decoder4k(int h, int a, int v, int av) { (void)h; (void)a; (void)v; (void)av; }
__attribute__((weak)) void stream_automatico_erro_decoder(int i, int c) { (void)i; (void)c; }
static int falhas;
static void espera(int esperado, const char *nome) {
  int i = stream_automatico();
  printf("%s %s: fonte %d (esperada %d)\n", i == esperado ? "PASS" : "FAIL", nome, i, esperado);
  falhas += i != esperado;
}
int main(void) {
  Stream s[3] = {0};
  ajustes_dir(getenv("NUVIO_DADOS"));
  for (int i = 0; i < 3; i++) {
    snprintf(s[i].url, sizeof s[i].url, "https://example.invalid/%d", i);
    snprintf(s[i].rotulo, sizeof s[i].rotulo, "%s", i == 2 ? "1080p" : "4K UHD VidFast");
    s[i].altura = i == 2 ? 1080 : 2160;
  }
  stream_definir_lista(s, 3);
  espera(0, "capacidade desconhecida preserva 4K");
  stream_definir_decoder4k(0, 0, 0, 0);
  espera(2, "sem decoder 4K pula 2160p");
  assert(stream_n() == 3 && stream_item(0)->altura == 2160);
  stream_preferir(0);
  // Inclusive preferida e caminhos de antecipacao devem obedecer ao decoder.
  if (stream_automatico_disponivel(0)) { puts("FAIL preferida sem decoder disponivel"); falhas++; }
  stream_definir_decoder4k(1, 0, 0, 0); espera(0, "HEVC 4K mantem codec desconhecido");
  const char *codecs[] = {"H.264", "VP9", "AV1", "HEVC"};
  for (int c = 0; c < 4; c++) {
    snprintf(s[0].arquivo, sizeof s[0].arquivo, "filme.2160p.%s.mkv", codecs[c]);
    s[1].altura = 1080;
    stream_definir_lista(s, 3);
    stream_definir_decoder4k(c != 3, c != 0, c != 1, c != 2);
    espera(1, codecs[c]);
    stream_definir_decoder4k(1, 1, 1, 1); espera(0, "codec 4K suportado");
  }
  s[0].arquivo[0] = 0; s[0].badges = badges_bit("co-x264");
  stream_definir_lista(s, 3); stream_definir_decoder4k(1, 0, 1, 1);
  espera(1, "codec por selo AVC");
  s[0].badges = 0; s[1].altura = 2160;
  for (int c = 4001; c <= 4005; c++) {
    if (c == 4002) continue;
    stream_definir_decoder4k(-1, -1, -1, -1);
    stream_definir_lista(s, 3);
    stream_automatico_erro_decoder(0, c);
    stream_automatico_excluir(0);
    espera(2, "erro decoder nao repete tamanho/codec");
  }
  stream_definir_lista(s, 3); espera(0, "lista nova nao herda falha");
  stream_automatico_erro_decoder(0, 2004); stream_automatico_excluir(0);
  espera(1, "erro de rede nao muda decoder");
  return falhas ? 1 : 0;
}
