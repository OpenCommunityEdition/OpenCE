# Renderer preparation and irregular hitches

The [Windows investigation](windows_stutter_investigation.md) measured shader
linking and MCC texture handling on gameplay frames. The preparation controls
move some of this work into loading without changing simulation, network
messages, texture quality or map-family selection. All controls default off.

```toml
[display]
shader_prepare = true
texture_prepare_mb = 128
mcc_texture_prepare_mb = 128
```

These controls are available on Windows and Linux OpenGL. They do not promise
stall-free gameplay: a shader pair outside the loading pass is still prepared on its first draw,
and textures outside the preparation budget retain their existing lazy path.
The tradeoffs are loading time, program memory and earlier texture residency.
The texture budget estimates image storage; it is not a driver memory limit.
Changes to these preparation controls take effect on the next map load.

## Reading the shader path

1. `game_load`, `game_initialize_for_new_map`, `game_unload` and `game_dispose` in
   [game.c](../source/game/game.c) supply map boundaries. A failed load clears
   the preceding map's preparation state too. These small shared hooks are
   necessary because the GL backend does not own map lifecycle or identity.
2. `render_prepare_map` in [d3d8_gl.c](../port/linux/src/d3d8_gl.c) checks that
   the GL device exists and passes its existing binary-cache services to
   [shader_prepare.c](../port/linux/src/shader_prepare.c). GLES stays a no-op.
3. `shader_prepare_begin` validates a map's recipe file before issuing GPU
   work. It loads matching driver binaries or compiles the saved GLSL pairs.
   Missing pairs are submitted before completion checks; drivers supporting
   parallel compilation expose completion status, with synchronous validation
   retained elsewhere. The feature never draws an incomplete program.
4. `program_get` records actual shader pairs in memory and takes a prepared
   program only when both source hashes match. It initializes the same uniforms
   and uses the same failure/fallback path as before. Transferred programs
   belong to the existing renderer cache; unconsumed ones belong to preparation.
5. Map unload saves the recipes and releases unused prepared programs. Process
   exit also saves CPU recipe data, without calling GL from an exit handler.

For example, the first visit to CE Death Island records a weapon/effect pair
when it is drawn. A later load prepares that pair before gameplay. The recipe
contains source rather than a driver binary, so it can also repopulate the
driver cache after a driver change. A new combination of render state is not
covered until observed unless selected by the material pass below. The recipe
part remains repeat-visit preparation, not exhaustive shader discovery.

Each map identity includes its full family/package-qualified path. The recipe
format permits at most 512 pairs, 64 KiB per source and 16 MiB of source data
per map. Lengths, termination, source hashes, record count and trailing bytes
are checked before compiling. Incomplete or malformed files are ignored and
can be rebuilt by recording valid draws. Recipe recording performs no file I/O
on a draw; unseen pairs still use the renderer's existing binary-cache/link path.
Changing settings can encounter new source hashes; an obsolete source cannot
replace a different current shader. Cached recipes are local derived data,
not game assets intended for distribution.
Old recipes remain until the map's bounded recipe file is removed; source
changes can therefore leave unused entries consuming some of its capacity.

## Preparing unseen materials

[material_prepare.c](../port/linux/game/material_prepare.c) runs after map
initialization, before gameplay. It asks the existing model and transparent
renderers to select shaders for loaded environment, model, generic and Chicago
materials, using supported vertex formats and fog/lighting variants. It does
not simulate a scene or submit geometry. Limits are 512 materials, 8,192 draw
variants and 4,096 temporary bitmap descriptors. These are admission limits,
not a promise to cover every map or setting combination.

The draw interception in `rasterizer_draw` and
`rasterizer_transparent_geometry_group_draw__internal` is necessary because
the existing material selectors combine state selection with geometry
submission. The feature-owned pass supplies dummy geometry metadata and the
hooks call the backend's ordinary shader selection without drawing it. Extra
transparent layers needing real geometry, such as water, glass and plasma,
are skipped during preparation and retain their normal gameplay paths.

The small `texture_cache_prepare_descriptor` wrapper reuses native descriptor
validation and construction without fetching Xbox/CE pixels. Its call is
behind the loading-only feature guard; the existing MCC branch runs first.
MCC descriptors continue to come from `mcc_texture_cache_get`, and
`xgpu_mcc_texture_describe` reads only registered MCC metadata. This separate
lookup is necessary for MCC dimensions that cannot be encoded by a native
texture header. Neither path uploads pixels during material selection.

The pass restores the window parameters and statistics, pairs renderer
begin/end calls, clears temporary texture bindings, frees descriptors and
turns its interception off before returning. Shader programs remain in the
existing source-keyed renderer cache, as programs encountered in gameplay do.
Material variants do not fill the observed-recipe file unless actually used
in gameplay. This trades additional loading time and program residency for
fewer first-use links; it does not change shader math or drop gameplay draws.

