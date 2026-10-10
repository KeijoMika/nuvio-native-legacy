#!/usr/bin/env python3
"""Executa os corpos reais de abrirMain/onPlayerError em JVM com fila e relogio controlados.
A compilacao integral contra Media3 e coberta por cacheboost_android.py.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt').read_text()

def bloco(inicio):
    start = source.index(inicio)
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

abrir = bloco('    private fun abrirMain(').split('        novaSuperficie(act)')[0] + '        criados.add(url)\n    }'
erro = bloco('        override fun onPlayerError(').replace('override fun', 'fun')
fixture = r'''
import java.util.concurrent.atomic.AtomicInteger
object C { const val TRACK_TYPE_UNKNOWN = -1; const val TRACK_TYPE_AUDIO = 1; const val TRACK_TYPE_VIDEO = 2 }
object SystemClock { var agora = 100L; fun elapsedRealtime() = agora }
object Log {
    val linhas = mutableListOf<String>()
    fun i(t: String, s: String) { linhas.add(s) }
    fun w(t: String, s: String) { linhas.add(s) }
}
open class PlaybackException(val errorCode: Int) {
    val errorCodeName = "decoder"; val message = "fixture"
    companion object { const val ERROR_CODE_DECODER_INIT_FAILED = 4001; const val ERROR_CODE_DECODING_FAILED = 4003 }
}
class ExoPlaybackException(code: Int, val rendererIndex: Int, val type: Int = TYPE_RENDERER) : PlaybackException(code) {
    val rendererName = "fixture"
    companion object { const val TYPE_RENDERER = 1 }
}
class ExoPlayer(val tipos: List<Int>) {
    val rendererCount get() = tipos.size
    fun getRendererType(i: Int) = tipos[i]
    val currentPosition = 0L
}
class Fila {
    data class Item(val quando: Long, val r: Runnable)
    val itens = mutableListOf<Item>()
    fun postDelayed(r: Runnable, ms: Long) { itens.add(Item(SystemClock.agora + ms, r)) }
    fun ate(fim: Long) {
        while (itens.any { it.quando <= fim }) {
            val i = itens.minBy { it.quando }; itens.remove(i)
            SystemClock.agora = i.quando; i.r.run()
        }
        SystemClock.agora = fim
    }
}
object Fixture {
    const val TAG = "NvPlayer"; const val EV_ERRO = 5; const val RETRY_DECODER_MS = 5000L
    val principal = Fila(); var activity: Any? = Any()
    val pedidos = AtomicInteger(1); var pedidoAtivo = 1
    val liberacoesEmCurso = AtomicInteger(); var semRecriar = true
    var releaseMs = 0L; var releases = 0; val criados = mutableListOf<String>()
    val minha get() = sessao
    var sessao = 1; var abriuEm = 0L; var retentou = true
    var urlAtual = "url"; var cabAtual = ""; var inicioAtualMs = 0; var geracaoNative = 0; var fracaoAtual = 0
    var player: ExoPlayer? = null; var evento = -99
    fun atual(minha: Int) = minha == sessao && pedidoAtivo == pedidos.get()
    fun confirmarRetomada(g: Int, a: Boolean) {}
    fun ev(t: Int, a: Int, b: Int) { evento = b }
    fun liberar() {
        releases++; sessao++
        if (releaseMs > 0) {
            liberacoesEmCurso.incrementAndGet()
            principal.postDelayed({ liberacoesEmCurso.decrementAndGet() }, releaseMs)
        }
    }
    ABRIR
    ERRO
    fun abrir(u: String) { abrirMain(u, "", false, 0, 0, pedidos.get()) }
    fun reset() {
        principal.itens.clear(); SystemClock.agora = 100; pedidos.set(1); pedidoAtivo = 1
        liberacoesEmCurso.set(0); criados.clear(); releases = 0; semRecriar = true; releaseMs = 0
        Log.linhas.clear(); activity = Any()
    }
}
fun main(args: Array<String>) {
    val f = Fixture
    if (args[0] == "renderer") {
        // Ordem invertida: o indice do renderer NAO e o tipo.
        f.player = ExoPlayer(listOf(C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_VIDEO))
        for (code in listOf(4001, 4003, 4004, 4005)) {
            f.onPlayerError(ExoPlaybackException(code, 0))
            check(f.evento == C.TRACK_TYPE_AUDIO) { "P2 erro audio $code enviado como renderer=${f.evento}" }
            f.onPlayerError(ExoPlaybackException(code, 1))
            check(f.evento == C.TRACK_TYPE_VIDEO)
        }
        f.onPlayerError(PlaybackException(4003)); check(f.evento == C.TRACK_TYPE_UNKNOWN)
        f.onPlayerError(ExoPlaybackException(4003, 99)); check(f.evento == C.TRACK_TYPE_UNKNOWN)
        f.onPlayerError(ExoPlaybackException(4003, 1, 0)); check(f.evento == C.TRACK_TYPE_UNKNOWN)
        f.pedidos.incrementAndGet(); f.evento = -99
        f.onPlayerError(ExoPlaybackException(4003, 1)); check(f.evento == -99)
        check(Log.linhas.any { it.contains("renderer=audio") && it.contains("loadMs=") })
        check(Log.linhas.any { it.contains("renderer=video") })
        println("PASS renderer: audio/video/desconhecido, 4 codigos e callback obsoleto")
        return
    }
    f.reset(); f.releaseMs = 600; f.abrir("nova")
    check(f.criados.isEmpty()) { "player novo criado antes do release anterior" }
    f.principal.ate(699); check(f.criados.isEmpty())
    f.principal.ate(725); check(f.criados == listOf("nova") && f.releases == 1)
    check(Log.linhas.any { it.contains("[player] esperou o release anterior 600 ms") })
    f.reset(); f.releaseMs = 5000; f.abrir("prazo")
    f.principal.ate(3099); check(f.criados.isEmpty())
    f.principal.ate(3100); check(f.criados == listOf("prazo") && f.releases == 1)
    f.reset(); f.releaseMs = 600; f.abrir("cancelada"); f.pedidos.incrementAndGet()
    f.principal.ate(4000); check(f.criados.isEmpty())
    f.reset(); f.releaseMs = 600; f.abrir("velha"); f.pedidos.incrementAndGet(); f.abrir("ultima")
    f.principal.ate(1000); check(f.criados == listOf("ultima"))
    f.reset(); f.releaseMs = 600; f.abrir("encerrada"); f.activity = null; f.pedidos.incrementAndGet()
    f.principal.ate(4000); check(f.criados.isEmpty())
    f.reset(); f.semRecriar = false; f.releaseMs = 5000; f.abrir("TCL")
    check(f.criados == listOf("TCL"))
    f.reset(); f.abrir("primeira"); check(f.criados == listOf("primeira"))
    println("PASS release: espera, prazo 3s, cancelamento, substituicao, encerramento e TCL")
}
'''.replace('ABRIR', abrir).replace('ERRO\n', erro + '\n')
cache = Path.home() / '.gradle/caches/modules-2/files-2.1'
def jar(group, artifact, version):
    return str(next((cache / group / artifact / version).glob('*/*.jar')))
stdlib = jar('org.jetbrains.kotlin', 'kotlin-stdlib', '2.0.21')
classpath = ':'.join([jar('org.jetbrains.kotlin', 'kotlin-compiler-embeddable', '2.0.21'), stdlib,
    jar('org.jetbrains.kotlin', 'kotlin-script-runtime', '2.0.21'),
    jar('org.jetbrains.intellij.deps', 'trove4j', '1.0.20200330'),
    jar('org.jetbrains.kotlinx', 'kotlinx-coroutines-core-jvm', '1.6.4'),
    jar('org.jetbrains', 'annotations', '13.0')])
java = os.environ.get('NUVIO_TEST_JAVA') or str(next((Path.home() / '.local/jdks').glob('jdk-17*/Contents/Home/bin/java')))
with tempfile.TemporaryDirectory(prefix='nuvio-decoder409-') as tmp:
    tmp = Path(tmp); file = tmp / 'DecoderFixture.kt'; file.write_text(fixture)
    subprocess.run([java, '-cp', classpath, 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
                    '-no-stdlib', '-no-reflect', '-nowarn', '-classpath', stdlib,
                    '-jvm-target', '17', '-d', str(tmp / 'classes'), str(file)], check=True)
    failed = 0
    for case in ('renderer', 'release'):
        failed += subprocess.run([java, '-cp', f'{tmp / "classes"}:{stdlib}', 'DecoderFixtureKt', case]).returncode != 0
    raise SystemExit(bool(failed))
