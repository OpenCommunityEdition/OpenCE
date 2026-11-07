# MCC refactor milestones and validation

## Milestones and current position

Updated 7 November 2026. Milestones retain the existing stage numbers, so
"Stage 6" and "Milestone 6" refer to the same work. Stages 1–8 are complete;
Stage 9's combined review, testing and cleanup are complete. Ruby co-op
recovery failed on both original main and current, so its compatibility
sign-off remains withheld.
The shared boat and console-syntax corrections remain explicitly deferred,
as distinguished from their completed investigations.
The stage evidence below records the changes, actual validation and limits.

| Milestone | Scope and outcome | Status |
| --- | --- | --- |
| 1 | Establish executable regression coverage, CI and Workshop runtime baselines. | Complete |
| 2 | Reproduce the shared boat defect and establish the crossplay constraint. | Investigation complete; shared fix deferred |
| 3 | Remove redundant MCC resource ordering and bitmap ownership lookup work. | Complete |
| 4 | Replace repeated per-texture stream scans with a local per-tag cursor. | Complete |
| 5 | Correct HUD ownership, remove bitmap-name dependence and trim duplicate normalization. | Complete |
| 6 | Consolidate duplicate checksum work with exact compatibility and linkage proved. | Complete |
| 7 | Consolidate script resolution, reject unused checkpoint object handles before restoration, and audit state ownership. | Complete; shared console lifetime fix deferred |
| 8 | Retain justified HUD exceptions and support MCC-wide AI deselection through its real native service. | Complete |
| 9 | Review the combined branch and extend runtime, lifecycle and performance validation. | Review/testing/cleanup complete; Ruby co-op recovery sign-off withheld |

