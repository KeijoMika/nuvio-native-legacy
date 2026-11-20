## What this fixes

Frame-by-frame signs re-rasterize identical geometry every frame. In the Kaguya-sama S3E2 phone scroll (1334–1338.2 s), 38 to 53 moving layers per frame come from events 40 to 50 ms long whose text and vector paths never change. Static positioned signs and the rigid `\move` banners in the 1340 s wipe were also re-rasterized every frame.

The obvious version of this is wrong in two visible ways, both found on the TV: shifting cached bitmaps by integer pixels in a reduced-resolution buffer makes neighbouring lines wobble 1–2 px, and caching a line that crosses the viewport edge freezes a clipped or empty bitmap.

## What it does

Snapshots the active events per frame and caches three kinds of layer in `assRigid`:

- repeated rigid `\pos(x,y)` layers
- static positioned layers (longer than 150 ms)
- rigid `\move(x1,y1,x2,y2[,t1,t2])` banners without `\org`/`\frx`/`\fry`

with a two-hit promotion filter, a 160 ms retry cooldown for border-clipped layers, 384 entries and a 32 MB cap.

Shifts are computed in each pass's own scale (`lw/lh` versus `lwB/lhB`) from floating-point PlayRes coordinates, and the fractional remainder goes into the `GL_LINEAR` quad position as `subX`/`subY`. That is what removes the wobble: the cache stores a sub-pixel reference, not a rounded one.

## Evidence

QE65Q80R, Custom auto 50% text / 25% blur, `nuviom-ass-minimal` build:

- rigid hits: 1,530 in the phone scroll, 4,290 in the battle wipe
- phone scroll: 600/600 UI frames, 4.5 ms average render, 17.1 ms max
- battle wipe: 582/600 UI frames, 9.5 ms average, 84.2 ms max, 16 ms average delay

Confirmed by eye: the 1–2 px inter-line shake is gone and entering text is unclipped.

## Review fixes included

_Compile break._ `ass_rigid_parse()` read `ASS_Event.nv_sign`, which only the `.tpk`'s patched libass declares. webOS, the `.wgt` and three host tests build against a stock header and stopped building. The read is now behind the same `ASS_NV_API` guard the other uses already have.

_Memory leak._ The snapshot track copies active events every frame and used to set `render_priv` to `NULL`. That field is libass's per-event collision state: allocated lazily in `get_render_priv()`, freed in `ass_free_event()`, which a copied event never reaches because teardown is a plain `free()` of the array. Instrumented run: 320 allocations created, 0 released. Now freed before clearing, freed again for the `rigidEvent` copy at the end of the miss pass, and released for live slots on teardown.

Note for the reviewer: keeping the pointer across frames is _not_ the fix. libass only resets collision state when `render_id` changes, and that id is per renderer, so a slot reused for a different event would keep the previous event's collision box. Clearing is required; releasing first is the fix.

## Verified after the fixes, on the TV

`ass-minimal` (`db896da7e`), same episode. Different build from the numbers above, which is why the hit counts differ:

```
[ass] tv: cores=4 online of 4 (usable=1) | ... bitmap engine=NEON, blur threads=1
```

- 1336 s: `rigid=4237/115`, render 29.5 ms avg / 100.2 ms max
- 1343 s: `rigid=3963/2`, render 5.9 ms avg / 63.9 ms max
- 1353 s: `rigid=2178/0`, render 9.4 ms avg / 56.2 ms max
- subtitle textures peak at 768 (the cache cap) then drop back when the scene ends

## Not tested

Tizen 6/6.5/8+, webOS, Android, `.wgt`.
