# Dynamic lighting implementation history

See the
[release guide](dynamic_lighting_release.md) for the current feature
set, menu controls, known limitations and acceptance status. The
[emitter additions](reading/emitter_expansion.md) describe the subsequent sword,
charge/heat, grenade, flame, vehicle and world-light extension. Muzzle lighting,
Covenant projectiles, held-weapon emitters and shield lighting are implemented;
CStencil and Modern remain separate work. `chesters-cool-stuff` remains a local
reference.

The sections below preserve the chronological implementation and test record,
starting with `plasma-lighting-prototype`. Earlier radii, exclusions, deferred
features and test limits describe those stages; later entries supersede them.
They are not the current release checklist.

Set `plasma_lighting = true` in config.toml's `[display]` section, or
`HALO_PLASMA_LIGHTING=1`. The default is false. Keep `display.interpolation`
enabled for motion between simulation ticks.

## Rendering

The feature selects existing light attachments on four exact projectile tags:
`weapons\plasma rifle\bolt`, `weapons\plasma pistol\bolt` and
`weapons\plasma rifle\charged bolt` (the pistol overcharge) and
`weapons\rocket launcher\rocket`, up to 128 authored attachments per frame.
Stock rockets have no authored light and use the separate bounded flight-light
module described below. It refreshes their marker
poses before visibility and receiver selection. New objects without a previous
snapshot use the engine's existing fallback pose.

Live captures revealed that Xbox Blood Gulch's bolt light has flags zero:
its attachment supplies a lens flare but is not marked dynamic. The first
temporal prototype therefore did not illuminate world surfaces on that map.
Selected attachments now become dynamic during rendering, with their original
flags restored afterward. Tags are never edited and the authored-attachment
path allocates no extra light.
Already-dynamic attachments retain their authored flag after rendering.

World diffuse illumination uses a feature-owned 64³ attenuation texture:
`max(1 - distance_squared / radius_squared, 0)²`. Both value and slope approach
zero at the outer boundary. Texels are sampled at their centres and filtered
linearly. The authored colour and radius remain unchanged. The existing
additive pass still applies material colour and surface facing.

Model point lights now use the same squared falloff. The model shader's
existing contribution is multiplied by the extra distance factor only for
selected plasma slots. Ambient, distant and other point lights keep their
existing arithmetic. Models remain vertex lit unless the separate existing
per-pixel setting is enabled; that path applies the same curve per pixel.
No setting is silently enabled. Coarse model meshes can still reveal vertex
interpolation in the lighting.

Omnidirectional world specular passes use the same texture on their own
attenuation stage, preserving the authored specular radius multiplier,
material mask, exponent and highlight colour. Spotlight gel passes and model
cubemap reflections remain stock; this does not add a new model specular BRDF.
Optional collision occlusion is described below. Custom projectile tag names and projectiles without
light attachments are outside this prototype.

## Optional occlusion prototype

Enable `plasma_occlusion = true` alongside `plasma_lighting = true` in
`[display]`, or set both `HALO_PLASMA_OCCLUSION=1` and
`HALO_PLASMA_LIGHTING=1`. Both default to false. Occlusion alone does nothing.

`plasma_occlusion.c` builds a 16³ local visibility field using the existing
structure, scenery and machine collision queries. Bipeds, vehicles and
projectiles do not cast occlusion. The origin is the rendered light position;
queries do not modify simulation state or collision geometry. The extent
includes the authored world specular radius. Trilinear sampling avoids
switching an entire receiver on or off at a doorway. Visibility uses elapsed
time, with an 8 ms blocking half-life and a 25 ms clearing half-life.

World diffuse and omnidirectional specular passes combine visibility with
soft attenuation in separate 16³ volume textures. Models sample the same
visibility field in their existing vertex or optional per-pixel lighting path.
The private specular-radius getter in `object_lights.c` is the only new game
source bridge: the feature module cannot safely access that private layout.
The existing falloff, model selection and lifecycle hooks supply the remaining
integration. `plasma_occlusion_gl.c` binds model volumes on extra texture units
6/7 and restores the active unit; it does not replace the game's four stages.

Eight stable light identities can hold fields. At most eight field updates
occur per view. Each collision refresh uses 1536 directional depth rays; the
16³ grid interpolates visibility comparisons along them, not blocker distances.
Depths are reused for less than 12 ms, with immediate invalidation for radius
changes or motion exceeding one eighth of the field extent. Projection and
fading still run at every rendered pose. Active fields are not evicted
for an overflow light; excess lights retain unoccluded soft lighting. An unused
field becomes reusable after 100 ms. Upload failure also falls back. Fields
and their textures reset at the existing disable, map and shutdown boundaries;
model slots reset per receiver/view. A constant white GL fallback texture is
renderer-lifetime state and contains no map or feature history.

This is deliberately approximate, not CStencil or Modern. Thin geometry and
cell boundaries can leak light; the cell width is twice the largest light
radius divided by 16. History is reprojected at world-space receiver
positions. Cached collision depths and temporal filtering can still introduce
short edge lag. Collision meshes may differ from visible meshes, and coarse
vertex-lit models interpolate visibility between vertices. Previously
overflowing lights fade into occlusion over 80 ms after admission. Projectile
creation/destruction is not cross-faded. The feature does not alter the stock
bolt lens flare, which can remain visible when its projected illumination is
blocked. Split-screen and every map/material remain unverified; the hardening
captures below cover scripted sustained fire and a controlled moving-door
fixture.

## Isolation and lifetime

Selection, render tracking and texture generation live in the dedicated
`plasma_lighting.c`, `plasma_falloff.c` and `plasma_model_shader.c` modules. The
small bridges in `object_lights.c` are necessary because light storage and cluster ownership
are private there. They validate handles and ownership before refreshing
attachments or changing the runtime dynamic flag. No map-family loading,
conversion, gameplay or network protocol changes are intended.

World diffuse and omnidirectional specular begin/end hooks are necessary
because the attenuation sampler belongs to that existing pass. Only selected plasma draws bind the replacement
texture (stage 2 for diffuse, stage 1 for specular). End restores the stock
texture and filter, including if the option has just been disabled. Allocation failure falls back to stock attenuation.
The texture is released on disable, map initialization/unload, light recovery
and shutdown. Per-view rasterizer indices reset for each split-screen view.

The model slot hooks in `rasterizer_xbox.c` are necessary because only that
submission path maps receiver light slots to rasterizer light indices. A
feature-owned two-bit mask is cleared before each model, each view and at
frame/map cleanup, and gated again by the option at draw time. The renderer
uploads it as a separate uniform, without borrowing stock constants or flags.
Minimal translator hooks call the feature emitter for recognized two-point-light
model shaders and the existing per-pixel model shader. The vertex path changes
only the point light colour multiply-add's RGB operand; the parallel scalar
instruction and alpha are untouched. These shared paths serve Xbox, CE and MCC
receivers, with no changes to map admission, loading or conversion.

After interpolation ends, the feature restores original dynamic flags and
simulation-time positions and cluster membership. Map boundaries discard old
handles. Excess lights beyond the bounded tracking array retain stock behaviour.
The disabled path performs no light refresh or texture substitution.

## Model and specular validation

