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

## insta and pretty_assertions (2026-10-25)

The shell's text is kept as insta snapshots (`crates/dev/tests/snapshots`):
the command listing, a command's help, a variable's help and a variable
listing, where the tests had held two of them as hand-written strings and
the listing not at all. The import's tag index is kept the same way, a
tag a line, for the Xbox and Custom Edition test levels
(`crates/import/src/snapshots`; the test levels are the crate's own
made-up ones, so no game data is in them). A changed text now fails with
a diff and a `.snap.new` to accept with `cargo insta review`; a missing
snapshot fails too (checked before the snapshots were written). The
registry's other comparisons go through `pretty_assertions`, which prints
a diff in place of two long values.

The dev state's and the bake report's snapshots are left for now: their
code (`ffi/src/dev_state.rs`, `ffi/src/bake.rs`) has another session's
uncommitted changes, and a snapshot of it now would fail when they land.

| check | result |
|---|---|
| a missing snapshot | fails |
| clippy, workspace, `-D warnings` | clean |
| cargo deny | ok (insta, similar, pretty_assertions, yansi, diff: MIT or Apache-2.0) |
| nextest, all tests (`test-opt`) | 1029 passed, 75.7 s |

## Phase 1's check (2026-10-25)

| check | result |
|---|---|
| nextest, all tests (`test-opt`) | 1029 passed |
| the build tools' tests (pytest) | 11 passed, 1 failed |

The failure is the Swift suite's, in two tests that are not this phase's:

- `HudTests` differs from its approved frame by 3.5% (limit 0.5%). The
  difference is in the lit world and the rifle (the lower half; the HUD's
  panels match), and the approved frame is from 2026-10-22, before the
  lighting controls changed the shaders (bbeeda1c). The maps this phase
  changed in `ui.rs` and `render_level.rs` are looked up, never iterated,
  so they cannot reorder a draw. The frame wants approving again, which is
  the user's to do (`build/engine/golden` to `assets/golden/engine`).
- `ZZFarHillsTests` ("no cached bake") is another session's uncommitted
  scratch test.

## Property tests (2026-10-25)

proptest, with a fixed seed (`0x0CE0_0CE0`) and no failure files, so each
run tries the same cases and a failure prints its smallest input:

| crate | properties | cases |
|---|---|---|
| import (`src/properties.rs`) | a slice is the bytes it names or `OutOfRange` with its exact fields; reads at the end of the address space fail; a word is its little-endian bytes (u32, i32, f32 bits); text is printable, within its longest, and ends at its terminator | 512 each |
| import | a pack gives back the level and header written to it (with arbitrary bitmap references); any archive byte changed is a `Fingerprint` error; a shortened or lengthened pack is refused | 64 each |
| script (`tests/properties.rs`) | any text is run or refused; a line of the console's own words gives the same answer twice; 63 levels of nesting run and 64 are refused (`MAX_DEPTH`); a line past `MAX_TEXT` is refused | 1024 each |
| physics (`tests/properties.rs`) | a sphere or box thrown at a wall at 1–60 units a second never crosses it or the floor in 120 ticks; a body dropped from up to 20 units with restitution 0–1 never gains energy (1% and 0.05 allowed) | 48 each |

None found a fault. The first nesting test used `not`, which is a game
function, not one of the console's forms, and was refused as an unknown
function: the test now nests the console's own one-argument `and`.

| check | result |
|---|---|
| clippy, three crates, `-D warnings` | clean |
| cargo deny | ok (proptest and rand: MIT or Apache-2.0) |
| nextest, all tests (`test-opt`) | 1042 passed, 3 skipped (212 s, beside a mutants run) |

## Coverage floors (2026-10-25)

`tools/engine_coverage.py` runs the workspace's tests under
`cargo llvm-cov nextest` (the `test-opt` profile) and counts each crate's
own `src/` lines; `--write-floors` writes `engine/coverage.toml`, a floor a
point or so under the day's share (rounded down). Measured with the user's maps
(without them the tests that need them skip, and the script prints the
numbers without checking):

| crate | lines covered | share | floor |
|---|---|---|---|
| audio | 2856 of 2930 | 97.5% | 96 |
| bake | 4225 of 4356 | 97.0% | 95 |
| campaign | 2919 of 3326 | 87.8% | 86 |
| cli | 267 of 371 | 72.0% | 70 |
| content | 2552 of 2624 | 97.3% | 96 |
| dev | 2234 of 2297 | 97.3% | 96 |
| ffi | 17438 of 18466 | 94.4% | 93 |
| import | 10661 of 11153 | 95.6% | 94 |
| net | 954 of 1001 | 95.3% | 94 |
| physics | 1030 of 1104 | 93.3% | 92 |
| script | 1051 of 1164 | 90.3% | 89 |
| sim | 18007 of 18837 | 95.6% | 94 |
| ui | 1981 of 2110 | 93.9% | 92 |

