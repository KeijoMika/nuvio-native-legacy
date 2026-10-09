/* A sonda do adaptador DTS roda uma vez por processo: no webOS 3 o
 * dts-starfish-webos4.so nao carrega (GLIBCXX_3.4.21) e cada video repetia o
 * dlopen que falha, mais o do webos3 e o da libplayerAPIs. O firmware nao muda
 * com o app aberto. Aqui: 5 sondas automaticas -> uma tentativa so. */
#define _DEFAULT_SOURCE
#include "dts/dts_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  if (argc < 2) return 2;
  setenv("NUVIO_DTS_ADAPTER_DIR", argv[1], 1);
  for (int i = 0; i < 5; i++)
    if (!dts_pipeline_available(0)) { fprintf(stderr, "sonda %d recusada\n", i + 1); return 1; }
  return 0;
}