Windows portable release, 2026-10-14: 20 harness tests passed. Coverage adds
model-slot selection, disable/view/frame cleanup, specular stage/filter
restoration, and translation of the actual stock shader payloads (both model
point-light slots reached; non-point programs untouched). Light-storage ordinary
and full-LTO checks and their negative control passed. The normal release
executable was rebuilt after removing all capture instrumentation.

Twelve hidden runs captured three real projectiles apiece, with plasma lighting
off/on at each cap. Blood Gulch used a spawned Spartan receiver; Carousel used
world diffuse lights disabled via its existing debug setting, isolating the
world specular contribution. Counts below are actual selected model draws or
specular receiver draws, not just light submissions.

| Cap | Model off/on FPS | Model draws on | Specular off/on FPS | Specular draws on | Most poses per tick |
| --- | --- | --- | --- | --- | --- |
| 30 | 30.00 / 29.57 | 88 | 30.00 / 29.72 | 56 | 1 |
| 60 | 59.99 / 58.89 | 165 | 60.00 / 59.56 | 114 | 2 |
| 120 | 119.99 / 119.99 | 348 | 119.61 / 119.99 | 270 | 4 |

All runs exited normally; disabled runs had zero selected draws. Median frame
intervals were 33/17/8 ms. A further enabled 60 FPS model run with per-pixel
lighting passed (59.17 FPS, 150 selected draws). Measurements include capture
and initial shader work and are not a comparative performance benchmark.

Reviewed off/on images and consecutive frames show cyan illumination traversing
the armour and highlights crossing the indoor floor. The 60/120 FPS sequences
and telemetry contain intermediate poses within each simulation tick. Off/on
frames are matched to the nearest projectile position, not pixel-identical
simulation timestamps. Short-lived bolts still disappear at normal impacts;
no artificial lifetime or impact fade was added. Coarse vertex-lit meshes and
stock material highlight sharpness remain visible limitations.

Evidence: `build/plasma-visual/model-comparison.png`, `model-sequence.png`,
`specular-comparison.png`, `specular-sequence.png`, `model-specular-results.json`,
and each `*-model-final` / `*-specular-final` directory. The test-only binary
and build manifest are retained; copied test maps and caches were removed.
The initial short-trajectory probe runs are diagnostic evidence only.

Production Xbox Blood Gulch -> CE Death Island -> MCC Mercury Falling -> Xbox
transition smoke checks passed off/on: three console commands each, confirmed
CE conversion/MCC loading, exit zero and no logged exceptions. Logs are in
`build/plasma-model-transitions`. This is not visual certification of CE/MCC
plasma shaders or split-screen.

A two-instance Windows loopback Slayer smoke test also passed with the host's
option on and the client's off: both logged two players through tick 870 and
exited normally. This checks session compatibility, not networked plasma combat.
Logs remain in `build/plasma-model-network-complete`. The first attempt lacked
the private profile's lobby map and never entered play; it is not counted.

Android and web builds were attempted but those targets are unavailable in the
local toolchain. Linux/mobile/web runtime and browser-network interoperability
remain untested. The new code uses existing GL entry points and both new modules
are included by the native, Android guest and web source globs; no new Android
host imports or network-version change is needed.

## Earlier world diffuse validation

Windows portable release, 2026-10-14: 17 harness checks passed, including
negative controls, disabled selection, view boundaries, stale ownership,
restoration of both dynamic and lens-flare-only flags, allocation failure,
texture/filter restoration, and monotonic falloff with a zero boundary slope.
Existing light-storage checks passed ordinary and full-LTO links and their
negative control.

Hidden Xbox Blood Gulch captures launched three real plasma-rifle projectiles
along a repeatable trajectory across visible ground. The fixture creates real
projectile objects using normal physics and impacts; it does not simulate
controller firing. Screenshots and per-render-frame telemetry were collected
with the option both off and on:

| Cap | Off average | On average | On median frame interval | Most distinct rendered poses within one tick |
| --- | --- | --- | --- | --- |
| 30 FPS | 30.01 | 29.67 | 33 ms | 1 |
| 60 FPS | 60.00 | 59.56 | 17 ms | 2 |
| 120 FPS | 119.99 | 120.04 | 8 ms | 4 |

The measurements include capture overhead and initial shader/texture work.
Images show a feathered pool following the bolt across the ground. Successive
60/120 FPS frames and telemetry show intermediate poses rather than a light
held at each 30 Hz tick. Selected lights return to zero after projectile removal.
No crashes occurred in these six runs. Captures are under `build/plasma-visual`;
`sequence-contact.png` shows consecutive enabled frames at each cap.

Separate hidden runs with the production executable exercised Xbox Blood Gulch,
Custom Edition Death Island and MCC Mercury Falling with the option off and on.
Both runs sent all three transition commands and exited normally without logged
exceptions; the logs confirm CE conversion and MCC loading. These were loading
and transition smoke checks, not projectile-lighting visual tests on CE/MCC.
Their logs remain in `build/plasma-falloff-transitions`; copied test maps were
removed. An earlier sandboxed attempt could not connect to the local console
and is not counted as a successful transition test.

Those earlier tests did not establish softer model/specular shading, wall occlusion,
split-screen visual quality, every plasma tag variant or full map compatibility.
Android/web targets are unavailable in the local build configuration. Linux,
Android, web, browser networking and multiplayer runtime were not tested.
The change introduces no wire-format or simulation contract changes, so the
network version is unchanged.

## Reproducing the captures

With the repository's Windows compiler environment configured:

```
python tools/plasma_visual_build.py
python tools/plasma_visual_run.py --data-root C:/Games/GulchCE --fps 30 --enabled 1
```

For a visible model receiver, add `--model --distance 1.3
--receiver-distance 1.8 --height -0.3`. For an indoor specular isolation run,
add `--map carousel --distance 1.5 --height -0.25 --specular-only`; this uses
an existing debug setting to disable world diffuse lights in the private test
session. Add `--per-pixel` to exercise the existing model lighting option.
The scene flags affect only the capture executable and private configuration.

Repeat for caps 30, 60 and 120 and enabled values 0/1. Use `--label` for a new
output directory when repeating a combination. The builder temporarily installs
`tools/fixtures/plasma_lighting_capture.c`, restores the production source and
configuration even on failure, and rebuilds the normal executable. The fixture
is only present in `build/plasma-visual/halo-capture.exe`. The runner uses hidden
windows and private data/save folders, removes copied maps and generated map
caches, and retains screenshots and diagnostic logs.

## Occlusion validation (2026-10-14)

The Windows portable release build and 21 focused harness tests pass. These
include synthetic wall/doorway visibility, partial clearing, equal elapsed-time
response at 30/60/120 FPS, the four-field limit, model-slot reset, disabled
queries, allocation failure and resource cleanup. Light-storage tests pass
ordinary/full-LTO links and their negative control.

Hidden Xbox Carousel captures compare occlusion off/on with plasma lighting
on. A diagnostic carrier sweeps across the nearby pillar/opening; this fixture
holds and moves a real projectile object and may pass through geometry. It is
not a combat trajectory. Separate normal projectiles use ordinary physics and
impact quickly in this scene. Neither probe logic nor telemetry enters the
production executable. Off/on comparisons show reduced illumination beyond
the pillar; consecutive-frame inspection shows a spatially softened transition
in the tested scene, rather than whole-receiver rejection. A synthetic doorway
is covered by the collision harness; moving-door visual behaviour is untested.