The instrumented run took 396 s of testing against 212–280 s for the plain
one (both beside a mutants run), so coverage is not in every test step: it
is its own check, run at each phase's end (`python tools/engine_coverage.py`,
which exits non-zero below a floor), and its numbers go in that phase's
report. `cli` is the weakest: its line editor's loop is driven by a
terminal the tests do not have.

| check | result |
|---|---|
| `tools/test_engine_coverage.py` | 6 passed (the command, per-crate counting, shortfalls, floors from numbers, the written file, every crate has a floor) |
| the check against the written floors | passes |

## Fuzz targets (2026-10-25)

Six `cargo fuzz` targets, each with the same entry in the suite's smoke
tests (`crates/import/src/fuzz_smoke.rs`, two seconds each):
`cache_file`, `xbox_cache`, `pack`, `ui`, and new for this task `sound`
(an Xbox cache's sounds, given as its header and inflated body, as
`xbox_cache` is) and `bitmap` (each of a cache's bitmaps).
`import_sounds` and `import_bitmap` are public for them.

Two tests tie the targets to the suite: `every_fuzz_target_has_an_entry`
(engine/fuzz/Cargo.toml's bins against the entries) and
`the_saved_fuzz_inputs_are_read_or_refused`, which replays every input
`cargo fuzz` kept on this machine (its corpus and any crash) through its
target's entry, so a crash found in a long run stays fixed. The corpora
are never committed.

No target had seeds: random bytes never passed the cache header's checks
(a first minute of `sound` and `bitmap` reached 42 coverage points with a
one-byte corpus). `the_seeds_are_each_targets_fake` writes each target's
fake (made by the tests, not from the game's files) to its corpus when
`OPENCE_FUZZ_SEEDS` is set. A minute of each, from the seeds:

| target | runs | coverage (start → end) | corpus | crashes |
|---|---|---|---|---|
| cache_file | 609,604 | 3053 → 5794 | 619 | 0 |
| xbox_cache | 91,315 | 3748 → 5698 | 615 | 0 |
| pack | 1,675,983 | 2490 → 2589 | 30 | 0 |
| ui | 604,364 | 894 → 1924 | 439 | 0 |
| sound | 367,400 | 1080 → 1727 | 424 | 0 |
| bitmap | 1,093,820 | 403 → 888 | 210 | 0 |

The 2,337 saved inputs replay in 0.25 s in the suite.

| check | result |
|---|---|
| clippy `-D warnings`, the workspace | clean |
| the fuzz crate on nightly (`cargo check --bins`) | builds |
| nextest, all tests (`test-opt`) | 1047 passed, 3 skipped (280 s) |

## Golden frames (2026-10-25)

`StationFramesTests` starts each of devgulch's nine stations (weapons, ai,
effects, sounds, shaders, animation, hud, physics, vehicles; vehicles last,
since it leaves the player seated), lets it run a second, and draws the
player's view headless at 480×270. Each frame is drawn twice and must match
to the bit, is written to `build/engine/golden`, and is compared with its
approved frame in `assets/golden/engine` (share of differing pixels under
0.005). Both folders are ignored by git: the frames are drawn from the
user's map. `tools/engine_golden.py` lists each frame as new, same or
changed and approves frames by name, prefix or `--all`.

The plan named swift-snapshot-testing. The tests already have a capture,
a PNG writer and `CapturedFrame.difference` with a tolerance, and the
approved frames already live in `assets/golden/engine`. A snapshot library
would be an SPM dependency fetched inside the Nix build, for no new check,
so the existing ones are used.

The bit-for-bit check found that a new renderer's first frame with
particles differs from its second by one level in 12 pixels (the weapons
station, 38 particles). Later frames match exactly, and so does a first
frame without particles. Exposure is not the cause: it stays at 0 and
inside the level's limits (−0.5 to 0.5) in both frames. The test draws one
frame first and throws it away; the cause is left for the fidelity track's
renderer work. The capture draws the level, its objects and particles, but
not the HUD or the first-person view: the hud station's frame shows the
level only.

| check | result |
|---|---|
| `StationFramesTests`, nine stations | each drawn twice: identical |
| a second run against the approved frames | all nine: share 0.00000, largest 0 |
| `tools/test_engine_golden.py` | 4 passed |

## Mutation testing (2026-10-25)

`cargo mutants` (with nextest, the `test-opt` profile, four jobs) was run
over `physics` and `script`, and then again with `--iterate` after the new
tests, so the second run tests only the first run's survivors. `sim`
cannot be mutated yet. `ragdoll.rs` includes
`port/assets/ragdolls/ragdolls.txt` from outside the workspace, so the
copy cargo-mutants builds in has no such file, and its unmutated baseline
fails. The fix is to move the file under `engine/`. That also changes the
old port's tools and `nix/engine/rust.nix`, so it is left for the merge,
or for the next tools pass.

**physics.** 240 mutants survived at first, and 44 now. The new tests
cover the mover's step, crouch and jump, the ground's material cells, and
the world's contacts and ragdolls. Equivalents among the 44:
- the layer masks' `|` → `^` (the bits are disjoint);
- `from_raw`'s unit conversions and its `GROUND_UNSUPPORTED` arm, which
  Jolt does not report for a character on the ground;
- `World::drop`, whose leak no test can observe;
- the boundary comparisons in `cell_of`, `cells_along`, `slope_scale`,
  `clamp_length` and the mover's thresholds, where an equal value gives
  the same result either way;
- `closest_point`'s and the mesh's index arithmetic on the shapes the
  tests use.

The rest are float boundaries that a test could hit only with an exact
value.

**script.** 164 of 414 mutants survived at first, and 10 now. The new
tests are in `tests/runtime.rs`, `tests/console.rs` and `tests/values.rs`.
They cover:
- each type's default, and the casts between numbers, booleans, objects,
  their names, lists and encounters;
- arithmetic folding and comparisons;
- `begin_random`'s order from the runtime's xorshift;
- sleeps of other scripts, and `sleep_until` with constant and changing
  conditions;
- runaway scripts and globals;
- each special form given the wrong number of arguments;
- the console line's length limit and its errors, and how a name is looked
  up for each parameter type.

All 10 survivors are equivalent:
- `run`'s `>` → `>=` and `==`: every evaluation takes an odd number of
  steps, so the limit is never met exactly;
- `evaluate`'s `<` → `<=`, and `resolve`'s two `<` → `<=` (at 1017 and
  1060): the index one past the end gives the default, or nothing;
- `step`'s `frame.step == 0` guard: always true where it is reached;
- `ObjectToUnit`'s `-1` (860): the index is replaced before it is read;
- `slot`'s `<` → `<=`: the slot one past the globals is read through `get`
  and gives the default;
- `count`'s `<` → `<=`: this differs only when the count is both the least
  and above the most;
- `cast`'s `Void` arm: without it, the cast falls to the default, which is
  `Void`.

One finding is for the fidelity track. A startup or continuous script
whose root expression is a `sleep` (not inside a `begin`) ends at once,
since its stack is empty after the sleep returns. Whether
`hs_thread_main` does the same has to be checked against `source/`. It
goes on the fidelity bugs list.

| check | result |
|---|---|
| physics, first run → `--iterate` | 240 → 44 survived (5 of 49 caught on the rerun) |
| script, first run → `--iterate` | 164 → 10 survived (154 caught, 1 timeout), all 10 equivalent |
| sim | not run: `include_str!` outside the workspace |
| `cargo clippy --workspace --all-targets -D warnings` | clean |
| `tools/engine_build.py --test --suite rust` | 1103 passed, 3 skipped |

## blake3 and lz4 for the packs (2026-10-25)

A level's pack is the rkyv archive of its imported level, keyed by the
map's fingerprint. Two things change:
- The fingerprint is now the first eight bytes of blake3, in place of
  SHA-256's. It is used both for the map, as the cache key, and for the
  archive, as its check.
- The archive is compressed with lz4_flex, in its safe encoder and
  decoder. The header grows from 0x28 to 0x30 bytes to hold the
  unpacked length, and a pack whose archive does not decompress to that
  length is refused before rkyv's bytecheck runs.

`FORMAT_VERSION` goes from 50 to 51, so every existing pack is imported
again once. The bake key includes the version, so the bakes are redone
too.

`tests/load_timing.rs` (in import) times each step over the user's maps,
in the `test-opt` profile. The table has the numbers measured before the
change, then after it. They were measured again on a quiet machine before
this commit:

| map | | fingerprint | pack | check and unpack | load from the pack |
|---|---|---|---|---|---|
| bloodgulch (22.6 MB) | before | 36.2 ms | 49.4 MB | 280.8 ms | 361.3 ms |
| | after | 12.7 ms | 28.3 MB | 40.5 ms | 59.2 ms |
| a10 (173.9 MB) | before | 577 ms | 333.4 MB | 1220.7 ms | 1687.8 ms |
| | after | 97.0 ms | 225.3 MB | 267.1 ms | 397.8 ms |

Loading from a pack is now 6.1 times as fast on bloodgulch and 4.2 times
on a10, and the packs are 43% and 32% smaller.

The tests:
- the fingerprint is blake3's first eight bytes;
- a pack is its archive compressed;
- an archive that decompresses to the wrong length is refused;
- a changed map is imported again and leaves the other maps' packs alone;
- the pack fuzz smoke test now mends the fingerprint behind the new
  header.

| check | result |
|---|---|
| `content::pack` tests | pass |
| `import::level_cache` `a_changed_map_leaves_the_other_maps_packs_alone` | pass |
| `import::fuzz_smoke` `mutated_packs_are_read_or_refused` | pass |
| `load_timing`, bloodgulch and a10 | a pack loads as the level imported |
| `tools/engine_build.py --test --suite rust` | 1103 passed, 3 skipped |

## bytemuck adopted, memmap2 declined (2026-10-25)

The structures the snapshot hands to Metal now derive `Pod` and
`Zeroable`. These are the vectors, transforms, camera, nodes, objects,
lights, decals, effects, particles, debug lines and labels, and the
lights' `ClusterView`. Pod's derive refuses a type with padding, so a
layout with a hole in it no longer compiles. Before, only the hand-kept
offsets in `the_layout_is_fixed` caught one.

The snapshot's slot was made by an `unsafe` `Box::new_zeroed().assume_init()`.
It is now made by `bytemuck::zeroed_box`, whose `Zeroable` derive checks
the claim that the SAFETY comment made by hand. That removes one of
snapshot.rs's 17 `unsafe` uses. The rest are the slot handoff's
`UnsafeCell` and the C interface's pointers.

memmap2 is declined. A mapping is `unsafe` to make, because another
process may change the file under it, and the workspace denies
`unsafe_code` outside the FFI. The measurements also leave it nothing to
win. Reading the whole map with `fs::read` takes 2.9 ms of bloodgulch's
59 ms load from its pack, and 19.8 ms of a10's 398 ms. Opening an Xbox
cache inflates all of it whether it is mapped or not.

| check | result |
|---|---|
| `the_buffer_structures_are_plain_data_with_no_padding` | 14 types are `Pod`; `Vec3`'s bytes are its fields, little-endian |
| `a_new_slot_is_zeroed_but_for_its_version_and_pointers` | pass |
| `the_layout_is_fixed` | unchanged, passes |
| map read share of a pack load | bloodgulch 2.9 of 59.2 ms, a10 19.8 of 397.8 ms |
| `tools/engine_build.py --test --suite rust` | 1103 passed, 3 skipped |

## binrw and zerocopy declined (2026-10-25)

The plan was to rewrite the tag and cache readers declaratively, with
binrw for the layouts that branch and zerocopy for the plain ones. We
measured first, and the rewrite has nothing to speed up.

Opening a cache takes 137.1 ms for bloodgulch and 974.4 ms for a10
(3357 tags). Nearly all of that is zlib inflating the Xbox cache. The
part a declarative reader would replace is the header and the tag index,
which is about 107 KB on a10, a few milliseconds at most. Play also
never opens a cache: a level loads from its pack (59 ms and 398 ms), and
the pack's checks and unpacking are blake3 and lz4, not the readers.

Safety does not argue for the change either. The hand-written readers
already go through the bounds-checked `Reader`, and the fuzz targets from
Phase 2 run them on every test pass. binrw would add a proc-macro crate
to every build of import without removing a single check.

If inflating is ever worth speeding up, the lever is the inflater
(zlib-rs, or flate2's other backends), not the layouts. That belongs to
a later fast-loop track, measured the same way.

| check | result |
|---|---|
| `Cache::open`, bloodgulch | 137.1 ms |
| `Cache::open`, a10 (3357 tags) | 974.4 ms, mostly inflate |
| a10's tag index | about 107 KB |
| a level loaded from its pack | bloodgulch 59.2 ms, a10 397.8 ms; no cache opened |
| the readers' fuzz targets | run on every test pass (Phase 2) |

## tracing and benchmarks (2026-10-25)

The log book now sits on `tracing` in place of `log`. It is one layer of a
`tracing-subscriber` registry, with its own filter (`render=debug,warn`, as
before) and its own `Level`, so the control socket and the shell read it as
they did. The dependencies that still log through `log` (bevy_ecs, goblin)
come in through tracing-log. `OPENCE_LOG_JSON=<file>` adds a JSON log, one
object a line, each event with the spans it was in. The `tracy` feature of
the dev crate adds Tracy's layer (on demand, localhost only). A
`profile_scope!` named by a literal is now a span as well as a profiler
scope, so the frame's `advance` > `ticks` > `tick` nesting shows in the JSON
log and in Tracy. A scope named at run time (the ones Swift sends) stays the
profiler's only, as a span's name must be fixed. With no JSON log and no
Tracy, the book's filter turns spans off at their callsites, so they cost
nothing.

divan benchmarks, `cargo bench -p <crate>` (`cargo test --benches` runs each
once). Medians on this machine:

| benchmark | median |
| --- | --- |
| collision: a ray across a room | 162 ns |
| collision: a character walks into a wall, a tick | 2.08 µs |
| bake: the furnace's hierarchy built | 1.71 µs |
| bake: a texel traced, 16 / 64 paths | 19.9 / 75.7 µs |
| animation: a biped's base pose blended (bloodgulch) | 448 ns |
| AI: a tick of a10 with every encounter placed | 691 µs |

The first blend measured 62 ns, too fast for a biped, because animation 0
moved none of its nodes. The benchmark now picks a base animation of all
the graph's nodes and checks that the pose has them. At 691 µs, a10's tick
is 4% of the 16.7 ms frame.

| check | result |
| --- | --- |
| spans nest, and the JSON log parses with them (`dev/tests/tracing.rs`) | pass |
| a scope named at run time is no span | pass |
| events reach the log book through its layer | pass |
| the benchmarks run (`cargo test --bench`, each crate) | pass, 4 of 4 |
| clippy `-D warnings`, the whole workspace, with and without `tracy` | clean |
| the whole suite | 1106 passed, 3 skipped |

## Allocation in the frame

The engine's allocator is now mimalloc, wrapped in stats_alloc's counter,
and `stat allocations` shows the last whole frame's allocations and bytes
(from one `advance` to the next, on every thread). On devgulch's AI station
a frame made 823 allocations. A sampler kept only while measuring (not
committed) named where they came from. Two causes made most of them:

- The lights gathered each carried object's rest pose again every frame.
  They now keep it with the object (`Carried.rest`), which took the frame
  to 708.
- bevy's `World::query` builds a new query state each call, 19 to 35
  allocations for its access sets and archetype lists, and the simulation
  called it at every tick's query site (the lit and drawn objects, the
  function values, damage's targets, pick-up and swap offers, the seats'
  new vehicles, the platforms). The new `sim::cached::Cached` keeps a
  state from one tick to the next and builds it again only for another
  world. A kept state takes in the archetypes made since and iterates in
  the same order, so the reference traces are unchanged. `Simulation::object`
  now lends the object instead of copying it (a copy cloned its tag's path).
  Pick-up checks the distance before it looks at the tag.

That took the frame to 151 allocations (112 kB), 185 at most, and the
budget test holds it to 200. bumpalo, smallvec and arrayvec were declined:
what is left is spread over a dozen sites of under 20 each, none of them a
buffer a frame arena would hold.

| check | result |
| --- | --- |
| a kept query sees what is spawned after it, in the same order (`sim::cached`) | pass |
| a kept query is built again for another world | pass |
| an empty world's frames are counted (`ffi/tests/allocations.rs`) | pass |
| devgulch AI station, allocations a frame | 823 → 708 → 151 (budget 200) |
| the sim reference traces | pass, unchanged |
| clippy `-D warnings`, the whole workspace | clean |

## The audio thread

The output's pull no longer waits for the tick. Before, `AudioOutput::pull`
locked the player, and the tick holds that lock while it moves the
listener, sets the environment and starts impulses (an impulse decodes its
permutation the first time it is played), so a pull that came during the
tick waited for it. Now the pull only tries the lock. When the player is
free, it mixes up to 512 frames (about 11 ms) beyond what was asked for and
keeps them. When the tick holds the player, it plays on from what it
kept, and it counts any frames it could not cover (`missed`) rather than
waiting. A level unloaded clears what was kept.

rtrb was declined. A ring buffer joins a producer thread to a consumer
thread, but here the mixing and the playing happen on the same thread, the
output's. What it needed was a pull that does not block and a store of
frames mixed ahead, and a `try_lock` with a `VecDeque` is both.

A measurement (a scratch test, not committed) pulled 512 frames on a
real-time cadence while Blood Gulch played fire, reload and walk sounds. The
longest pull was 4.71 ms in a debug build and 6.35 ms in release with the
blocking pull, and 3.46 ms and 6.30 ms with the new one, with 0 frames
missed in both builds. So the lock was not contended enough to measure on
this machine. The longest pull is the mixing itself. The change is a
guarantee by construction: a tick that runs long can no longer stall the
output.

The sounds are now resampled to the output's 48 kHz once, as they are
decoded (`audio::resample`, rubato's FFT resampler). The mixer steps
through a voice's samples by linear interpolation. At 22.05 kHz, the
rate of 895 of Blood Gulch's 916 permutations, that leaves images above
11 kHz and smears the top of the band. At 48 kHz the mixer's step is the
pitch alone, which is close to exact. Against an analytic sine of amplitude
16000, the band-limited result is within about 1 LSB, and linear
interpolation is much further off:

| source | tone | rubato, worst error | linear, worst error |
| --- | --- | --- | --- |
| 22050 Hz | 1 kHz | 1.1 | 161.8 |
| 22050 Hz | 7 kHz | 1.0 | 7313.6 |
| 44100 Hz | 5 kHz | 1.0 | 1001.5 |
| 44100 Hz | 15 kHz | 1.0 | 8271.6 |

On Blood Gulch, resampling all 916 permutations (827 s of sound) takes 336
ms in release: 0.41 ms for each second of sound, and 1.75 ms for the longest
permutation. Since a permutation is resampled the first time it is played,
under the tick's lock, it costs a few percent of a tick at most, and the
mix-ahead store covers the output meanwhile. The decoded cache holds only
what has been played. That cache is 2.07 times as large at 48 kHz (all of
Blood Gulch's sounds would take 90.1 MB instead of 43.5 MB). Samples already
at 48 kHz, and any the resampler refuses, are left for the mixer to
interpolate as before.

| check | result |
| --- | --- |
| a pull while the tick holds the player does not wait (under 20 ms, 256 frames missed) | pass |
| what is mixed ahead carries the output through a held lock (no silence, 0 missed) | pass |
| what was mixed ahead is dropped with the level | pass |
| a sine at 22.05 and 44.1 kHz resampled is within 4 LSB of the sine at 48 kHz, and closer than linear | pass, 4 tones |
| stereo keeps its channels apart | pass |
| samples at 48 kHz or at no rate are left alone | pass |
| every stock sound decodes | pass |
| clippy `-D warnings`, the whole workspace | clean |
| the whole suite | 1116 passed, 3 skipped |

## Bake and import in parallel

The bake traces its light on the GPU. Its CPU side plans the bake (scene,
BVH, atlas, seams) and then finishes it: it holds down the fireflies,
stitches the seams, fits the exposure, and encodes the pages. Timed stage by
stage in release, the finish was the largest part, and within it the
exposure fit. That fit bisects the exposure over 24 steps, and each step
shows up to 65 536 squares of light through the level shader's curve. The
measures that compare a bake with the stock pages are dev checks and are
not on the bake's path.

Two passes now run on rayon's threads: the firefly hold, chart by chart,
and the exposure fit, square by square. Neither may change a bit of the
output, because a bake is cached by its key and must be the bake any
machine would make. So each result is collected in order and summed in the
old order. For the hold, the first texel that shows each point is settled
first, in one serial pass, and then the charts are held at once. A test
finishes Blood Gulch's bake on one thread, on two and on eight, with
specks and lost texels in the light, and compares every bit. A digest of
the finished bake on Blood Gulch and a10 was taken before the change; the
parallel bake gives the same digest.

| BSP | finish before | finish after | firefly hold before | after |
| --- | --- | --- | --- | --- |
| bloodgulch 0 | 83.4 ms | 41.7 ms | 26.2 ms | 14.7 ms |
| a10 0 | 211.1 ms | 97.9 ms | 28.5 ms | 11.3 ms |
| a10 1 | 310.7 ms | 122.7 ms | 51.9 ms | 20.7 ms |

These timings are on 10 cores (8 performance, 2 efficiency). The firefly
hold gains less than the fit, because one large chart dominates each BSP.

The edge measures (67 to 291 ms each) would split the same way, page by
page, but `bake/src/edge.rs` carries another session's uncommitted work on
size bands. They are left serial until that work lands.

Loading a level from its pack hashes the whole pack (225 MB on a10) to
check it, and the map is hashed again for its fingerprint. BLAKE3 hashes
its chunks as a tree, so `update_rayon` hashes them on all the cores and
gives the same digest. A test checks the fingerprint against the one-thread
hash on 8 MiB, on 1000 bytes, and on nothing. No cached key changes.

| map | fingerprint before | after | load from pack before | after |
| --- | --- | --- | --- | --- |
| bloodgulch | 13.8 ms | 2.5 ms | 57.2 ms | 36.8 ms |
| a10 | 96.6 ms | 16.2 ms | 388.2 ms | 205.4 ms |

The import itself is left serial. Its largest part on a10 is opening the
Xbox cache (930 of 2000 ms), which inflates one zlib stream and cannot be
split. The import also runs once per map before it is cached.

meshopt was declined. A BSP has 5 500 to 55 000 triangles. Reordering them
for the vertex cache would change the bake's key and the atlas's order, and
Apple's tile-based GPUs gain little from that order. It would also bring a
C++ build into the Nix closure for no measured gain.

| check | result |
| --- | --- |
| a bake finished on one thread is the bake on two and on eight, bit for bit | pass |
| the parallel bake's digest is the serial bake's (bloodgulch, a10) | pass, identical |
| the fingerprint on many threads is the one-thread BLAKE3 hash | pass, 3 sizes |
| a pack loads as the level imported | pass |
| clippy `-D warnings`, the whole workspace | clean |
| the whole suite | 1120 passed, 3 skipped |

## The release build

A new benchmark, `ffi/benches/frame.rs`, measures what the release build is
for. It starts devgulch with a station running, plays its first second, and
then times one `advance` of 1/60 s, through the calls the app makes. It
times four stations: AI, physics, the warthog and the pistol. Each profile
was built clean in its own target folder, and the frames were run back to
back on the same machine, more than once, interleaved.

| profile | clean build of the library | rebuild after an edit to sim | `.a` size |
| --- | --- | --- | --- |
| default (no LTO, 16 units) | 64 s | 15 s | 158.6 MB |
| fat LTO, 1 codegen unit | 94 s | 49 s | 208.3 MB |
| thin LTO, 16 codegen units | 62 s | 16 s | 299.7 MB |

Medians of a frame over several runs, in µs (the fastest sample in brackets):

| station | default | fat LTO | thin LTO | PGO |
| --- | --- | --- | --- | --- |
| AI | 132–141 (124–128) | 140–141 (130–134) | 143–146 (136–139) | 148–154 (139–147) |
| physics | 131–141 (115–119) | 140–144 (118–123) | 137–149 (122–125) | 145–150 (124–129) |
| warthog | 273–290 (224–234) | 282–295 (231–242) | 290–292 (233–235) | 293–301 (240–248) |
| pistol | 161–179 (124–128) | 174–186 (128–129) | 175–187 (134–141) | 177–181 (131–140) |

Neither kind of LTO made a frame faster. Fat LTO did speed up some of the
micro-benchmarks: animation blending went from 445 to 364 ns, the ray from
161 to 156 ns, and the mission tick from 569 to 551 µs. But devgulch's frames
were 3 to 8 per cent slower, the library grew by a third, and an edit's
relink took three times as long. Thin LTO cost no build time, but its frames
were slower too, and its library was almost twice the size. A frame here is
bound by the ECS's queries and the world's memory, not by calls across
crates. So the release profile stays as it was: `panic = "abort"`, with line
tables for the profiler, and no LTO.

PGO was recorded with cargo-pgo 0.3.0. The instrumented frame bench made
one 4.2 MiB profile, and a second build was optimised with it. The
optimised frames were 4 to 13 per cent slower than the default, in two
interleaved runs. The profile left 2 143 functions without data, because a
frame of four stations does not reach the whole library. A profile recorded
on a whole game might do better, but not one that a build can record on
devgulch without the user's play. On the PGO build the reference trace
tests (animation, first person, items, unit state) give the same traces,
so it would be safe. It is declined because it is slower.

cargo-show-asm was not added to the shell. The shell's tool list is in
`nix/repo/toolchain.nix`, which is part of another session's uncommitted
Nix restructure. It joins the shell with that work; until then it is
`nix run nixpkgs#cargo-show-asm`.

| check | result |
| --- | --- |
| the release profile aborts on a panic and is not linked whole (`tools/test_cargo_profiles.py`) | pass |
| the frame bench runs on devgulch's four stations and skips without assets | pass |
| fat LTO, thin LTO and PGO frames against the default | all slower; declined |
| the PGO build's reference traces (animation, first person, item, unit state) | identical |
| clippy `-D warnings`, the whole workspace | clean |
| the whole suite | 1120 passed, 3 skipped |

## The track's report (2026-10-25, at eb52ae34)

Measured again with `tools/engine_timings.py --profile test-opt`, on the
same machine and maps as the starting point.

| step | start (5fd62cd) | now | |
| --- | --- | --- | --- |
| debug build, cold | 47.2 s | 79.4 s | dependencies at opt-level 2, more crates |
| debug build after touching the sim | 2.4 s | 2.3 s | |
| test build, after the debug build | 38.8 s (`test`) | 146.4 s (`test-opt`) | the price of the optimised tests, once |
| release library, cold | 52.3 s | 64.9 s | |
| the app, Rust half incremental | 72.2 s | 99.5 s | |
| **the test suites, one after another** | **3349.6 s (85 binaries)** | **270.7 s (101 binaries)** | 12.4 times faster |
| **the whole suite under nextest** | — | **64.3 s (1120 tests)** | the loop's real cost |
| devgulch, release, frame p50 / p99 | 9.21 / 9.74 ms | 9.28 / 9.64 ms | unchanged (GPU bound) |
| devgulch, GPU p99 | 12.76 ms | 12.86 ms | unchanged |
| devgulch, start and load from a cached pack | 20.9 s | 15.3 s | blake3, lz4, parallel hashing |

An edit's check went from 56 minutes of suites to about a minute: the
optimised test profile (11 times), the ready caches, and nextest running the
tests across the cores. The cold builds are slower. They run once, and an
edit's rebuild is unchanged. The frame is drawn on the GPU, and the CPU work
the track removed (672 allocations a frame, the query states) does not
show in its p50.

Adopted, each with its measurement in the section above:

| crate or tool | for | measured |
| --- | --- | --- |
| `test-opt` profile | the game-data suites | 3184 → 291 s |
| ready caches (`opence_import::testing`) | maps held once per test binary | placements 189 → 90 s |
| dependencies at opt-level 2 in debug | the debug suites | 10–25% |
| cargo-nextest | the test step | 291 → 74 s |
| cargo-deny | licences, bans, sources | clean |
| insta, pretty_assertions | snapshot and readable diffs | — |
| proptest | readers, packs, script, physics | 12 properties |
| cargo-llvm-cov floors | coverage per crate | 72–97.5% held |
| cargo-fuzz | six readers | 0 crashes in 4.4 M runs |
| golden frames | nine stations drawn twice | identical |
| cargo-mutants | physics, script | 240 → 44, 164 → 10 survivors |
| blake3, lz4 | the packs | a10 load 1688 → 398 ms |
| bytemuck | plain-data buffers | 14 types `Pod` |
| tracing, tracing-tracy, divan | logs, spans, benches | — |
| mimalloc and kept query states | allocation in the frame | 823 → 151 a frame |
| rubato | 48 kHz resampling at decode | below 4 LSB on a sine |
| rayon (with blake3's `rayon`) | the bake's finish, hashing | a10 finish 311 → 123 ms |

Declined, each with its reason or measurement: sccache (no gain on
incremental crates); memmap2 (the map read is 3 to 5 per cent of a load);
binrw and zerocopy (the bounds-checked readers already cover it); bumpalo,
smallvec and arrayvec (no allocation site left above 20 a frame); rtrb (one
audio thread); a parallel import (one zlib stream); meshopt (it would change
the bake's key, with little gain on tile GPUs); fat LTO, thin LTO and PGO
(slower frames).

Not done: Phase 5's Nix tasks (treefmt-nix and git-hooks.nix, nix-direnv
and nix-output-monitor, the crane checks) and cargo-show-asm in the shell.
They change `flake.nix` and `nix/`, which hold another session's
uncommitted move to std cells and crane. Those files already have
`nix fmt` (nixfmt) and checks for fmt, clippy, nextest and deny, and
`.envrc` already uses the flake. The tasks follow once that work is
committed.

What the next track should take up: the frame's CPU time, now that the
benches can show it (warthog's 280 µs is the largest station), and a
parallel edge measure in the bake once `bake/src/edge.rs`'s band work lands.