## Xbox/CE texture preparation and Windows allocation costs

[texture_prepare.c](../port/linux/game/texture_prepare.c) enumerates loaded
bitmap tags only when its own budget is enabled and MCC is inactive. It
prioritizes large non-paletted cached textures and calls the existing blocking,
validated texture-cache API. The 128 MiB maximum is half the native 256 MiB
texture arena. Each candidate is charged for the larger of its native pixel
size and estimated RGBA mip-chain storage, including row alignment, cube faces and cache page
rounding. Failed preparations remain charged, and normal lazy loading remains
available. The existing CPU and GPU caches retain ownership and invalidation;
this feature adds no separate residency pool.

Two general Windows/desktop fixes also reduce first-use costs with preparation
off: [xgpu_texture_names.c](../port/linux/src/xgpu_texture_names.c) batches
unused GL texture names, and
[win32_memory_watch.c](../port/windows/src/win32_memory_watch.c) protects
contiguous unwatched page runs together. The latter falls back to individual
pages across separate Windows reservations and keeps per-page write-generation
tracking. Texture names allocate no image storage; ownership transfers to the
existing cache when consumed. Only unused names are deleted at map boundaries.
Shader preparation optionally reserves 1,024 names during loading.

Android and web keep individual texture-name allocation. All preparation
modules are no-ops under `HALO_GLES`, and the Android CE cache, mirror uploads,
static geometry, texture invalidation and disabled dynamic-lighting modules
retain their established paths. The shared honeycomb shader specialization
below also applies to GLES; it changes source scope, not the enabled effect's
math. Compilation and host-side tests do not replace an Android device test.

## Reading the MCC texture path

`mcc_cache_tags_load` invokes `mcc_bitmaps_prepare_gpu` only after conversion
and validation succeed and MCC ownership is active. Both functions are in
MCC-owned modules: [mcc_cache.c](../port/linux/game/mcc_cache.c) and
[mcc_bitmaps.c](../port/linux/game/mcc_bitmaps.c). No Xbox or CE loading hook
is used by this MCC preparation pass. The separate Xbox/CE preparation
feature never routes their resources through the MCC adapter.

The pass prioritizes large converted allocations. It registers them through
the existing [mcc_texture_cache.c](../port/linux/game/mcc_texture_cache.c)
adapter and asks `mcc_texture_bridge_prepare` in
[mcc_texture_bridge.c](../port/linux/src/mcc_texture_bridge.c) to prepare those
fitting the remaining budget. Small textures, palette-dependent P8 textures,
and active high-resolution HUD replacements retain their own paths. Failure
leaves the normal first-use path available. Existing MCC disposal releases
prepared textures along with other MCC textures; no separate ownership pool
or package-wide unbounded upload is introduced.

## Avoiding redundant desktop shaders

Desktop translation also canonicalizes dead alpha combiners in
`discard_dead_r0_alpha`. Model material selection can retain a preceding
material's unused alpha stage, creating different GLSL and extra links despite
identical output. The pass clears only alpha portions writing discard or
`r0.a` when no later input, mux selector or final combiner can read `r0.a`.
It retains other destinations and bypasses normalization for diagnostic
expressions that may inspect registers. GLES translation is unchanged by this
optimization. Tests exercise explicit reads, muxes, implicit final output and
debug expressions; no gameplay render state is modified.

## Honeycomb shader scope and experiment

`prepare_draw` sets `nv2a_pixel_shader_key.shield_honeycomb` from the existing
draw-local activation value. `nv2a_pixel_shader_to_glsl` emits the existing
surface code only for that variant. Shield draws keep the same surface math,
uniforms and settings. Ordinary draws omit a surface that previously returned
their original color immediately. Xbox, CE and MCC follow the same activation
contract; no map-family defaults change.

The independent omission experiment is retained in
[render_stutter_build.py](../tools/render_stutter_build.py) as
`--omit-honeycomb`. It removes the surface entirely from a temporary diagnostic
build and must not be used as a production fix or visual-equivalence test.
The builder instruments scratch copies, preserving production sources and
objects. [render_stutter_run.py](../tools/render_stutter_run.py) runs hidden,
isolated matches, records frame timing, archives verified saves and
removes each run's temporary binaries and generated map caches.
The diagnostic `--record-prepared` option also saves speculative material
sources for coverage investigation; production records observed draws only.
Use the builder's `--deterministic` mode for a single-process controlled
comparison: split-screen matches use the network-server path, which otherwise
ignores the engine's ordinary fixed-step option. This mode patches scratch
copies to use a fixed clock and both engine and bot random seeds; never use it for multiplayer
compatibility checks or normal frame-rate acceptance measurements.
The runner's `--compare <reference-game.log>` checks sampled player state,
excluding local machine-slot numbers that differ with a build's bot capacity.
Missing or different samples fail this comparison. Matching player samples
are useful evidence, not proof that every world object or GPU operation matched.