| Cap | Corner sweep, occlusion off | Corner sweep, on | Wider sweep, on | Normal projectiles, on |
| --- | --- | --- | --- | --- |
| 30 FPS | 29.55 | 30.00 | 29.61 | 30.00 |
| 60 FPS | 58.62 | 58.61 | 58.60 | 60.00 |
| 120 FPS | 119.22 | 120.02 | 120.02 | 119.99 |

These are average measured FPS including capture overhead and startup work,
not GPU benchmarks. One active light is exercised, not the four-light worst
case. The optional per-pixel model path passed at 58.70 FPS under a 60 cap.
With plasma lighting disabled and occlusion enabled, selection and field-update
counts remained zero (59.46 FPS). No shader/GL errors or exceptions were found
in these capture logs. An 8-unit distant scout produced no selected lights and
is explicitly excluded from successful validation.

Production transition runs, off at 30 FPS and on at 120 FPS, loaded Xbox Blood
Gulch, CE Death Island, MCC Mercury Falling and returned to Xbox. Both completed
three console transitions, logged MCC loading and exited without exceptions.
These are loading/lifecycle smoke checks, not CE/MCC lighting visual validation.
Android/web build attempts report unknown targets in this Windows configuration;
Linux, Android, web and browser networking runtime remain unverified. The
prototype changes no simulation or network contract, so the network version
is unchanged.

Evidence remains under `build/plasma-visual/*occ*` and
`build/plasma-occlusion-transitions`. The comparison and consecutive-frame
contact sheets are `occlusion-comparison.png`, `occlusion-corner-sequence.png`
and `occlusion-opening-sequence.png`. Copied test maps are removed; diagnostic
logs and images are retained. The capture manifests identify the source used;
subsequent source polish hardens overflow budget admission and removes
an unnecessary upper limit on the private radius getter. None changes the
successful single-light, stock-radius scenes above.

To reproduce the controlled wide sweep after building the capture executable:

```
python tools/plasma_visual_run.py --data-root C:/Games/GulchCE --fps 60 --enabled 1 --occlusion 1 --map carousel --distance 3 --height -0.1 --probe --span 4 --model --receiver-distance 2 --label occlusion
```

Repeat at 30/120 FPS. Drop `--probe --span 4` for normal projectiles; use
`--occlusion 0` for the soft-lighting control. Output labels must be unique.

A hidden two-instance native loopback Slayer smoke test also passed with plasma
lighting/occlusion enabled on the host and disabled on the client. Both peers
reported two players through approximately tick 870 and exited normally. Logs
are in `build/plasma-occlusion-network`; this is connection/gameplay smoke
coverage, not a multiplayer plasma-combat or browser transport test. Final
review added a regression ensuring an overflow light submitted before the four
admitted lights cannot spend their update budget. That correction is covered
by the harness and final Windows build; the captured single-light paths are
unchanged.

## Sustained-fire hardening (2026-10-14)

The selected budget is now eight occluded lights, increased from four. Sixteen
was evaluated but rejected as the fixed budget: the isolated 16-light fixture
fell to 85.92 FPS under a 120 cap, versus 120.02 without occlusion. With the
final eight-slot budget, the same 16 visible bolts averaged 115.52 FPS; eight
retain soft unoccluded lighting. Active identities are never evicted, and a
previously overflowing light blends into occlusion over 80 ms when admitted.
This is bounded coverage, not a claim that every bolt in arbitrary combat is
occluded. Allocation failure still uses the unoccluded fallback.

The collision work changed from 4096 rays per field to 1536 directional rays.
The 16³ output grid filters comparisons against those depths. This avoids
inventing intermediate wall distances across openings, but widens the soft
edge slightly: the synthetic near-jamb sample is about 27% visible, while the
farther sample is fully blocked. Thin features narrower than angular/grid
sampling can still leak. Depths are cached for less than 12 ms at rendered
samples and invalidated by fast movement/radius changes. Light motion and
world-space history reprojection continue every frame. A clear cached ray uses
an explicit no-hit sentinel so moving across its previous endpoint cannot
create a false dark border; a regression failed before that correction.

The production change stays in the plasma module and its existing setting
comment. No new shared-engine hook, map-family conversion, network protocol,
CStencil or Modern work was needed. New combat, burst, door, toggle and map-reset
controls live only in the capture fixture. The builder also temporarily
instruments input for actual trigger presses and restores all source files.
Normal builds contain none of that test logic.

### Visual and performance evidence

Windows portable release, hidden 640×480 captures, real Xbox/CE assets copied
into private test folders. Sustained-fire fixtures equip an actual plasma rifle
and hold/release the normal trigger; they do not manually spawn those combat
bolts. The stress fixture deliberately places simultaneous diagnostic carriers
on a controlled trajectory. Door testing spawns an existing A10 door tag with
normal runtime device groups and alternates its desired position; captures
show both animated open and closed states. These are controlled fixtures, not
full campaign playthroughs.

| Scene | 30 cap | 60 cap | 120 cap |
| --- | --- | --- | --- |
| Xbox Blood Gulch, actual rifle fire, up to six selected bolts | 29.98 | 59.97 | 119.90 |
| CE Death Island, actual rifle fire | 29.99 | 59.99 | 119.71 |
| A10 moving-door probe | 29.99 | 59.79 | 119.99 |

The eight-bolt stress case measured 114.67 FPS under a 120 cap; sixteen visible
bolts with the final eight-slot budget measured 115.52. Their 95th-percentile
frame interval was 11 ms. These meet the runner's 5% average tolerance, not a
claim of a locked 120 FPS. Measurements include logging/capture overhead and
startup work. Early simultaneous runs are retained for diagnosis but are not
the final isolated stress measurements. The earlier independent-ray candidate
managed only 86.74 FPS for eight bolts; directional reuse without caching
improved it to 105.93 before the final cache was added.

Consecutive images cover the corner sweep, moving door and CE muzzle-to-impact
lighting. No whole-receiver flicker was observed in these inspected sequences;
this does not establish artifact-free behaviour for every mesh or doorway.
Thirteen rapid setting changes with eight live carriers alternated both states;
the disabled intervals contained no occlusion updates. The settings remain off
by default. Changing the setting itself is intentionally immediate.

Evidence is in `build/plasma-visual/*hard*`, notably `hardening-door-comparison.png`,
`120-1-hard-final-eight`, `120-1-hard-budget-overflow`, `*-hard-door-final`,
`*-hard-ce-combat`, `*-hard-combat-final` and `60-1-hard-toggle`.
Each capture records its source hashes. The final no-hit boundary correction
was validated by the failing/passing harness regression and production build;
those images precede that narrow correction. The initial A10 attempt (cinematic
active, door without control groups) is excluded; the corrected fixture logs
both 0.0 and 1.0 door positions and active light fields.

### Regression checks and limits

Twenty-one focused harness tests pass, including multiple assertions for eight
slots, stable overflow, admission fading, cached-depth reuse/fast-motion
invalidation, moving-door clearing/blocking, a thin plane across cubemap seams,
bounded FPS-dependent response, disabled queries, model slots and texture
cleanup. The cache allows a small bounded difference in sampled visibility at
different frame rates; the test checks that explicitly rather than claiming
bit-identical results. Ordinary/full-LTO light-storage tests and their negative
control also pass. Windows portable release builds pass. Android/web attempts
report unavailable targets in this local configuration; Linux, Android, web,
browser networking and split-screen runtime remain unverified.

