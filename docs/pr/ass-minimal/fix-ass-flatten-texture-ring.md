## What this fixes

Heavy animated ASS scenes draw 45 to 66 overlapping layers per frame. On a Mali-G51 that saturates fill rate before the CPU is anywhere near busy: 73.1 to 96.2 ms of GL draw time per frame, 38.7 FPS with 8 janks.

Uploading the merged result as a single texture had its own problem: reallocating or overwriting one texture stalled the driver, giving a 216.6 ms worst frame.

## What it does

Composites the frame on the worker into one straight-RGBA buffer when it is worth it — more than 24 layers covering more than one screen area, or more than 16 layers and at most 512 KB. Uses a 256-entry reciprocal table for the unpremultiply and an opaque-layer fast path to skip the blend.

The result goes into a 3-slot, grow-only, bucketed RGBA texture ring, drawn as one `GFX_TEXTO` quad with sub-rect UV scaling. Three slots because overwriting the one being read is what stalled the driver.

## Evidence

QE65Q80R, Kaguya-sama S3E2:

- drawn rects per frame: 45–66 → 2
- GL draw time: 73.1–96.2 ms → 0.3–0.8 ms (5.7 ms max during 1329–1339 s)
- 1329–1332 s: 38.7 → 52.1 FPS, janks 8 → 3
- 1329–1339 s: average render 40.4 → 31.0 ms

## Review fix included

`ass_frame_flatten()` called `ass_cor_final()` with the colour arguments zeroed, so a flattened frame ignored the viewer's chosen dialogue colour, which the per-image path applies. That colour is applied at draw time and a flattened frame cannot follow it changing, so such frames are now handed back to the per-image path rather than having a colour baked in. Latent today — nothing calls `assrender_definir_cor(1, ...)` — but it would have been a trap.

## Known trade-off

The straight-RGBA round trip loses 1–2 levels per channel on edges (53% exact, 35% off by one, 2.5% off by more than 3 for alpha ≥ 16). Below what is visible on a TV; noted so a before/after screenshot comparison does not look like a regression.

## Not tested

Tizen 6/6.5/8+, webOS, Android, `.wgt`.
