## What this fixes

On a Tizen 4/5 TV the subtitle worker was being starved. `sysconf` reports 4 cores, but the `dotnetzone` cpuset pins every app thread to a single core, and `nice 10` on that one shared core left the worker about 5.5% of a CPU.

Kaguya-sama S3E2, QE65Q80R, 1329–1339 s: 13–26.5 ms of actual CPU work per frame was stretching into 318–477 ms of wall-clock time. The subtitles were not slow, they were not being scheduled.

## What it does

Counts the CPUs the process may actually run on (`SYS_sched_getaffinity`) instead of the ones the TV has, and keeps the worker at `nice 0` with 1 libass blur thread when that count is 1 or less.

## Evidence

QE65Q80R, Tizen 5.0:

- 1329–1339 s: UI frames 522 → 596 of 600 (59.6 FPS), delivered cues 60 → 97, render max 477.4 → 120.8 ms (−75%), delay max 449 → 81 ms (−82%)
- 1339–1349 s: render max 318.0 → 102.3 ms (−68%), delay max 325 → 77 ms (−76%)

Confirmed in the device log after install:

```
[ass] tizen 4/5: 1-core sandbox, subtitle worker at normal priority (nice 0)
[ass] tv: cores=4 online of 4 (usable=1) | ... blur threads=1
```

Builds: `.tpk`, `.tpk40`, webOS, Android — 0 errors, 0 warnings under `-Werror=implicit-function-declaration -Werror=int-conversion -Werror=incompatible-pointer-types`.

## Not tested

Tizen 6/6.5/8+, webOS on hardware, Android on hardware, `.wgt` (no emsdk here).