A two-peer hidden native Slayer run used real plasma-rifle trigger input on the
host and moved the remote player into range using host-owned fixture setup.
The host had plasma lighting/occlusion enabled; the client had both disabled.
Both peers reached approximately tick 870 and exited normally; their logs
agree on a kill/death and the client's respawn. The host continued submitting
plasma light fields. This covers scripted local combat, not internet latency,
all weapons or a multiplayer compatibility guarantee. Logs are in
`build/plasma-hardening-combat-network-v2`. Initial helper launches lacked the
capture executable's DLL search path and are excluded from successful checks.

Reproduce actual rifle captures with `--combat --map bloodgulch` or
`--combat --map ce-deathisland`; repeat caps 30/60/120. For stress, add
`--probe --burst 8 --map carousel --distance 3 --height -0.1`.
Use `--burst 16` to exercise overflow, `--toggle` for rapid activation changes,
and `--door --probe --map a10 --distance 3 --span 0.4 --height -0.1` for the
door fixture. Keep `--enabled 1 --occlusion 1` and a unique `--label`.

Active transition runs (off at 30 FPS, on at 120 FPS) also completed Xbox Blood
Gulch → CE Death Island → MCC Mercury Falling → Xbox. Both sent three console
commands, logged CE conversion/MCC loading and exited without exceptions.
The capture fixture reinitializes after game-time reset and re-equipped the
rifle on Xbox/CE and after returning to Xbox; MCC is loading/lifecycle smoke
coverage, not an MCC lighting visual claim. Logs are retained in
`build/plasma-hardening-active-transitions-v2`; copied maps and generated
private cache maps were removed. Original maps and saves were preserved.

A separate 60 FPS moving-door control with occlusion off averaged 59.14 FPS;
`hardening-door-off-on.png` contains representative off/on frames.


## Plasma pistol extension (2026-10-14)

Normal pistol bolts and overcharged shots use the existing feature-owned
selection, falloff, occlusion and cleanup paths. Inspection of the local Xbox
Blood Gulch and CE Death Island data, followed by runtime captures, identified
`weapons\plasma pistol\bolt` and `weapons\plasma rifle\charged bolt`.
The overcharge lives in the rifle directory despite being fired by the pistol.
The normal bolt's light has flags 0 and the charged bolt's flags 3; both report
an authored radius of 1.0 in these captures. The no-specular definition bit
(value 2) disables specular lighting: the charged bolt deliberately retains no world specular
highlights. Promotion and restoration preserve the original dynamic state;
definition flags are never edited. No tag, colour, radius, projectile physics
or network messages change, so no network version increment is required.

This general feature extension changes only the selector in the existing
feature module and the setting's description. It adds no shared engine hooks,
MCC adapters or loading changes. Custom projectile names still remain stock.
The eight-field occlusion budget is shared by all three selected projectile
kinds, with the existing overflow/admission policy.

The capture runner accepts `--kind pistol` and `--kind charged`. Add `--model`
to give controlled charged shots a real biped homing target through the existing
projectile interface. These shots retain ordinary projectile simulation;
`--probe` instead holds and sweeps diagnostic projectiles and does not validate
homing. `--mixed --probe --burst 12` creates four of each supported kind to
exercise overflow. `--combat` equips the actual weapon and drives its normal
trigger (short taps for pistol, hold/release for charged). These mechanisms are
linked only into the temporary capture build, never the normal executable.

Births use the existing interpolation fallback, and expired projectile lights
leave selection immediately. This does not add a birth/death crossfade or alter
stock impact flashes, contrails, muzzle flashes or the weapon's charging glow.
Existing low-resolution occlusion and coarse-model lighting limitations apply.


Validation used a portable Windows release based on `c05f2388` plus this
extension, hidden at 640x480 with capture logging enabled. The normal Windows
build passes and contains none of the capture instrumentation. The focused
harness has 23 passing cases, including a negative control which removes pistol
selection and must fail. Light-storage ordinary/full-LTO and corruption-control
checks pass. Local `ninja android` and `ninja web` both report unknown targets;
those builds, Linux runtime, browser networking and split screen remain
unverified for this extension.

Measured capture averages (FPS, not isolated GPU benchmarks):

| Scene | 30 cap | 60 cap | 120 cap |
| --- | ---: | ---: | ---: |
| Xbox Blood Gulch, controlled normal bolts | 29.54 | 60.00 | 120.02 |
| Xbox Blood Gulch, controlled homing overcharges | 30.00 | 60.00 | 119.99 |
| CE Death Island, controlled normal bolts | 30.00 | 60.04 | 120.02 |
| CE Death Island, controlled homing overcharges | 30.00 | 59.99 | 120.02 |
| Xbox Blood Gulch, actual tap-fired pistol | 29.99 | 59.98 | 120.11 |
| Xbox Blood Gulch, actual charge/release pistol | 29.99 | 59.99 | 119.99 |
| Twelve mixed diagnostic projectiles | 30.00 | 60.00 | 119.80 |

All runs met the capture runner's 5% average-frame-rate tolerance. The
controlled-shot captures exercised model draws and returned to zero selected
lights after their projectiles expired. Logs at 60/120 FPS show changing render
poses within individual simulation ticks. Image sequences and the FPS overview
sheets were inspected for both map families and projectile kinds. Sampling
phase, first-use shader stalls and capture overhead prevent pixel-identical
comparisons or a general locked-frame-rate claim.

Disabled controls at 60 FPS for both kinds on both map families recorded zero
selected lights and zero occlusion updates, even with the occlusion setting on.
Twelve mixed lights selected all three tags and updated exactly eight fields per
active frame. Eight charged diagnostic lights survived 13 rapid occlusion
setting changes with no field updates during disabled intervals. Additional
60 FPS captures exercised the existing per-pixel model path (58.99 FPS) and
Carousel's isolated world specular passes (normal: 59.51 FPS, 789 draws;
charged: zero draws, respecting its authored no-specular flag). The initial
charged specular run exited successfully but the runner wrongly failed it for
having zero draws; the corrected runner checks that no draws occur when every
reported light definition disables specular. The rerun
`60-1-pistol-specular-authored` passed at 59.30 FPS with zero specular draws
and reported flags 3. These are targeted scenes, not exhaustive map coverage.

Evidence is retained under `build/plasma-visual/`, in directories named
`{fps}-1-pistol-{map}-{kind}`, `pistol-combat-{kind}`, `pistol-mixed`,
`pistol-toggle`, `pistol-perpixel`, `pistol-specular-{kind}` and disabled
`60-0-pistol-off-{map}-{kind}` captures. Each holds its report, logs, screenshots
and capture build manifest. Overview images are `pistol-bloodgulch-fps.png`,
`pistol-ce-deathisland-fps.png` and representative `pistol-off-on.png`; the last
uses nearby phases rather than asserting identical projectile positions.
Copied maps and derived map caches from those runs were removed.


Two separate hidden, two-peer native Slayer runs used normal pistol taps and
charged shots, with lighting/occlusion enabled on the host and disabled on the
client. In each run both peers exited successfully and agreed on one host kill,
one client death and the client's respawn. Only the host logged feature
occlusion updates. These are local loopback checks, not internet-latency or
browser transport coverage; no networking code changed. Evidence and private
runner scripts are `build/plasma-pistol-network-{pistol,charged}` and
`build/plasma-pistol-network.py`.

