# Energy, vehicle and world lighting: local validation

This records the `dynamic-emitter-expansion` work based on testing commit
`0bc618eac6a4d22630a4e74fa3b606fe780e8df7`. It does not replace the historical
1.0 acceptance or certify every device or custom map. The
[implementation guide](reading/emitter_expansion.md) explains the profiles,
controls, reading route and lifecycle.

**The current target is sustained 240+ FPS in busy combat; acceptance remains
open.** The earlier 120 FPS investigation below is historical evidence, not
acceptance against the new target. The November optimisation section records
the follow-up work from commit `0973085ab3a7110e758bc41d77ffba622ff02571`.

## Implemented scope

- Elite energy swords; projectile plasma grenades; held plasma-pistol
  charge and plasma-weapon heat lights reuse their authored attachments.
- Ghost, Banshee and Covenant dropship engine/running illumination, plus
  Warthog and Scorpion headlights. Vehicle sources request chassis blocking
  even with general Light Occlusion off.
- Flamethrower streams use one bounded, smoothed sample per firing owner.
- Visible overshield/active-camouflage pickups and the green teleporter
  surface cast local light. Collected pickups and unlit teleporter bases do not.
- Both settings frontends group Weapon Glow and Shield Emitters, add Vehicle
  Lights and World Emitters, and retain the separate projectile, muzzle,
  occlusion and host-governed Shield Glow controls. Old individual preferences
  remain supported; mixed groups display CUSTOM without overwriting them.

All new settings default off. Stock models, particles, explosions, damage,
vehicle controls and network simulation remain unchanged. Android excludes
the feature modules and uses inert hooks. No shader translator, shader-cache,
AMD/NVIDIA workaround or map-family loader was changed.

## Review and optimisation

The combined review covered the new modules, existing emitter pool,
attachment restoration, occlusion registration, menus, generators, platform
stubs, fixtures and documentation. An additional game-source argument
exposes the private light-definition identity at the existing render hook;
the rationale is in the implementation guide. Feature policy stays in port
modules, and that source change retains its `port:` marker.

Exact tag identities are cached for the current map. Enabled categories
restrict object scans; a projectile-only configuration with no live flame
stream skips the emitter-owner walk. Flame grouping and ambient lights remain
bounded, with no new independent GPU pass or increased emitter budget. An
existing held-weapon/shield emitter can reclaim the weakest ambient slot when
the shared pool fills, preventing the additions from starving baseline lights.
The saturation regression also checks that stable lights are not recreated.

The review corrected a broader-than-needed menu-save fallback, regenerated
the optional frontend's stale settings page, and retained all existing shader
and compatibility checks. Shared changes target measured costs; unrelated
renderer cleanup and future shadow features remain outside this change.

### CPU/GPU investigation

At 2560x1440, 8x MSAA, 16x anisotropic filtering and highest model detail,
the initial 15-bot Blood Gulch comparison measured 30.00/59.97/113.81 FPS
at the three caps with lighting on, and 120.00 FPS at the 120 cap with it off.
These were 60-second measurements after 15 seconds of warmup, without
screenshot readback. They establish a local shortfall, not a device-wide limit.

Sampled profiling attributed about 2.07 ms per frame to collision batches,
0.71 ms to visibility projection/filtering, and 0.42 ms to texture preparation.
Sampled GPU draws totalled about 1.38 ms. These diagnostics have overhead;
nested CPU timings must not be summed. GPU draw queries do not cover resolves,
presentation or every driver wait. World BSP queries dominated the collision
cost, making CPU-side lighting the primary measured optimisation target.

The current candidates retain ray count, update cadence, filtering and light
budgets. Lighting proves a common collision-tree prefix for a ray cube and
passes that entry to the existing traversal. A bounded 8 KiB batch cache
reuses node/plane data for the identical BSP and origin. Stock callers still
enter at the root with no cache. Original traversal depth and node budgets
include the skipped prefix. A same-pose comparison of 34,913,280 rays found
zero differences against stock or uncached batches; cached batches took
8.53 seconds cumulatively versus 9.38 seconds without that node cache.
That is a batch cost comparison, not a whole-frame FPS improvement.

