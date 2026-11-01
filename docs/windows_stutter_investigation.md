# Windows irregular-hitch investigation

This records the initial investigation. The subsequent implementation,
validation and remaining limits are documented in
[Renderer preparation](render_preparation.md).

Players reported irregular hitches on Custom Edition and MCC maps in both
singleplayer and multiplayer, on two successive GulchCE builds. Reports
include Dynamic Lighting enabled and disabled; switching to current OpenCE
reportedly removed the hitches. This investigation does not yet establish
the cause of that difference or provide a gameplay fix.

## Scope and source findings

The inspected GulchCE revision is `d0b597a59`, built on Windows with
`--portable --release`. Upstream main was verified remotely on 29 October
2026 as `f2597b704d59e66257dba7629f22a0f78f5f3d09`, matching the local
upstream reference. An upstream executable was not run for this investigation.

The shared renderer's `program_get` in
[d3d8_gl.c](../port/linux/src/d3d8_gl.c) loads a saved program binary or
synchronously links a program when that cache misses. This work happens on
the drawing thread when a shader combination is first encountered. Both
upstream and GulchCE contain this mechanism.

The existing `--stalls` diagnostic in
[lighting_performance_build.py](../tools/lighting_performance_build.py)
predated the direct desktop link path. Its status-query matching also became
ambiguous when `program_cache_load` added another query with identical text.
`instrument_shader_stalls` now brackets compilation and both linking paths,
including their completion-status queries, without timing the cache loader
as a link. These hooks exist only in diagnostic builds. The regression tests
inspect the instrumented real functions and check the cache-loader exclusion.

The optional honeycomb surface is emitted into every translated pixel shader,
with a uniform selecting the disabled path. A diagnostic-only alternative
omitted its pixel-shader declarations and call. No production shader or
feature setting was changed. Removing this code did not eliminate shader-link
hitches, so this experiment does not justify changing the feature as a fix.

## Custom Edition runtime evidence

The successful runs used an NVIDIA RTX 5070 Ti, driver 617.42, Windows OpenGL,
1920x1080, a 120 FPS cap, V-sync off, Dynamic Lighting off, and the classic
shield look. They ran CE Death Island in a local match with seven Heroic bots
and automated player input. Audio and screenshots were disabled. Each process
was hidden, used isolated saves, disabled internet hosting, and exited normally
after 110 seconds. Pacing summaries cover process seconds 20 through 105.
The combat histories differed; the numbers are observations, not an A/B speedup.

| Diagnostic | Worst frame | Frames over 50 ms | Observation |
| --- | ---: | ---: | --- |
| Production renderer with timing hooks, empty application shader cache | 35.117 ms | 0 | A separate 32.953 ms frame included 2.132 ms compiling and 21.671 ms linking. |
| Honeycomb pixel code omitted, empty application shader cache | 254.669 ms | 1 | The worst frame included 3.177 ms compiling and 224.898 ms linking 15 programs. |
| Same experimental renderer, reusing its previous application shader cache | 34.711 ms | 0 | The worst frame included 26.604 ms linking one new program. |

The cached repeat made 78 binary-cache lookup attempts and linked four
programs, compared with 90 links in its preceding run. Its total measured
link time was 46.564 ms rather than 1,142.732 ms. Different combat histories
and driver-cache state prevent treating these totals as a controlled speedup.
They support cache misses as a contributor, while showing that a previous
playthrough does not necessarily cover every program a later one encounters.

The empty application caches do not imply cold driver caches. The separate
driver cache was not cleared or otherwise controlled. The temporary probe also
timed application binary-cache reads and writes; neither accounted for the
large link stall above. The worst production-renderer frame was not explained
by its measured shader calls, so shader work is not established as the only
source of slow frames.

Three earlier sandboxed runs failed their local loopback connection and stayed
in menus. They are excluded from gameplay conclusions. Retrying outside the
sandbox reached gameplay.

## Matching Digsite reproduction

The reporter subsequently identified an RTX 5070 Ti and
`mcc_maps/DigsiteMP/10.map`. The installed Digsite package was found outside
the build directory. The map, its bitmap/sound resource files, and menu data
were copied into an isolated test directory; original maps, configuration,
saves and shader caches were not modified.

These runs used the same revision and GPU above, 2560x1440, a 120 FPS cap,
V-sync off, MSAA 8x, 16x anisotropic filtering, highest model detail,
1024-pixel shadows, per-pixel lighting, first-person legs and viewmodel
shields. The user's shield settings were Shield Glow enabled and
`classic_honeycomb2`. Other Dynamic Lighting switches were already off.
Audio processing remained enabled with volume zero. Each hidden process
ran a local Slayer match with seven Heroic bots and automated player input
for 160 seconds, with isolated saves and internet hosting disabled.

