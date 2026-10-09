#!/usr/bin/env bash
# A sonda automatica do adaptador DTS so tenta carregar os adaptadores uma vez
# por processo. Linux (dlopen/.so); no Mac roda no container:
#   docker run --rm -v "$PWD":/src -w /src nuvio-webos-sdk-ac3 bash tests/dts_sonda_cache.sh
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/good" "$tmp/native"
cc -std=c11 -D_GNU_SOURCE -fPIC -Isrc -c src/js.c -o "$tmp/js.o"
g++ -std=c++11 -fPIC -shared -D_GLIBCXX_USE_CXX11_ABI=0 -Isrc -Itests/dts_pipeline_sdk \
  tests/dts_pipeline_native.cpp "$tmp/js.o" -o "$tmp/native/libplayerAPIs.so"
# So o adaptador do webOS 3, como na TV de 2017: o do webOS 4 falta e falha.
g++ -std=c++11 -fPIC -shared -D_GLIBCXX_USE_CXX11_ABI=0 -Isrc -Isrc/dts/adapter -Itests/dts_pipeline_sdk \
  src/dts/adapter/starfish.cpp "$tmp/js.o" -o "$tmp/good/dts-starfish-webos3.so" -ldl -pthread
cc -std=c11 -Wall -Wextra -Werror -Isrc tests/dts_sonda_cache.c src/dts/dts_pipeline.c -ldl -pthread -o "$tmp/test"
LD_LIBRARY_PATH="$tmp/native" "$tmp/test" "$tmp/good" > "$tmp/saida" 2>&1
falhas=$(grep -c 'adapter load failed' "$tmp/saida" || true)
prontos=$(grep -c 'firmware adapter ready' "$tmp/saida" || true)
if [ "$falhas" = 1 ] && [ "$prontos" = 1 ]; then
  echo "PASSA: 5 sondas, 1 tentativa (webos4 falhou 1x, webos3 pronto 1x)"
else
  echo "FALHA: 5 sondas carregaram o adaptador de novo (webos4 falhou ${falhas}x, webos3 pronto ${prontos}x)"; exit 1
fi