Unchanged lighting texels reuse the last successful upload, checked byte for
byte. This uses a fixed 384 KiB across all fields and skips memory watching,
texture decoding and GPU upload only when the result is identical. A failed
upload invalidates that copy. Lighting also caches its local boolean options
until `config_changes()` advances, avoiding repeated locks and full settings
table searches. Host policy remains independently evaluated.

Successful pass textures also survive between views when all texel inputs
remain identical. Rotation invalidates directional profiles independently.
The disabled reset path does not repeatedly clear already-empty field storage.
A proposed early trace-refresh scheduler was removed: its combat measurement
did not demonstrate a gain, so the original refresh cadence remains intact.

The resulting build measured 30.00/60.00/118.10 FPS at the respective caps.
A separate three-minute run with the limiter explicitly disabled measured
140.12 FPS overall, with minute averages of 142.46/145.19/132.70. Its p95/p99
frame times were 10.40/12.49 ms, worst 27.96 ms, with no frames over 50 ms.
This establishes local average headroom, not a locked 120 FPS or universal
hardware acceptance. Combat evolves between runs; timings are not a
deterministic replay or an exact causal percentage improvement.
A later disabled 120-cap control averaged 114.55 FPS and contained twelve
frames over 50 ms, whereas the initial disabled control held 120.00. Its cold
shader-cache log and timing record are retained; the cause of those stalls was
not established. They are not silently discarded or attributed to lighting.

Benchmark correction: early `*-headroom` reports used `max_fps=0`, which
means twice the display refresh rate. Their `uncapped` flag was incorrect.
They remain preserved with a correction record and are not used as unlimited
measurements. `emitter-unlimited-final` explicitly records `frame_limit=-1`.
The runner now records the actual limiter and bounds memory windows before
shutdown, avoiding a false plateau from exit-time deallocation.

The corrected unlimited pre-optimisation comparison averaged 120.94 FPS over
one minute (p95/p99 13.68/16.45 ms). The final run's frames over 8.33 ms averaged
61 selected lights and 2.99 ms of occlusion work, versus 32 lights and 1.67 ms
within budget. This correlation points to crowded-light frames for further
profiling, rather than a blanket GPU rewrite or reducing light quality.
Frames with a simulation tick were slower on average, but render-only frames
also exceeded budget. Bounded process-memory windows averaged 657.28 then
659.03 MiB during the final run; this small increase is not proof of a plateau.

## Automated and menu checks

The final focused suite passed **150 tests**, with **one platform-dependent
skip**. It exercised production profile matching, map cache invalidation,
missing assets, frame restoration, split views, allocation limits, flame
aggregation, emitter priority, vehicle occlusion, stock shader/GPU isolation,
host shield policy, generated menus, Android exclusion and MCC shader checks.
These are code-level checks, not hardware or whole-map compatibility proof.

Windows builds passed. The touch-menu C unit also passed with the installed
toolchain's explicit x86 target; its generic pytest command attempts to link
Unix `libm` on Windows and fails before running the unit.
Final ordinary/full-LTO light-storage checks also passed, including the
intentional pointer-corruption negative control. The final document/source
audit checked 185 local links and 95 recorded source hashes; the normal binary
contains no capture, policy-probe or trace-comparison fixture identifiers.

Hidden real-game menu checks verified fresh defaults, every consolidated
control, Save, Cancel, Defaults followed by Cancel/Save, process restart,
unchanged CUSTOM values and independent weapon/shield groups. The classic
screen was inspected at 640x480 and the optional GulchCE frontend at 1280x720.
The latter also passed grouped Save, Cancel, Defaults and restart checks.
The production executable was checked for absence of capture/profiling
fixture identifiers.
After the settings-cache change, Save, Cancel, Defaults and process restart
were refreshed successfully in the real classic menu. A supported projectile
probe repeatedly toggled occlusion at runtime and verified that disabled
intervals performed no occlusion updates.

## Rendered coverage

Captures used one hidden Windows instance, an NVIDIA RTX 5070 Ti with driver
617.42, per-pixel lighting and 1280x720 output. Xbox Blood Gulch/c40, CE Death
Island and MCC DigsiteMP were exercised at 30, 60 and 120 FPS caps. Real object
attachments, enhanced model draws and occlusion submissions were logged;
representative frames were visually reviewed. Test fixtures are not shipped.

