# MCC refactor milestones and validation

## Milestones and current position

Updated 7 November 2026. Milestones retain the existing stage numbers, so
"Stage 6" and "Milestone 6" refer to the same work. Stages 1–6 are complete;
Stage 7 is next. Stages 7–9 remain planned and have not started.
The stage evidence below records the changes, actual validation and limits.

| Milestone | Scope and outcome | Status |
| --- | --- | --- |
| 1 | Establish executable regression coverage, CI and Workshop runtime baselines. | Complete |
| 2 | Reproduce the shared boat defect and establish the crossplay constraint. | Investigation complete; shared fix deferred |
| 3 | Remove redundant MCC resource ordering and bitmap ownership lookup work. | Complete |
| 4 | Replace repeated per-texture stream scans with a local per-tag cursor. | Complete |
| 5 | Correct HUD ownership, remove bitmap-name dependence and trim duplicate normalization. | Complete |
| 6 | Consolidate duplicate checksum work with exact compatibility and linkage proved. | Complete |
| 7 | Audit scripts, campaign/checkpoint state and lifecycle ownership; implement justified corrections and cleanup. | Planned; investigation first |
| 8 | Investigate remaining compatibility exceptions and the Quarantine script gap. | Planned; evidence required |
| 9 | Review the combined branch and extend runtime, lifecycle and performance validation. | Planned; final verification |

The latest completed implementation is `71dcfe6dd`; its four debug platform
builds and all three MCC CI jobs passed. Stage 6 recorded 2,059 local test passes,
nineteen skips and twelve existing boat expected failures, four additional
real-asset checks, exact before/after save-byte comparisons, paired Ruby/Mercury
save/revert/core checks and Xbox/CE/MCC transitions.
The detailed sections distinguish these bounded checks from full compatibility.
Six completed milestones are not an estimate of percentage effort remaining,
and do not establish that every MCC module has received the same depth of audit.

All future changes remain evidence-backed, with general engine defects fixed
in the responsible implementation and MCC format defects fixed systemically
inside MCC-owned modules. Preserve Xbox/CE behavior, map-family ownership and
OpenCE crossplay. Each milestone includes design review, meaningful regression
coverage, the applicable platform/network checks, final review and cleanup.
Later integration checks supplement these checks; they do not postpone them.

### Milestone 6: checksum consolidation

Consolidated repeated CRC32 calculation and table construction in
[`mcc_checkpoint.c`](../port/linux/game/mcc_checkpoint.c),
[`mcc_script_threads.inl`](../port/linux/game/mcc_script_threads.inl),
[`mcc_resources.c`](../port/linux/game/mcc_resources.c) and
[`mcc_resource_check.inl`](../port/linux/game/mcc_resource_check.inl).
The existing `crc_checksum_buffer` in [`crc.c`](../source/memory/crc.c) was
evaluated; the already-linked port-prefixed zlib service was selected after
checking initialization, integer widths and standalone-tool linkage.

