package space.nuvio.nativelegacy

// QUEM NAO RECRIA A SUPERFICIE NO HDR. A recriacao do primeiro quadro HDR (e a
// segunda, 1,5 s depois) existe por causa do painel da TCL (MediaTek), que so
// liga o modo HDR quando a Surface nasce com o decoder ja em HDR. Em outras
// familias ela so custa:
// - MStar (OMX.MS.*, caixas Shinon): o hwcomposer refaz o overlay a cada
//   GONE/VISIBLE e a tela pisca/trava.
// Devolve o nome da familia dispensada, ou null para recriar.
object RecriaHdr {
    fun dispensa(decoders: List<String>): String? {
        if (decoders.any { it.startsWith("OMX.MS.") }) return "MStar"
        return null
    }
}
