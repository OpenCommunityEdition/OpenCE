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

## Ready caches (2026-10-25)

The game-data suites load their maps through `opence_import::testing`:
Blood Gulch held once per test binary, the campaign levels read from their
packs one at a time (held, ten 300 MB levels would sit in memory at once),
all through the game's cache folder (`build/engine/cache`, or
`OPENCE_CACHE`). The devgulch, control, cli and effects suites, which kept
their own cache folders (two of them wiped each run), use it too.

| suite | before | after |
|---|---|---|
| placements | 188.9 s | 89.9 s |
| stock_sounds | 267.5 s | 200.0 s |
| actors | 736.7 s | 688.4 s |
| stock_scenes | 72.0 s | 46.1 s |
| cutscenes | 46.7 s | 27.3 s |
| devices | 50.0 s | 31.1 s |
| sounds (ffi) | 25.0 s | 3.8 s |
| ui_walk | 21.0 s | 9.4 s |
| levels | 986.6 s | 983.0 s |
| **all suites** | **3349.6 s** | **3184.1 s** |

Importing was 5% of the run. The rest is the debug sim stepping the
levels (`levels` and `actors` alone are 52%), which is the optimised test
profile's task. The baseline's `scenarios` (8.3 s) was the script crate's:
two crates have a suite of that name and one overwrote the other; the
timings now name such suites by their crate, and campaign's takes 224.7 s.

## The optimised test profile (2026-10-25)

`[profile.test-opt]` builds the tests as release does, with debug
assertions, overflow checks and incremental builds kept on
(`sim/tests/checks.rs` fails if either check is off;
`tools/test_cargo_profiles.py` checks the profile). All 92 suites pass
under it, the reference traces included.

| step | debug (`test`) | `test-opt` |
|---|---|---|
| test build, after the debug build | 38.8 s | 118.1 s |
| test rebuild after touching the sim | 12 s | 7 s |
| **the test suites** | **3184.1 s** | **290.8 s** |

| suite | before | after |
|---|---|---|
| levels | 983.0 s | 55.0 s |
| actors | 688.4 s | 47.0 s |
| devgulch_stations | 218.6 s | 19.1 s |
| stock_sounds | 200.0 s | 20.0 s |
| stock_lightmaps | 143.6 s | 17.9 s |
| placements | 89.9 s | 9.4 s |
| campaign | 85.1 s | 10.9 s |
| scripts | 84.1 s | 10.7 s |
| animation_resolution | 83.0 s | 10.5 s |
| opence_import (lib) | 70.6 s | 9.2 s |
| stock_scenes | 46.1 s | 4.6 s |

The cold build costs 80 s more once, and is then paid back on every run:
a whole test run is now under seven minutes, where it was 53. The
rebuild after an edit is faster too, as the optimised dependencies are
built once and the workspace's crates rebuild incrementally. Release's
`panic = "abort"` does not reach the tests (cargo builds test targets to
unwind), so a failing test still reports as before.

## A faster debug loop (2026-10-25)

`[profile.dev.package."*"] opt-level = 2`: the dependencies are optimised
in debug builds and in the `test` profile, which inherits dev; the
workspace's crates stay at opt-level 0.

| step | before | after |
|---|---|---|
| debug build, cold | 47.2 s | 67.4 s |
| debug build after touching the sim | 2.4 s | 2.3 s |
| test build, after the debug build | 38.8 s | 48.2 s |
| devgulch_stations, debug | 218.6 s | 180 s |
| stock_sounds, debug | 200.0 s | 181 s |
| placements, debug | 89.9 s | 69 s |

A 10–25% gain on the debug suites, for 20 s once on a cold build, and an
edit's rebuild unchanged. The gain is small because most of the sim's
time is in the workspace's crates, and glam's generics are compiled in
the crate that calls them, at its opt-level; `test-opt` is the profile
for the game-data suites. (The first placements run took 112 s on a
loaded machine; alone it took 69 s.)

sccache is left out: it cannot cache crates built incrementally, which
is every crate an edit rebuilds, and the dependencies are already kept
by the target folder and, for `nix build`, by crane.

## nextest and cargo deny in the test step (2026-10-25)

`engine_build.py --test` now runs `cargo deny` (licences, bans and
sources, offline) and then the workspace under `cargo nextest` in the
`test-opt` profile, before the XCTests; `--suite rust` or `--suite swift`
picks one half. `engine/.config/nextest.toml` marks a test slow after 60 s
(a campaign level stepped to its end is slow, not stuck), runs every test
when one fails, and retries the net session's UDP suite twice.

| run | time |
|---|---|
| the suites one after another (`test-opt`) | 290.8 s |
| **nextest, all 1026 tests (`test-opt`)** | **74.0 s** |

All 1026 passed, none retried; two were slow (`halo_plays_to_its_end`,
68 s, and `an_enemy_before_the_player_shoots_it`, 62 s), and they set the
whole run's length. nextest runs each test in its own process, so the
level a suite held once is loaded by each test; the packs are read from
the cache folder, so that costs a pack read (a second or two), not an
import, and running tests from every binary at once more than pays it
back. A deny.toml with MIT removed fails the check
(`test_a_forbidden_licence_fails_the_check`), so the check is not a no-op.

## Deterministic maps (2026-10-25)

std's `HashMap` and `HashSet` take a random seed each run, so whatever
iterates one goes in another order each run. One did matter: the
contrails were kept in a `HashMap` and aged with `retain`, each trail
drawing the game's random numbers as it went, so the same start trailed
differently from run to run (`the_same_start_trails_the_same_each_tick`,
four runs of 40 ticks, failed: "run 1 trailed otherwise"). They are now
kept as the original keeps them (`source/memory/data.c`): in slots, a
new trail in the lowest free slot, updated and drawn in slot order
(`particles/slots.rs`; `a_trail_takes_the_lowest_free_slot_and_they_age_in_slot_order`).

The rest of the workspace's 14 files of hash maps use `rustc_hash`'s maps,
whose seed is fixed, and the two test files that keyed by name use a
`BTreeMap`. `engine/clippy.toml` now bans std's two types (a use of one
fails clippy: checked with a test function added and removed); UniFFI
lowers only std's map, so the two shell-variable methods take it by a
`Dictionary` alias, which is copied into an ordered map at once.

| check | result |
|---|---|
| clippy, workspace, all targets, `-D warnings` | clean |
| nextest, all tests (`test-opt`) | 1028 passed, 75.8 s |
| contrail tests | 13 passed (the determinism test failed before) |

(Clippy also found `assert!(cfg!(debug_assertions))` in `sim/tests/checks.rs`,
a constant to it; the value goes through `black_box` now.)
