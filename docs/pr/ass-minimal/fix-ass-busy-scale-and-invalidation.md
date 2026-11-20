## What this fixes

Three separate problems in how the render resolution and the frame cache react to a busy scene.

**The hold timer measured the wrong thing.** In Kaguya-sama S3E2 the 1.79 s static sign at 1334–1338.2 s dropped Auto 50% back to 100%, because the 5 s hold was counted from the start of the busy burst instead of its end. `AUTO_MIN100_S` then locked 50% out for 2 s, so the following wipe rendered at 100% with a 418.1 ms max.

**Aegisub layer labels forced per-frame rerenders.** `Effect` was treated as animation whenever it was non-empty. A label like "Signs" rerendered a static cue every frame.

**False invalidations on authored ASS.** A future incremental batch, or a player-control margin change, threw away queued frames on a track that had not changed.

## What it does

Enters reduced resolution the moment a scene turns busy, and refreshes the hold timer while it stays busy. Extends the early forecast window to 2.2 s and pre-checks it before frame 0 of a burst. Matches only `Banner` and `Scroll` in `Effect`. Ignores `PlainAssStyle` updates on authored tracks. Resets the request state in `ass_inc_lote` only when a new event starts at or before `assPedidoMs + 100 ms`.

## Evidence

Builds clean on `.tpk`, `.tpk40`, webOS and Android. `assperf_resolucao` test passes. The TV behaviour is measured jointly with the other three PRs in `ass-minimal`; this one is the reason the wipe stays at 50% instead of jumping to 100%.

## Not tested

Tizen 6/6.5/8+, webOS, Android, `.wgt`.
