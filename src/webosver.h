#ifndef NV_WEBOSVER_H
#define NV_WEBOSVER_H
#include <stddef.h>
// UMA fonte da versao maior do webOS desta TV. Ordem: "webos_release" de
// /var/run/nyx/os_info.json (a mesma da linha [tv]; existe nas TVs de 2017,
// webOS 3.9, que NAO tem /etc/starfish-release), depois a linha "release N" de
// /etc/starfish-release. 0 = desconhecida: cada porta decide o que isso
// significa, e nada aqui chuta. Valor guardado no primeiro uso (nao muda com o
// app aberto); seguro entre fios.
int nv_webos_major(void);
// Fonte que valeu: "nyx", "starfish" ou "-". Linha do starfish-release ("" se
// o arquivo falta) para o log.
const char *nv_webos_fonte(void);
const char *nv_webos_starfish_linha(void);
// Parsers puros. Devolvem 0 para qualquer coisa que nao seja uma versao
// inteira e fechada: o valor tem de ser string JSON "N.N..." (dois numeros
// separados por ponto, sufixo "-xx" ou ".x" depois disso e aceito).
int nv_webos_parse_nyx(const char *json);
int nv_webos_parse_starfish(const char *texto, char *linha, size_t cap);
// Capacidade de leitura de cada arquivo (o que passa disso e lido como
// truncado e, no nyx, vale desconhecido se cortou o valor).
#define NV_WEBOS_LEITURA 4096
// So para teste: troca os caminhos (NULL = os de verdade) e esquece o valor.
void nv_webos_testar(const char *nyx, const char *starfish);
#endif
