// ESCOPO DOS AJUSTES (2.1): TV | perfil+conta | perfil local; migracao unica do
// ajustes.txt; copia entre perfis. Sem SDL na tela.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

static int tem(const char *t, int op) {
  char alvo[96];
  snprintf(alvo, sizeof alvo, "%s ", CHAVE[op]);
  return t && strstr(t, alvo) != NULL;
}

static void escopos(void) {
  int i, n[4] = { 0, 0, 0, 0 };
  for (i = 0; i < AJ_N; i++) {
    int e = ajustes_escopo(i);
    assert(e >= 0 && e <= 3);
    n[e]++;
    // Nada com cara de segredo viaja com o perfil.
    if (e >= 2) {
      const char *c = CHAVE[i];
      assert(!strcasestr(c, "token") && !strcasestr(c, "senha") && !strcasestr(c, "password") &&
             !strcasestr(c, "secret") && !strstr(c, "Chave") && !strcasestr(c, "apikey"));
    }
    // Acao, leitura e chave "-" nunca tem escopo.
    if (OPCOES[i].tipo == OP_ACAO || OPCOES[i].tipo == OP_LEITURA || CHAVE[i][0] == '-') assert(e == 0);
    // O que e local nunca entra no blob da conta.
    if (e == 3) assert(somenteDesteAparelho(i));
    if (e == 2) assert(!somenteDesteAparelho(i));
  }
  assert(n[1] > 20 && n[2] > 20 && n[3] > 40);
  // Corte aprovado pelo dono (06/10): estes passam a ser do perfil...
  assert(ajustes_escopo(AJ_VIDRO) == 3 && ajustes_escopo(AJ_FUNDO) == 3 && ajustes_escopo(AJ_TAMANHO_UI) == 3);
  assert(ajustes_escopo(AJ_RELOGIO) == 3 && ajustes_escopo(AJ_HERO_TRAILER_SOM) == 3);
  assert(ajustes_escopo(AJ_NT_IMDB) == 3 && ajustes_escopo(AJ_LEG2_POS) == 3 && ajustes_escopo(AJ_CW_RETIDO_TAMBEM) == 3);
  // ...e estes continuam da TV.
  assert(ajustes_escopo(AJ_RESOLUCAO) == 1 && ajustes_escopo(AJ_TEX_MB) == 1 && ajustes_escopo(AJ_IDIOMA) == 1);
  assert(ajustes_escopo(AJ_LIVETV_RES) == 1 && ajustes_escopo(AJ_FONTE_UI) == 1 && ajustes_escopo(AJ_GPU_EFEITOS) == 1);
  assert(ajustes_escopo(AJ_TEMA) == 2 && ajustes_escopo(AJ_HERO) == 2);
}

static void blobIgnoraLocal(void) {
  char blob[512];
  char *saida = NULL;
  valor[AJ_VIDRO] = 1; valor[AJ_RELOGIO] = 1;
  snprintf(blob, sizeof blob, "{\"%s\":0,\"%s\":0}", CHAVE[AJ_VIDRO], CHAVE[AJ_RELOGIO]);
  ajustes_aplicar_blob(blob);
  assert(valor[AJ_VIDRO] == 1 && valor[AJ_RELOGIO] == 1);
  // A costura tambem nao toca nas chaves locais, mesmo que o blob as traga.
  snprintf(blob, sizeof blob, "{\"%s\":true,\"%s\":true}", CHAVE[AJ_VIDRO], CHAVE[AJ_RELOGIO]);
  ajustes_mesclar_blob(blob, &saida);
  assert(!saida || (strstr(saida, "true") && !strstr(saida, "false")));
  free(saida);
}

static void migracao(void) {
  int perfis[2] = { 1, 2 };
  char *t, *a, *b;
  // TV 2.0.x: ajustes.txt com vidro desligado e relogio ligado; o perfil 2 ja
  // tinha uma copia antiga (so chaves de conta) com o destaque desligado.
  valor[AJ_VIDRO] = 1; valor[AJ_RELOGIO] = 0; valor[AJ_HERO] = 0; valor[AJ_RESOLUCAO] = RES_4K;
  assert(gravar());
  { char antigo[96]; snprintf(antigo, sizeof antigo, "%s 1\n", CHAVE[AJ_HERO]); dados_gravar("ajustes-p2.txt", antigo); }
  assert(ajustes_migrar_escopo_para(perfis, 2, 1) == 1);
  // Backup do arquivo da TV, uma vez.
  t = dados_ler("ajustes.txt.pre21");
  assert(t && tem(t, AJ_VIDRO));
  free(t);
  // Perfil 1 (ativo, sem copia): completo, com as chaves locais de agora.
  a = dados_ler("ajustes-p1.txt");
  assert(tem(a, AJ_VIDRO) && tem(a, AJ_RELOGIO) && tem(a, AJ_HERO) && !tem(a, AJ_RESOLUCAO));
  // Perfil 2: manteve o que era dele e ganhou as locais com o valor da TV.
  b = dados_ler("ajustes-p2.txt");
  { char alvo[96]; snprintf(alvo, sizeof alvo, "%s 1\n", CHAVE[AJ_HERO]); assert(strstr(b, alvo)); }
  { char alvo[96]; snprintf(alvo, sizeof alvo, "%s 1\n", CHAVE[AJ_VIDRO]); assert(strstr(b, alvo)); }
  { char alvo[96]; snprintf(alvo, sizeof alvo, "%s 0\n", CHAVE[AJ_RELOGIO]); assert(strstr(b, alvo)); }
  assert(!tem(b, AJ_RESOLUCAO));
  free(a); free(b);
  // Segunda vez: nada muda (nem o backup, mesmo com o ajustes.txt diferente).
  valor[AJ_VIDRO] = 0; gravar();
  assert(ajustes_migrar_escopo_para(perfis, 2, 1) == 0);
  t = dados_ler("ajustes.txt.pre21");
  { char alvo[96]; snprintf(alvo, sizeof alvo, "%s 1\n", CHAVE[AJ_VIDRO]); assert(strstr(t, alvo)); }
  free(t);
  // Sem perfis na mao, nao marca como feito.
  dados_apagar("ajustes-escopo.txt");
  assert(ajustes_migrar_escopo_para(NULL, 0, 1) == 0);
  t = dados_ler("ajustes-escopo.txt");
  assert(!t);
  // Troca de perfil agora leva o vidro junto.
  valor[AJ_VIDRO] = 0;
  ajustes_perfil_guardar(1);
  valor[AJ_VIDRO] = 1;
  assert(ajustes_perfil_restaurar(1) == 1 && valor[AJ_VIDRO] == 0);
}

int main(void) {
  char dir[] = "/tmp/nuvio-aj-escopo-XXXXXX";
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1);
  dados_iniciar(dir);
  ajustes_dir(dir);
  escopos();
  blobIgnoraLocal();
  migracao();
  puts("ajustes_escopo: tudo ok");
  return 0;
}