The cases covered sword-bearing Elites, plasma grenades, held charge/heat,
Ghost/Banshee/dropship sources, both headlight vehicles, both powerups,
teleporter scenery and Xbox/CE flame streams. Off controls for the new groups
selected no enhanced lights and made no optional occlusion updates.

Two capture limitations are retained rather than counted as passes:

- An ordinary MCC grenade placement disappeared on its first simulation
  update in a narrow doorway and never submitted a visible light. Controlled
  nearby grenade probes rendered correctly at all three caps. Those probes
  freeze projectile simulation: they validate rendering, not MCC throwing,
  sticking or detonation gameplay. Xbox natural-projectile captures provide
  separate flight/impact coverage.
- An initial 60-cap Banshee capture averaged 54.94 FPS. Its focused repeat
  averaged 58.93, versus 60.00 disabled. The original hitch remains recorded.
  The earlier 60-cap plasma-rifle heat capture also missed its timing target;
  a visible effect is not evidence of sustained performance.

One MCC pickup run reached the short exit deadline before the fixture began.
The runner now allows package loading more time while preserving the same
85..240-tick measurement window; incomplete runs remain failed records.

Final optimisation checks refreshed CE Ghost and MCC pickup captures at all
three caps, Xbox held-AR captures at 30/120 and rotating first-person AR at 60,
plus a campaign Grunt/Needler case at 60. Representative images were inspected;
these short captures met the runner's 5% timing tolerance. Three final ray
comparisons covered 924,672 CE, 465,408 MCC and 370,176 campaign Xbox rays with
zero differences from both stock and uncached traversal.

Invalid fixture combinations remain recorded as failures: Blood Gulch lacks
the requested Marine/Grunt actor tags, so supported player/campaign fixtures
replaced those attempts. The held-weapon tick handler also never executed the
projectile toggle control. The runner now rejects that combination before
launching; separate rotation and projectile-toggle runs passed. These setup
failures are not treated as valid effect or lifecycle coverage.

## Platform and acceptance limits

Refreshed two-instance Xbox, CE and MCC local compatibility runs passed with
differing lighting preferences, joining, respawning, weapon changes and host
policy on the final implementation. The comparator found no divergences over
81 paired snapshot seconds for Xbox/CE and 74 for MCC. This is local transport
and representative-map coverage, not an internet or full-campaign run.

Full `ninja android` and `ninja web` were refreshed with the installed NDK and
Emscripten outside the sandbox. Unix shell operators were passed literally to
helper executables; the web build also encountered its Python launcher setup.
A targeted web-object attempt still stopped at the SDL shell prerequisite.
The normal Windows configuration was restored. WSL is not installed.
Build-graph, inert-hook, menu and shader isolation tests
passed, but full Linux/Android/web builds still require a supported builder/CI.
No Android device or AMD GPU runtime result is claimed.

The baseline's [performance and memory limitations](dynamic_lighting_release.md#known-limitations)
remain. Local visibility fields approximate occlusion; model lighting retains
its existing two-light limit. Authored world spotlight cones survive, while
model lighting retains the baseline point-light approximation. Unknown custom
tag names and missing markers remain stock. Bounded pools can reduce optional
coverage in crowded scenes. Neither short captures nor capped timing samples
establish universal 120+ FPS, a memory plateau or exhaustive map compatibility.

Local logs, build/source hashes, reports and representative frames live under
`build/emitter-expansion`, `build/plasma-visual`, `build/lighting-performance`
and `build/lighting-compatibility`. Only this assignment's staged maps and
caches are removed; test saves are archived losslessly and verified first.
Final retention removed approximately 2.45 GB of this assignment's redundant
test binaries, raw/duplicate captures, interrupted-run map caches and loose
archived saves, after an earlier 1.02 GB cleanup. A final inventory confirmed
that no staged map files remained in this assignment's run folders.
About 383 MB of representative visual/menu evidence,
reports and archived saves remain, alongside small manifests/logs, the normal
build and one passive performance build retained for the open performance
investigation. Original maps, user saves and other worktrees were untouched.

## November optimisation: 240 FPS target (in progress)

This follow-up uses 2560x1440, AA off, 16x anisotropic filtering, highest model
detail and per-pixel lighting on the same RTX 5070 Ti/617.42 setup. Release
comparisons use full LTO and the existing PGO profile. Earlier rapid builds
with both disabled are labelled separately in their manifests and must not
be compared as equivalent release builds.