Old/new and independent-oracle checks establish unchanged seeds, complements,
empty and chunked results, resource ordering/framing, checkpoint bytes and
failure behavior. Required platforms and consumers pass. The saved per-check
storage and isolated checksum timing are measured, with their practical limits
recorded in [Stage 6 evidence](#stage-6-checksum-consolidation).

### Milestone 7: scripts and state lifecycle

Review script resolution/execution, campaign and checkpoint restoration,
script-thread and named-object ownership, and failed-load/unload cleanup.
The relevant routes include `mcc_scripts`, `mcc_script_runtime.inl`,
`mcc_script_threads.inl`, `mcc_checkpoint`, `mcc_campaign` and
`mcc_object_names`. These are investigation areas, not established new defects
or promised deletions. Keep host/client authority and saved-state contracts
explicit when evaluating a change.
Stage 6 also retained a baseline assertion when a checkpoint captured a
sleeping temporary console expression. Investigate that expression's syntax
lifetime separately from CRC validity; the recorded standalone-save controls
pass. This finding is queued here, not an implemented Stage 7 correction.

Complete when reviewed paths and ownership rules are recorded, demonstrated
defects/redundancy are addressed within scope, and affected save/revert,
restart, failure and unload behavior has been checked. Retain supported code
when no practical benefit from changing it is established. Any unresolved
semantic or compatibility question must have an explicit disposition and the
evidence needed to revisit it.

### Milestone 8: compatibility exceptions and known gaps

Review the 21 retained HUD structural profiles and other content selectors
encountered in the affected MCC paths. The immediate HUD question is whether
validated authored information or usage context can reliably distinguish
native and MCC safe margins. A replacement needs intended matches, near
misses and actual stock/custom HUD checks, including Ruby and Descent.
Record retained exceptions' rationale, activation/cleanup, regression evidence
and removal conditions; do not remove a profile merely to shorten the table.

Separately diagnose Quarantine's `life_lost_0` refusal at `ai_deselect`.
Establish the operation's intended semantics and implementation boundary
before enabling it. A systemic MCC fix must benefit any map using those
semantics; accepting an unproven no-op or a map-specific bypass is insufficient.
Complete this milestone with validated fixes where justified and explicit
retention/deferral decisions where evidence or supported scope is insufficient.
An investigation completed with a deferral does not mean the defect is fixed.

### Milestone 9: combined review and extended validation

Review the resulting implementation across all stages, including code, tests,
integration and reader documentation. Reconcile the MCC boundary inventory in
`CODINGSTANDARDS.md` with the work actually reviewed: identify changed,
reviewed-and-retained, and unaudited/deferred areas. Further code changes need
their own demonstrated benefit and must receive affected regression checks.

Extend representative Workshop checks with save/revert/restart, two-instance
co-op, repeated load/unload and failed-load recovery across map families.
Use Ruby's Rebalanced and other maps selected for the affected behavior, with
isolated saves and at most two hidden game instances. Compare loading cost
and memory retention under matched conditions; distinguish intended bounded
storage costs from leaks and do not infer speed gains from fewer lines.

Complete when the final combined state has passed the applicable platform,
network and runtime checks, evidence and remaining limits are recorded, and
temporary outputs are cleaned. Full campaigns, live split-screen, Android
devices and browser-map gameplay remain unverified until specifically tested;
build success alone does not close those limits. This milestone prepares the
branch for delivery review; PR descriptions and merging remain separate.

### Explicitly deferred: unconditional shared boat fix

The defect is reproduced, but the user chose to preserve OpenCE crossplay.
Keep the existing MCC correction and twelve strict legacy expected failures.
Revisit the shared fix only when compatible upstream support exists or the
user explicitly changes that compatibility requirement. This is a recorded
deferred defect, not unfinished implementation within completed Stage 2.

## Stage 1: validation baseline

Stage 1 of `mcc-code-refactor` establishes repeatable checks before changing
engine or MCC compatibility behaviour. It changes test tooling, CI and this
documentation only. Xbox, Custom Edition and MCC implementations, simulation,
network version and map-family boundaries are unchanged.

## Revision and build

The baseline is main commit
`db1697c9b0c46289596f22213a5bef8ee327be24`, checked on 7 November 2026.
The isolated worktree is `build/mcc-code-refactor` under the original checkout;
all paths below are relative to that worktree. The original working tree was
left untouched. Do not reuse another checkout's executable as this baseline.

The Windows build used clang 23.1.4, MSVC 14.44.35207 headers/libraries, Windows
SDK 10.0.26100.0, SDL 3.4.16 and these options:

```powershell
python configure.py --portable --release --pgo=off --lto=off
ninja windows
```

The build completed all 1,007 steps. Existing Expat import warnings remain in
the log. `build/windows/halo.exe` SHA-256 is
`f1ec0dca876aa3cf746a4095ac9115cc026902f4679d1c3941deb20928c85d56`;
the adjacent SDL DLL is
`47d508b4232cf9462096bdb6f613a330d9a94a7eb09c305fad073430c6531920`.
These identify a baseline, not a reproducible-binary guarantee.

## What the CI change makes executable

[mcc.yml](../.github/workflows/mcc.yml) now explicitly runs the previously
unlisted avoidance, cameras, Chicago shader, damage-tag, environment-shader,
HUD-layout, movement, particle and seat tests on Linux and Windows. The
Windows job also runs its memory reservation and startup-order checks.
[test_mcc_ci.py](../tools/test_mcc_ci.py) checks that MCC test modules have
an executable CI route, including modules discovered through the harness
directory. It distinguishes a pytest module from a standalone Python runner.

[test_mcc_hud_meter_render.py](../tools/test_mcc_hud_meter_render.py) must run
directly: passing it to pytest executes no GPU checks. Its separate Linux
job builds the project's checksum-pinned SDL version, then uses Xvfb and Mesa
software GL. The job needs clang, SDL3, NumPy, Pillow and SciPy. The shared
[HUD GL builder](../tools/test_hud_ammo_thresholds.py) now includes the
production shader emitters and missing renderer state needed by the current
translator, and preserves 32-bit colour words on an LP64 Linux host.

Two existing harness problems were reproduced and corrected without changing
production code. The pause-menu harness's unused counters failed clang 23's
warnings-as-errors build; they now assert upload/deletion balance, restoration
and diagnostics. The Windows startup fixture assumed the loader would honour
a preferred DLL address despite ASLR policy. Its fake DLL now explicitly
claims an allocation in the MCC window. The test still checks early reservation,
deferred diagnostics and preservation of a conflicting allocation.

## Run installed Workshop maps in place

Use [mcc_native_smoke.py](../tools/mcc_native_smoke.py) on Windows. Supply a
disposable data folder containing a privately sourced Xbox `maps/ui.map`;
`maps/loading.tga` supplies the loading artwork when available. Keep installed
Workshop maps and resources in their Steam folders. No MCC map copy, junction
or alternate Python discovery implementation is needed.

```powershell
python tools/mcc_native_smoke.py `
  --executable build/windows/halo.exe `
  --data-root build/stage1/data `
  --output build/stage1/workshop-final `
  --workshop --steam-root C:/Steam --campaigns --seconds 60 `
  2915693904/a10 2962107814/a10 3021510694/mercury2 `
  3675162225/halo 3003419983/spasm_playground `
  3021510694/mercury_falling 3675162225/ring_mp
```

The engine resolves these virtual `mcc_maps/<item>/<map>` names through its
normal Workshop discovery. `--steam-root` requires `--workshop`; omit it to
use the engine's normal Steam discovery. `--campaigns` enables campaign mission
discovery. Local `mcc_maps` invocation remains supported without these flags.
The tool accepts an optional `mcc_maps/` prefix and either slash convention.

Each case gets a fresh save/log folder and records its effective `HALO_*`
settings, executable hash, elapsed time, exit status and cache cleanup result.
Inherited `HALO_*` overrides are cleared. The copied executable gets a fresh
configuration; rendering uses a hidden 640x480 window, with audio and internet
discovery disabled. `HALO_NULL_RENDERER` must be absent: its presence enables
the null renderer even when its value is `0`. Elapsed time includes loading
and the bounded wait; it is not a performance benchmark.

`run_case` temporarily writes and restores the disposable data root's
`init.txt` through atomic replacement. If the file existed, its original bytes
are retained as `init.original.txt` in the case output for recovery if restoration
fails. A launcher error stops the remaining cases. Do not run two instances
against that data root or console port.
The runner refuses an occupied console port and reports console initialization
or socket-access failures. Before sending a script it checks Windows' TCP
connection table for the exact connection and launched process ID. A passing
result requires the expected loaded-map
log, an exact unique console output line after 150 simulation ticks, a normal
exit and no recognized fatal diagnostic. An echoed command is insufficient.

Only generated regular `saves/z/cacheNNN.map` files inside the fresh case are
removed; linked directories/files are refused. Saves, configuration and useful
logs remain, while copied test executables/DLLs are removed. The caller must
archive inactive saves if necessary and remove its own disposable asset copies
when finished. Never delete original Workshop packages or player saves.

## Reproduce the asset-free and optional fixture checks

Use a Python environment with pytest, PyYAML and the prerequisites named by
the workflow. Initialise the x86 MSVC compiler environment on Windows. The
complete workflow commands are authoritative; a useful local MCC selection is:

```powershell
$mccTests = Get-ChildItem tools/test_mcc_*.py |
  Where-Object Name -ne test_mcc_hud_meter_render.py |
  ForEach-Object FullName
python -m pytest -q -ra $mccTests tools/test_windows_mcc_startup.py `
  tools/test_steam_workshop.py tools/test_cache_file_formats.py
python -m pytest -q tools/harness/tests/test_mcc_boat.py `
  tools/harness/tests/test_mcc_damage.py tools/harness/tests/test_mcc_mods.py `
  tools/harness/tests/test_mcc_script_threads.py
python tools/test_mcc_hud_meter_render.py
python tools/test_hud_ammo_thresholds.py
```

Set `MCC_TEST_MAP` to a real Workshop level to opt into the cache/geometry
fixture checks. Set `HALO_CUSTOM_EDITION_MAPS` to a private CE directory for
the CE map/resource checks. These variables do not install or copy assets.
CI remains asset-free. Windows containment tests require permission to create
their temporary junctions or symlinks; keep those tests intact when a sandbox
prevents fixture creation.

## Evidence and limits

Private logs and results are retained in `build/stage1`; map assets and test
outputs are not committed. The initial broad run recorded 1,646 passes,
18 skips, 13 pause-harness compilation errors and one sandbox-denied junction
fixture. The unchanged preview containment test then passed with junction
creation permitted. The pause harness subsequently passed all 13 cases,
including its missing-unbind negative control. The added CI selection passed
149 checks. Optional Ruby cache/geometry and available CE map/resource checks
passed four checks; OpenSauce and the recorded CE Blood Gulch fixture were
absent. The four MCC modules under `tools/harness/tests` passed 45 checks.

After the combined review, the full local MCC selection (excluding the smoke
runner, checked separately), Windows startup checks, the existing frontend
CI selection, Workshop/CE format tests and those four harness modules passed
1,813 checks with 18 skips. The skips comprise JDK absence, Linux-only
filesystem/sanitizer/fuzzer cases, and optional asset fixtures; four of the
asset checks were exercised separately as described above.
The reviewed smoke-runner regression suite passed 47 checks with one skipped
file-symlink case because Windows did not grant that privilege. Real directory
junction checks and the deterministic file-reparse guard both passed.

The hidden GPU harness passed 36 MCC meter renders; the shared stock HUD
builder passed 15,360 ammunition renders. These compare output pixels for
their specified conditions, not full-map screenshots. The Windows machine
used NVIDIA OpenGL 4.5 on an RTX 5070 Ti. The new Linux Xvfb/Mesa job was
reviewed but had not run at local Stage 1 completion; its subsequent remote
result is recorded under Stage 2 below.

The first sandboxed Workshop attempt loaded and rendered Ruby and Cursed,
but loopback connections failed with Windows error 10013. Neither run counted
as simulation success. The interrupted next case, failed results and cleanup
record are retained separately from the final runs.

The final run used the command above with loopback access permitted. All seven
cases passed the loaded-map, delayed simulation output, diagnostic and normal
exit checks; each ran for the configured 60-second interval:

| Package | Virtual level | Result |
| --- | --- | --- |
| Ruby | `2915693904/a10` | Passed |
| Cursed | `2962107814/a10` | Passed |
| Mercury | `3021510694/mercury2` | Passed |
| Tiny Halo | `3675162225/halo` | Passed |
| Spasm | `3003419983/spasm_playground` | Passed |
| Mercury | `3021510694/mercury_falling` | Passed |
| Tiny Halo | `3675162225/ring_mp` | Passed |

The Mercury logs confirm package-owned bitmaps/sounds; both Tiny Halo cases
selected the installed shared singleplayer bitmap resource. Original Workshop
map hashes were unchanged after the runs. `baseline.json` records map, binary
and resource identities; `workshop-final/results.json` and the individual case
logs retain the checks and settings. The post-review smoke regression run
again passed 47 checks with the same one privilege-related skip.

Cleanup removed 7,675,576,320 bytes of generated map-cache files across the
successful and interrupted attempts, the disposable Xbox UI copy and temporary
test fixtures. No generated map caches or copied test executables remain in
the run folders. Fifty inactive save/cache-support files (221,548,296 bytes)
were archived losslessly to `inactive-smoke-saves.zip` (2,160,738 bytes); archive
members were hash-verified before loose copies were removed, and original
paths/hashes are recorded in `inactive-smoke-saves.json`. The baseline build
and SDL/build inputs (about 229 MB), test environment (150 MB) and diagnostic
evidence including that archive (about 3 MB) remain for the next stage.

Stage 1 establishes test execution and a bounded baseline. It does not prove
campaign completion, full visual/audio fidelity, save/revert, multiplayer
agreement, map-family transitions, performance improvements or Android/web
runtime compatibility. No simulation, protocol, engine or platform build
generator changed, so Android/web rebuilds and transport interoperability runs
were not required for these tooling changes. Later engine changes still need
their affected platform, networking and cross-family checks under
[AGENTS.md](../AGENTS.md) and [CODINGSTANDARDS.md](../CODINGSTANDARDS.md).

## Stage 2: boat proof and preserved crossplay

The [boat investigation](mcc_maps.md#shared-engine-investigation-7-november-2026)
establishes a shared producer-initialization defect. The user chose to preserve
the pinned OpenCE 41 crossplay contract and defer an unconditional engine fix.
This stage therefore changes regression coverage and documentation, with no
production or protocol change. The existing MCC correction remains active.

The initial expanded test ran against unchanged production code and reported
eight failures with eighteen passes. Non-MCC NaN controls reached the real
force consumer; finite stale values produced incorrect friction. The finished
harness also exercises the actual radius-based dispatch and `physics_update_old`,
checking its resulting linear velocity. The newer path checks computed forces
before its separately stubbed integrator. Both use an axis-aligned fixture with
controlled transforms and world queries, not a real map or a whole game.

The boat suite passed 52 checks and recorded twelve strict expected failures.
The latter are the identified legacy force defect, not successful compatibility
checks. Only its dedicated force-failure exception is eligible for an expected
failure; compilation, active-control and scratch-boundary errors remain failures.
Mutations verify that distinction. Removing the expected-failure handling for
the two legacy water/NaN cases reproduced failures in both physics paths.

```powershell
python -m pytest -q -rx tools/harness/tests/test_mcc_boat.py
# Deliberately reproduces the two deferred failures:
python -m pytest -q --runxfail tools/harness/tests/test_mcc_boat.py -k legacy_nan_water
```

Eight additional cases remove the MCC guard only from an extracted temporary
test copy. They show that unconditional initialization corrects the measured
forces with either poison pattern and both physics paths. This is evidence for
the deferred change, not shipped behavior. Production still uses its original
MCC guard. No performance improvement or new runtime compatibility is claimed.

The first remote Stage 1 run exposed a missing `libxtst-dev` dependency while
configuring SDL for the new Linux GPU job, before any render check ran. Commit
`4acb59d66` adds that dependency; the job retains SDL's enabled X11 functionality.
Workflow syntax and its 65 coverage checks passed locally. Remote results and
the post-review checks are recorded below. Private investigation logs are
retained under `build/stage2`.

Self-review and an independent read-only review of the combined tests,
documentation and CI dependency repair found no remaining corrections.
The final local boat, neighboring MCC harness, protocol, camera-protocol and
CI-coverage selection passed 167 checks with the twelve documented expected
failures. Documentation checks verified 55 local links, the investigation
anchor, named functions and Python syntax, and confirmed that no production
file changed. No new game run was needed for these test/documentation changes;
Stage 1's unchanged binary and runtime evidence remain applicable within
their original limits.

The original remote [Build run at `b8d2f0dc2`](https://github.com/chestahh/GulchCE/actions/runs/42373756516)
passed Windows, Linux, Android and web debug builds. Android included its APK
and importer checks; web included the browser/native transport harness.
The [MCC run](https://github.com/chestahh/GulchCE/actions/runs/42373756545)
passed its Linux/Windows pytest jobs but failed SDL configuration in the new
GPU job as described above.

The subsequent [Build run at `24fc89cdd`](https://github.com/chestahh/GulchCE/actions/runs/42374909776)
passed all four platform jobs with the expanded boat coverage. These are debug
builds; this workflow builds releases only on main. Android compiled the guest
and host, built its APK and passed the importer checks. Web passed its build
and browser/native transport checks with Chromium and Firefox, including the
configured packet-loss cases. These transport fixtures do not exercise a real
map or establish crossplay gameplay compatibility.

Linux's combined regression invocation passed 2,478 checks with 27 skips,
twelve expected failures and seven passing subtests. Across its pytest
invocations the job recorded 2,794 passes, 52 skips and the same twelve expected
failures; its ordinary and full-LTO ELF light-storage checks also passed.
The expected failures are the explicitly deferred boat defect described above.
Skips are retained as skips, including checks requiring unavailable retail
assets; they are not evidence of gameplay coverage.

The corresponding [MCC run](https://github.com/chestahh/GulchCE/actions/runs/42374909804)
passed all three jobs: Linux readers/validators, Windows adapters and Linux
GPU rendering. The corrected GPU job built the pinned SDL version with X11
enabled and passed all 36 meter render checks under Xvfb/Mesa. The Linux
reader/validator job passed 1,615 checks with sixteen skips; its separate
ASan/UBSan invocation passed 38 checks.

These results validate code and test revision
`24fc89cdd46761f010d81a41f16ad5ba6170487e`. The following documentation-only
commit records the completed runs; it does not change that implementation.
No production source or protocol changed in either stage. The twelve deferred
legacy boat failures remain unresolved, and the Stage 1 runtime limits still
apply. No Android-device gameplay, browser-map gameplay, full campaign,
save/revert or new map-family transition result is claimed.

Stage 2 launched no game instances and created no map copies or saves.
Temporary harness binaries and pytest fixtures/cache were removed after
checking their resolved paths and rejecting links. About 1.9 MB of diagnostic
logs and supporting evidence remain under `build/stage2`; no generated game
caches or test executables remain there. The Stage 1 build, environment and
verified save archive remain available as described above.

## Stage 3: resource ordering and bitmap ownership lookup

This stage removes two demonstrated sources of unnecessary work within
MCC-owned code. It changes no format interpretation, map-family dispatch,
simulation, network version, content-identity framing or resource lifetime.

`mcc_resource_open` no longer sorts its private directory by byte offset.
Every consumer finds entries by name and checks the whole directory for
duplicates; none depends on offset order. The comparator and sort call were
removed, while all bounds, duplicate-name and grouped-slice checks remain.
The expanded resource suite passed 117 checks before and after the change.
Compiled old/new readers also returned identical payloads, CRC32 and SHA256
for four fixed-input comparisons covering bitmap/sound files and both directory
orders. Different on-disk directory orders still have distinct raw identities;
only the unused in-memory reordering was removed.

`mcc_bitmap_valid_texture` now returns the owned, validated texture to both
`mcc_bitmaps_valid` and `mcc_bitmaps_pixels`. A successful pixels request no
longer searches the table twice. Validation order, first-match behavior,
failure-output clearing, descriptor identity, handles, ranges, channel flags
and storage checks remain intact. No lookup cache or new persistent state was
added. The ownership fixture checks both public entry points, copied/foreign
and mutated descriptors, missing storage, unchanged descriptor bytes, verifier
short-circuiting, duplicate entries, pressure beyond the legacy cache budget
and disposal. The media/flags/bridge selection passed 59 checks.

In isolated extracted old/new routines, the same 39 successful bindings over
13 descriptors required 546 pointer comparisons before and 273 after; returned
allocations, sizes and sampled pixel bytes matched. This measures removed
traversals, not frame rate or map-loading speed. The separate per-conversion
stream-offset scan and content-specific HUD exceptions were not changed.

Private evidence is retained under `build/stage3`. The original Stage 1 Windows
executable was preserved at `build/stage3/baseline/halo.exe` before rebuilding
`build/windows/halo.exe`; its recorded SHA-256 remains unchanged. Runtime
checks use installed Workshop maps directly and isolated Xbox/CE fixture
copies, saves and logs. Final review, build and runtime results follow below.

Self-review and an independent read-only review covered all Stage 3 code,
callers, lifecycle paths, tests and documentation; no further correction was
needed. After that review, the broad local MCC, Workshop, CE-format, protocol
and neighboring MCC harness selection passed 1,909 checks with nineteen skips
and the twelve previously documented boat expected failures. Skips covered
the JDK missing from this shell's PATH, Linux-specific checks, file-symlink
privilege and optional map fixtures. Four optional checks were then run with
Ruby's Workshop `a10` and the available CE Death Island/resources; all passed.
The Windows release build succeeded with the original configure options;
the existing Expat import warnings remained. Its executable SHA-256 is
`7608089e8b1c003ba7ff15c3120392d07342af473f93a50b3471e0f60e330c3d`.
The direct GPU runners passed 36 MCC meter cases and 15,360 stock HUD ammunition
cases on the local NVIDIA renderer.

The baseline and updated executables each completed this sequence in one
hidden 640x480 process, with fresh saves and actual rendering:

1. Workshop `3021510694/mercury_falling`, selecting package-owned resources.
2. Xbox `levels/test/bloodgulch/bloodgulch`.
3. Custom Edition `custom_maps/deathisland`.
4. A disposable, header-valid MCC fixture with an invalid tag index; the
   loader refused it and returned to the menu.
5. Workshop `2915693904/a10`, restoring MCC operation after the failed load.
   Ruby's log reports no external resource selection in this run.

Each valid map required a fresh simulation-tick epoch, an exact console marker
after 150 simulation ticks, and a subsequent rendered capture. Both processes
exited normally with no fatal diagnostic. Before/after captures were visually
inspected: the MCC intros and Xbox/CE gameplay rendered their expected scenes
and textures. They are not frame-synchronized image-equivalence tests; moving
cinematics and unique console markers differ. The malformed fixture tests early
load recovery before resource allocation, not every possible allocation failure.
These runs establish the named transitions and bounded rendering/simulation,
not campaign completion or new multiplayer, save/revert or device compatibility.

An additional 45-second hidden Tiny Halo run (`3675162225/halo`) passed the
loaded-map, delayed simulation marker, diagnostic and normal-exit checks. Its
log explicitly selects `mcc_maps/_shared/singleplayer/bitmaps.map`, covering
the installed shared-resource path separately from Mercury's package resources
and Ruby's recovery run.

Input verification rehashed all six original Xbox/CE fixture files and their
copies, the three exercised Workshop maps, Mercury's bitmap/sound resources
and the installed shared singleplayer bitmap resource; all hashes matched.
The three game runs removed 2,302,672,896 bytes of generated map caches.
Temporary test binaries, fixture directories and asset copies were removed.
The 107 inactive save/support/capture files (98,548,658 bytes) were archived
losslessly to `build/stage3/inactive-runtime-evidence.zip` (7,455,474 bytes).
Every member was hash-verified before removing its loose copy; the adjacent
JSON manifest records original paths, sizes and hashes. The baseline executable
and SDL DLL (about 27 MB) remain for later refactor comparisons, alongside the
current Windows build, test environment and diagnostic evidence.

The [MCC workflow at `829ef117b`](https://github.com/chestahh/GulchCE/actions/runs/42377561461)
passed all three jobs. Its Linux reader/validator selection passed 1,652 checks
with sixteen skips, followed by 38 passing ASan/UBSan checks; the Windows job
passed 1,853 checks with seventeen skips across its invocations. The direct
Xvfb/Mesa job passed all 36 MCC meter renders.

In the corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42377561452),
all four debug platform builds passed. Branch builds do not run the main-only
release job; the local Windows release build is recorded above. Android built
its guest/host and APK and passed touch, gyro and importer checks. Web passed
its JS/browser and native transport assertions for Chromium and Firefox, including configured packet
loss. These remain asset-free transport checks, not multiplayer gameplay
qualification. Linux's combined invocation passed 2,478 checks with 27 skips,
twelve expected failures and seven passing subtests. Across its pytest
invocations it recorded 2,794 passes, 52 skips and the same twelve expected
failures; ordinary and full-LTO ELF light-storage checks also passed. The
expected failures remain the deliberately deferred Stage 2 boat defect.

Both workflows validated implementation revision
`829ef117b945152dcffd61d9bba9994efcec5b66`. The following documentation-only
commit records their completed results and cleanup. Final documentation review
checked the claims against logs and archive metadata; local-link, named-symbol,
Python-syntax and whitespace checks passed. Roughly 36 MB remains under
`build/stage3`, including the 27 MB baseline build and 7.5 MB verified archive.
No disposable map copies, game caches or temporary test binaries remain there.

## Stage 4: per-tag bitmap stream preparation

This stage simplifies MCC-owned conversion bookkeeping. It does not change
pixel decoding, format interpretation, shader selection, simulation, network
version or Xbox/Custom Edition implementations. The old private converter
scanned every texture entry to recover the next stream offset for each texture.
Preparation now passes a local cursor to `mcc_texture_prepare`, resets it when
the full runtime handle changes, and advances it only after success.

The ordering contract is established by the actual constructors: validated
original handles have unique tag indices, `mcc_bitmaps_prepare` appends each
original bitmap block contiguously, and `mcc_bitmap_specialize` appends each
clone block under a fresh handle. Reusing a clone appends nothing. Empty blocks
add no descriptors. The items are not reordered before conversion. A resource
handle is not a runtime handle: clones can share the former while owning
separate streams. No persistent index, extra allocation or new shared-engine
hook is needed. Existing per-allocation, per-tag and aggregate bounds, failure
publication, loading progress and disposal remain unchanged.

The expanded extracted-source fixture exercises the actual preparation,
tag/block and clone constructors, with controlled consumer selection. It covers
multiple descriptors and original tags, both clone creation orders, clone
reuse, empty groups, exact stream boundaries, aggregate storage, and failures
in allocation, internal reads, external-resource reads and packing. Failed
descriptors remain unpublished; disposal releases earlier successful textures.
The same contracts were first checked against unchanged Stage 3 production
code. Private evidence and baseline executables are under `build/stage4`.

All 45 converter cases passed before and after. The construction cases also
produced identical full descriptor/pixel traces, read counts and storage ends.
Two deliberately broken private variants failed the expected offset assertion:
using resource handles for boundaries, and omitting cursor advancement. The
focused converter/media/channel/bridge selection passed 72 checks. Self-review
and an independent combined review found no remaining production cleanup or
structural correction justified within this stage.

Private Windows release probes were built from Stage 3 revision `53fe6673b`
and the updated bitmap module, replacing only that translation unit. Their
999 shared response-file inputs, configure options and probe framing matched. After
successful conversion, the probe hashes every initialized pixel-allocation byte
and 21 explicitly framed semantic metadata fields for every texture, excluding
addresses and structure padding. It also records ordered handle groups, total
storage and the maximum per-tag stream end. The comparator verifies complete
records, identities, counters and digests before comparing them. Instrumented
timings are not used to claim a loading-speed improvement.

All seven paired Workshop runs matched every recorded texture, handle group
and stable summary field:

| Installed Workshop map | Textures | Clones | Converted bytes | Old table visits | New cursor advances |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ruby's Rebalanced `2915693904/a10` | 1,275 | 10 | 107,505,536 | 1,625,625 | 1,275 |
| Ruby's Rebalanced `2915693904/b40` | 1,181 | 11 | 115,751,552 | 1,394,761 | 1,181 |
| Ruby's Rebalanced `2915693904/d40` | 1,444 | 11 | 150,436,608 | 2,085,136 | 1,444 |
| Cursed `2962107814/a10` | 1,531 | 29 | 265,864,960 | 2,343,961 | 1,531 |
| Spasm `3003419983/spasm_playground` | 908 | 12 | 200,100,480 | 824,464 | 908 |
| Mercury `3021510694/mercury2` | 1,577 | 47 | 606,337,280 | 2,486,929 | 1,577 |
| Tiny Halo `3675162225/halo` | 846 | 9 | 138,963,328 | 715,716 | 846 |

Together these compare 8,762 textures, 129 clones and 1,584,959,744 converted
bytes. The removed scans made 11,476,592 table visits; the cursor advanced
8,762 times. All fourteen hidden processes used the real NVIDIA renderer,
required a fresh simulation-tick epoch and exact console output after 150
ticks, captured a subsequent rendered frame, and exited normally. The complete
native smoke failure-pattern scan passed for all log/console sinks. Before/after
captures were visually inspected and showed the same expected scenes and
textures; these are not synchronized image-equivalence tests. Ruby's runs are
opening cinematics, while Spasm reaches its initial gameplay view. Installed
Workshop maps were read in place, without package copies.

After combined review, the broad local MCC, Workshop, CE-format, protocol and
neighboring harness selection passed 1,920 checks with nineteen skips and the
same twelve deferred boat expected failures. The skips retain their earlier
meaning: unavailable JDK/symlink privileges, Linux-only checks and optional
assets. Four optional real-map checks then passed with Ruby's `a10` and the
isolated CE fixtures. Direct GPU checks passed 36 MCC meter cases and 15,360
stock ammunition cases. The Windows release build succeeded with the unchanged
`--portable --release --pgo=off --lto=off` configuration and existing Expat import
warnings. Its executable SHA-256 is
`2f22adeba256b26843cfb8375c55c26d95535ea47491d172b5aab85874edf52b`.

The uninstrumented baseline and updated release executables each passed the
same one-process sequence: Mercury Falling with package resources, Xbox Blood
Gulch, CE Death Island, the intentionally invalid MCC tag-index fixture, then
Ruby's `a10`. Every valid map required a fresh tick epoch, delayed console
marker and rendered capture; both processes exited normally. This confirms
early failed-load recovery and the named family transitions. The malformed
fixture does not exercise late allocation failure; the extracted-source
failure cases cover conversion publication/disposal separately.

Both uninstrumented executables also ran Ruby's `b40` long enough to leave
the opening cinematic and render the initial gameplay area, including its
HUD, weapon and environment. Each completed the console marker after 1,800
simulation ticks, saved a later frame and exited normally. Before/after
captures were visually reviewed. All eighteen game runs passed the complete
native smoke diagnostic scan, allowing only the deliberately requested
`refactor_invalid` refusal in the two transition runs. No protocol version or
simulation code changed; network version 41 and the deferred boat behavior
remain intact. These are bounded runtime checks, not full-campaign, new
multiplayer gameplay, save/revert, Android-device or browser-map qualification.

All six original Xbox/CE fixture files and their copies, the eight exercised
Workshop maps, Mercury's bitmap/sound resources and the installed shared
singleplayer bitmap resource were rehashed and unchanged. Each game run cleaned
its generated map caches: 13,816,037,376 bytes across eighteen sequential runs.
The 254 inactive save/support/capture files (516,681,213 bytes) were archived
losslessly to `build/stage4/inactive-runtime-evidence.zip` (23,450,825 bytes).
Every member was hash-verified before removing loose copies; the adjacent JSON
manifest records their original paths. Private probe executables, objects and
debug symbols, disposable asset copies and pytest fixtures were removed after
checking containment and rejecting links. The Stage 3 baseline executable/SDL
pair remains for comparison, alongside the current release build and useful
logs, reproducible proof sources and archive.

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42380303410)
passed all three jobs at implementation revision
`e670a18926acc4d69e574a6172f4d3e2ddd2bd62`. Linux readers/validators passed 1,663
checks with sixteen skips, followed by 38 passing ASan/UBSan preview checks.
Windows adapters passed 1,864 checks with seventeen skips across their
invocations. The Linux Xvfb/Mesa job passed all 36 MCC meter renders.

The corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42380303636)
passed all four debug platform builds; the main-only release job was skipped.
The local Windows release result is recorded above. Linux's combined invocation
passed 2,478 checks with 27 skips, twelve expected failures and seven passing
subtests. Across its pytest invocations, it recorded 2,794 passes, 52 skips and
the same expected failures and subtests; ordinary and full-LTO ELF light-storage
checks passed. Windows COFF light-storage and corruption-negative checks passed.
Android built its guest/host and APK and passed touch, gyro and MCC importer
checks. Web passed its JS/browser checks and native Chromium/Firefox transport
assertions, including configured packet loss. These remain asset-free transport
tests, not multiplayer gameplay qualification.

Final evidence review checked the combined Stage 4 claims against the retained
logs, manifests and archive. Local-link, named-symbol, Python-syntax and
whitespace checks passed. This documentation-only completion records CI for the
implementation revision above without changing its code or tests. About 60 MB
remains under `build/stage4`, primarily the 27 MB baseline build, 23.5 MB verified
archive and diagnostic logs. No disposable map copies, generated game caches
or temporary test/probe binaries remain there.

## Stage 5: HUD ownership and identity-independent normalization

Stage 5 starts at `a9aaa8b3ae70c7d0ac7490f0c5c181d45e663eae`. The preserved
Windows release baseline has the Stage 4 executable hash above. This stage
corrects an MCC compatibility-engine defect: supported interned HUD storage
was normalized in place for each consumer, so earlier writes could become
later consumers' input. Same-group root aliases could halve offsets, scales,
digit advances or margins twice. Shared item storage could also inherit the
wrong bitmap factor or frame marker. These are ownership errors, independent
of map names, Workshop IDs or checksums.

`mcc_hud_ownership.inl` plans from pristine data, identifies root aliases and
isolates mutable block consumers before normalization. Pointer-free overlay
and crosshair items receive snapshots even when no second HUD reference is
visible: their storage can be interned inside a different tag allocation.
This intentionally makes supported scalar interning independent of validator
traversal order. It does not preserve the old order-dependent rejection of
otherwise supported scalar storage. The actual-validator regression cases
retain rejection of conflicting pointer-bearing/root owners and partial
overlaps. For shared pointer-bearing prefixes, the largest original extent
remains reachable so a copy cannot hide a conflicting tail.

All snapshots finish before pointer publication. Temporary planning storage
is freed on success and failure; copies use the existing MCC runtime arena
and its normal teardown. The four-million-record budget is checked before
layout recognition; bitmap geometry walks retain their existing range checks.
No new shared-engine hook, persistent state, setting or network-version
change is involved. Xbox and Custom Edition loading and normalization remain
unchanged, as does the explicitly deferred shared boat fix.

The first whole-converter reproduction used production structures and source
with synthetic tag storage. Unchanged Stage 4 code failed 30 of 45 cases;
fifteen independent-storage controls passed. A separate fixture combines the
actual converter and actual MCC validator. Its baseline failed five cases:
scalar-first admission, scalar-last source corruption, repeated root conversion
and both shared-prefix orders. Eight controls passed. Replacing largest-owner
retention with shortest-owner retention in a private mutation made both
conflicting-tail tests fail, showing that these negative cases detect hidden
overlaps. No original map file was modified for these reproductions.

An inventory read ten installed Workshop maps in place: Ruby's Rebalanced
`2915693904/a10`, `b40` and `d40`; Mercury `3021510694/mercury2` and
`mercury_falling`; Cursed `2962107814/a10`; Tiny Halo `3675162225/halo`; Spasm
`3003419983/spasm_playground`; Descent `3042265379/firefight_descent_mcc`; and
Descent Quarantine `2963835242/firefight_descent_quarantine_mcc`. None contained
shared roots or placement blocks within the inspected HUD graph. The inventory
did not compare scalar addresses against every non-HUD allocation. These maps
are unchanged-behavior controls, not claimed reproductions of the
ownership bug. BioHalo was not found in the inspected `C:\Steam` library and
was not runtime-tested.

The inventory contains 487 component HUDs, including 92 unit HUDs: 71 recognized
and 21 unrecognized unit layouts. All eleven existing unit profiles were
represented. Removing bitmap names from recognition preserved all 487 original
classifications and introduced no recognized/unrecognized class collision in
this sample. Private production-C visitor checks passed 100 cases comparing
3,896 component records, including name-only changes, placement and bitmap
registration near misses, and unterminated names. Names remain bounds-checked.
The ten weapon/grenade profiles were already independent of bitmap names.

The 21 structural profiles remain a documented compatibility exception.
Supported anchors and scale flags do not establish the author's intended safe
frame; the unused canvas field supplies no demonstrated replacement rule.
Unknown layouts therefore retain the authored MCC frame. Replacing these
profiles requires evidence distinguishing stock and custom margins, including
near misses. This stage removes an irrelevant identity dependency without
claiming that every pre-existing exception is now eliminated or necessary.

Layout recognition now reads each distinct pristine root once. Conversion
writes frame markers alongside the corresponding placement changes, removing
the repeated geometry/name walk previously used to stamp recognized layouts.
The later `mcc_tags.c` placement pass was also redundant: successful HUD
conversion had already consumed every high-resolution placement flag it
tested. A private comparison of the actual old and trimmed metadata functions
restored the same arena before each call and matched every byte and return
value across 56 cases. The retained pass still clears unsupported meter
overlay metadata and checks the affected roots and meter blocks.
The ten-map inventory requires 798 scalar block copies containing 935 items,
104,732 bytes in total before arena alignment. This small added storage buys
correct consumer ownership; no loading-time or peak-memory improvement is
claimed. Private sources, inventories, reproduction logs and build manifests
are retained under `build/stage5`.

The first updated native probe exposed a fixture gap: `cseries.h` maps `free`
to `debug_free`, whose contract rejects NULL. The planner's initial nullable
free crashed before probe output. The batch stopped and its useful evidence
was retained. Nullable releases are now guarded. Both converter fixtures
enforce the engine's non-NULL free contract; the normalization fixture also
fills allocations with `0xCA` and enforces the allocator's individual size
limit. A strict private fixture fails the original planner and passes the
correction, including bitmaps-only input and every injected planning-allocation
failure. Final focused checks pass 579 cases, including 70 whole-normalization
cases and 132 validator cases. Independent re-review checked the correction,
remaining allocation bounds, ownership lifecycle, metadata trim, tests and
documentation before the final combined checks.

The final local MCC, Workshop, CE-format, protocol and neighboring harness
selection passed 2,004 tests, with nineteen skips and the same twelve deferred
boat expected failures. Skips cover unavailable JDK/symlink privileges,
Linux-only checks and optional assets. Four optional real-map checks then
passed using Ruby's `a10` and isolated CE fixtures. Direct GPU checks passed
36 MCC meter renders and 15,360 stock ammunition renders. Documentation checks
validated 136 local links, named routes and changed Python syntax; production
changes are confined to the four MCC HUD/metadata files.

The Windows release build passed with unchanged
`--portable --release --pgo=off --lto=off` options and the existing Expat import
warnings. Its executable SHA-256 is
`3e563cd2291cb72196350e68dac471f6f5b96799164177f4ecf9309dd647893a`.

Private Windows release probes compare the Stage 4 HUD module with the final
Stage 5 HUD module. Their 999 other link inputs match exactly, including the
old metadata object; the separate 56-case metadata proof and final
uninstrumented build cover that trim. Probe includes are copied privately and
hashed. The observer runs only after successful HUD conversion, reads without
allocating or mutating game state, and records selected normalized HUD fields
in a stable order rather than addresses or all tag data. Its comparator checks record framing, per-tag/aggregate
digests, inventory coverage, executable identity, runtime settings, capture
dimensions and all native smoke failure patterns across four log sinks.

All ten pairs match every recorded normalized field and stable summary:

| Workshop map | Records | Placements | Added arena bytes |
| --- | ---: | ---: | ---: |
| Ruby `2915693904/a10` | 847 | 180 | 6,832 |
| Ruby `2915693904/b40` | 909 | 228 | 7,504 |
| Ruby `2915693904/d40` | 1,019 | 221 | 7,616 |
| Cursed `2962107814/a10` | 1,273 | 325 | 16,128 |
| Quarantine `2963835242/firefight_descent_quarantine_mcc` | 983 | 169 | 7,520 |
| Spasm `3003419983/spasm_playground` | 1,188 | 508 | 22,768 |
| Mercury `3021510694/mercury2` | 1,425 | 324 | 12,992 |
| Mercury `3021510694/mercury_falling` | 1,003 | 204 | 7,152 |
| Descent `3042265379/firefight_descent_mcc` | 1,593 | 340 | 13,664 |
| Tiny Halo `3675162225/halo` | 730 | 149 | 6,128 |

Together these cover 10,970 records and 2,648 placements. The 108,304-byte
arena increase includes scalar-copy alignment. These are allocation sizes,
not measured peak-memory or loading-time improvements. All twenty hidden
processes advanced through a delayed 150-tick console expression, captured a
subsequent frame through the real NVIDIA renderer and exited normally.
Before/after captures were visually inspected; most short runs show opening
cinematics, Spasm shows gameplay, and Descent's short capture is still black.
They are not synchronized pixel-equivalence tests or complete HUD gameplay
qualification.

Nine map pairs pass the full diagnostic scan. Quarantine has the same
pre-existing refusal in both builds:
`mcc: script validation refused life_lost_0 (ai_deselect); 1 scripts won't run, 0 globals start at their defaults`.
Its normalized-output comparison passes, but its runtime is explicitly
script-limited, not a clean smoke pass. The comparator requires the exact
diagnostic and occurrence list to match and rejects additional failures.
This unrelated script-compatibility gap was not changed or hidden.

The final uninstrumented release and preserved baseline each passed the same
one-process sequence: Mercury Falling with its package resources, Xbox Blood
Gulch, CE Death Island, the deliberately invalid MCC tag-index fixture, then
Ruby's `a10`. Every valid phase required a fresh tick epoch, delayed console
output and a subsequent rendered capture. Both processes exited normally.
This verifies the named family transitions and early failed-load recovery;
synthetic allocation failures cover later conversion cleanup separately.

Both builds also ran Ruby's `b40` at 640-by-480 with High-Res HUD enabled and
1280-by-720 with it disabled, and Descent at 1280-by-720 with it enabled. Each
completed a console expression after 1,800 simulation ticks and captured later
gameplay, including the HUD, weapon and environment. Visual review found the
expected placement, scale and custom layout preserved. Timing-dependent
animation, camera and flashing meter differences are not pixel comparisons.
The eight paired uninstrumented runs and one exploratory baseline Descent run
passed an independent full diagnostic scan, executable/settings checks and
capture/simulation checks. Only the exact requested invalid-map refusal is
allowed in transition logs.

No original map files, network fields, simulation logic or map-family boundaries
changed. Network version 41 and the deferred boat behavior remain intact.
These results do not establish full-campaign, save/revert, multiplayer gameplay,
local split-screen, Android-device or browser-map compatibility. Existing
asset-free layout checks cover split-screen geometry; no new live split-screen
session was run.

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42385944116)
passed all three jobs at implementation revision
`809f8b8cdb9ba3dd9e5727cc261e8f3043eba931`. Linux readers/validators passed
1,747 checks with sixteen skips, followed by 38 passing ASan/UBSan preview
checks. Windows adapters passed 1,948 checks with seventeen skips across their
invocations. The Xvfb/Mesa job passed all 36 MCC meter renders.

The corresponding [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42385944018)
passed all four debug platform builds. The main-only release job was skipped;
the local Windows release result is recorded above. Linux's main invocation
passed 2,478 checks with 27 skips, twelve expected failures and seven passing
subtests; its pytest invocations totalled 2,794 passes and 52 skips, with the
same expected failures and subtests. Ordinary and full-LTO ELF light-storage
checks passed. Windows COFF light-storage and corruption-negative checks
passed. Android built the guest/host and debug APK and passed touch, gyro and
MCC importer checks. Web passed its page/browser checks and native
Chromium/Firefox WebRTC assertions, including configured packet loss. These
transport checks do not constitute map-based multiplayer qualification.

All six original Xbox/CE fixture files and their copies, ten Workshop maps,
Mercury bitmap/sound resources and the installed shared singleplayer bitmap
resource were rehashed and unchanged. Thirty sequential process attempts
included the one diagnosed allocator crash and 29 normal exits; generated
map caches were removed after every attempt, totalling 23,026,728,960 bytes.
The 375 inactive save/support/capture files, including the failed attempt's
useful evidence, totalled 942,965,724 bytes. They were archived losslessly to
`build/stage5/inactive-runtime-evidence.zip` (66,518,323 bytes). Every archive
member was hash-verified before loose copies were removed; the adjacent
manifest records original paths and hashes.

Verified disposable fixture copies, pytest outputs and private probe binaries,
objects and debug symbols were removed without following links. About 108 MB
remains under `build/stage5`, primarily the 66.5 MB archive, 27 MB preserved
baseline build, inventories and diagnostic evidence. The final release build
remains usable under `build/windows`. No disposable map copies, generated game
caches or temporary test/probe binaries remain under this stage.

Final evidence review checked the combined implementation, preserved caller
boundaries, ownership/failure paths, tests, CI and documentation. It corrected
the allocator-contract gap, removed the demonstrated redundant work, and found
no further justified structural change within this stage. Local links, named
symbols, Python syntax and whitespace checks passed. This documentation-only
completion records results for the implementation revision above; it changes
no production code or tests.

## Stage 6: checksum consolidation

### Boundary and exact compatibility contract

The baseline is `18c01ec34`, with Windows release executable SHA-256
`3e563cd2291cb72196350e68dac471f6f5b96799164177f4ecf9309dd647893a`.
The initial checkpoint, resource, saved-game, identity, precache, protocol and
script-thread selection passed all 213 tests before implementation.

The four MCC consumers implement the same reflected CRC32 polynomial
(`0xedb88320`). Checkpoint and thread blocks begin with an all-one internal
seed and complement the result. Resource checks begin with the complemented
stored map checksum and append selected bitmap then sound resources, each
framed by its one-byte type and a `0xff` terminator. Only the published zero
is changed to one. SHA-256 separately hashes the map and selected resources
with length/type framing; it is not replaced by CRC.

Design review rejected a direct use of `source/memory/crc.c`: its
`unsigned long *` interface does not match portable `uint32_t` storage on
LP64 standalone readers, and its lazy mutable table would introduce an
initialization dependency. The existing port-prefixed zlib CRC is already
linked by Windows, Linux, Android and web. Their build configuration uses
its static tables; Android includes the implementation in the ILP32 guest
without adding host imports. Explicit relative header includes select this
port library rather than the separate original game zlib. No vendored code,
shared engine implementation or build graph needs modification.

The MCC callers use zlib's finalized CRC convention: seed zero for saved
blocks, the map checksum for resource chaining, and no extra complement.
Resource framing bytes remain explicit unsigned bytes. Bounds, selected
resources, file errors, loading pulses, incremental work budgets and SHA
framing retain their existing paths. There is no new mutable checksum state
to initialize or dispose. This is content-independent MCC code maintenance;
no map identities or compatibility exceptions select the change.

### Implementation review and local evidence

Review covered the four consumers, their callers, standalone linkage, test
oracles and reader documentation. The prefixed header must follow `cseries.h`
in the resource translation unit: the first release build caught a collision
between C-library limits macros and the engine's enum names. Correcting that
include order required no shared-source or build changes. The resulting
Windows release build passed with the existing Expat linker warnings.
Independent review found no further production or test blocker before the
final local regression selection.

The new checkpoint/thread characterization passed 96 checks on both old and
refactored implementations. A private same-process comparison using extracted
old/current functions compared 4,194,304 persisted bytes exactly across five
cases: ordinary/max inventory, ordinary/max extended names, and the expanded
thread image. The committed tests independently check payloads, footer fields,
little-endian checksums, reserved bytes and thread coverage against Python's
zlib, retaining existing corruption/refusal and migration cases.

Resource/catalog characterization passed 196 checks before production edits;
the final focused selection passed 238 checks. CRC oracles cover every resource
mask, zero/high-bit/all-one seeds, 16/32 KiB boundaries, final-zero remapping,
an intermediate zero before the second resource, cancellation and failed reads
without publishing partial output. SHA-256 and package-isolation coverage
remain in place. The complete original Mercury bitmap (429,799,299 bytes) and
sound (257,054,056 bytes) resources produced identical baseline/current/Python
checksums for bitmap-only, sound-only and combined masks. They were read in
their installed Workshop folder and rehashed unchanged. This private comparator
tests the actual reader/checksum with explicit resource paths; it does not
claim to exercise Workshop discovery or network admission.

The removed incremental table reduces the x86 check object from 2,680 to
1,656 bytes, measured with `sizeof`; the synchronous 1,024-byte local table and
both per-check table-construction loops are also removed. No new allocation
or table is introduced. An isolated Windows x86 `-O2` comparison passed 24
old/new/oracle vectors. Seven alternating-order trials over 128 MiB per sample
measured median CRC time of 0.3883 to 0.03890 seconds for 256-byte calls and
0.3867 to 0.01832 seconds for 1 MiB calls (about 10x and 21x). These are checksum
microbenchmarks, not whole-game loading, checkpoint or frame-rate measurements.

The final broad local selection passed **2,059 tests**, with nineteen skips and
the twelve unchanged strict boat expected failures. Production changes remain
entirely in MCC-owned files. Serialized formats, SHA content proof, network
version 41, OpenCE crossplay and Xbox/CE implementations are unchanged.
Private proof sources, commands, logs and hashes are under `build/stage6`.
The final focused checkpoint/thread selection includes two additional exact
thread-length boundary cases and passed 98 checks. Four optional real MCC/CE
asset checks also passed. The post-review release rebuild required no further
work; its executable SHA-256 is
`5f3f449c13fddf4198157bd94fed9cdc1fad02290ec34d966c3d420d0aa998c7`.

### Native runtime, retained findings and cleanup

The preserved baseline and final release each passed the same hidden,
singleplayer save/revert/core sequence on Ruby's Rebalanced `b40` and Mercury
Falling, loaded from their installed Workshop packages. Each run advanced
150 ticks, saved a changed MCC campaign flag, proved a subsequent mutation,
restored the checkpoint and confirmed the saved flag after another 150 ticks.
It repeated mutation/restoration through `core_save`/`core_load` and exited
normally. The paired executable hashes, settings, runner and map checksums
were checked independently. All eight retained 20 MiB images passed an
independent check of their protected MCC payload/footer/thread/name blocks.
These independently validated runtime images are not claimed byte-identical
across runs; the controlled five-case comparison above establishes exact
before/after serialized-byte equivalence.

The final release also passed Mercury Falling, Xbox Blood Gulch, CE Death
Island, a deliberately invalid MCC tag-index fixture, then Ruby `a10` in one
process. Each valid phase required a new simulation epoch, an exact delayed
output marker and a complete rendered capture. The invalid map was refused
and the later MCC load recovered. Independent review checked the complete
logs, expected refusal, marker ordering, settings, captures and normal exit.
The runtime verifier additionally passed 83 negative controls for missing or
echoed outputs, extra acceptance events and changed protected bytes.

Four earlier attempts are retained as failed/incomplete evidence. Two Ruby
attempts exposed runner command-length and repeated-log-observation problems;
an earlier transition collector could not encode one received character.
The other Ruby baseline attempt asserted on a stale script-node index after
revert. Its MCC checksums are valid. The runner had requested a save from a
temporary console expression that remained asleep during capture; another
baseline run reprinted that saved expression's marker after restoration.
Source inspection shows finished console threads are deleted and unreferenced
syntax is collected. This is a baseline console-expression lifetime finding,
not evidence of a CRC regression, and is retained for Stage 7 investigation.
The final paired recipe issues standalone save commands and waits outside
the game before checking completion. No production workaround was added.

Nine sequential game attempts were stopped; all five final accepted runs
exited normally. Their generated caches, including those from failed attempts,
were removed (6,908,018,688 bytes). All six original Xbox/CE fixtures, three
tested Workshop maps and Mercury's two resource files were rehashed unchanged.
The 142 inactive save/support/capture files (338,155,372 bytes) were archived
losslessly to `build/stage6/inactive-runtime-evidence.zip` (9,989,586 bytes).
Every archive member was hash-verified before removal of the loose copy;
the adjacent manifest records original paths. Original maps and saves were
not modified. Disposable synthetic pytest fixtures, duplicate map copies and
test/probe binaries were removed; useful logs, proof sources and the baseline
build remain. There are no generated game caches or active owned game processes.
About 39 MB remains under `build/stage6`, primarily the 27 MB baseline build,
10 MB verified archive and 1.4 MB of CI evidence. The final release remains
usable under `build/windows`.

These checks establish bounded native loading, save/revert/core behavior and
map-family transitions. They do not establish full campaign, multiplayer,
live split-screen, Android-device or browser-map compatibility. Milestone 7
implementation has not begun.

### Platform CI and completion

Both push workflows passed on implementation revision
`71dcfe6dd74c59fd8ab2f54d8b9fa112e04d1a36`, attempt 1, without CI repairs or
reruns. The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42390735879)
passed Linux readers/validators (1,782 checks, sixteen skips), 38 separate
ASan/UBSan preview checks, Windows adapters (1,983 passes and seventeen skips
across invocations), and 36 software-GL meter renders.

The [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42390735872)
passed all four debug platforms. Linux's main invocation passed 2,498 checks
with 27 skips, the twelve existing boat expected failures and seven passing
subtests; all its pytest invocations totalled 2,814 passes and 52 skips, with
the same expected failures/subtests. Ordinary and full-LTO ELF/COFF
light-storage checks and the Windows corruption negative control passed.
Android built its guest, host and debug APK and passed touch, gyro and MCC
importer checks. Web passed page/server tests, browser startup without retail
assets, and native Chromium/Firefox WebRTC checks with and without configured
packet loss. These are transport checks, not real-map multiplayer qualification.
Main-only release/profile jobs were skipped; the Windows release was built
and exercised locally. Counts from overlapping jobs are not unique test totals.

CI logs, metadata and local log hashes are retained in `build/stage6/ci`.
Artifact metadata names the exact revision; CI binaries were not downloaded
or independently rehashed. Final documentation review checked 145 local links,
the new section anchor, named functions and test syntax. Production remains
confined to four MCC-owned files, with no shared-source, vendor, build-graph,
protocol or map changes. The combined review found no further justified change
within this milestone. Final evidence records both successful checks and the
retained baseline finding; Milestone 6 is complete.
