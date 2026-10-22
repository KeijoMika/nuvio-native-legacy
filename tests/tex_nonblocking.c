/* Cache contention must defer uploads; CPU mip allocation failure must not
 * move mip generation onto the render thread. Uses the real texture pump. */
#include "../src/sdlcompat.h"
#include "../src/gl_compat.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
static int gpuMips;
static void countMips(GLenum target) { (void)target; gpuMips++; }
#undef glGenerateMipmap
#define glGenerateMipmap countMips
#include "../src/tex_cache.c"
static SDL_sem *locked;
static void *busy(void *unused) {
  (void)unused;
  SDL_LockMutex(mtx);
  SDL_SemPost(locked);
  SDL_Delay(250);
  SDL_UnlockMutex(mtx);
  return NULL;
}
int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_Window *window = SDL_CreateWindow("texture regression", 0, 0, 32, 32,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window);
  SDL_GLContext context = SDL_GL_CreateContext(window);
  assert(context);
  mtx = SDL_CreateMutex(); locked = SDL_CreateSemaphore(0); nMax = 1;
  orcamento = 32L * 1024 * 1024;
  mipCpu = 1;
  pthread_t worker;
  assert(!pthread_create(&worker, NULL, busy, NULL));
  SDL_SemWait(locked);
  Uint32 start = SDL_GetTicks();
  assert(tex_bombear(1) == 0);
  Uint32 elapsed = SDL_GetTicks() - start;
  pthread_join(worker, NULL);
  printf("contended pump: %u ms (worker holds lock 250 ms)\n", elapsed);
  assert(elapsed < 50);
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 64, 64, 32,
                                                       SDL_PIXELFORMAT_ABGR8888);
  assert(surface);
  /* No userdata models an unsuccessful CPU mip allocation. */
  itens[0].estado = DECODIFICADO; itens[0].sup = surface; itens[0].card = 1;
  snprintf(itens[0].caminho, sizeof itens[0].caminho, "test-poster");
  assert(tex_bombear(1) == 1);
  assert(gpuMips == 0 && itens[0].mip == 0);
  glDeleteTextures(1, &itens[0].tex);
  SDL_DestroySemaphore(locked); SDL_DestroyMutex(mtx);
  SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
  puts("PASS: no wait on contended cache; no render-thread CPU-mip fallback");
  return 0;
}
