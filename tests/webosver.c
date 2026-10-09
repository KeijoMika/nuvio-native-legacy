// A versao maior do webOS vem de UM lugar: webos_release do nyx, depois o
// starfish-release, e "desconhecida" (0) quando nenhum dos dois diz com
// clareza. TV de 2017 (webOS 3.9) so tem o nyx.
#include "webosver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
static void grava(const char *caminho, const char *txt) {
  FILE *f = fopen(caminho, "wb");
  if (f) { fputs(txt, f); fclose(f); }
}
static int com(const char *dir, const char *nyx, const char *star) {
  char a[256], b[256];
  snprintf(a, sizeof a, "%s/os_info.json", dir);
  snprintf(b, sizeof b, "%s/starfish-release", dir);
  remove(a); remove(b);
  if (nyx) grava(a, nyx);
  if (star) grava(b, star);
  nv_webos_testar(a, b);
  return nv_webos_major();
}
int main(int argc, char **argv) {
  const char *d = argc > 1 ? argv[1] : "/tmp";
  // O caso real: 65SJ800V / OLED55B7P, sem starfish-release.
  CHECK(com(d, "{\"webos_release\":\"3.9.3\",\"webos_name\":\"x\"}", NULL) == 3);
  CHECK(!strcmp(nv_webos_fonte(), "nyx"));
  CHECK(com(d, "{ \"webos_release\" : \"4.10.0\" }", NULL) == 4);
  CHECK(com(d, "{\"webos_release\":\"11.2.0-5\"}", NULL) == 11);
  CHECK(com(d, "{\"webos_release\":\"6.0.0\"}", "Rockhopper release 4.10.2-31 (x)\n") == 6); // nyx manda
  // sem nyx: starfish
  CHECK(com(d, NULL, "Rockhopper release 5.1.0-2 (x)\n") == 5);
  CHECK(!strcmp(nv_webos_fonte(), "starfish"));
  CHECK(!strncmp(nv_webos_starfish_linha(), "Rockhopper release 5", 20));
  // nyx malformado cai no starfish
  CHECK(com(d, "{\"webos_release\":null}", "x release 4.5.1\n") == 4);
  // nenhum dos dois: desconhecida
  CHECK(com(d, NULL, NULL) == 0);
  CHECK(!strcmp(nv_webos_fonte(), "-"));
  // malformados (cada um = desconhecido)
  CHECK(com(d, "{\"webos_release\":\"3", NULL) == 0);                 // cortado
  CHECK(com(d, "{\"webos_release\":\"3garbage\"}", NULL) == 0);
  CHECK(com(d, "{\"webos_release\":\"3\"}", NULL) == 0);              // sem minor
  CHECK(com(d, "{\"webos_release\":null,\"3\":\"x\"}", NULL) == 0);   // nome de outro campo
  CHECK(com(d, "{\"webos_release\":3.9}", NULL) == 0);                // nao e string
  CHECK(com(d, "{\"webos_release\":\"-3.9.0\"}", NULL) == 0);         // negativo
  CHECK(com(d, "{\"webos_release\":\"3.9.0", NULL) == 0);            // string aberta
  CHECK(com(d, "{\"webos_release\":\"99999999999.1\"}", NULL) == 0);  // fora da faixa
  CHECK(com(d, "{\"webos_release\":\"0.0.0\"}", NULL) == 0);
  CHECK(com(d, "{\"webos_release\":\"3.9\\u0030\"}", NULL) == 0);     // escape
  CHECK(com(d, "{\"x\":\"webos_release\"}", NULL) == 0);
  CHECK(nv_webos_parse_nyx(NULL) == 0);
  {  // valor cortado exatamente no limite de leitura
    static char grande[NV_WEBOS_LEITURA + 64];
    int n = snprintf(grande, sizeof grande, "{\"pad\":\"");
    memset(grande + n, 'a', NV_WEBOS_LEITURA - 1 - n - 20);
    n = NV_WEBOS_LEITURA - 1 - 20;
    snprintf(grande + n, sizeof grande - n, "\",\"webos_release\":\"3.9.3\"}");
    CHECK(com(d, grande, NULL) == 0);   // o valor cai alem de 4095 bytes
  }
  // starfish malformado
  CHECK(com(d, NULL, "release abc\n") == 0);
  CHECK(com(d, NULL, "no release here\nRockhopper release 4.10.2-31\n") == 4);
  if (!fails) puts("webosver ok");
  return fails != 0;
}
