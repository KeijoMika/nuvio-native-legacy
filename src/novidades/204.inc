// CONTEUDO DO CARTAO DE NOVIDADES DA 2.0.4 (motor: novidades_cartao.c).
//
// Incluido UMA vez, no meio de novidades_cartao.c: as pecas de la (vidro,
// novArte, novArteDesfocada, sai3, janela, lerp) e os tipos NovItem, NovCena e
// NovConteudo ja existem aqui. A PROXIMA VERSAO copia este arquivo para
// src/novidades/<versao>.inc, troca o texto, as cenas e a marca, e muda o
// #include do motor. Nada no app.c.
//
// REGRAS DO TEXTO (iguais as da 2.0.2): frases curtas, o tom do app, nada que
// os commits ou docs/releases/<versao>/NOTAS.md nao provem. Toda frase passa
// pela tabela (src/idioma_tab.h e as irmas). As linhas de cada frase sao
// RESERVADAS (`linhas`): a captura (tests/novidades_cartao_shot.sh) mede nos
// 30 idiomas com a fonte da TV e reprova reticencias.
//
// AS CENAS desenham dentro do retangulo da previa (760 x 848); os 230 px de
// baixo sao da legenda. `t` e o segundo da cena; com Animacoes reduzidas, o
// estado final.

// Hotfix: vinhetas com os icones ja usados no app, sem inventar telas.
// Texto comprovado por docs/releases/2.0.4/NOTAS.md (#372/#328, #402, #344).
static void cenaIcone204(GfxRect p, float t, float a, const char *icone) {
  float r, g, b;
  float entrada = ajustes_animacoes_reduzidas() ? 1.0f : sai3(janela(t, 0.0f, 0.5f));
  float lado = 180.0f, cy = p.y + (p.h - 230.0f) * 0.5f;
  GfxRect ilha = { p.x + p.w * 0.5f - 140.0f, cy - 140.0f, 280.0f, 280.0f };
  ajustes_acento(&r, &g, &b);
  novArteDesfocada(24, p, PV_RAIO, a);
  gfx_cor(p, PV_RAIO / p.h, 0.016f, 0.017f, 0.021f, 0.70f * a);
  ilha.y += (1.0f - entrada) * 20.0f;
  vidro(ilha, 48.0f, entrada * a);
  gfx_icone((GfxRect){ ilha.x + (ilha.w - lado) * 0.5f,
                       ilha.y + (ilha.h - lado) * 0.5f, lado, lado },
            icone, r, g, b, entrada * a);
}
static void cenaTemporadas204(GfxRect p, float t, float a) {
  cenaIcone204(p, t, a, "aj_list-video");
}
static void cenaFontes204(GfxRect p, float t, float a) {
  cenaIcone204(p, t, a, "aj_layers");
}
static void cenaGuia204(GfxRect p, float t, float a) {
  cenaIcone204(p, t, a, "aj_tv-minimal-play");
}

static const NovCena CENAS_204[] = {
  { "Correções", "Temporadas", cenaTemporadas204, 0, 0 },
  { "Correções", "Fontes", cenaFontes204, 0, 0 },
  { "Correções", "TV ao vivo", cenaGuia204, 0, 0 },
};
static const NovItem ITENS_204[] = {
  { .grupo = "Correções", .icone = "aj_list-video", .nome = "Temporadas",
    .frase = "Temporadas certas, sem episódios repetidos.", .linhas = 2 },
  { .icone = "aj_layers", .nome = "Fontes",
    .frase = "FHD e Full HD agora em 1080p.", .linhas = 2 },
  { .icone = "aj_tv-minimal-play", .nome = "TV ao vivo",
    .frase = "Busca no guia sem fechar o app.", .linhas = 2 },
};
static const int ARTES_204[] = { 24 };
static const NovConteudo NOV_ATUAL = {
  .versao = "2.0.4",
  .arquivo = "novidades-204.txt",
  .subtitulo = "Correções",
  .itens = ITENS_204, .nItens = (int)(sizeof ITENS_204 / sizeof *ITENS_204),
  .cenas = CENAS_204, .nCenas = (int)(sizeof CENAS_204 / sizeof *CENAS_204),
  .artes = ARTES_204, .nArtes = (int)(sizeof ARTES_204 / sizeof *ARTES_204),
};