Plasma prongs now have a separate, default-off control in both frontends.
Their short forward spill and camera-distance fade retain stock flare art;
the existing Weapon Glow group cannot enable them. Prongs still share the
eight-field budget. The guide records their smaller ray grid; other sources
retain their existing grids and attenuation profiles.

Retained optimisations are conservative object rejection outside a ray batch's
cube, exact local collision snapshots for stationary-field reuse, cached
stationary visibility targets, seven-step trilinear interpolation and bounded
model texture/uniform/binding reuse. No simulation or networking code changes
are part of this follow-up. The binding cache owns only units 6/7 and expires
at view/reset boundaries. Failed texture resolution retries; sampler failure
retains the explicit texture-parameter path. Desktop and ES harness cases
exercise those paths and active-unit preservation.

The collision broad phase matched stock and uncached queries for 14,902,656
checkpoint rays with zero mismatches. That is a diagnostic result, not a
timing sample or proof for every map. The earlier model-node cache candidate
had no demonstrated benefit and was removed. A texture-coordinate candidate
regressed the release checkpoint measurement (about 172 versus 231 FPS in
its control); it was also removed.

The first two-minute, 15-heroic-bot Derelict run averaged 185 FPS with lighting
and 392 FPS with it disabled. These are independently evolving combat runs,
not identical replays. The lit second minute averaged 165 FPS. Bot telemetry
confirmed kills, deaths and respawns; captures confirmed combat. This fails
the sustained 240 FPS target. A later instrumented run averaged 243 FPS but
its second minute averaged 173 FPS: its headline average is not acceptance.

The user's copied Two Betrayals checkpoint is immediately before the snowy
Covenant encounter. Five seconds of ordinary forward controller input reaches
the exterior; a further 0.6-second right turn faces the fight. No teleport,
actor substitution or mission-script changes are used. These route captures
are diagnostics, with frequent screenshot readback excluded separately from
raw timings. Original maps and saves remain untouched.

In that same route, the model-binding change reduced the inclusive binding
timer from 4.58 to 3.46 ms per frame with approximately 115 binds per frame
in both runs. Diagnostic throughput increased from about 138 to 164 FPS;
neither is a sustained acceptance result. Trace time remained about 1.60 ms,
visibility reconstruction 0.83 ms and texture preparation 0.72 ms. Timers
overlap: binding includes field work, so these columns must not be added.
The remaining work is active optimisation, not a declaration that this
performance is an acceptable limitation.

The subsequent exact row-reuse change reduced field reconstruction from
0.83 to 0.69 ms in the same route; texture preparation was 0.69 ms with the
half-scale specialization. The combined diagnostic run averaged about
174 FPS. Ten focused occlusion/binding tests passed, including exact
per-cell history and half-scale sampling comparisons.

The subsequent passive release runs retained only sparse captures, excluded
their readback frames from pacing, and also recorded the raw distributions:

| Workload | Measured time | Mean FPS | 1% low FPS | p95 frame time | Frames above 4.17 ms |
| --- | --- | --- | --- | --- | --- |
| Two Betrayals snowy encounter, all lighting | 120 s | 193.3 | 128.7 | 6.68 ms | 89.5% |
| Derelict, 15 heroic bots, all lighting | 120 s | 279.8 | 146.8 | 5.37 ms | 34.2% |
| Same snowy route, lighting off | 60 s | 607.4 | 367.2 | 1.93 ms | 0.025% |

These independently evolving fights are not frame-identical replays. The
snowy lit run's second minute averaged 199.8 FPS; the bot run's second minute
averaged 264.3 FPS. Captures show active outdoor combat and bot telemetry
records kills, deaths and respawns. Neither lit result meets sustained 240
FPS. The stock control identifies substantial remaining feature overhead;
it does not establish that every stock encounter meets the target. The
remaining measured costs are primarily CPU collision and field/texture
preparation, not the added GPU lighting passes. No broad shared-renderer
change is retained in this follow-up.

No crash or shader error was reported in those runs. Private memory rose
about 2.4 MiB between windows in the snowy run and 4.3 MiB in the bot run;
these short runs do not establish a plateau or resolve the earlier memory
investigation.

