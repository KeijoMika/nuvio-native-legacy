# ass-minimal PR descriptions

Base: `namer/subtitle-rewrite` (`e64fa709e`). Four independent, single-commit branches, each of which applies cleanly on its own and together in any order:

| File | Branch |
|---|---|
| `fix-ass-worker-nice0-1core.md` | `fix/ass-worker-nice0-1core` (`950392021`) |
| `fix-ass-busy-scale-and-invalidation.md` | `fix/ass-busy-scale-and-invalidation` (`780e4a38a`) |
| `fix-ass-flatten-texture-ring.md` | `fix/ass-flatten-texture-ring` (`6596e9251`) |
| `fix-ass-rigid-layer-cache.md` | `fix/ass-rigid-layer-cache` (`d95ecd47f`) |

`ass-minimal` (`db896da7e`) is the four commits applied in that order, with no NuvioM flavor.

All five were pushed to `KeijoMika` on 2026-11-20 after a pre-PR review that found and fixed a compile break for webOS/`.wgt`/host tests and a libass memory leak in the rigid cache.

Open them with, for example:

    gh pr create --repo KeijoMika/nuvio-native-legacy \
      --base namer/subtitle-rewrite --head KeijoMika:fix/ass-rigid-layer-cache \
      --title "$(git log -1 --format=%s fix/ass-rigid-layer-cache)" \
      --body-file docs/pr/ass-minimal/fix-ass-rigid-layer-cache.md
