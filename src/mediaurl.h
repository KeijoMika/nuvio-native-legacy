// Long media URLs travel through existing value copies as short HTTP aliases.
#ifndef NV_MEDIAURL_H
#define NV_MEDIAURL_H
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NvMediaUrl {
  struct NvMediaUrl *next;
  char alias[384];
  char url[];
} NvMediaUrl;
__attribute__((weak)) pthread_mutex_t nv_media_mutex = PTHREAD_MUTEX_INITIALIZER;
__attribute__((weak)) NvMediaUrl *nv_media_urls;
__attribute__((weak)) size_t nv_media_bytes;

// Immutable entries survive Stream copies and asynchronous users. Never evict.
// ponytail: cap process-lifetime storage at 8 MiB on 32-bit TVs; use reference
// counting across all Stream owners if this ceiling becomes a practical limit.
static inline int nv_media_url_guardar(const char *url, char *dst, size_t tam) {
  size_t len = strlen(url), host;
  const char *p = strstr(url, "://");
  uint64_t hash = UINT64_C(1469598103934665603);
  NvMediaUrl *e;
  char alias[384];
  if (len < tam) { memmove(dst, url, len + 1); return 1; }
  if (!p || len > (8u << 20) - sizeof *e - 1) return 0;
  host = (size_t)(p + 3 - url) + strcspn(p + 3, "/?#");
  if (host > 256) return 0;
  for (size_t i = 0; i < len; i++) { hash ^= (unsigned char)url[i]; hash *= UINT64_C(1099511628211); }
  const char *ext = strstr(url, ".m3u8") ? ".m3u8" : strstr(url, ".mp4") ? ".mp4" :
                    strstr(url, ".mkv") ? ".mkv" : strstr(url, ".mpd") ? ".mpd" : "";
  snprintf(alias, sizeof alias, "%.*s/~nvm%016llx%s", (int)host, url, (unsigned long long)hash, ext);
  if (strlen(alias) >= tam) return 0;
  pthread_mutex_lock(&nv_media_mutex);
  for (e = nv_media_urls; e; e = e->next) if (!strcmp(e->alias, alias)) break;
  if (!e && sizeof *e + len + 1 <= (8u << 20) - nv_media_bytes) {
    e = malloc(sizeof *e + len + 1);
    if (e) {
      strcpy(e->alias, alias); memcpy(e->url, url, len + 1);
      e->next = nv_media_urls; nv_media_urls = e;
      nv_media_bytes += sizeof *e + len + 1;
    }
  }
  int ok = e && !strcmp(e->url, url);
  pthread_mutex_unlock(&nv_media_mutex);
  if (ok) strcpy(dst, alias);
  return ok;
}

// Transport boundaries use the exact signed bytes, without addon normalization.
static inline const char *nv_media_url(const char *url) {
  const char *out = url;
  if (!url || !strstr(url, "/~nvm")) return url;
  pthread_mutex_lock(&nv_media_mutex);
  for (NvMediaUrl *e = nv_media_urls; e; e = e->next)
    if (!strcmp(e->alias, url)) { out = e->url; break; }
  pthread_mutex_unlock(&nv_media_mutex);
  return out;
}
#endif