Active pistol-fire sessions traversed Xbox Blood Gulch → CE Death Island → MCC
Mercury Falling → Xbox Blood Gulch, once disabled at 30 FPS and once enabled at
120 FPS. Both completed all three transition commands, loaded the MCC map,
returned to Xbox and exited with no logged exceptions. This validates targeted
lifecycle transitions, not MCC visual correctness or campaign completion.
Reports, logs and the private runner remain in `build/plasma-pistol-transitions`
and `build/plasma-pistol-transitions.py`. All copied maps and derived caches
from these tests were removed; original maps and saves were untouched.


## Rocket flight lighting (2026-10-14)

The initial Xbox capture established that the stock rocket projectile has
contrail, particle, sound and effect attachments, but no light attachment.
Simply adding its tag to the plasma selector produced no illumination. The
feature therefore owns up to eight additional flight lights in
`port/linux/game/rocket_lighting.c`, using existing engine glow interfaces.
These use a prototype warm colour `(1.0, 0.45, 0.12)` and radius 1.0 world unit;
there is no authored rocket flight-light colour/radius to preserve. The stock
exhaust, rocket motion, damage, impact and explosion effects remain untouched.

The existing `display.plasma_lighting` setting enables both plasma and rocket
flight lighting. Its name is retained and its default remains false. Optional
`display.plasma_occlusion`, also false by default, shares its eight-field budget
between selected plasma and rocket lights. Muzzle flashes, CStencil and Modern
remain deferred, and `chesters-cool-stuff` remains a local reference.

Only exact `weapons\rocket launcher\rocket` projectile objects with a model
node and within 25 world units of a local camera receive new lights. A map's
existing light attachment takes precedence and uses the authored-attachment
path; no duplicate flight light is added. Missing glow templates, exhausted
light storage and rockets beyond the eight-owned-light limit keep stock
behaviour. No tag is changed and no light is attached to an explosion object.
The template comes from the engine's existing glow-definition service; this
also preserves that template's specular policy.

Owned light identities persist across frames for occlusion filtering. They are
made black and disconnected from the spatial partition after rendering, then
reconnected before the next scene pass. They are removed when their projectile
is gone, becomes ineligible or the feature is disabled. Salted handles, object
ownership and the glow definition are checked before touching or deleting a
light. Map unload, new-map initialization, light-array recovery and shutdown
forget feature handles as the engine discards its light storage/partition.
No projectile simulation or network contract changes are introduced.

The only new shared-engine wiring is in `source/objects/object_lights.c`:
a pre-visibility update and lifecycle reset calls, each marked `port:`.
A module-only attachment callback cannot create flight lights because stock
rockets have no light to invoke it. The new hook runs after light-array recovery
and before scene collection. All creation, selection and cleanup policy stays
in the feature module; existing Xbox, CE and MCC loading paths are unchanged.
The plasma module supplies existing falloff/model/occlusion integration.

Capture tools accept `--kind rocket`; `--kind rocket --mixed --probe --burst 12`
creates three of each supported projectile kind. `--combat` equips the actual
launcher and uses ordinary trigger input. These diagnostic mechanisms never
enter the normal build. The first exploratory capture (`30-1-rocket-first`)
failed lighting-selection checks because no authored attachment existed;
`rocket-owned-first` verified the added light before the connection-state fix.
Neither is final acceptance evidence.


### Rocket validation

The Windows release build and all 27 focused projectile-lighting, falloff,
model-shader, occlusion and owned-rocket-light harness tests passed. Negative
controls detect a removed rocket selector and a missing reconnect. The light
storage checks passed ordinary COFF, full LTO and their corruption negative
control. The capture builder restored production files and rebuilt the normal
executable; capture markers are absent from that executable.

Hidden 640x480 captures were inspected at 30, 60 and 120 FPS. Measured averages:

| Scenario, lighting and occlusion enabled | 30 cap | 60 cap | 120 cap |
| --- | ---: | ---: | ---: |
| Xbox Blood Gulch, flight and model impact | 29.99 | 60.03 | 119.99 |
| CE Death Island, clear flight path | 30.00 | 60.00 | 120.02 |
| Actual launcher trigger fire, Blood Gulch | 30.00 | 59.69 | 119.37 |

Blood Gulch disabled controls averaged 29.66/59.98/119.99 FPS and selected no
feature lights or occlusion fields. CE disabled controls likewise selected
none. Blood Gulch had 12/36 changed render poses within unchanged simulation
ticks at 60/120 FPS; CE had 63/189. Controlled flights ended with zero selected
lights. The inspected Blood Gulch sequences show the stock fireball, smoke,
debris and target blast response with the feature on and off; they are not a
pixel-identical effects comparison. CE lighting is subtle against daylight.
Reports, logs, source manifests and images are retained under
`build/plasma-visual/*-rocket-*`; overviews are `rocket-bloodgulch-fps.png` and
`rocket-ce-flight-fps.png`. Runners are `build/rocket-matrix.py` and
`build/rocket-followup.py`.

The first CE model-impact placement detonated within one tick and selected no
flight light. The revised clear-path capture resolves that fixture limitation.
Three runs failed before recording any gameplay frames with Windows exception
`c0000008` during graphics startup. Replacement CE captures and the repeated
120 FPS combat case passed; these startup failures remain recorded, not treated
as evidence of a gameplay lighting failure or a proven driver diagnosis.

Stress limits remain: twelve overlapping mixed lights averaged 83.03 FPS at a
120 cap, and eight rockets with occlusion toggling averaged 49.52 at a 60 cap.
Both cases passed at 30 FPS (30.00/29.99), including all four projectile kinds,
the eight-rocket limit and no occlusion updates during disabled intervals.
These screenshot-heavy captures do not establish a general 120 FPS performance
guarantee. The Carousel specular-only probe selected a rocket light but recorded
zero specular draws and failed that acceptance check; rocket specular output is
not validated. The inherited template policy has not been overridden to force
highlights.

Android and web builds were attempted, but this Windows configuration exposes
neither Ninja target (`unknown target`). Linux, Android, web and browser/native
internet interoperability remain unverified. No network data or simulation
rules changed, so no network version bump is required.


Two hidden native loopback peers completed ordinary launcher fire with lighting
and occlusion on for the host and off for the client. Both exited successfully
and agreed on one host kill, one client death and the client's respawn. The
host recorded 132 rocket submissions; the client recorded none and no feature
occlusion. Evidence is in `build/rocket-network-rocket`, with private runner
`build/rocket-network.py`. This checks local multiplayer with differing visual
settings, not internet latency or browser transport.

Both active-fire transition runs (disabled at 30 FPS and enabled at 120 FPS)
completed Xbox Blood Gulch → CE Death Island → MCC Mercury Falling → Xbox
Blood Gulch. Both exited with status zero, completed all three transition
commands, loaded MCC once and logged no exceptions. This is lifecycle coverage,
not MCC visual acceptance. Reports and console logs are under
`build/rocket-transitions`; the private runner is `build/rocket-transitions.py`.
All copied test maps and derived map caches were removed; original maps and
saves remain untouched.


## Projectile lighting hardening (2026-10-14)

This pass keeps both settings off by default and changes only the feature-owned
occlusion module and diagnostic tests/tools. It adds no shared-engine hooks,
map-family routing, network state, projectile simulation or explosion changes.
Muzzle flashes, CStencil and Modern remain deferred.

