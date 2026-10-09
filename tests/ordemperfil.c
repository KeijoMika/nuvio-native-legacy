// #392: switching A -> B -> A must give A back the exact Home row order it had.
// In the window after the switch the discovery pass still holds the PREVIOUS
// profile's addon list (the account answers a few seconds later). Registering
// those catalogs into the new profile's full file evicted A's own rows, and
// they came back at the end ("collections, catalogs and even Continue Watching
// rearranged"). The discovery pass asks fil_lista_e_deste_perfil() first.
#include "fileiras.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *addons_base_por_id(const char *id) { (void)id; return ""; }
int addons_n(void) { return 0; }
int addons_base_desligada(const char *b) { (void)b; return 0; }
const char *addons_base(int i) { (void)i; return ""; }
const char *addons_id_manifesto(int i) { (void)i; return ""; }
const char *sessao_usuario(void) { return "account-A"; }
char *dados_caminho(char *dst, unsigned n, const char *nome) {
  snprintf(dst, n, "%s/%s", getenv("NV_T_DIR"), nome); return dst;
}
int dados_gravar(const char *nome, const char *texto) {
  char path[1024]; dados_caminho(path, sizeof path, nome);
  FILE *f = fopen(path, "w"); if (!f) return 0;
  int ok = fputs(texto, f) >= 0;
  return fclose(f) == 0 && ok;
}
void fil_teste_recarregar(void);

static char antes[FIL_MAX][96];
static int nAntes;
static void guardar(void) {
  nAntes = fil_n();
  for (int i = 0; i < nAntes; i++) snprintf(antes[i], sizeof antes[i], "%s", fil_chave(i));
}
// What descoberta.c does with the declared catalogs of the list it holds.
static void passada(int perfilDaLista, const char *prefixo) {
  for (int i = 0; i < 40; i++) {
    char k[96]; snprintf(k, sizeof k, "%s_movie_new%d", prefixo, i);
    if (!fil_lista_e_deste_perfil(perfilDaLista)) continue;
    fil_registrar(k, k, "Addon", "movie", -1);
  }
  fil_gravar_registro();
}
int main(void) {
  fil_definir_perfil(1);
  fil_registrar("continue_watching", "Continue", "", "", 2);
  fil_registrar("social_activity", "Friends", "", "", 1);
  for (int i = 0; i < FIL_MAX - 2; i++) {
    char k[96]; snprintf(k, sizeof k, "addonA_movie_c%03d", i);
    fil_registrar(k, k, "AddonA", "movie", 5);
  }
  fil_gravar_registro();
  assert(fil_n() == FIL_MAX);
  guardar();

  fil_definir_perfil(2);                 // A -> B
  passada(2, "addonB");                  // B's own list
  fil_definir_perfil(1);                 // B -> A, account of A not answered yet
  fil_n();                               // loads A's file from disk
  passada(2, "addonB");                  // stale pass: list still belongs to B
  fil_gravar_registro();
  fil_teste_recarregar();

  assert(fil_n() == nAntes);
  for (int i = 0; i < nAntes; i++) assert(!strcmp(fil_chave(i), antes[i]));
  passada(1, "addonA2");                 // A's own list arrives: may register
  assert(fil_lista_e_deste_perfil(0) && fil_lista_e_deste_perfil(1));
  printf("ordemperfil ok\n");
  return 0;
}
