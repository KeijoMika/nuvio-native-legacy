#include "webosver.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static const char *caminhoNyx = "/var/run/nyx/os_info.json";
static const char *caminhoStarfish = "/etc/starfish-release";
static int lido, major;
static const char *fonte = "-";
static char linhaStarfish[128];

// "N.N..." -> N. Exige digitos, ponto, digitos (versao completa); N entre 1 e
// 999. Depois da segunda parte so entra fim, '.', '-' ou '_' (3.9.3, 4.10.0-xx).
static int versaoMaior(const char *s, const char **fim) {
  long n = 0; int d = 0;
  while (*s >= '0' && *s <= '9') { if (d < 4) n = n * 10 + (*s - '0'); d++; s++; }
  if (d < 1 || d > 3 || *s != '.') return 0;
  s++;
  if (!(*s >= '0' && *s <= '9')) return 0;
  while (*s >= '0' && *s <= '9') s++;
  if (*s && *s != '.' && *s != '-' && *s != '_') return 0;
  if (fim) *fim = s;
  return n >= 1 ? (int)n : 0;
}

int nv_webos_parse_nyx(const char *json) {
  const char *p;
  if (!json) return 0;
  p = strstr(json, "\"webos_release\"");
  if (!p) return 0;
  p += 15;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  if (*p++ != ':') return 0;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  if (*p++ != '"') return 0;               // null, numero, objeto: desconhecido
  {
    const char *fim = NULL, *q;
    int v = versaoMaior(p, &fim);
    if (!v) return 0;
    q = strchr(fim, '"');                  // string tem de fechar
    if (!q) return 0;
    for (const char *c = p; c < q; c++)    // sem escape nem quebra dentro
      if (*c == '\\' || *c == '\n' || *c == '\r') return 0;
    return v;
  }
}

int nv_webos_parse_starfish(const char *texto, char *linha, size_t cap) {
  const char *l = texto;
  int v = 0;
  if (linha && cap) linha[0] = 0;
  while (l && *l) {
    size_t n = strcspn(l, "\r\n");
    char um[256];
    const char *r;
    snprintf(um, sizeof um, "%.*s", (int)n, l);
    if (linha && cap && !linha[0]) snprintf(linha, cap, "%s", um);
    r = strstr(um, "release ");
    if (r && !v) {
      const char *s = r + 8;
      int d = 0; long m = 0;
      while (*s >= '0' && *s <= '9') { if (d < 4) m = m * 10 + (*s - '0'); d++; s++; }
      if (d >= 1 && d <= 3 && m > 0) v = (int)m;
    }
    l += n;
    while (*l == '\r' || *l == '\n') l++;
  }
  return v;
}

static size_t ler(const char *caminho, char *buf, size_t cap) {
  FILE *f = fopen(caminho, "rb");
  size_t n;
  if (!f) return 0;
  n = fread(buf, 1, cap - 1, f);
  buf[n] = 0;
  fclose(f);
  return n;
}

static void resolver(void) {
  char buf[NV_WEBOS_LEITURA];
  size_t n;
  int v = 0;
  fonte = "-"; major = 0; linhaStarfish[0] = 0;
  n = ler(caminhoNyx, buf, sizeof buf);
  if (n && n < sizeof buf - 1) v = nv_webos_parse_nyx(buf);  // cortado no limite = desconhecido
  if (v) { major = v; fonte = "nyx"; }
  n = ler(caminhoStarfish, buf, sizeof buf);
  if (n) {
    v = nv_webos_parse_starfish(buf, linhaStarfish, sizeof linhaStarfish);
    if (!major && v) { major = v; fonte = "starfish"; }
  }
  lido = 1;
}

int nv_webos_major(void) {
  int v;
  pthread_mutex_lock(&trava);
  if (!lido) resolver();
  v = major;
  pthread_mutex_unlock(&trava);
  return v;
}
const char *nv_webos_fonte(void) { nv_webos_major(); return fonte; }
const char *nv_webos_starfish_linha(void) { nv_webos_major(); return linhaStarfish; }

void nv_webos_testar(const char *nyx, const char *starfish) {
  pthread_mutex_lock(&trava);
  caminhoNyx = nyx ? nyx : "/var/run/nyx/os_info.json";
  caminhoStarfish = starfish ? starfish : "/etc/starfish-release";
  lido = 0;
  pthread_mutex_unlock(&trava);
}