The combined implementation review retained bounded caches, conservative
fallbacks, unchanged host policy and Android exclusion. The focused suite
passed 82 tests, covering real field/collision calculations, menu staging,
activation, GL/ES binding state and shader/Android isolation. Runtime menu
checks passed independent prong Save, Cancel, Defaults and restart persistence.
The modern short label was inspected at 640x480 without clipping. Xbox/CE
prong captures passed at 30/60/240 FPS; the CE 120 FPS capture exercised
occlusion toggling, and a disabled prong capture selected no enhanced lights
and performed no occlusion updates. These small visual fixtures do not prove
busy-combat performance.

MCC Digsite validation is incomplete. Its stock Plasma Rifle weapon tag is
absent; the tag named Plasma Pistol is a custom weapon in the rendered view.
The capture runner now starts this multiplayer package as an isolated local
match, excludes menu-scene frames, and reports unavailable/rejected weapons.
The pistol equipped successfully, but neither that prong case nor the rifle
projectile fixture selected a light during the measured interval. Those are
failed lighting captures, not passing MCC acceptance or proof of a renderer
regression. Their logs and frames are retained; this remains an unresolved
map-specific validation issue. No MCC adapter or gameplay code was changed
to force the fixtures to pass.

Windows release and capture builds succeeded. Full Android and web builds
were attempted with the installed NDK/Emscripten, but stopped at missing Unix
shell helpers (`sed` and the SDL copy prerequisite). The original Windows
configuration was restored. The passing exclusion/inert-hook and GLES tests
are not substitutes for those full builds. No AMD device, Android device,
Linux runtime or fresh two-peer multiplayer result is claimed for this pass.

Run manifests, raw samples, captures and reports are retained under
`build/lighting-240` and `build/lighting-performance`. The earlier validation
sections do not certify these later changes. Sustained 240 FPS and the
outstanding platform/multiplayer checks remain open acceptance work.

Cleanup removed 497.6 MiB of superseded diagnostic/capture binaries from this
follow-up. Its final visual saves were archived with verified hashes and raw
captures converted losslessly to PNG. The build/evidence area retains about
142.3 MiB, including the clean Windows candidate, one passive performance
build, the current capture build for the unresolved MCC check, menu evidence
and useful logs. Performance-run evidence is retained separately. Original
maps, active user saves and unrelated worktrees were not modified. The normal
Windows configure arguments are restored to portable release with LTO/PGO
off; the delivered candidate and reported release runs used full LTO and the
existing PGO profile, as their manifests record. No branch was pushed.

## CPU collision and visibility follow-up, 5 November

The retained follow-up caches the exact sphere predicate's batch invariants,
proves separate common BSP prefixes for eight ray octants, reuses interpolation
rows when preparing half-scale textures, and keeps large scratch buffers off
the repeated field/texture lookup paths. Fully clear depth samples skip distance
feathering while retaining the original ordered weight sum. Long blocked fades
now settle values below `1e-20` to zero rather than retaining subnormal values;
this is below 8-bit visibility precision and does not change the simulation's
floating-point mode. Light counts, ray resolution, cadence and visible profiles
are unchanged by this follow-up.

Before any shared model-query experiment, the two-minute release snowy run
`c40-phase2-sustained` averaged 211.6 FPS, with a 138.3 FPS 1% low, 6.07 ms
p95 and 75.5% of frames above 4.17 ms. Its two minutes averaged 205.7 and
217.5 FPS respectively. This improves on the earlier 193.3 FPS run, but is
not sustained 240 FPS acceptance. Independent combat evolution prevents
treating the difference as a frame-identical A/B measurement.

The octant collision path agreed with stock and uncached queries for
21,003,360 snowy-encounter rays, with zero depth mismatches. An experimental
nearest-hit bound and synchronous model-transform cache also matched stock
for 21,677,088 rays and reduced diagnostic time per ray by roughly 7% relative
to the octant run. However, its subsequent two-minute release run averaged
191.0 FPS, with a 107.3 FPS 1% low and 7.00 ms p95. That failed to demonstrate
a whole-frame benefit, so the shared model API, cache and candidate-only tests
were removed. `source/physics/collision_models.c` and its header are restored
exactly to HEAD. No new shared-engine change is retained from this follow-up.