Visibility fields retain the existing collision-ray count, refresh rules,
receiver reprojection, time-based clearing/blocking and overflow-admission fade.
The module now generates each model/diffuse/specular texture only when that
pass consumes it, at most once per view. It avoids trilinear sampling where
coordinates land exactly on texel centres and outside the attenuation sphere.
A failed texture upload falls back for that pass until the next view; it cannot
retry on every model draw. Texture masks are invalidated with each new field
update, and all allocated passes still release on reset.

The regression harness checks all texels against the previous expression for
specular scales 0.5, 1 and 2 with partial admission, allowing one 8-bit rounding
level. It checks per-pass upload reuse, next-view invalidation, allocation
failure/retry, map reset, synthetic doorway/corner filtering and overflow fade.
A negative control removing mask invalidation must fail.

The Carousel rocket specular investigation found the selected glow template to be
`weapons\plasma grenade\explosion`, flags `3`: dynamic plus no-specular.
Consequently zero rocket world-specular draws are intentional in that probe.
Blood Gulch selects `weapons\plasma rifle\muzzle flash`, flags `0x21`, which
permits specular; its outdoor receiver did not produce specular draws. Template
policy is map-dependent, and positive rocket-highlight coverage is not claimed.
The fixture now records the template flags, and the runner verifies this
policy instead of requiring highlights the template forbids. No explosion tag
is edited: the existing glow service supplies a definition for a separate
flight-light instance. Authored projectile light flags still take precedence.

Performance diagnostics can use `tools/plasma_visual_run.py --no-images` to
remove screenshot readback/writes while retaining frame and selection logs.
Reports include active-light average FPS and 95th-percentile frame intervals,
as well as the whole capture average. These are instrumented 640x480 tests,
not a guarantee for every resolution, machine or map.

Before optimization, twelve mixed lights at a 120 cap averaged 116.16 FPS with
screenshots and 119.99 without them; disabling occlusion also yielded 119.99.
The earlier 83 FPS result was not reproduced. Baseline evidence is in
`build/plasma-visual/*-harden-before-*` and `build/hardening-before.log`.
The baseline specular runner failed its old positive-draw expectation, prompting
the template-flag correction described above.

An O2 harness benchmark exercises 800 field updates with synthetic collision
and model/diffuse consumers. Uploads fall from 2,400 to 1,600. Warm samples were
about 0.160–0.162 seconds before and 0.095–0.096 after; first-launch timings are
excluded. This measures CPU field generation with mocked collision/upload,
not GPU cost or a whole-game speedup. Script/results:
`build/hardening-cpu-benchmark.py` and `build/hardening-cpu-benchmark.json`.


### Hardening acceptance results

The portable Windows release build and 28 focused harness tests passed, as did
ordinary/full-LTO light-storage checks and their negative control. The capture
manifest identifies base commit `5cc72528` plus the tested source hashes. Those
hashes match the final production modules, and the normal executable contains
no capture markers or fixture code. Self-review covered the complete diff;
no independent reviewer was used.

| Final scene | 30 cap | 60 cap | 120 cap |
| --- | ---: | ---: | ---: |
| Twelve mixed lights, no screenshots | 29.75 | 60.00 | 119.99 |
| Rocket flight/model impact, screenshots | 30.00 | 59.99 | 119.99 |
| A10 moving door, plasma probe, screenshots | 30.00 | 60.00 | 119.99 |

The 120-cap twelve-light screenshot run also averaged 119.99 FPS; its active
95th-percentile frame interval was 12 ms. Eight-rocket occlusion toggling
averaged 119.75 (11 ms active p95); the eight-rocket Carousel corner sweep
averaged 119.70 (12 ms p95). These demonstrate the tested average targets,
not a locked frame deadline. CE Death Island rocket flight averaged 120.02.
The 60 FPS disabled control selected no feature lights and did no occlusion
updates. Captures were inspected for rocket impacts, corner sweeps, moving
open/closed doors, model illumination and specular output. The stock fireball,
smoke and blast response remain visible. Coarse field-edge leakage remains an
approximation; this is not proof of artifact-free output on every map.

The Carousel plasma specular-only case recorded 2,049 selected specular draws.
The rocket case recorded none, correctly matching its template's no-specular
flag. The three initial A10 rocket-door cases failed because A10 lacks the
rocket projectile tag; each logged three missing-tag messages. They are excluded
from acceptance, and the replacement plasma-door cases pass at all three caps.
Budget admission/turnover smoothing is covered by the synthetic harness;
real door motion and twelve-light pressure are separate visual scenarios.

Final reports/images are under `build/plasma-visual/*-harden-final-*`, including
`sequence.png` contact sheets and `*-door-plasma/door-states.png`. Matrix and
follow-up scripts/results are `build/hardening-matrix.*` and
`build/hardening-followup.*`; `build/hardening-remainder.log` records follow-up,
network and transition execution. All 16 valid final visual/measurement cases
passed; the three unavailable-tag fixtures remain in the record.

Two hidden native loopback peers, with lighting/occlusion enabled only on the
host, agreed on the host kill, client death and respawn and exited successfully.
The disabled peer logged no feature occlusion. Both transition runs (disabled
at 30, enabled at 120) completed Xbox → CE → MCC → Xbox with zero exit status,
all three commands, one MCC load and no logged exceptions. Evidence is in
`build/hardening-network-rocket` and `build/hardening-transitions`. This covers
local networking and map lifecycle, not internet/browser transport or MCC
visual acceptance. Copied test maps and derived caches were removed.

The normal release executable also passed a hidden startup/load smoke test
with both settings false, no capture markers and a successful OpenGL context;
logs and its SHA-256 are in `build/hardening-playtest-smoke`. A Windows playtest
package uses these same executable/DLL bytes and keeps both options false.

Android/web build attempts still report unknown targets: configuration records
no Android NDK and no Emscripten. WSL reports that it is not installed, so Linux
runtime validation is unavailable here. These platforms remain unverified.
No simulation or network payload changed; the network version is unchanged.


## Projectile visibility tuning

The optional soft falloff now uses larger render radii: rifle bolts and authored
rocket attachments multiply their authored radius by 1.75, normal plasma-pistol
bolts by 2.25, and charged plasma-pistol bolts by 2.75. The charged bolt still
uses the rifle-directory tag. Feature-owned rocket flight lights use radius
1.75 instead of 1.0. The exact four-tag allowlist is unchanged. Plasma grenades,
needles, muzzle flashes and explosion lights are not added by this change.

The wider radius raises illumination at a given surface distance through the
existing smooth attenuation curve. It does not multiply or clip RGB colours,
change lens-flare brightness, or remove authored no-specular flags. This keeps
plasma colour and soft edges while making the smaller pistol lights easier to
see. Bright outdoor materials will still show less contrast than dark interiors.
Both feature settings remain false by default.

Policy remains in plasma_lighting.c and rocket_lighting.c. Minimal additive
hooks in object_lights.c are needed because its private light datum supplies
world/model receiver selection, and its private maximum-bound calculation
supplies visibility and cluster membership. Changing only rasterizer parameters
would leave the expanded light culled at its original bounds. The hooks apply
only to registered attachments or owned rocket lights with matching handles
and owners. Attachment registration precedes render-pose reconnection so the
partition sees the expanded bound immediately. Each attachment saves its prior
radius, restores it before simulation resumes, and reconnects with stock bounds;
map unload/recovery discards identities. Split views recompute from authored
radius rather than scaling previous output. Other lights retain their old bounds.