The temporary probe timed shaders, simulation, rendering, input, UI,
network frame work, presentation and MCC texture handling. It retained
frame measurements in memory and wrote them on exit. Summaries below use
gameplay frames at simulation tick 600 or later, excluding map startup
and the first 20 simulation seconds. Rendering contains the shader and
texture measurements: these columns are nested, not additive.

| Run | Shader-cache seed | Worst settled frame | Frames over 50 ms | Observation |
| --- | --- | ---: | ---: | --- |
| User settings | A copy of the player's existing cache | 74.868 ms | 1 | 62.433 ms linking three programs, 4.056 ms compiling, 0.572 ms in simulation. |
| Dynamic Lighting disabled | Cache saved by the preceding run | 12.974 ms | 0 | Worst frame included 9.480 ms linking. |
| User settings restored | Cache saved by the disabled run | 30.196 ms | 0 | Worst frame included 26.710 ms in MCC texture handling and no shader compile/link calls. |

The first run also recorded a separate 27.083 ms frame with 26.010 ms in
MCC texture handling and no shader compile/link calls. The restored-settings
run still encountered one 20.195 ms frame containing 17.390 ms of linking.
Its final program counters were 126 binary-cache loads, three links and no
driver-refused binaries. Thus cache reuse reduces observed shader work even
with Shield Glow restored; it does not cover every later shader combination.

These are different bot combat histories with progressively populated
application and driver caches. The disabled run is not a controlled measure
of the cost of Dynamic Lighting, and none of these timings establishes a
general speedup. The measured 75 ms hitch was predominantly synchronous
shader work, rather than a simulation or network-frame stall. That does not
rule out other causes in other frames or sessions.

MCC texture timing initially wrapped the entire `xgpu_mcc_texture_get` in
[mcc_texture_bridge.c](../port/linux/src/mcc_texture_bridge.c), including
identity lookup, palette handling and first-use upload. It must not be
reported as GPU upload time alone. Source inspection shows retained GPU
textures, with replacement on P8 palette changes and cleanup on disposal.
The adapter does not rebuild every texture on every frame.

A fourth run restored the same user settings and reused the third run's
cache, with additional clocks inside `xgpu_mcc_texture_upload` in
[xbox_textures.c](../port/linux/src/xbox_textures.c). Its worst settled
frame was 15.104 ms: 8.916 ms in MCC texture handling, including 2.300 ms
decoding pixels and 0.762 ms in texture image upload calls for 18 texture
preparations. No program linking occurred in that frame. The remaining
texture-handling time includes allocation, GL state changes and other
adapter work; the probe does not attribute it more precisely.

That run also measured two 2048x2048, 12-level texture preparations during
startup at 23.022 and 21.581 ms. Respectively, 9.653/8.834 ms was CPU decode
and 8.777/8.352 ms was spent in GL texture image calls. These startup events
demonstrate first-use preparation costs but do not identify the exact
textures responsible for the earlier settled 26 ms measurements. The fourth
run did not reproduce those long settled texture stalls. Its upload detail
messages were emitted only after preparations over 2 ms; extra logging and
clocks make this diagnostic unsuitable as a clean performance comparison.

The evidence now includes a reproduction on the reported map and GPU.
Campaign, remote multiplayer, AMD and Intel were not tested. A controlled
OpenCE executable comparison on a shared Xbox/CE map remains necessary to
attribute the reported fork-specific difference; OpenCE does not provide
the same MCC loading path. Shader/resource preparation during gameplay is
an established source of hitches, but a safe gameplay fix is not established
by this investigation.

## Validation and retained evidence

The normal Windows release build passed. The corrected diagnostic backend
and the Digsite diagnostic variants compiled separately, and the nine focused
tests in
[test_lighting_performance.py](../tools/test_lighting_performance.py) passed.
The review covered timing boundaries, the direct-link and binary-cache paths,
test assertions, MCC texture ownership, and the limits of the runtime evidence.
No game, network,
map-family, or optional-feature behavior changes were made. Android and web
builds were not run for this Windows diagnostic-tool correction.

Local scripts, settings, build metadata, frame CSVs, logs, summaries and
verified save archives are under `build/stutter-20261029` and
`build/stutter-digsite10`. These local outputs
are not committed. The experimental renderer is not a proposed gameplay fix.
Generated map caches and temporary diagnostic binaries were removed; the
normal Windows build and original maps and saves were preserved. The Digsite
cleanup additionally removed its copied map/resource files after comparing
their hashes with the originals: 1,158,338,456 bytes removed, approximately
16 MB retained as diagnostic evidence. Save archives were verified before
removing their loose copies. Production source hashes and local document
links were checked after testing.