Compare cold and warm application caches separately, verify matching simulation
traces, and report the uncontrolled driver cache. A change in average FPS alone
does not establish a hitch improvement. Required checks include optional
settings off/on, shader failure, malformed recipes, bounded uploads, palette
changes, map-family transitions, visual output and network compatibility.

## Measured results and validation limits

Windows release testing on an RTX 5070 Ti (driver 617.42) used CE Death Island
and `mcc_maps/DigsiteMP/10`. The diagnostic builds use fixed simulation ticks,
seven bots and a 120 FPS cap. The settled comparison window is ticks 600–1200,
601 frames; driver caches were not controlled. Timings include instrumentation
and are not a general FPS prediction.

The CE warm-cache control's worst frame was 27.797 ms. The final prepared run
was 8.747 ms, with all 40 sampled player states matching the control after
normalizing machine-slot IDs. The measured respawn/new-view hitch at tick 1057
fell within that final bound. Texture-name allocation, page protection and
first-use texture uploads were measured separately before selecting the fixes.

On Digsite at 2560×1440, MSAA 8x, highest model detail, per-pixel lighting,
classic honeycomb and Shield Glow enabled, the final settled interval had a
9.568 ms worst frame and no shader links or frames above 16.7 ms. The preceding
material-pass run still linked five programs in that interval. Source inspection
found unused alpha-stage differences in the missing variants; the conservative
desktop normalization removes those differences. MCC bot traces did not match
across repeated controls, including before the material pass, so these runs do
not establish a controlled end-to-end speedup or the cause of the reported
OpenCE/GulchCE difference.

The full early-gameplay log is retained too: the first rendered gameplay frame
(tick 3) reached 18.307 ms on CE and 74.788 ms on MCC; the latter included one
20.794 ms shader link. Unseen effects and resources outside the loading passes
remain first-use work. The result addresses the measured recurring hitch, not
every startup stall or every map. Removing honeycomb entirely in the separate
experiment did not eliminate the hitch, so the shipping change preserves it.

The final self-review covered all modules, shared hooks, settings, diagnostics
and tests. It corrected partial shader-allocation cleanup, conservative native
row-pitch budget accounting and preservation of diagnostic register reads.
The final Windows release build passed, and the focused regression suite passed
310 tests, with seven platform-dependent skips and five passing subtests.
It includes real C tests for recipe validation/failure/ownership, texture
budgets and guards, Windows protection across reservations, MCC descriptors,
shader translation and the existing Android renderer fixes.

Normal, uninstrumented Windows binaries also passed two-instance loopback
checks: new host/OpenCE client on CE Death Island, OpenCE host/new client on
Xbox Blood Gulch, and new host/testing-baseline client on MCC Digsite 10.
`net_compare.judge_run` reported no divergences or problems over respectively
52, 52 and 42 compared seconds, with damage, pickup and death cues enabled.
The references were OpenCE `f30c7a406` and testing `e83a45e49`. These are local
compatibility checks, not long-session or Internet latency coverage.

Two clean-exit sequences, with preparation off and on, traversed Xbox b30,
MCC Digsite 10, CE Death Island and Xbox Blood Gulch. Every phase advanced
beyond 150 simulation ticks. Captures at 1280×720 were inspected for scene,
texture and HUD output with no obvious regression; they are not pixel-identical
comparisons or full campaign validation. Delayed console-print markers were
not observed because default chatter filtering suppresses them after the
command yields; tick logs and captures establish the narrower transition
result, not completion of those diagnostic scripts. The first two network
attempts lacked the stock lobby map and never entered gameplay; corrected
fixtures were rerun successfully and the original failed reports retained.

All 30 affected Android/web translation-unit checks passed. Full Android and
web builds were attempted but blocked by the Windows host's command/toolchain
setup (Android compiler wrapper/SDL configuration and web shell commands).
No Android device, browser gameplay, Linux runtime or Internet/WebRTC run was
completed. Network messages and simulation rules are unchanged, so this work
does not change the network version. Device validation remains necessary;
cross-compilation and host tests do not establish Android visual equivalence.

Local evidence is retained under `build/stutter`: frame CSVs and reports,
settings and executable/source hashes, source-difference records, platform
logs, compatibility captures and verified save archives. The normal Windows
release build is retained for delivery. Copied maps are hash-checked against
their originals before cleanup; diagnostic binaries, reference build copies,
generated caches and disposable cross-build outputs are removed. Original
maps, saves, installed toolchains and the primary checkout are preserved.
