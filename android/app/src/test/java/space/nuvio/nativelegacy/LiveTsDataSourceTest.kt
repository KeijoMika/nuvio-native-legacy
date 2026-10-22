package space.nuvio.nativelegacy

import android.net.Uri
import androidx.media3.common.C
import androidx.media3.datasource.DataSource
import androidx.media3.datasource.DataSpec
import androidx.media3.datasource.TransferListener
import org.junit.Assert.*
import org.junit.Test
import java.io.IOException

class LiveTsDataSourceTest {
    private class Source : DataSource {
        val requests = mutableListOf<DataSpec>()
        var remaining = 0
        var closes = 0
        var empty = false
        override fun open(spec: DataSpec): Long {
            requests += spec
            remaining = if (empty) 0 else 188
            return remaining.toLong()
        }
        override fun read(buffer: ByteArray, offset: Int, length: Int): Int {
            if (length == 0) return 0
            if (remaining == 0) return C.RESULT_END_OF_INPUT
            val count = minOf(length, remaining)
            buffer.fill(0x47, offset, offset + count)
            remaining -= count
            return count
        }
        override fun close() { closes++ }
        override fun getUri(): Uri = Uri.parse("https://edge.test/redirected.ts")
        override fun getResponseHeaders() = mapOf("Content-Length" to listOf("188"))
        override fun addTransferListener(listener: TransferListener) = Unit
    }

    @Test fun finiteLiveResponsesContinueWithoutExtractorEofOrRanges() {
        val uri = Uri.parse("https://origin.test/live/user/pass/42.ts?signature=keep")
        val upstream = Source()
        val factory = DataSource.Factory { upstream }
        val source = LivePlaylistDataSource.Factory(factory, factory, liveTsUri = uri).createDataSource()
        val spec = DataSpec.Builder().setUri(uri).setHttpRequestHeaders(mapOf("Authorization" to "token")).build()
        assertEquals(C.LENGTH_UNSET.toLong(), source.open(spec))
        val buffer = ByteArray(188)
        repeat(100) { assertEquals(188, source.read(buffer, 0, buffer.size)) }
        assertEquals(0, source.read(buffer, 0, 0))
        assertEquals(100, upstream.requests.size)
        upstream.requests.forEach {
            assertEquals(uri, it.uri)
            assertEquals(0L, it.position)
            assertEquals("token", it.httpRequestHeaders["Authorization"])
        }
        upstream.empty = true
        try {
            source.read(buffer, 0, buffer.size)
            fail("An empty reconnect must fail rather than spin or publish EOF")
        } catch (_: IOException) { }
        assertEquals(101, upstream.requests.size)
        source.close()
        assertEquals(101, upstream.closes)
    }

    @Test fun vodAndHlsSegmentsKeepFiniteLengthAndEof() {
        val live = Uri.parse("https://origin.test/live/u/p/42.ts")
        for (path in listOf("movie/u/p/42.ts", "live/u/p/segment.ts")) {
            val upstream = Source()
            val factory = DataSource.Factory { upstream }
            val source = LivePlaylistDataSource.Factory(factory, factory, liveTsUri = live).createDataSource()
            assertEquals(188L, source.open(DataSpec(Uri.parse("https://origin.test/$path"))))
            assertEquals(188, source.read(ByteArray(188), 0, 188))
            assertEquals(C.RESULT_END_OF_INPUT, source.read(ByteArray(188), 0, 188))
            assertEquals(1, upstream.requests.size)
            source.close()
        }
    }
}
