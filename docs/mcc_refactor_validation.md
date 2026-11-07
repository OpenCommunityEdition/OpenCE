# MCC refactor validation baseline

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
