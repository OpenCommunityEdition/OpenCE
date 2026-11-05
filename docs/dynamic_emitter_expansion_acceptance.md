# Energy, vehicle and world lighting: local validation

This records the `dynamic-emitter-expansion` work based on testing commit
`0bc618eac6a4d22630a4e74fa3b606fe780e8df7`. It does not replace the historical
1.0 acceptance or certify every device or custom map. The
[implementation guide](reading/emitter_expansion.md) explains the profiles,
controls, reading route and lifecycle.

**Local 120+ average headroom is established; full performance acceptance
remains open.** The changes below address measured CPU costs without lowering
visual quality. Slow-frame consistency and supported-platform builds still
need acceptance; an average above 120 is not a locked frame rate.

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
