# Rust tools: timings

Measured with `tools/engine_timings.py` inside `nix develop`, on the M1 Max,
with the user's maps in `assets/maps`. Cold builds use their own target
folders; the suites are each test binary run on its own, one after another
(as `cargo test` runs them).

## The starting point (2026-10-25, at 5fd62cd)

| step | time |
|---|---|
| debug build, cold (`cargo build --workspace`) | 47.2 s |
| debug build after touching the sim | 2.4 s |
| test build, after the debug build (`cargo test --no-run`) | 38.8 s |
| release library, cold (`-p opence_ffi --lib`) | 52.3 s |
| the app (`tools/engine_build.py`, Rust half incremental) | 72.2 s |
| **the test suites (85 binaries)** | **3349.6 s (55.8 min)** |
| devgulch, release, frame p50 / p99 | 9.21 / 9.74 ms |
| devgulch, GPU p99 | 12.76 ms |
| devgulch, start and load from a cached pack | 20.9 s |

The builds are not the loop's cost: the tests are. Ten suites take 2872 s,
86% of the run:

| suite | time | what it does |
|---|---|---|
| levels (campaign) | 986.6 s | steps every campaign level in the debug sim |
| actors (campaign) | 736.7 s | AI actors on the campaign levels |
| stock_sounds (audio) | 267.5 s | decodes every stock sound |
| devgulch_stations (ffi) | 223.4 s | builds devgulch, steps each station |
| placements | 188.9 s | object placement traces |
| stock_lightmaps (import) | 143.8 s | imports and checks lightmaps |
| campaign | 85.4 s | |
| scripts | 84.0 s | |
| animation_resolution | 83.1 s | |
| stock_scenes (bake) | 72.0 s | |

All passed but `session` (net), which passed when run again on its own
(5 tests, 5.2 s): its UDP games are timing-sensitive when the machine is
loaded, so it is a flaky suite to watch under nextest.

What follows from it: the debug sim stepping the stock levels is most of
the time (the optimised test profile), and each suite imports its map
from the file, often once per test, where a held pack would do (the
ready caches).

## The nix packages

`nix run .#engine` built and ran (`OpenCE Engine 0.1.0`) once the
vendored uniffi_bindgen was copied in (askama reaches its `askama.toml` by
a path that climbs out of crane's links) and the ragdolls' overrides were
in the source. The dependencies' derivation now has a fixed version: before,
each commit renamed it and built every dependency again (3–4 minutes);
after a new commit only the workspace and the app rebuild.