The rejected candidate's two-peer Blood Gulch check compared it with the
previous release using different lighting preferences. Both peers exited
normally, observed 66 two-player samples, damage and weapon pickups; the host
recorded two kill events. The comparator paired 59 seconds with no divergences,
and recorded zero movement drift in 1,770 samples. This is evidence for that
candidate, not a substitute for final-state integration checks.

Further candidates were checked against the same live inputs. Finer face-tile
BSP prefixes agreed with stock on 21,225,792 rays. A subsequent comparison of
8-by-8 tiles, 4-by-4 tiles and octants in the same batches found less than 1%
timing separation, with zero differences on 15,252,864 rays. The extra range
API and tile traversal were removed; the simpler octant path remains. The
earlier large per-cell projection cache was also removed because it did not
demonstrate a whole-frame benefit. Neither discarded cache is a new runtime
memory requirement.

The compiler had inlined the large scratch helpers back into lookup functions;
explicit no-inline attributes now keep that allocation off cache hits. Flat
stationary fading and ordinary model conversion retain the same arithmetic.
A nearby control of the older 211.6-FPS binary averaged only 186.4 FPS over
60 seconds, versus 191.1 for the flat-loop candidate. These independently
evolving encounters demonstrate substantial run variation, so small differences
and earlier candidate slowdowns cannot be attributed solely to code changes.

The half-scale radial conversion candidate was compared against the actual
generic loop in live combat, alternating their timing order without an intervening
simulation step. All 198,352 masks matched byte-for-byte. Conversion totals were
2,897.7 ms specialized versus 3,515.0 ms generic (17.6% less CPU conversion time).
This comparison includes the flat model path and is not a whole-frame FPS gain.
The subsequent unit-scale extension is covered by the same per-pixel harness
comparison; this earlier live measurement does not measure that extension.
A small model-selection uniform cache was evaluated, but removed to keep the
follow-up focused on the dominant CPU costs; no meaningful frame-time gain
was established for it.


### CPU batching and packed reconstruction

Before these CPU changes, `c40-phase3-final` measured 200.6 FPS over 120 seconds,
110.9 FPS 1% low, 6.80 ms p95, and 75.2% of frames above the 4.17 ms budget.
That is not sustained 240 FPS acceptance. A phase-3 two-peer check paired 59
seconds without divergences; those results predate the CPU batching changes.
Android and web builds were attempted again: the Windows NDK compiler-wrapper
invocation and missing Unix shell tools respectively prevented completion.
These are environment failures, not successful platform checks.

`phase4-batch-compare` compared model-major collision batches, the previous
ray-major batches, uncached batches and stock queries in the same live Two
Betrayals poses, rotating timing order. All 18,693,120 rays agreed exactly
across all four paths. Model-major batches took 6,552.5 ms cumulatively versus
6,828.2 ms for the previous batches: 4.0% less collision CPU time. Stock queries
took 23,090.8 ms. The previous-batch comparison is the relevant incremental
gain; the stock difference includes earlier optimisations. This diagnostic
executes four queries and must not be used as an FPS acceptance run.

Model-major work remains in feature-owned code and preserves the shared model
implementation. Tests compare actual engine matrix/query functions with the
batch on translated, rotated, scaled, masked and malformed synthetic inputs.
The packed reconstruction candidate likewise preserves the scalar output;
live timing and final-state acceptance are recorded separately below.


The live `phase4-visibility-compare` comparison alternated scalar and packed
reconstruction on 80,316 identical fields (328,974,336 cells). Every field
matched byte-for-byte. Packed reconstruction took 2,090.7 ms cumulatively
versus 4,665.0 ms scalar: **55.2% less CPU reconstruction time**. The isolated
synthetic benchmark's larger gain is not used as the live performance claim.
Neither diagnostic changes ray count, resolution, update cadence or light
budget. The rendered capture shows the snowy stairs, Jackal shield and plasma
fire intact; it is a sampled frame, not a full visual acceptance pass.

The final focused regression suite passed 171 tests; one POSIX socket test was
skipped on Windows. This includes actual collision/model math, packed/scalar
field and texture equality, 30/60/120/240-Hz synthetic filtering, isolation,
host policy, menu/configuration, Android exclusion and web build-graph checks.