No tag data, projectile simulation, stock explosion effects, network payloads,
map-family adapters or CStencil/Modern code changes. No network-version bump is
needed for this render-only policy. Xbox, CE and MCC continue through their
existing loading paths.

Validation on Windows x86 portable release (full LTO):

- Thirty focused harness cases pass, including negative controls, exact profile
  scales, owner validation, maximum/actual/specular bounds, registration before
  partition reconnection, split-view non-compounding and disabled cleanup.
  Light-storage checks pass ordinary COFF, full LTO and the corruption control.
- All 32 main capture cases pass: each of four projectile types indoors on
  Carousel and outdoors on Blood Gulch at 30/60/120 FPS; twelve simultaneous
  mixed lights with model receivers at all three caps; disabled control;
  CE pistol flight; a moving A10 door; isolated specular output; rocket impact.
  Before/after images show wider coloured ground/wall illumination and stronger
  nearby model lighting, with the soft edge and stock explosion effects retained.
  Charged shots and Carousel's rocket template still forbid specular highlights.
- Actual weapon firing was captured at all three caps for all four types.
  Eleven of twelve initial cases passed timing; the first 120 FPS pistol capture
  averaged 111.85 FPS with 132/103 ms frame gaps. A targeted repeat averaged
  119.99 FPS with screenshots and 120.01 without; the old build also averaged
  119.68 without screenshots. The slowdown was not reproduced, but these runs
  do not establish its cause. The original failed report remains in the record.
  One old-build baseline rocket capture also missed timing outside its active
  lighting interval; it is used for visual comparison, not performance acceptance.
- The twelve-light image captures average approximately 30/60/120 FPS, with
  active-frame p95 of 34/18/12 ms respectively. These are short 640x480 tests on
  this machine, not a locked-120 guarantee or a high-resolution GPU benchmark.
- Two hidden native loopback peers, feature enabled only on the host, exited
  successfully and agreed on player state at tick 870. The disabled peer had no
  occlusion updates. This run establishes local state agreement, not internet
  transport, kill/respawn coverage or full networking compatibility.
- Disabled-at-30 and enabled-at-120 runs completed Xbox -> CE -> MCC -> Xbox,
  with three transition commands, one MCC load and no logged exceptions each.
  This is lifecycle coverage, not full MCC visual acceptance. Private copied
  maps and derived caches were removed; original game data was untouched.
- The normal Windows executable passes hidden startup/load with stock settings,
  contains no capture markers, and matches the capture manifest's source hashes.
  Android/web build attempts report unknown targets (toolchains unavailable);
  WSL/Linux is not installed. Those platforms remain unverified.