The latest verified implementation revision is `f25c4886a`, with fresh passing
MCC and four-platform debug workflows. Stage 9 recorded 2,193 local passes,
24 skips and twelve existing boat expected failures, four real-asset checks,
64 successful matched loads, sixteen failed-load recoveries, standalone
save/restore/reset checks, Mercury co-op and all four Xbox/CE mixed-build host
directions. Ruby co-op recovery remains a failed observation, including an
original-main reproduction; completion of the investigation does not fix it.
All 21 structural HUD profiles remain with measured evidence and removal
conditions. MCC protocol is 65; OpenCE remains 41. The shared console-syntax
defect remains unfixed, as explicitly requested. The detailed sections
distinguish these bounded checks from full compatibility. Milestone counts
are not an estimate of percentage effort remaining, and do not establish
that every MCC module has received the same depth of audit.

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
pass. Stage 7 reproduced this on Xbox, CE and MCC and isolated a shared
syntax-lifetime defect. Its correction is explicitly deferred pending a shared
save/ownership design; see [Stage 7 evidence](#stage-7-scripts-and-state-lifecycle).

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

The combined review and planned runtime observations have completed. Platform
and representative runtime checks pass, but Ruby co-op recovery does not meet
the all-passing compatibility criterion above. It also fails on original main;
repairing the shared recovery path requires a separate contract/version review
under the standing OpenCE-crossplay constraint. This exception is recorded,
not silently accepted or converted into a passing check. Final documentation
and cleanup are complete. Findings and coverage are recorded in
[Stage 9 evidence](#stage-9-combined-review-and-extended-validation).

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
live split-screen, Android-device or browser-map compatibility. At this Stage 6
completion point, Milestone 7 implementation had not begun.

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

## Stage 7: scripts and state lifecycle

Stage 7 starts from `42176d5423cf6430b29ac39549a034a8feebb7bd` in the same
dedicated worktree. The preserved Windows release has SHA-256
`5f3f449c13fddf4198157bd94fed9cdc1fad02290ec34d966c3d420d0aa998c7`.
The initial checkpoint, campaign, names, progress, scripts, syntax and thread
selection passed 229 tests before production edits. Private commands, extracted
sources, logs and hashes are retained under `build/stage7`.

### Justified changes and boundaries

Two production files change, both MCC-owned. No map name, package ID, checksum
or asset fingerprint selects either behavior. No shared hook, native Xbox/CE
implementation, serialized format, network message or protocol version changes.
OpenCE version 41 and MCC protocol 64 remain unchanged: valid resolution and
checkpoint behavior are preserved; the correction rejects malformed saved data.

[`mcc_scripts.c`](../port/linux/game/mcc_scripts.c) had two copies of seven
MCC function mappings, the vibration alias and campaign fallback. The private
`mcc_script_resolve` now owns that mapping. Preparation still runs before MCC
activation and may use native fallback; public runtime lookup retains its
loaded-MCC/null guard and does not gain fallback. A recognized vibration alias
whose rumble target is absent still returns `NONE`, even if the native table
contains the original alias. Thirty-six source-based before/after traces match
exactly, including function IDs, rewritten call/predicate IDs, refusal and lookup
counts. The scripts/parameters/syntax selection passed 188 checks before and
after. This removes duplicate maintenance and drift risk; no game-speed or
frame-time improvement is claimed.

[`mcc_checkpoint.c`](../port/linux/game/mcc_checkpoint.c) now rejects a zero
object identifier in `mcc_checkpoint_units`. Native `datum_try_and_get` treats
an identifier of zero as an unused slot. A freed header can retain a bounded
datum pointer and unit type; matching a saved handle's zero salt therefore
does not establish a live unit. The old MCC preflight allowed that record
through. Native `game_state_image_accept` then copied the incoming image and
MCC names before the real grenade restore rejected it. This is an MCC
checkpoint-admission defect; fixing its preflight benefits every MCC checkpoint
using the format and requires no native change.

The new regression extracts the real native array layout, array validation,
datum deletion/lookup and grenade snapshot/restore. It deletes slot 0 while
slot 1 remains live, producing a legal high-water count of two and one active
entry, then recomputes both MCC CRCs. On the old code, generic validation and
MCC preflight succeeded, acceptance failed, and live bytes/names changed;
campaign restoration was not reached. The focused baseline had 59 passes and
that one intended failure. With the one-condition fix, the malformed image is
refused before copying anything, while a valid salted unit restores normally.
Stale-salt and out-of-range-pointer controls also pass. Checkpoint, grenades and
thread checks passed all 105 cases. Names/campaign and unrelated pool validation
remain adapters in this fixture; this is not a claim to validate every native
checkpoint allocation or every runtime state.

### Ownership reviewed and retained

The combined review examined the following routes before final checks. Sound
code was retained where removing it would lose an established contract or where
no practical benefit was demonstrated.

| Owner and route | Retained contract and evidence |
| --- | --- |
| `mcc_scripts_prepare`, `mcc_script_find`, `mcc_script_call_valid` | Preparation and guarded runtime resolution have different fallback rules. Unsupported names and malformed calls still refuse. Exact resolver traces and scripts/syntax tests cover the changed route. |
| `mcc_parameters_prepare`, `mcc_hs_release_arguments`, `mcc_hs_release_thread` | Prepared signatures/lexical owners are separate from live saved stack arguments. Publication counts clear on preparation failure/unload; return, cancellation, deletion and overflow release held object-list references. Existing recursion/sleep/overflow tests are retained. Full-capacity local-global reuse is not established by this audit. |
| `mcc_script_threads_prepare`, `mcc_hs_threads_activate`, `mcc_script_threads_dispose` | Reserved CPU storage owns extended frames; disposal restores the previous array and arena bytes before releasing MCC storage. Save-version bridges and address checks remain necessary. Thread lifecycle and checkpoint tests exercise these contracts. |
| `mcc_checkpoint_validate`, `mcc_checkpoint_restore` | Pre-copy validation and post-copy restoration are separate responsibilities. Preserve format/version, fixed saved-pointer addresses, CRC and bounds checks; the unused-unit correction closes one demonstrated preflight gap without a speculative transaction rewrite. |
| `mcc_campaign_validate`, `mcc_campaign_restore`, `mcc_campaign_dispose` | Validate the full supported snapshot before publishing campaign fields; restore owned engine globals and effects at lifecycle boundaries. Existing snapshot-version and malformed-state coverage remains. |
| MCC effects, game speed and progress | Keep host authority, reliable host-packet/size/stale-tick checks, map-owned restoration and progress journal's valid-prefix recovery with refusal to append after corruption. Inspection did not establish safe further trimming. |
| `mcc_object_names_*`, native name dispatch, `mcc_cache_tags_unload` | Extended names activate only for MCC scenarios beyond the native 512-name table. Deletion must scan aliases even for an object whose native name is `NONE`; checkpoint restoration intentionally restores mappings while restart/unload clears them. |

Named-object coverage grew from seven to sixteen passing cases. It checks
complete snapshots at 513, 640 and 32,767 names, output sentinels, refused
null/length restores preserving mappings, unnamed aliases, native restart
clearing, repeated unload, and partial cleanup with the loaded flag already
false. Four private negative controls caused nine expected failures while two
unaffected controls stayed green. The actual cache-unload function is extracted,
but other owners are inert there; the loader's failure funnel was source-reviewed.
These tests do not replace the native transition checks or prove network play.

### Shared console syntax finding: investigated, fix deferred

Three baseline runs reproduced an accepted core load followed by an expired
script-node assertion: Xbox Blood Gulch, CE Death Island and Ruby's Rebalanced
`b40`. Each saved inside a sleeping temporary console expression, let that
expression finish, waited externally through at least two collection
opportunities, then loaded the core. The assertions and abnormal exits are
retained as defect reproductions, never counted as passing gameplay checks.

Native `hs_runtime_initialize` stores threads in saved CPU state; MCC extended
threads use their reserved saved arena. Temporary console syntax lives outside
that saved state. `hs_thread_main` deletes a completed console thread, and
`hs_runtime_update` later runs `hs_node_gc`, which removes nonpermanent nodes.
Restoring the saved thread can therefore restore references to already-collected
syntax. A ten-case private proof uses the actual allocator, interpreter,
collector and MCC thread capture/restore. Native and MCC fail after collection,
with or without slot reuse, but resume the sleeping expression before collection.
Permanent scenario nodes and completed standalone commands are controls. Parser,
core admission and networking are outside that isolated proof; the native runs
establish the real admission/assertion sequence.

This is a shared engine ownership defect, not an MCC-only compatibility need.
Discarding all saved console threads would also remove working replay behavior;
retaining all temporary nodes would consume the bounded syntax pool. A correct
fix needs a defined shared lifetime/admission design for syntax and strings,
saved-image compatibility, and comparison with upstream OpenCE's checkpoint
contract. No network-version requirement has yet been established by this
investigation. Preserve the user's crossplay requirement while resolving those
questions; do not introduce an MCC-only bypass or claim the defect is fixed.
The scoped investigation is complete, and that shared correction is explicitly
deferred. The user confirmed this deferral before starting Milestone 8; it does
not block the remaining MCC refactor. The evidence needed to reopen it is a reviewed ownership design with
legacy/MCC replay, collection, slot reuse, save compatibility and relevant peer
agreement tests. It is separate from Milestone 8's compatibility exceptions.

### Final validation and completion

The combined production/test/reader-guide review found no further justified
change within this milestone. The final local regression selection passed
**2,108 tests**, with nineteen skips and the twelve unchanged strict boat
expected failures. Four additional real MCC/CE reader/resource checks passed.
The Windows release built with the existing portable/release/PGO-off/LTO-off
configuration; a post-review rebuild required no work. Existing Expat import
warnings remain. Executable SHA-256 is
`5786761be690e78678c97ce8c3dc241934d30d33a49db524efc3d2315e7956f3`.

The baseline and current releases each passed standalone save/revert/core-load
checks on Ruby B40 and Mercury Falling from their installed Workshop folders.
Each saved a changed campaign flag, proved the subsequent mutation, accepted
the saved image, confirmed the restored value and advanced another 150 ticks.
The current release also passed native `map_reset` on both MCC maps, Xbox
Blood Gulch and CE Death Island: a fresh low-tick epoch and 150 further ticks
distinguish a restart from merely continuing the old simulation. These restart
checks do not establish reset semantics for every campaign field.

One current-build process traversed Mercury, Xbox Blood Gulch, CE Death Island,
an invalid MCC tag-index fixture and Ruby A10. The invalid map was refused and
the later MCC load recovered; each valid phase produced fresh simulation and
a rendered capture. Inspection of the four captures confirmed coherent scenes
and the expected HUD where visible, not exhaustive visual equivalence.
All nine successful runtime checks exited normally. The three baseline console
assertion reproductions above are separate failing gameplay cases. Runs were
sequential, hidden and used isolated saves; no multiplayer gameplay is claimed.

Independent verification checked exact commands/results, map and executable
identity, paired settings, fresh epochs, normal exits, failure diagnostics and
all eight successful 20 MiB save/core images' protected MCC blocks. The three
reproducer cores retain their identities and hashes; the Ruby reproducer also
has valid MCC CRCs. All 188 host-only verifier controls passed. An initial
checker incorrectly required restart cues at exactly tick 1 and assumed CE's
launch path was its saved scenario name. Source inspection and the authenticated
CE tag index established the actual contracts; the corrected checks retain
strict epoch/order/identity requirements. Both checker versions and the initial
failed verification are preserved. No game rerun or production change was used
to hide those checker assumptions.

All twelve owned game processes stopped. Their 72 generated map-cache files
(9,210,691,584 bytes) were removed after each attempt. The 113 inactive
save/support/capture files (429,544,430 bytes) were archived losslessly to
`build/stage7/inactive-runtime-evidence.zip` (6,732,992 bytes). Every member and
loose file was hash-verified before removing the loose copy; the manifest records
original paths. Useful logs and reproducible proof sources remain available.
Final cleanup removed 431,648,191 bytes of disposable synthetic fixtures,
test binaries and the six copied map/resource inputs; earlier proof-binary
cleanup is recorded separately. All eleven original map/resource inputs were
rehashed unchanged. About 44 MB remains under `build/stage7`, principally the
27 MB baseline build, 6.7 MB verified archive, proof sources, manifests and CI
logs. No generated game caches or active owned game processes remain. The
current release stays usable under `build/windows`.

Both push workflows passed on implementation revision
`f8e621acf6505e1a414e4e52142c7efcbb7b61a9`, attempt 1, without repairs or reruns.
The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42394316405)
passed Linux readers/validators (1,795 checks, sixteen skips), 38 separate
ASan/UBSan preview checks, Windows adapters (2,032 passes and seventeen skips
across invocations), and 36 software-GL meter renders.

The [Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42394316106)
passed all four debug platforms. Linux's main invocation passed 2,498 checks
with 27 skips, the twelve existing boat expected failures and seven passing
subtests; its nine pytest invocations totalled 2,814 passes and 52 skips, with
the same expected failures/subtests. Ordinary and full-LTO ELF/COFF storage
checks and the Windows deliberate-corruption control passed. Android built
the guest, host and debug APK and passed signing, touch/gyro and importer
checks. Web passed Node/page tests, asset-free browser startup and native
Chromium/Firefox WebRTC checks with and without configured packet loss.
These are transport checks, not real-map multiplayer qualification. The main-only
release job was skipped; Windows release was validated locally.
Overlapping job counts are not unique test totals.

Complete job logs, revision/attempt metadata, artifact metadata and local hashes
are retained in `build/stage7/ci`. Upload digests were matched to job logs;
CI binaries were not downloaded or independently rehashed. Final documentation
review checked 134 local links, the Stage 7 anchor, seventeen named functions,
test syntax and the two-file MCC production boundary. The final resulting
implementation, tests, integration and reader documentation were reviewed;
no further justified changes remain within this milestone. Full campaigns,
real-map co-op, live split-screen and physical Android/browser-map gameplay
remain unverified by these bounded checks. Milestone 7 is complete with the
shared console correction explicitly deferred. At that completion point,
Milestones 8–9 had not begun.

## Stage 8: compatibility exceptions and known gaps

### Retained HUD selectors

The 21 structural profiles in
[`mcc_hud_layout.inl`](../port/linux/game/mcc_hud_layout.inl) remain unchanged.
No simpler replacement was justified by the inspected authored fields or parent
usage. This is a retention decision with explicit limits, not proof that the
table is the only possible implementation or that every unknown layout intends
the wider MCC safe frame.

The fresh production-visitor audit covered **670 HUD components across seventeen
installed MCC maps**, with 495 recognized components. Inputs were seven stock
MCC maps and ten Workshop maps, including Ruby's Rebalanced A10/B40/D40,
Mercury, Descent and Quarantine. Every retained profile had an installed match:

| Profile label | Matching components |
| --- | ---: |
| assault rifle | 17 |
| master rounds | 23 |
| empty weapon/vehicle layouts | 210 |
| cyborg | 15 |
| cyborg_mp | 15 |
| warthog driver | 15 |
| passenger_2 | 15 |
| gunner | 15 |
| scorpion | 3 |
| passenger_4 | 11 |
| passenger | 9 |
| passenger_3 | 9 |
| pistol | 19 |
| master plasma | 15 |
| grenade counter panel | 36 |
| banshee | 8 |
| ghost | 4 |
| rocket launcher/needler | 26 |
| sniper rifle | 10 |
| shotgun | 13 |
| master fuel | 7 |

These labels describe evidence; names do not select the frame. The audit
extracts the current C visitor and checks every fingerprint against an
independent bounded numeric inventory. All 670 bitmap-name controls retained
their fingerprint. All 670 private root-anchor perturbations changed it, and
all 495 recognized components became unknown. Original map files were opened
read-only, and the private tag arena was compared byte-for-byte after all
private mutations were restored. The regeneration script, input identities, source hashes,
compiler invocation and per-component metadata are retained under
`build/stage8/hud`; run `audit_profiles.py` there using the recorded stage1
environment. These controls establish classification coverage and sensitivity,
not how hypothetical edited maps ought to render.

Seven combinations of tag group, root anchor, the set of scaling flags and the
set of scales occurred in both recognized and unknown components. Examples
include stock gunner geometry versus Ruby B40's shadow gunner, and stock master
plasma versus Ruby's master sword. That evidence defeats this particular
aggregate selector; it does not disprove every richer authored-field rule or
establish unknown layouts' intended margins. Parent inheritance also propagates
an already classified marker: `mcc_hud_weapon_frame_begin` cannot supply the
initial decision for a populated root without a parent.

`mcc_hud_plan_prepare` fingerprints validated pristine bytes once at load time,
before normalization. The ownership plan isolates mutable aliases; temporary
plan arrays are freed on success or failure. Runtime-owned markers and converted
data are released by failed-load/unload cleanup. Authored padding is cleared
before the converter writes its own marker. Drawing remains guarded by loaded
MCC tags. The sole weapon-frame caller brackets rendering with begin/end, without
an intervening return or goto; both boundaries clear transient state. No new
lifetime defect or cleanup edit was justified by this review.

A separate retained exception is `mcc_hud_message_frame` in
[`mcc_hud.c`](../port/linux/game/mcc_hud.c): top-left messages at normalized
offset `(0, 60)` use native safe margins. This recognizes stock placement
independently of replacement fonts or icon art; it is not an explicit authored
safe-frame flag. Integer coordinate conversion can map distinct raw positions
to that same normalized offset. A custom author choosing that geometry with
different intended margins remains indistinguishable. Its marker has the same
MCC runtime ownership and cleanup as other converted placements.

Existing public coverage includes 26 actual-visitor layout cases for geometry,
name independence, excluded fields, malformed references, bounds and read-only
hashing. Normalization tests cover a positive grenade profile, a two-pixel
authored near miss and aliased mutable blocks. Eleven stock-message cases cover
both coordinate units, nearby positions, other anchors and erasure of a seeded
marker. Together with the fresh all-profile audit, no material test gap was
found that justifies duplicating all authored profile layouts in tracked tests.
Runtime/rendering results are recorded separately below; classification
checks alone do not validate visual intent.

Remove or reduce either layout exception only after a demonstrated authored
field or usage rule expresses the intended margin and preserves recognized
layouts, nearby custom layouts, parent ownership and independently centered
components. Check renamed resources, bounds, aliases, failure cleanup and
MCC-to-Xbox/CE transitions, with actual rendered comparisons on Ruby's
Rebalanced, Descent and other affected maps. Rarity or line count alone is not
evidence for removing a profile.

The bounded adjacent-selector review distinguished these exceptions from
semantic contracts. `mcc_hud_multiplayer_text` selects
`ui\multiplayer_game_text`, string 100, because the existing score-hint consumer
in [`game_engine.c`](../source/game/game_engine.c) reads that exact resource and
index; resolving its MCC formatting argument is not a per-map bypass. Script
function/global names identify language operations, controller-entry ownership
identifies glyph roles, and structure offsets, bitmap flags and child anchors
describe validated formats. `mcc_hud_title_bounds` applies its non-wrapping
title policy to authored flags/alignment, without selecting Warthog Rally or a
160-unit box. No adjacent map-name/checksum dispatch or justified selector trim
was found in the inspected HUD/script paths. This bounded result is not a
repository-wide audit; the broader combined review remains Milestone 9.

### MCC-wide script admission

The installed Quarantine map (`2963835242`, SHA-256
`5b89b330291e7cd2a5c098b2c379aee3335e1349bc85aa1677638883b26ac8cf`)
contains 114 scripts and 19,331 syntax nodes. Its continuous `life_lost_0`
contains one valid, permanent, zero-argument `ai_deselect` call. Native map
policy deliberately excludes that operation, so the previous build disabled
the entire script. This was an MCC language-compatibility gap, not malformed
map data or evidence that Xbox/CE's policy should change.

The existing native `ai_scripting_deselect` service performs real selection
clearing when AI is initialized, including the native debug record/idle-look
cleanup. An eighteen-line addition to `mcc_scripts.c`, plus its private ID,
exposes that service through the existing MCC registry and validator. It adds
no persistent state, save fields, shared hook, map-name selector or no-op.
Existing loaded-MCC guards and unload ownership apply. Xbox/CE's allowlist,
adjacent `ai_select` refusal and client console authority remain unchanged.

MCC protocol advances from 64 to 65 because previously disabled authored
gameplay now executes. OpenCE stays at 41; packet layouts and the camera
envelope versions remain unchanged. The composed admission regression compiles
the complete actual server dispatcher and both envelope implementations:
MCC 64 is refused before the ordinary join handler with or without a camera
wrapper; MCC 65 reaches it with MCC capability. Camera-only requests do not
gain MCC access, and legacy control joins retain their path. Removing the
inner revision check makes the negative control fail. This routing harness
uses an observed join sink; separate existing actual-handler tests cover
decoding, tokens, bans, slots and session authorization.

Four focused resolver/admission cases failed before the adapter and pass
afterward. The final focused selection passed 121 checks, including real-source
native clearing and inactive-AI behavior, malformed calls, unload guards,
unchanged native/client policy and three deliberate-fault controls. The combined
review found no further necessary production change or justified HUD trimming.
It reviewed the final implementation, integration, tests and reader guides;
no shared `source/` implementation changed in this milestone.

### Runtime and local validation

The reviewed Windows release builds with the unchanged portable/release,
PGO-off/LTO-off configuration. Its SHA-256 is
`816055002eeb8f5434ec78d5dad817e5397d443e8ccb58e66d9d852e755cafbb`;
existing Expat import warnings remain. The local regression selection passed
2,123 tests, with nineteen skips and the twelve unchanged strict boat expected
failures. Four additional real MCC/CE reader/resource checks passed.

Matched baseline/current standalone runs on Workshop Ruby B40, Descent and
Quarantine each advanced at least 1,800 ticks and produced a fresh complete
capture. Baseline Quarantine deliberately records `passed=false` for its exact
script refusal, despite completing the observation; the current run has no such
refusal. The other five runs pass and all six exit normally. Visual inspection
found coherent scenes and no apparent HUD placement/clipping regression in
these views. Ruby used 640x480 and Descent 1280x720; this is bounded rendering
evidence, not pixel-identical animation or proof of all authored layout intent.

The baseline/current two-peer Quarantine comparison used separate saves and
authenticated host consoles. Both runs first established real cooperative mode,
two living players, fresh simulation, completed authored introduction, untouched
start device (`game_started=false`), positive lives and false end/cinematic
flags. The only assigned scenario value was `lives=0`, with immediate readback.
The baseline still had false end/cinematic flags after 1,000 additional ticks.
The current host reached `game_ended` at logged tick 1,770, the unique authored
`one_time_cinematic` marker at 2,640 and continued to 2,730. Source inspection
places `ai_deselect` after that unique marker and `ai_erase_all`, without an
intervening sleep.
Both peers continued and exited normally. This demonstrates the authored branch
after condition injection, not player death, life decrement, a full campaign
or a full synchronization verdict.

Two earlier baseline setup failures are retained. The first lacked stock
Carousel, which native network fast setup loads before choosing the requested
MCC map; adding one verified private input copy resolved that prerequisite.
The second stopped waiting for a living second player at 45 seconds. Quarantine's
authored co-op introduction lasts 1,430 ticks, and native cooperative spawning
holds extra players during a cinematic. A 75-second wait accommodates that
existing behavior. Neither failure required a production change or was relabeled
as a gameplay pass.

One current process traversed Mercury Falling, Xbox Blood Gulch, CE Death Island,
a bounded invalid MCC fixture and Ruby A10. All four valid phases produced a
fresh tick epoch, 150 further ticks and a complete capture. The invalid map was
refused, returned to the menu and did not prevent the subsequent MCC load.
Visual inspection confirmed coherent scenes and the native HUD where visible;
Mercury and Ruby were cinematic views. The process exited normally and its
unique invalid fixture was removed after containment/content checks.

The newly reached authored `garbage_man` calls the existing native
`rasterizer_decals_flush` operation, whose documented effect is to flush all
decals. The current branch emits two permanent-decal deletion diagnostics during
that operation, before `ai_deselect`. Source inspection traces them through the
native cache flush and decal deletion callbacks; removal is consistent with
the command's stated effect, while permanent bookkeeping/logging remains
inconsistent. These are the only new unique error/warning classes in the
baseline/current branch comparison, on the host; no new client class appears.
They are explicitly retained findings, not an assertion that the logs are clean.
A separate baseline control waited 1,800 ticks, called only authored
`garbage_man`, then advanced another 150 ticks. Neither diagnostic existed
before that call; both appeared once afterward, reproducing the same native
behavior without the new adapter. The control completed and exited normally,
while retaining `passed=false` for the baseline's original script refusal.
No shared decal implementation was changed. Repeated flush bookkeeping and
longer-term rendering effects remain outside this bounded attribution test.
Independent verification covers twelve sessions and sixteen stopped processes,
with no forced termination or nonzero exit. It checks executable/map identity,
settings, ordered commands and response frames, tick epochs, complete captures,
all log sinks, actual failure verdicts and diagnostic inventories. Fourteen
isolated negative controls reject altered outcomes, identities, missing responses
with echoes retained, hidden fatal messages, missing transitions, partial captures
and an incomplete flush control. `archive_ready` means evidence integrity and
inactive processes; it does not turn a known refusal or warning into a pass.

No old-checkpoint migration is established:
a checkpoint created when a continuous script had been disabled is not proven
to acquire its missing thread on restore. The supported admission path being
tested here is a fresh map load. Shared console-syntax and boat corrections
remain explicitly deferred; Milestone 9 has not begun.

### Platform validation

The [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42399542277)
passed on implementation revision `99341deebf53cafdb82c9c901dfa31091a7b85a0`,
attempt 1: 1,796 Linux reader/validator passes with sixteen skips, 38 separate
ASan/UBSan preview checks, 2,037 Windows adapter passes with seventeen skips
across invocations, and 36 software-GL meter renders.

The initial Build workflow exposed one Linux compilation failure in the new
camera admission test: its helper had static linkage after a public external
declaration. Windows' MSVC-target compiler had accepted it. The one-word fixture
correction matches that header; no production code or runtime binary changed.
Ten targeted camera/MCC protocol checks passed after correction, and a
Linux-target front-end control reproduces the old error and accepts the fix.
The original failed job/logs remain in the evidence.

The [corrected Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42400612375)
passed all four debug platforms on `15d2ef13096751ba266d8845f18b4caae2f217e8`,
attempt 1. Linux's nine invocations totalled 2,824 passes and 52 skips, with
the twelve existing boat expected failures and seven passing subtests.
Windows storage/linkage checks, Android guest/host/debug-APK and importer/input
checks, and web Node/page/startup and Chromium/Firefox native WebRTC checks
passed, including configured packet-loss transport controls. These are not
full real-map internet/co-op or physical-device gameplay qualification. The
main-only release job was skipped; Windows release was validated locally.

The repair changes only the camera fixture; exact revision comparison proves
production, MCC workflow YAML and its test inputs unchanged. That file is
outside the MCC workflow's path filters and invocations, so its earlier green
result is reported on its actual revision rather than attributed to the repair.
Overlapping job counts are not unique test totals. Complete logs, revision and
attempt metadata, the original failure, revision proof and artifact metadata
are retained under `build/stage8/ci`. All six Build artifact upload digests
match their logs; binaries were not downloaded or independently rehashed.

### Final review, cleanup and completion

The final combined Stage 8 implementation, integration, tests and documentation
were reviewed before the required checks. The CI-discovered fixture correction
was reviewed and its affected local/platform checks rerun. The final document
check validates 170 local links, eight named symbols, Python syntax and the
MCC-owned production boundary. No additional code change or speculative
profile trimming is justified by this milestone's evidence.

All sixteen owned game processes exited normally. Their 96 generated map
caches, totalling 12,280,922,112 bytes, were removed after each attempt. The
462 inactive save/support/capture files (490,433,888 bytes) were archived
losslessly to `build/stage8/inactive-runtime-evidence.zip` (38,048,459 bytes).
Every archive member and loose file was hash-verified before loose removal;
the manifest records original paths. Cleanup removed another 439,092,050 bytes
of disposable fixtures, generated bytecode and seven private input copies.
All 26 original map/resource inputs, including the full seventeen-map HUD audit
corpus, were rehashed unchanged after cleanup. No generated map caches remain.

The cleanup gate checks session result and supporting-evidence hashes, resolved
path containment, linked ancestors and process identity. Windows reused one old
test PID for an unrelated application process; cleanup first refused it, then
verified its different executable and later creation timestamp without stopping
that process. Both failed preflight logs and the identity proof remain available.
Thirty-two private cleanup safety controls passed. About 79 MB remains under
`build/stage8`, chiefly the 38 MB verified archive, 27 MB baseline build, CI logs
and reproducible metadata/source evidence. The current Windows release remains
usable under `build/windows`.

Milestone 8 is complete. Its remaining limits are explicit: no full campaign,
full multiplayer synchronization, old-checkpoint migration, live split-screen,
physical Android or browser-map gameplay qualification; no correction of the
shared boat, console-syntax or native flush bookkeeping behavior.

## Stage 9: combined review and extended validation

This milestone reviews the resulting branch against original main
`db1697c9b0c46289596f22213a5bef8ee327be24`, including all earlier production
changes, tests, build integration and reader documentation. The shared boat
and console-syntax corrections remain explicitly deferred. No additional
game-code change is justified by this review; the demonstrated corrections
below affect validation tooling and a stale documentation statement.

### Combined review and boundary coverage

The resource directory sort had no ordering-dependent consumer; complete
name scans and duplicate refusal remain. Bitmap cursor correctness follows
from checked original handles and complete fresh-handle clone groups, with
reuse appending nothing. HUD planning snapshots shared owners before writes
or pointer publication, and temporary planning storage is freed on failure
and success. The remaining tag pass clears meter metadata after placement
normalization. Checksum callers preserve their distinct framing and seeds.
Script resolution retains preparation/runtime separation; checkpoint
validation rejects freed slots before copying live state. AI deselection
uses the existing native service through the MCC registry.

The final pass also inspected relevant activation, failure, unloading,
worker-join and GPU-before-pixel cleanup routes. These ownership boundaries,
required stubs, malformed-input checks and distinct regression cases remain
necessary. There is no production `source/` change across this refactor.
There is no new map-name, map-checksum or package-specific gameplay bypass.
The 21 structural HUD exceptions and separate message-frame rule retain the
evidence and removal conditions recorded in Stage 8. Existing optional HUD
artwork CRC selectors identify two known replacement sheets, after format
and size checks; they are not map selectors. Bridge tests use mocked CRCs,
so they do not independently verify those original artwork fingerprints.

This reconciles the navigation inventory in `CODINGSTANDARDS.md` with the
actual review. "Partial" identifies a bounded inspection, not a known defect
or a claim that the remaining implementation has been audited.

| Boundary | Changed and reviewed across the branch | Reviewed and retained; remaining limits |
| --- | --- | --- |
| Selection, loading, packages and resources | Resource ordering/checksums and HUD metadata conversion | Cache load/unload, validation worker teardown, relevant handle/root-alias checks and bitmap access reviewed. Catalog, loading UI, package/identity/SHA implementations and remaining parser/converter behavior are partial or unaudited. |
| Scripts, parameters and object names | Resolver, thread checksum, MCC AI deselection | Syntax, parameters, runtime/thread ownership, name tables and game speed reviewed. Full-capacity local-global reuse and every legacy interpreter route remain unestablished. |
| Checkpoints and campaign state | Checksum and unused-unit preflight | Campaign snapshot/lifecycle, script effects and progress journal reviewed. Individual campaign evaluators/rendering are partial; older disabled-script checkpoint migration remains unverified. |
| Explicit packages and progression | None | Full manifest, map-catalog and Android importer audit remains outside this review. Their automated checks are separate evidence. |
| Weapon-authored cameras | None | Existing camera/MCC admission-envelope composition reviewed as a network dependency; camera implementation not deeply audited. |
| Networking and names | MCC protocol version and combined manifest | Protocol, network adapter, names synchronization and relevant shared admission/cleanup routes reviewed. Workshop metadata and every native transport implementation not deeply audited. |
| Player, inventory, movement, damage, boats, seats and recordings | None | Boat initializer/native force consumers and checkpoint grenade lookup dependency reviewed. Remaining gameplay modules not fully audited. |
| Rendering, HUD and audio | Bitmap conversion/access, HUD ownership/layout and redundant normalization | Texture registry/bridge, relevant HUD frame/glyph cleanup and native decal-flush attribution reviewed. Geometry, shader, particles, sprites, audio/Vorbis and remaining HUD draw behavior not fully audited. |
| Menus and pause | None | Implementation audit remains outside this review; existing executable tests remain in the regression selection. |

No further production trim is supported by these findings. Removing required
family boundaries or retained exceptions merely to reduce code would lose
supported behavior. Private review notes are retained under
`build/stage9/reviews/`.

### Validation-tool corrections

Failure injection into the real `mcc_native_smoke.run_case` and `main`
reproduced seven adverse cases: kill failure, wait timeout, console-close
failure, stdout-only fatal diagnostics, and survivor/unknown/exception
completion from a case. Previously, caches or staged executables could be
deleted despite uncertain child exit; a close error bypassed shutdown and
init restoration; stdout-only failure could be reported as success.

Cleanup now requires observed exit or no launched child, uncertain completion
stops subsequent cases and preserves staged binaries, close errors retain
diagnostics while shutdown/restoration continues, and all four log sinks
contribute to the verdict. Results retain PID, output path and shutdown state.
Two normal-completion controls accompany the seven adverse cases. These tests
use fake processes and literal fixture bytes; they do not leave live games.
The before-fix log records the adverse behavior; the two normal controls
initially failed only because the new shutdown-result fields were absent.

The focused suite passed 56 tests, with one unavailable file-symlink privilege
skip. Two earlier sandbox-denied junction setup failures were retained and
the complete selection rerun with native Windows operations available.
`port/linux/NETCODE.md` also corrects one stale current MCC version from 64 to
65; historical version statements remain unchanged. OpenCE remains 41.

### Local and platform validation

Implementation revision `f25c4886a0ee1567b8b3808eee4bd8b4e1e3e6be`
passed the combined local selection: 2,193 tests, 24 skips and the twelve
unchanged strict boat expected failures. The selection includes all MCC test
modules, MCC harness cases, frontend tests, CE cache readers, Workshop/startup,
camera admission, network-comparison and HUD threshold checks. Four additional
installed Ruby B40/CE reader, geometry and resource checks passed. Windows
release was already up to date; the wire-format manifest matches its record.
Local skips include unavailable file-symlink privilege, opt-in assets,
Linux discovery and POSIX process-lifecycle checks; the four selected asset
checks were then run with explicit inputs. The document check covered 232
local links, eleven named symbols, changed Python syntax and the combined
MCC-only production boundary.

Both fresh workflows passed on that exact revision, attempt 1:

- [MCC map validation, run 42403748356](https://github.com/chestahh/GulchCE/actions/runs/42403748356):
  Linux reader/validator 1,802 passes and nineteen skips, plus 38 sanitizer
  passes; Windows adapters 2,046 passes and seventeen skips; 36 real
  software-GL meter-render checks.
- [Build, run 42403748394](https://github.com/chestahh/GulchCE/actions/runs/42403748394):
  Linux, Windows, Android and web debug builds passed. Linux recorded 2,824
  passes, 52 skips, twelve boat expected failures and seven subtests across
  its invocations. Windows storage, Android touch/gyro/importer, web startup,
  Node and Chromium/Firefox native-WebRTC checks passed, including configured
  packet loss. Branch release jobs were intentionally skipped; the local
  Windows runtime build is release.

These selections overlap and are not a unique-test total. Nine decoded job
logs, ten job records and six artifact metadata records are retained under
`build/stage9/ci`; all six artifact IDs, revision identities and upload digests
match their logs. No CI binaries were downloaded. Build and transport checks
do not establish physical Android play, browser-map gameplay or an official
upstream-executable crossplay result.

### Repeated loading, timing and memory

Four sequential, matched Windows sessions completed 64 valid loads and sixteen
deliberately invalid MCC loads with actual menu recovery. Each process repeated
Mercury Falling, Xbox Blood Gulch, CE Death Island, refusal/menu and Ruby A10
four times: one warm-up and three measured cycles. Original main and the
current build ran in both orders. All valid phases advanced at least 150 fresh
logged ticks; every process closed normally. Workshop originals stayed in
place. Renderer, 640x480 resolution, high-resolution HUD, 30 FPS cap, audio-off
settings and SDL DLL matched; captures were disabled during measurement.

The original-main executable remains SHA-256
`f1ec0dca876aa3cf746a4095ac9115cc026902f4679d1c3941deb20928c85d56`;
the unchanged current game executable is
`816055002eeb8f5434ec78d5dad817e5397d443e8ccb58e66d9d852e755cafbb`.
Milestone 9 changes validation tools/documentation, not that game implementation.
Private Win32 measurements use the owned process handle for private memory,
working set and handle counts. A read of the source-defined game-state header
confirms each actual map; it performs no process write, scan or save operation.

Six measured samples per build give the following median seconds and observed
minimum–maximum ranges. These include command queueing, log publication and
approximately 100 ms observer cadence; they are not engine-only loading timers
or statistical confidence intervals.

| Boundary | Original main | Current |
| --- | ---: | ---: |
| Mercury first logged tick | 13.567 (12.239–15.926) | 13.916 (11.657–14.467) |
| Xbox first logged tick | 1.613 (1.574–1.626) | 1.634 (1.583–1.680) |
| CE first logged tick | 2.035 (2.019–2.061) | 2.007 (1.984–2.072) |
| Ruby A10 first logged tick | 13.449 (11.796–14.949) | 14.606 (11.990–15.127) |
| Exact refusal/menu diagnostic | 0.444 (0.434–0.459) | 0.443 (0.430–0.451) |

Time order materially affects the result: original-main Mercury/Ruby medians
fell from approximately fifteen seconds in its first process to twelve in its
last; the second current process also became faster during its sequence.
Neither the first apparent improvement nor the pooled Ruby increase isolates
a build effect. The Xbox median difference is below observer cadence and its
ranges overlap. These observations demonstrate no clear attributable loading
regression, but do not statistically establish speed gains or performance
parity. File-cache state and scheduling were uncontrolled.

The first pair's measured menu-memory growth was 1.49 MiB on main and 10.11 MiB
on current, which prompted the reverse-order repeat. That pair grew 12.45 MiB
on main and 2.86 MiB on current. Current's observed menu maximum was 563.75 MiB
versus main's 564.72 MiB; Mercury maxima were 1,220.06 versus 1,223.80 MiB.
Some boundaries decreased, both builds released hundreds of MiB between MCC
and menu, and handle counts declined. There is no repeatable current-only
retention excess. This is not a universal plateau, attribution of every
retained byte, GPU-memory measurement or proof against long-duration leaks.

Logical converted-tag storage is separately bounded and identical on every
repeat: Mercury adds 7,152 occupied arena bytes and Ruby A10 adds 6,832, chiefly
the required HUD ownership copies. Bitmap counts and pixel bytes are unchanged.
Temporary planner arrays are freed; retained copies belong to the map arena's
unload/decommit path. Windows commits the same fixed 64 MiB arena in both
builds, so these occupied-byte deltas are not equivalent extra PrivateUsage
and do not explain the larger process-memory variation.

The first collector attempt remains failed: it incorrectly required a second
Xbox precache-start message, although a cached load does not emit that message.
The corrected observer verifies actual scenario identity/checksum from the
fixed native header and fresh tick epochs. Thirty-seven helper controls and
96 receipt-invalidation controls passed. Full trajectories, source identities,
raw logs and limitations are in `build/stage9/cycle-summary.md` and
`cycle-verification.json`; the initial failure was not relabelled successful.

### Saves, restarts and co-op findings

Current Ruby B40 and Mercury passed standalone save/revert, core save/load and
map-reset checks. Each restore proves a saved boolean's true–false–restored
round trip and at least 300 further fresh simulation ticks. Independent
decoding validates the retained 20 MiB checkpoint/core images and their MCC
footer/thread checksums. Xbox Blood Gulch and CE Death Island reset controls
also passed. These checks use completed standalone commands; they do not
snapshot a sleeping temporary console expression or close the separately
deferred console-syntax lifetime defect.

The first Mercury attempt correctly stopped when its checkpoint image changed
before the planned revert. Both images remain retained. The original helper
called this an auto-save, but the evidence establishes replacement, not its
writer. Removing the optional idle window between capture and revert allowed
the retry to verify the same captured image; all post-restore tick checks stayed.
The helper did not overwrite the live checkpoint with its retained copy, and
no authored script was disabled for that retry.

Extended current/current Mercury co-op passed admission, two living-player
checks and 1,800 further ticks. The unchanged strict network comparator passed
with sixty paired seconds and 1,799 on-foot drift samples, no threshold breaches
and normal exit of both peers. Nonzero drift remains recorded; this is not a
claim of bit-identical views or full campaign compatibility.

Ruby B40 exposed a separate **pre-existing shared co-op checkpoint-recovery
failure** in the OpenCE Crossplay profile. Both current/current and
original-main/original-main runs fail the unchanged strict comparator after
team-wipe reverts, with sufficient samples (1,031 and 1,271 respectively).
An additional current idle-input run also fails after reverts with 744 samples;
the finding is not an insufficient-sampling verdict or restricted to bot combat.
Both peers stay connected, but the client lacks a player-unit binding while
the host has a positive-vitality unit. The logger's `alive` field represents a
unit binding, not a direct client health measurement.

In the current bot run, mismatches span 21 and nine consecutive paired samples,
approximately twenty and eight seconds. Three samples is the failure trigger,
not their total duration. The baseline also crosses that threshold; subsequent
raw logs still disagree after their clock offset exceeds the comparator's
pairing tolerance. These runs prove the failure class predates the refactor;
they do not establish identical recovery timing or severity. A final one-shot
`(players)` conditional also failed in the current cases; it tests unit objects,
not session membership or continuous absence throughout its wait.

A final aim-only control completed 900 fresh ticks (1110–2010), using only
right-stick test input. Its strict verdict is also failed, with 840 drift
samples and no insufficient-sampling problem. The host reverted at tick 2012,
just after that window; fifteen paired samples then disagreed from 2040 to
2461, approximately fourteen seconds through shutdown. An earlier isolated
mismatch at 1920 did not reach the unchanged threshold. Thus the sustained
failure in this run occurred during the final confirmation wait, not within
the completed 900-tick window. The absent final unit-count receipt and failed
judge remain recorded; this is not an ordinary-co-op pass.

The responsible shared `network_coop`, `network_objects` and
`network_distributed` implementations are unchanged across this refactor.
Source inspection finds the live-unit recovery/replacement operations guarded
by `third_person_session_active`; both peers' recorded profile is
`opence_crossplay`. As documented in `NETCODE.md`, that profile retains the
existing checkpoint behavior and does not execute the separately versioned
recovery extensions. This supports a shared profile/recovery classification;
the exact stale-handle mechanism remains an inference because these logs do
not expose every object handle. It is distinct from console-syntax ownership.

No map bypass, profile change or guard broadening was made. Preserving OpenCE
crossplay and feature isolation precludes silently importing those extensions
into this cleanup. A separate shared-engine contract/version review is needed
before repairing or qualifying this recovery path. This is a retained failure,
not an MCC refactor fix or full co-op checkpoint compatibility sign-off.

### Legacy mixed-build checks and evidence audit

Original-main/current pairs passed on Xbox Blood Gulch and CE Death Island
in both host directions. Each completed 1,800 fresh post-admission ticks and
61 paired seconds with the unchanged strict comparator, normal process exits,
and no reported divergence or comparator problem.

| Map family | Host build | On-foot drift samples | Maximum position difference |
| --- | --- | ---: | ---: |
| Xbox | Original main | 1,822 | 0.079467 |
| Xbox | Current | 1,828 | 0.079051 |
| Custom Edition | Original main | 1,496 | 0.159609 |
| Custom Edition | Current | 1,518 | 0.079756 |

These establish bounded mixed-build behavior on the identified OpenCE 41
paths. They are not tests against an official upstream executable. Original
and current MCC peers were tested separately within their own protocol
versions; the earlier MCC version bump intentionally prevents mixed admission.

Independent verification binds every runtime session, raw logs, source
snapshots, settings, binary identities and supporting files. The final ledger
contains nineteen sessions: thirteen positive observations and six retained
failures (the first collector, the replaced Mercury checkpoint, and four Ruby
co-op runs). Its coverage-completed flag is true; its overall runtime-pass flag
is false. Archive readiness establishes integrity and process inactivity,
not compatibility. Additional support files are explicitly identified as
preservation-only where they received hashing rather than semantic decoding.
The state/pair verifier passed 37 negative/positive controls, the inventory
merger twenty, the frozen state/pair helper twenty and the cleanup gate 41.
These controls supplement the cycle controls; they are not gameplay samples.

### Final review, cleanup and delivery limits

The final combined implementation, integration, tests and reader documentation
were reviewed before the required regression checks. The resulting smoke-tool
correction received focused failure injection, the full local selection and
both fresh CI workflows. Subsequent changes record the completed runtime
observations and limits; they do not modify the tested implementation, workflow
or test inputs. Both workflows exclude documentation-only pushes, so their
successful results remain attributed to `f25c4886a`. The final document check
validates 232 local links, eleven named symbols, changed Python syntax and
the combined production boundary. No further game-code change or speculative
trim is justified by the review.

All 28 owned game processes exited normally. Their 168 generated map caches,
totalling 21,491,613,696 bytes, were removed after each run; none remain.
Cleanup removed 7,102 verified disposable fixtures and private input copies,
totalling another 440,218,858 bytes. No original maps or saves were removed.
The 1,097 inactive runtime save/support files and fifteen literal test-save
fixtures (745,741,325 bytes combined) were preserved losslessly in
`build/stage9/inactive-runtime-evidence.zip` (7,455,234 bytes). Its manifest
records original paths and SHA-256 values. Every archive member and loose
source was verified before removal of the loose copy.

The separate final delivery check reverified archive inventory, CRCs, sizes
and hashes, all 309 remaining loose runtime evidence files, exact session
inventory, process inactivity, missing disposable files and the cleanup
receipts. All fifteen original map/resource inputs and four executable/DLL
records were rehashed unchanged after cleanup. Roughly 29 MB remains under
`build/stage9`, including the archive, raw runtime/CI logs, small fixture-source
evidence and the original and final verification/cleanup receipts. Required
baseline and current builds remain at their existing locations.

Milestone 9's review and planned validation work are complete. The all-passing
runtime acceptance criterion remains unmet for Ruby co-op recovery; that
shared path is left unchanged under the standing crossplay and feature-isolation
constraints. Its original-main reproduction establishes a pre-existing failure
class, not parity of severity or proof against an indirect timing regression.
The branch is ready for delivery review with that explicit exception, not
unconditional compatibility sign-off. Full campaigns, live split-screen,
physical Android, browser-map gameplay, official upstream-binary crossplay and
long-duration leak/performance qualification remain unverified. The explicitly
deferred shared boat and console-syntax corrections remain unfixed. No pull
request description or merge is part of this milestone.