The combined normal release timing build (`c40-phase4-final`) then ran the
same copied checkpoint/normal-input route for 120 seconds after 15 seconds
of warmup, uncapped at 2560x1440, AA off, 16x AF and highest models. It measured
219.5 FPS average, 123.7 FPS 1% low, 6.12 ms p95 and 7.12 ms p99. Six capture
frames were excluded; 63.2% of remaining frames still exceeded 4.17 ms.
The two minutes averaged 208.5 and 230.5 FPS. Mean occlusion work was 1.29 ms,
versus 1.64 ms in the preceding 200.6-FPS run; light selection means were both
about 11.7. These fights are not deterministic frame replays, so the directly
paired CPU comparisons establish causality more strongly than the FPS change.

**Sustained 240 FPS remains unfinished work, not accepted performance.**
World BSP traversal remains the next collision target; the measured 4% model
batch gain does not remove that cost. No additional GPU optimisation was
retained in this phase. Private memory window means rose from 662.54 to
665.37 MiB in this short run; this neither proves a leak nor establishes a
long-session plateau. The run exited normally without logged renderer errors.


Final-state CE Death Island and MCC Digsite two-peer checks paired the current
normal Windows build (lighting enabled) with the previous normal build
(lighting disabled), using host shield policy `players`. Both peers exited
normally, joined, exchanged damage, observed kills/respawning and weapon
pickups. The comparator found no divergences or problems over 59 paired CE
seconds and 50 paired MCC seconds. These are bounded rendered integration
checks, not complete map-family or long-session acceptance.

Windows release compilation passed. The final Android/web attempts again
failed before completing their builds: Android could not launch the local
NDK compiler wrapper as a Windows executable; web could not launch required
Unix shell commands. Build arguments were restored. Android exclusion and
web graph/scalar-fallback tests pass, but full builds and Android/AMD-device
validation are not claimed. No Linux runtime check was performed in this phase.

The final review covered batching gates, model transforms, node ordering,
bounded scratch ownership, fallbacks, scalar/SSE2 equality, lifecycle isolation,
Android source exclusion, diagnostics and documentation. No new shared-engine
or renderer edit is retained in this CPU phase. Superseded diagnostic binaries
and the older normal candidate were removed after hash verification, reclaiming
418.1 MiB. Test maps/caches were removed; test saves were losslessly archived
with verified manifests. Logs and captures remain. The current normal Windows
candidate (26.8 MiB) and passive timing build (24.5 MiB) remain for ongoing work.

## World collision traversal, 6 November 2026

The retained optimisation prepares one-sided internal BSP chains for each
ray octant, including chains below branching nodes. Preparation belongs to
`plasma_collision.c`; the small shared traversal extension consumes caller-
proven skip counts through the existing optional batch cache. A feature-only
root entry cannot skip these private internal traversal steps. Every skipped
node still consumes the original depth and visit budgets. Boundary planes,
leaf ordering, surface tests, breakable state and cache-miss traversal retain
the engine's existing behaviour. No map-family conversion or shader changes
are involved. Simulation callers use no cache; model batches use zero skip
counts. All scratch is bounded and cleared between batches. Lighting defaults,
host policy, Android exclusion and AMD/NVIDIA shader paths are unchanged.

Review consolidated the identical prefix/preparation plane bounds in
`trace_side`. Regression cases compare complete traversal results and state,
including exhausted budgets, cyclic nodes and cache-slot eviction. Deliberately
broken bounds, budget accounting and eviction must fail those checks.

The whole-cube trial matched 19,466,976 live Two Betrayals rays but saved only
1.98% of tracing time. The retained octant trial matched all 19,312,992 rays:
6326.253 ms prepared versus 6966.486 ms without preparation, a 9.19% reduction.
The comparison rotates execution order on identical rays and poses, outside
simulation updates, and compares outputs outside the timed intervals. The
unprepared variant retains model batching and uses one node cache; both
variants include the new bounded scratch allocation/clearing. These are CPU
batch timings, not gameplay FPS. A further leaf/plane lookup cache matched
19,652,544 rays but did not establish an additional gain, so it was removed.