Evidence: build/tuning-matrix.json, build/tuning-combat.json,
build/tuning-timing.json, build/tuning-network-rocket,
build/tuning-transitions and build/tuning-playtest-smoke. Per-run logs, timing,
images and build hashes are in build/plasma-visual/*-tuned-*; old-build comparison
runs use *-tuning-before-*. Comparison sheets are tuning-all-*.png,
tuning-caps-*.png and tuning-combat-caps.png in that directory. The refreshed
Windows package retains both options off and includes PLAYTEST.txt beside halo.exe.


## Dynamic lighting baseline review

The polish review covers the existing optional feature, with every setting
still off by default. Production changes stay in feature-owned modules; no
`source/` hooks, loaders, simulation state, explosion effects, network messages,
CStencil or Modern code are changed. No network-version bump is needed for
these render-only changes.

| Area reviewed | Outcome |
| --- | --- |
| `plasma_lighting.c` and attachment hooks in `object_lights.c` | Exact projectile selection, bounded attachment storage, rendered-pose refresh and restoration of radius/dynamic/partition state remain intact. Owned glows are excluded by the engine's parent-light contract. |
| `rocket_lighting.c`, `covenant_lighting.c` | Authored attachments retain precedence, missing-light fallbacks remain bounded, salted handles/owners are checked, and stock impacts/explosions are unchanged. No manager rewrite or broader custom-tag matching. |
| `muzzle_lighting.c` and successful-shot hook | Render-time envelope, duplicate-shot guard, marker selection, expiry and owner validation retained. Only Warthog pulses add vehicle collision blockers. |
| `covenant_emitter_lighting.c`, AR/Needler material and pose modules | Damage colours, held/dropped policy, first/world poses and map-owned shader caches reviewed. Fixed AR view-gain leakage; revised Needler falloff as documented in `covenant_emitter_lighting.md`. |
| `ammo_counter_falloff.c` | Reuse unchanged masks across views; invalidate on shape changes. Keep independent AR mask when occlusion is disabled or unavailable. |
| `plasma_occlusion.c` | Stationary fields reuse the previous cell directly; moving/resized fields retain trilinear reprojection. Removed a redundant same-view check. Collision refresh, admission, edge filtering, fade rates and eight-field limit remain unchanged. |
| `plasma_falloff.c` and world/specular hooks | Stock texture/filter restoration, allocation fallback and cleanup retained. Shared radial volume remains lazy and bounded. |
| `plasma_model_shader.c`, `plasma_occlusion_gl.c`, shader translation and binding hooks | Selected two-light slots preserve stock alpha/material/cone paths. Reserved samplers, fallback texture and active texture restoration reviewed. Uniform-location/sampler caching is a possible later measured optimization; no shared renderer rewrite is justified by this pass. |
| Settings/build/lifecycle integration | Existing registrations, false defaults, feature-off paths, map cleanup and failed-load/recovery boundaries retained. Xbox/CE/MCC loaders and ownership boundaries are unchanged. |

The stationary shortcut is compared against full reprojection cell by cell over
synthetic moving lights, changed extents, door transitions and 30/60/120 Hz
sequences (absolute tolerance 0.00001). Cache tests cover unchanged views,
translation, radius, rotation, profile changes and cleanup. These establish
saved work and equivalent visibility, not a claim of a measured whole-game
FPS percentage. Geometry queries and GL binding remain potential costs;
reducing their quality or cadence was not part of this optimization.

### Polish validation and limits

- All 57 focused lighting regressions pass on the final production sources.
  Light-storage COFF/LTO checks and the negative legacy-corruption control pass.
  The broader harness/cache/viewmodel run has 506 passes, four skips and 28
  compile failures outside changed files: missing pthread support (10), missing
  clock_gettime/CLOCK_MONOTONIC (6), and Windows __int64 typedef conflicts (12).
  That broader run is not a green suite.
- The primary 53-case Windows capture matrix covers AR/Needler first/world views,
  both Jackal colours, stationary shields, held fuel rods, Warthog muzzle pulses,
  mixed plasma/rocket shots, twelve-Needle stress, moving doors, runtime toggles
  and per-pixel receivers at 30/60/120 FPS, plus eleven disabled cases at 60 FPS.
  All functional checks pass; 49 runs meet the timing threshold. Original
  screenshot timing failures remain recorded: AR first-person 30 (26.65), AR
  world 60 (49.37), Warthog 30 (26.91) and Warthog 60 (52.17 FPS).
  Separate final-source runs without screenshot readback pass at 30.01, 60.00,
  30.00 and 60.00 FPS respectively. Capture overhead is a plausible contributor;
  the repeats do not prove the cause of every original stall.
- Final-source follow-ups exercise front-facing radial Needler spill with wall
  occlusion off at all three caps and first-person fallback at 60. The front
  standing-player images show weapon/upper-body spill without a foot-level pool.
  The c40 Grunt fixture creates an emitter but does not frame the holder usefully;
  it does not establish low-NPC visual acceptance. A front-facing Marine on b40
  passes at 60.01 FPS and shows upper-body/weapon glow without a foot-level pool.
  Unusual/crouched poses remain
  a player acceptance item, rather than a claim of precise mesh shielding.
- A synthetic five-pair benchmark of 1,000 stationary updates reduces median
  CPU time from 88 to 53 ms, with the same 1,536,000 stub collision queries.
  Two unchanged mask variants over 1,000 views require two uploads instead of
  2,000. This measures avoided work with fake collisions/uploads, not game FPS.
- Hidden Xbox c40 -> MCC Mercury Falling -> Xbox Blood Gulch transitions pass
  disabled at 30 FPS and enabled at 120 FPS, with two commands, one MCC load,
  clean exit and no logged exceptions each. The first sandboxed attempt could
  not connect its localhost console and is retained as a failed fixture run.
  This checks lifecycle/load boundaries, not full MCC visual compatibility.
- Two hidden native loopback peers exit cleanly after 100 seconds and both
  submit Needler emitter samples (4,919/4,925). Of 84 common player snapshots,
  19 match exactly; all 84 match excluding loaded-ammo counts, which differ
  during firing. Final tick 2520 matches exactly. This is a bounded native
  regression check, not bit-identical live prediction or browser/internet
  transport acceptance. See `build/polish-network-needle/comparison.json`.
- The normal Windows portable release builds successfully, matches the final
  capture manifest's production hashes and contains no capture markers.
  Local Linux is unavailable (no WSL), and Android/web targets are unavailable
  in this Windows configuration. Those builds require CI; runtime acceptance
  on other platforms/GPUs remains separate. Local CE assets are absent, so no
  current CE runtime compatibility claim is made. Shared loaders are unchanged.

Evidence remains under `build/polish-acceptance.json`, `build/polish-followup.json`,
`build/polish-regressions-final.log`, `build/polish-full-tests.log`,
`build/polish-benchmark.json`, `build/polish-production-check.json` and the
per-run reports/contact sheets in `build/plasma-visual`. Captures are short,
640x480 fixtures on one Windows/NVIDIA machine, not a locked-120 guarantee.
The primary matrix predates only the final unoccluded specular-height correction;
final-source regressions/follow-ups cover that correction. Original game maps
and saves are preserved; private map copies are removed after each run.

## October 25 integration and menu acceptance

`main` at `2e69a88e` was merged without conflicts as `e5c43aeb`. The fixes
in `e91ba84f` retain source hue in enhanced world-specular reflections and
restore generated setting-row input after pointer/controller focus changes.
See the [rendering walkthrough](reading/rendering.md#dynamic-lighting-branch-notes)
and [menu walkthrough](reading/settings_input_and_menus.md#evidence-and-useful-checks)
for the call paths and isolation rationale. No settings defaults, network
protocol, projectile simulation, effect tags or stock explosions changed.

The final combined review covered both fixes, the merged interfaces,
feature selection/reset lifetime, stock constant identity, setting metadata,
generated-menu consistency, regression tests and these reading routes.
The inactive tint path returns the original packed colour directly. No
broader renderer or Xbox-widget refactoring was needed.

Local Windows portable release validation:

- The build passed. The merge's focused MCC HUD/loading/pause/UI checks
  passed (445 tests); final lighting, menu, XML and settings checks passed
  (96 tests). The colour and focus regressions include negative controls.
  Light-storage ordinary/full-LTO checks passed and detected the deliberate
  legacy-pointer corruption control.
- Fourteen controlled Carousel captures passed: pistol, rifle, rocket and
  Needler at 30/60/120 FPS, plus disabled and specular-only pistol captures.
  Measured average ranges were 29.26–30.00, 59.99–60.02 and 119.99–120.01 FPS.
  These are short 640x480 fixtures on one Windows/NVIDIA machine. Rockets
  and Needler fallback templates on this map prohibit world specular;
  that authored flag remains respected. The hue correction is exercised
  by the pistol and rifle, not inferred from those no-specular cases.
- Custom Edition Death Island's pistol fixture passed at 60 FPS, including
  selected model lighting. This scene did not submit world-specular draws;
  it is a CE runtime/feature compatibility check, not CE specular acceptance.
- The normal executable switched all eight menu toggles on and saved them,
  loaded them in a fresh process, discarded edits through both Cancel and
  controller Back, discarded pending Defaults through Cancel, saved Defaults,
  saved all eight off, reloaded off, and saved an unchanged screen. Logs and
  configuration files were checked against expected values. Captures show
  pending values as well as loaded values. Video Setup entry and return worked;
  640x480 and 1280x720 layouts were inspected. No menu XML edit was necessary.
- The first menu attempt used the executable directory's configuration rather
  than the runner's data directory. It was excluded from acceptance and the
  runner was corrected to use isolated executable/config locations. A separate
  a10 projectile placement attempt never reached a selected draw and is also
  excluded; Carousel supplied the reflective test surface.

Evidence is retained in ignored `build/oct25-final-tests.log`,
`build/oct25-merge-tests.log`, `build/oct25-light-storage.log`,
`build/oct25-menu-results.json`, `build/oct25-menu/`,
`build/oct25-visual-results.json`, `build/oct25-final-contact.png`,
`build/oct25-ce.log` and the `oct25`-labelled `build/plasma-visual/` runs.
Local Android/web targets were unavailable and WSL is not installed;
platform builds are checked by CI rather than claimed as local runtime tests.

The [four-platform Build run](https://github.com/chestahh/GulchCE/actions/runs/40222493166)
passed for `e91ba84f`. Xbox c40 → MCC Mercury Falling → Xbox Blood Gulch
transitions completed with all lights off at 30 FPS and on at 120 FPS,
each with one confirmed MCC load, both transition commands and no exception.
These verify load/cleanup boundaries, not MCC campaign or visual acceptance.
The sandboxed attempt could not reach its loopback console and is excluded;
the successful runs are under `build/oct25-transitions-localhost/`.

A normal-build Windows two-peer Blood Gulch smoke test used all eight
lighting settings on for the host and off for the client. Both exited
normally, with 37/33 two-player snapshots, eight scripted shoot events each
and no logged assertion, exception or shader failure. This verifies basic
mixed-setting session operation, not full state equivalence or internet/
browser crossplay. `tools/net_compare.py` could not run on Windows because
its process driver requires Unix signal APIs; the local Windows runner and
results are retained in `build/oct25-network-smoke.py` and
`build/oct25-network-smoke/`.

The [MCC validation workflow](https://github.com/chestahh/GulchCE/actions/runs/40222493204)
also passed. Temporary capture executables, copied maps and generated map
caches were removed. Selected screenshots remain as lossless PNGs; inactive
test saves were losslessly archived in `build/oct25-inactive-saves.zip`, with
original relative paths and SHA-256 hashes verified before removing loose
copies. Approximately 128 MiB of diagnostics remain, plus the 13.4 MiB
Windows playtest ZIP outside this worktree. Original game data, active
saves and existing toolchains/build outputs were preserved.
