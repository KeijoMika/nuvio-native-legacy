// #402: fonte rotulada "FHD" (sem o numero 1080) caia em "Outras". O parser de
// verdade sobre uma lista de streams em JSON; altura e selo r-1080 tem de sair.
#include "streams.h"
#include "badges.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int falhas;
static void caso(const Stream *s, int altura, int r1080, const char *nome) {
  int tem = (s->badges & badges_bit("r-1080")) != 0;
  if (s->altura != altura || (r1080 >= 0 && tem != r1080)) {
    printf("FAIL %s: altura=%d (quer %d) r-1080=%d (quer %d)\n",
           nome, s->altura, altura, tem, r1080);
    falhas++;
  } else printf("ok   %s\n", nome);
}
int main(void) {
  Stream *v = NULL;
  int n = stream_extrair("{\"streams\":["
    "{\"url\":\"https://example.invalid/0\",\"name\":\"FHD | REMUX | SDR\"},"
    "{\"url\":\"https://example.invalid/1\",\"name\":\"FHD | SDR\",\"description\":\"WEBRip HEVC\"},"
    "{\"url\":\"https://example.invalid/2\",\"name\":\"Full HD BluRay\"},"
    "{\"url\":\"https://example.invalid/3\",\"name\":\"FullHD\"},"
    "{\"url\":\"https://example.invalid/4\",\"name\":\"UHD | WEB-DL\"},"
    "{\"url\":\"https://example.invalid/5\",\"name\":\"N/A | SDR\",\"description\":\"Dual Audio / JA\"},"
    "{\"url\":\"https://example.invalid/6\",\"name\":\"Movie\",\"description\":\"DTS-HD MA 5.1\"},"
    "{\"url\":\"https://example.invalid/7\",\"name\":\"XFHDX\"},"
    "{\"url\":\"https://example.invalid/8\",\"name\":\"1080p\",\"description\":\"FHD WEB-DL\"},"
    "{\"url\":\"https://example.invalid/9\",\"name\":\"2160p\",\"description\":\"FHD HDR10\"}"
    "]}", "fixture", &v);
  if (n != 10) { printf("FAIL contagem %d\n", n); return 1; }
  caso(&v[0], 1080, 1, "FHD | REMUX | SDR");
  caso(&v[1], 1080, 1, "FHD | SDR + WEBRip HEVC");
  caso(&v[2], 1080, 1, "Full HD BluRay");
  caso(&v[3], 1080, 1, "FullHD");
  caso(&v[4], 2160, 0, "UHD | WEB-DL");
  caso(&v[5], 0, 0, "N/A | SDR + Dual Audio");
  caso(&v[6], 0, 0, "DTS-HD MA 5.1 nao vira 720");
  if (v[6].badges & badges_bit("r-720")) { puts("FAIL DTS-HD com r-720"); falhas++; }
  caso(&v[7], 0, 0, "XFHDX dentro da palavra");
  caso(&v[8], 1080, 1, "1080p + FHD");
  caso(&v[9], 2160, 0, "2160p + FHD: numero explicito vence");
  if (!(v[9].badges & badges_bit("r-4k"))) { puts("FAIL 2160p+FHD sem r-4k"); falhas++; }
  free(v);
  if (falhas) { printf("FAIL #402: %d caso(s)\n", falhas); return 1; }
  puts("PASS #402: FHD/Full HD agrupam como 1080p.");
  return 0;
}