The reviewed release build used full LTO/PGO at 2560x1440, AA off, 16x AF,
highest models and per-pixel lighting. The copied checkpoint/save hashes
matched the preceding phase. Normal controller input advances five seconds
and turns right for 0.6 seconds into incoming Covenant fire on the snowy
stairs. All optional lighting, including prongs, was enabled. After 15 seconds
of warmup, the 120-second run measured:

| Metric | Reviewed traversal | Previous phase |
| --- | ---: | ---: |
| Average FPS | 228.52 | 219.50 |
| 1% low FPS | 149.95 | 123.68 |
| p95 frame time | 5.607 ms | 6.119 ms |
| p99 frame time | 6.164 ms | 7.123 ms |
| Frames above 4.167 ms | 59.39% | 63.18% |
| Mean occlusion update time per frame | 1.218 ms | 1.286 ms |

The two one-minute windows averaged 223.00 and 234.04 FPS. Selected lights
averaged 11.53, peaked at 19, and were present throughout. Six screenshot
frames were excluded from pacing statistics; no frame exceeded 50 ms. Both
retained screenshots show the snowy combat encounter with active shield and
weapon lighting. This is a sustained fixed-view encounter check, not a
playthrough of every busy area. Independently evolving fights prevent claiming
the entire FPS difference as an exact A/B gain; the identical-ray comparison
establishes the traversal improvement separately.

A separate 60-second lighting-off control averaged 624.57 FPS with 403.77 FPS
1% lows, no selected enhanced lights and zero occlusion updates. Both runs
exited normally without reported runtime errors. Their reports and settings
are under `build/lighting-performance/c40-world-final-on` and
`c40-world-final-off`. The enabled run's private-memory window means rose
from 663.27 to 666.54 MiB; this short run does not establish a memory plateau.
**Sustained 240+ FPS is not achieved.** The stock control supports continued
work on enabled-feature overhead; the traversal improvement does not close
the remaining frame-time gap or resolve the previously tracked memory issue.

The performance runner now marks its deliberately authored lighting settings
as current, avoiding the one-time legacy-preference reset introduced on
testing. Player preferences and migration behaviour are unchanged.

Final validation passed 163 lighting, collision, menu, isolation and build-graph
checks, with one POSIX-pthread socket harness skipped on Windows. The local
test runner needed UTF-8 mode and a compiler wrapper with a consistent CRT
warning macro; neither required a production change. Light-storage checks
passed ordinary and full-LTO COFF builds, including their corruption control.

Two-peer CE Death Island and Workshop Bigass sessions used lighting on at the
host, off at the client, and host shield policy `players`. Ordinary release
runs compared 60 and 55 seconds respectively, with normal exits and no
sustained divergence. Both covered joining, shooting, respawning and weapon
pickup. The first, instrumented CE attempt reported a brief scripted-pickup
position difference; the ordinary release repeat passed without mismatches.
Its collision comparison independently matched 1,118,208 rays. Bigass was
loaded through Workshop item `3010816711`; its matching `halo1/maps` resources
were copied only into the isolated test's shared-resource folder after an
initial missing-resource refusal. The installed Workshop files were unchanged.

Testing advanced to `d91cd8599` during validation. That bot/lobby update was
fast-forwarded without conflicts; it changes none of this optimisation's
rendering or collision files. Its 128 focused integration checks passed, and
the combined state rebuilt successfully as Windows full LTO/PGO. The final
MCC mixed-setting run compared 55 seconds with no state mismatches or
divergences and matched all 1,559,040 live collision rays. Xbox snowy combat,
CE and MCC captures were inspected; these are representative checks, not
exhaustive map/device compatibility coverage. The sustained FPS measurements
above predate the unrelated bot/lobby integration.

Local Android and web builds were attempted but remain blocked by the local
toolchain launch failures recorded in the preceding phase (NDK executable
wrapper and missing Unix commands respectively). Build arguments were
restored. The published commit's Linux/Android/web/Windows CI must be checked
separately; no local Android device, AMD GPU or Linux runtime test is claimed.

Cleanup removed 98.80 MiB of superseded diagnostic executables after verifying
their recorded hashes. No loose test map files remain. Test saves are preserved
in verified lossless archives. About 89.62 MiB remains for this phase's logs,
captures, archives and two measurement executables; normal build outputs and
the user's installed maps/saves were preserved.
