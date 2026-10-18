## 2026-10-16 — Retail volume textures decoded and uploaded on both G4s

The globals bitmap visitor now includes real bitmap dependencies. After fixing
the stale model-buffer provider, the original frame reached first-person weapon
rendering and required the globals default textures. Converting the complete
globals bitmap set exposed the 32×32×32 distance-attenuation volume.

Native bitmap owners now support bounded, uncompressed, unswizzled retail 3D
mips. Each mip is linear slice-major data; the PPC CPU-sampling copy holds native
ARGB words, while GPU decoding uses the original little-endian bytes. The scoped
binder uploads real GL_TEXTURE_3D mip chains and requires every declared mip to
be available; it cannot silently truncate a failed lazy decode.

The actual Blood Gulch `rasterizer\distance attenuation` bitmap
`e7c2064e` (one 32³ A8R8G8B8 image) passes comparison of every CPU-sampling word
against the source bytes, plus full GPU RGBA-volume readback against an
independent byte conversion. Combined regression: **292 checks / zero failures
on native MacBook i386 and both G4s**. Logs:
`build/mac-audit/volume-upload-{build,powerbook,ibook}.txt`; PPC binary
`volume-upload-ppc`, SHA-256
`75d4de84556741d07c026d680845c978d3a9f6cdc4693d3443d28fabd512fdb9`.
Binder source SHA-256 `a3849be3dbfac8dad02ec05a7fc495b8e502f4d260da981570fa95ce29a5ac64`;
bitmap decoder `e37518a647768e1a73c6c82d2cd305e02463f6097be2f2f172406a9bf7059af0`.

Handle clarification: `e7c90655` is `rasterizer\default 2d`, containing four
4×4 X8R8G8B8 images. `e7ca0656` is `rasterizer\default 3d`, containing four
4³ X8R8G8B8 images. The earlier inference that e7c90655 itself was the 32³
volume was incorrect; direct retail records distinguish these dependencies.
The complete-frame integration remains pending; these checks do not establish
playability.

## 2026-10-16 — Real visibility queries integrated with the native renderer

Native D3D visibility calls now use actual ARB passed-sample queries, with
bounded slot ownership, asynchronous last-completed results and cleanup before
context destruction. Counts use actual viewport/sample scaling, excluding
power-of-two framebuffer padding. Headless behavior remains explicit.

The integrated test exposed a Radeon R300 behavior: with physical depth
storage present but depth testing disabled, a visible draw reported zero
samples. During queries the equivalent GL_ALWAYS comparison with depth writes
off restores counting while preserving logically disabled D3D depth behavior.
The private native target depth attachment already exists; no fake counts or
hidden gameplay callbacks were introduced. The original GL state is restored.

The combined device test passes **279 checks / zero failures on native MacBook
i386, PowerBook G4 and iBook G4**. The color-only logical target reports positive
counts (30 Intel, 42 on each Radeon); stencil-pass/reject reports 54/0 on all
three. Exact raster-count parity is not claimed. Logs:
`build/mac-audit/visibility-radeon-{build,powerbook,ibook}.txt`; PPC binary
`visibility-radeon-ppc`, SHA-256
`0a506586eacae9f7f3e23b0a7102d6f5498b0b2d8b99f362ddd607bb28b1387c`.
Device-state source SHA-256:
`2ed6611a8d2eec71343b876c8239940f0d315d046e8224fae4cb1f880b3951c7`.

This remains focused GPU evidence. The full renderer gets through the corrected
session/cache ABI boundary and genuine gameplay ticks, but its latest run stops
in the sky draw because reused model-buffer code lacks the new resource-range
registration. That provider is being rebuilt; no completed original frame or
playable session is claimed.

## 2026-10-16 — Native texture stages and combined GPU regression pass

The native draw path now uses a scoped four-stage texture binder. It selects
sampler types for the original pixel-shader key, uploads texture scale and
sampler uniforms, and restores bindings and temporary uploads after each draw.
The prior stage-0-only/point-wrap restriction is removed for supported native
resources. A checked view of the shared CPU decoder supplies native default
texture decoding without introducing the shared persistent GL cache.

The expanded device probe draws an actual Blood Gulch bitmap through each of
stages 0, 1, 2 and 3, checking the pixel result. Together with map geometry,
two-stream QUADLIST, off-screen target, alpha/fog/blend and stencil tests it
passes **254 checks / zero failures on MacBook i386 and both G4s**. Evidence:
`build/mac-audit/texture-stages-draw-{build,powerbook,ibook}.txt`; exact PPC binary
`texture_stages_draw_ppc`, SHA-256
`ccbfab219269fbf138f4217a9d280a4deb360e2f9b15bb1b3a81613b2cba48fb`.
This is a focused rendering milestone, not a complete Blood Gulch frame.

The separate integrated original renderer now gets beyond the previous stencil
clear failure and stops at its first unsupported live-stream vertex declaration.
The diagnostic identifies element 4, stream 0, type 33 (`D3DVSDT_NORMSHORT2`),
register 4, four bytes at offset 24 with stride 32. The subsequent 261-check batch adds that source-defined type and the other
formats in the original declaration table. A fresh full-renderer attempt then
exposed a stale session/cache object layout: the new map-directory field adds
1024 bytes before the session assets. Rebuilding its consumers from the same
header snapshot is required; old object manifests cannot be mixed blindly with
current headers. No completed original game frame is claimed yet.

## 2026-10-16 — Real depth/stencil framebuffer storage on legacy GPUs

Hidden CGL framebuffers now allocate an actual packed 24-bit depth / 8-bit
stencil renderbuffer and attach both aspects. Resize replaces and restores
both attachments together. The platform probe verifies depth occlusion,
stencil clear/readback value 37, and a fresh frame after resize: **three frames,
zero failures on MacBook i386, PowerBook G4 and iBook G4**. The observed GPUs
are GeForce 9400M, Radeon 9600 XT and Radeon 9550. No desktop window is opened.
Logs: `build/mac-audit/window-stencil-{build,powerbook,ibook}.txt`.

The D3D draw/clear bridge and render-target textures have corresponding stencil
changes; the later 233/254-check combined runs above and below verify stencil
write/read and color rejection on all three hosts. The hidden platform currently requires `GL_EXT_packed_depth_stencil`;
these results do not establish availability on the original minimum-spec Mac
GPUs or Tiger drivers.

## 2026-10-16 — Owned off-screen draws and multi-sampler GLSL verified

The native draw path now accepts registered render targets and validates its
viewport against their actual dimensions. Viewport/scissor Y conversion uses
the target height. A 16×8 owned framebuffer draw inside a 16×12 device verifies
the resulting pixel, alongside existing Blood Gulch geometry/texture checks:
**213 checks / zero failures on native MacBook i386 and both G4s**. Evidence:
`build/mac-audit/render-target-draw-build.txt`,
`render-target-draw-{powerbook,ibook}.txt`; PPC binary `render_target_draw_ppc`
SHA-256 `87c65f14b46bd0623363890963c97cc19bbf71aa3d8e5c288c4c5b9c8243a6cd`.

GLSL compatibility now selects `texture2D`, `texture3D` or `textureCube` from
the explicit declarations for stages 0–3. Unknown or undeclared samplers still
fail. The CPU translator passes 24 checks locally. The MacBook-built GPU probe
simultaneously samples separate 2D, volume and cube textures on stages 1–3 and
reads `(64,128,192,255)` on native Intel and both G4s: **12 checks / zero
failures** each. Logs: `build/mac-audit/glsl120-samplers-{build,powerbook,ibook}.txt`.
This verifies actual legacy GPU shader execution; connecting native texture
owners to all these stages remains separate work. Full game-frame integration
is still pending.

## 2026-10-16 — Real game audio reaches native output on all three Macs

A separate copy of the final38 two-cycle probe calls original `sound_render()`
after observer updates, matching the rendering lifecycle. The earlier headless
probe did not call it, so output callbacks alone produced silence. With this
source lifecycle call, all three machines pass **789 checks / zero failures**
and submit nonzero PCM through their active native output, with no silent-clock
fallback:

| Native host | Output callbacks | Output frames | Nonzero frames |
| --- | ---: | ---: | ---: |
| MacBook i386 | 11,108 | 6,190,255 | 191,641 |
| PowerBook G4 | 11,166 | 6,222,577 | 242,909 |
| iBook G4 | 13,691 | 7,629,706 | 258,970 |

These are observed mixer/output counters, not a listening or audio-quality
assessment. The variant adds no synthetic sound and does not replace the
production render callback. Exact logs are
`build/mac-audit/audio-stats-session-cache-low-soundrender/runtime_{i386,powerbook,ibook}.txt`.
The original rasterizer startup additionally passes 219 checks on all three
hosts, including little-endian default texture texel readback; evidence is in
`build/mac-audit/rasterizer-init-focus/`. Complete game-frame rendering remains
the next integration milestone.

## 2026-10-16 — Native GPU draw state and packed-input boundary verified

The native draw path now uses the shared pixel-shader key for alpha testing
and fog, uploads real fog/alpha uniforms, applies source-equivalent GL blending,
and supports color write masks. It preserves the previous blend state and GL
program. Focused framebuffer checks verify alpha rejection/acceptance, constant
alpha blending, an actual uniform fog-color result and red-only writes.

The expanded checker passes **206 checks / zero failures** on MacBook i386 and
both G4s, including the existing real Blood Gulch bitmap and packed/indexed
geometry cases. This is a bounded draw test, not the full map renderer.
Two integration issues were exposed and fixed: the generated shared SetIndices
body did not populate the native index-buffer state, and already expanded
packed vertex inputs were incorrectly requested as packed GLSL attributes.
The current generator excludes SetIndices; the native function owns both its
state and the shared D3D index pointer. Indexed shader generation now uses the
float-vector input contract after bounded CPU expansion.

Use current `mac_device_state_view.py`, `mac_glsl120.[ch]` and native resource
memory together; the old canonical graphics snapshots lacked newer texture
translation and range ownership. Evidence: `build/mac-audit/raster-state-fog-build.txt`,
`raster-state-fog-{powerbook,ibook}.txt`, exact PPC `raster_state_fog_ppc`
(SHA-256 `f75efba1b2b3d59899529ca01090b674d0345ff68ee541fe22e5e43c24ec8a0d`).
Engine source SHA-256 is
`b872eca6d62f515595e3e9af9df7b080f540b90c4beeb1205afce136d7dfe323`.

## 2026-10-16 — Two complete native gameplay cycles pass on all three Macs

Final38 passes **789 checks / zero failures** on native MacBook i386 and both
G4s. Both independently owned Blood Gulch sessions now perform real map
initialization, automatic player creation, thirty native-key movement frames,
twelve mouse-look frames, native mouse firing, old-map disposal and unload.
The selected Slayer engine persists across unload, matching the source lifecycle.
There are no allocator errors and both cycles yield identical results:

- movement delta `(-1.81338501, 0.889755249)`;
- first-person observer `(103.212738, -161.319427, 0.717425108)`,
  FOV `1.22173047`, cluster 3;
- mouse-look forward `(-0.897669673, 0.440669, 0)` to
  `(-0.846220791, 0.532832384, 0)`;
- selected weapon `e2cb005c`, firing time `0 -> 113`, seven frames with
  an actual player-owned projectile.

Asset ownership is 63,917,560 bytes on Intel and 97,879,728 bytes on PPC,
with 349 bitmap owners. Original sound-cache lifecycle and biped footstep
material-effect dependencies are included. This is headless engine evidence,
not a rendered playable session or verified audible output. The renderer's
original main_game_render graph links separately; device startup remains under
investigation. Evidence: `build/mac-audit/session-restart-build.txt`,
`session-restart-{intel,powerbook,ibook}.txt`, exact PPC `session_restart_ppc`
(SHA-256 `14fcd7d855eed67511247197cfcff37603e068ee47257d9acc949395b3e12f25`).

## 2026-10-16 — Biped material effects and repeated gameplay lifecycle batch

Final37 adds the selected biped's existing typed effect/sound/material-effect
reference traversal. `mac_native_object_effect_assets_load` now accepts `bipd`;
a separate resource wrapper preserves lightweight asset-loader callers. The
native session invokes it before shader/animation dependency completion.
This loads genuine `foot` data using the existing bounds-checked converter.

The same batch adds twelve frames of native mouse motion and asserts that the
original observer's forward direction changes. Both session ownership cycles
now load and initialize the map, create a player, move, look, fire, dispose,
and unload; earlier successful tests ran gameplay only during cycle one.
MacBook PPC/i386 builds succeed. All three native hosts complete the first
cycle with identical movement, first-person observer position/FOV, mouse-look
change and seven owned projectile frames. The PowerBook's second cycle exposes
a probe error: it tries to reselect the already selected game engine. Final38
selects Slayer only at startup and verifies that it persists across map unload,
matching the original repeated-map lifecycle. Expanded two-cycle results remain
pending; final37 is not a complete pass.
Evidence: `build/mac-audit/session-footsteps-build.txt`,
`session-footsteps-{intel,powerbook,ibook}.txt`, exact PPC `session_footsteps_ppc`.

## 2026-10-16 — Camera updates expose missing scenario sound-cache lifecycle

Final35 runs the original director/observer updates after the game clock.
All three hosts reach the automatically spawned player, then halt on
`data->valid` in the sound-cache datum array as ambient sound becomes audible.
The Intel debugger traces `sound_refresh_looping` through `game_sound_update`
and `game_frame`. This is a failed runtime test, not a camera pass.

The native scenario adapter replaced tag loading but omitted the original
`scenario_tags_load` call to `sound_cache_open`. Final36 restores that lifecycle
and closes the cache during native scenario unload, after old-map sound disposal
and before native permutation owners disappear. No private validity flags or
sound callbacks are substituted. Final36 builds for i386/PPC on the MacBook;
all three native hosts now pass that boundary and reach an actual biped footstep.
They stop on unconverted `foot` handle `e3a0022c` (`globals\\cyborg`). The
existing material-effect decoder supports this family; the next fix must add
the biped dependency traversal rather than bypassing footsteps. No final36
camera pass or clean teardown is claimed.
Evidence: `build/mac-audit/session-observer-{intel,powerbook,ibook}.txt`,
`session-soundcache-build.txt`, exact PPC `session_soundcache_ppc`.

## 2026-10-16 — Native mouse firing and projectiles pass the full engine loop

Final34b passes 451 checks / zero failures and both ownership cycles on
MacBook i386 and both G4s. After native keyboard movement, an 80-frame native
mouse hold/release sequence fires the actual selected plasma pistol.
Every host reports weapon `e2cb005c`, firing time `0 -> 101`, and four frames
with a projectile owned by the spawned player. Original weapon, AI-hearing,
collision, effects and frame updates run; no projectile is manually spawned.
Teardown has no allocator errors. This is still headless gameplay verification,
not a rendered playable session.

The BSP owner copies the real 435-byte triangular sound-distance matrix and
frees it with the world. Focused matrix tests pass 11,058/0 on MacBook i386/PPC
and both G4s, digest `fc9a4c00`. Evidence:
`build/mac-audit/session-soundclusters-{intel,powerbook,ibook}.txt`, exact PPC
`session_soundclusters_ppc`; `build/mac-audit/world-sound-cluster/`.
Final34 was a compile-only draft missing the new allocation's destructor free;
review caught it before runtime. Final34b includes the free and is the verified
binary. Final35 adds original director/observer and non-deterministic game-engine
updates from main's post-clock path, with first-person camera checks.

## 2026-10-16 — Native mouse firing reaches real AI hearing and exposes BSP sound data

Final33 preserves the clean native-keyboard movement path, then drives mouse
hold/release through native input/player control and the original clock.
All three hosts stop in `structure_bsp_get_cluster_encoded_sound_data` because
the native BSP has no `sound_cluster_data` yet. Intel debugger traces
`actors_handle_unit_effect` from `weapon_trigger_fire` / charged-weapon release
through original weapon/object updates and `game_time_update`.

The next owner change copies and validates the actual BSP triangular sound
matrix (one byte for each unordered cluster pair); only tag-data metadata
needs endian conversion. Do not bypass the original AI-hearing consumer.
No successful firing or clean final33 teardown is claimed.
Evidence: `build/mac-audit/session-nativefire-{intel,powerbook,ibook}.txt`;
exact PPC `session_nativefire_ppc`.

## 2026-10-16 — Native keyboard drives the full engine loop on both G4s

Final32 passes 287 checks / zero failures and both ownership cycles on all
three Macs. Native ANSI-W snapshots flow through input_update, input
abstraction, player_control_update and game_time_update; no direct queued
movement action or manual object update remains in this test. After automatic
player spawn, 30 movement frames produce identical displacement
`(-1.81338501,0.889755249)` on Intel and both G4s. Teardown stays clean.

Evidence: `build/mac-audit/session-nativeinput-{intel,powerbook,ibook}.txt`;
exact PPC `session_nativeinput_ppc`. This uses deterministic native snapshots,
not observed physical keypresses or rendered playability. Final33 adds an
80-frame native mouse hold/release sequence and checks the selected weapon's
firing timestamp plus observed player-owned projectiles without assuming ammo.

## 2026-10-16 — Clean original clock/frame lifecycle on both G4s

Final31 passes 256 checks / zero failures on MacBook i386 and both G4 PPCs.
The original `game_time_update(1/30)` schedules 31 ticks and runs `game_frame`
on every frame; the engine spawns its real player at tick 1 and moves it for
30 ticks with identical displacement `(1.07649231,-7.62939453e-05)`.
Both ownership cycles finish and no allocator warning occurs. The authentic
startup old-map disposal resolves the earlier double free without changing
native script ownership or the source engine.

Evidence: `build/mac-audit/session-clock-{intel,powerbook,ibook}.txt`; exact PPC
`session_clock_ppc`. This remains headless simulation, not rendered playability.
Final32 replaces explicit queued test actions with native W-key snapshots,
input abstraction and original `player_control_update`, then the same clock.

## 2026-10-16 — Invalid free traced to omitted authentic startup transition

Debugger `malloc_error_break` identifies `mac_native_scenario_scripts_free`
via world/session close. Source lifecycle audit explains the double free:
startup `hs_initialize` allocates syntax and sets its owned flag; main normally
calls `game_dispose_from_old_map` before first map load. The probe omitted
that transition, so map postprocess replaced the syntax pointer while leaving
the startup ownership flag set. Normal map disposal then freed cache-owned
syntax before its native owner did.

Final31 adds the authentic pre-load old-map disposal after `game_initialize`,
without changing the destructor or source engine. It also replaces manual
clock advancement/direct ticks with queued actions and original
`game_time_update(1/30)`, checking each clock increment. This additionally
runs original `game_frame` and interpolation boundaries. Results pending.
Evidence: `build/mac-audit/session-animsounds-free-gdb.txt`; source main's
`main_setup_connection` and `main_new_map` order. Final30 ends 225 checks/0
on all three hosts but is still a failed lifecycle test due to allocator errors.

## 2026-10-16 — Full engine spawning and thirty movement ticks match on all three Macs

Final30 resolves sounds from every loaded animation graph, including salted
handles represented as negative native longs. MacBook i386, PowerBook G4 and
iBook G4 each complete 31 original `game_tick` calls, with automatic player
spawn at tick 1 followed by 30 movement ticks. All report displacement
`(1.07649231, -7.62939453e-05)` and a valid cluster. This is headless simulation
proof; it does not establish rendered playability or the game-time/frame loop.

During session teardown after original map disposal, all three report an
invalid `free` (Intel `0x25cc3000`, PPC `0x1b165000`) before opening cycle 2.
Treat this as a failed lifecycle gate even if the probe's check counter stays
zero. A debugger breakpoint at `malloc_error_break` is being used to locate
the exact owner rather than masking the allocator diagnostic.

Assets: 349 bitmap owners, 62,384,473 bytes Intel and 96,346,641 PPC.
Evidence: `build/mac-audit/session-animsounds-{intel,powerbook,ibook}.txt`;
exact PPC `session_animsounds_ppc`. Focused animation sound visitor checks pass
3,278/0 on MacBook i386/PPC and both G4s, including signed salted handles,
NONE skip, malformed block/count and callback rejection; logs
`build/mac-audit/native-animation-sounds-{intel-final,powerbook,ibook}.txt`.

## 2026-10-16 — Original game_tick spawns a real player on both G4s

Final29 loads the separate globals first-person hands model and its shaders.
All three hosts complete the first original `game_tick`; the actual engine
creates player unit `e2c80059` at tick 1. The next tick reaches original
`first_person_weapons_update`, then fails on animation sound `e76205ee`
(`sound\sfx\weapons\weapon_anims\plaspistol_ready`). This is actual full-tick
player spawning, not manually attached objects. Thirty movement ticks, visible
playability and complete teardown remain unverified.

Assets: 349 bitmap owners, 60,534,779 registry-owned bytes Intel and 94,496,947
PPC. Evidence: `build/mac-audit/session-hands-{intel,powerbook,ibook}.txt`;
exact PPC `session_hands_ppc`. The next batch resolves sound references from
every loaded native animation graph before binding.

Focused lazy bitmap ownership and format9 engine sampling separately pass
65,748 checks on fresh MacBook i386/PPC binaries and both G4s. The earlier
Intel-only sampler discrepancy came from a missing test declaration and C89
float-to-double argument promotion, fixed with the exact source prototype.
Logs: `build/mac-audit/native-bitmap-lazy-ownership-{intel,powerbook,ibook}.txt`.

## 2026-10-16 — Full tick passes scenario decals and reaches first-person weapon setup

Final28 loads the actual scenario decal palette through the existing native
decal/bitmap owner (347 bitmap owners, 60,085,226 bytes Intel and 93,785,210 PPC).
All three hosts advance beyond the earlier `structure_decals_update` failure.
Intel debugger now records actual `objects_update` -> `unit_update` ->
`weapon_ready` -> first-person weapon switching, which requests missing globals
hands model `e8c3074f` (`characters\cyborg\fp\fp`). Both G4s reproduce it.
This proves further original tick execution, not a completed tick or verified
playable session. Final29 resolves actual model references from globals before
model shader closure, including first-person hands.

Evidence: `build/mac-audit/session-scenariodecals-{intel,powerbook,ibook}.txt`;
exact PPC `session_scenariodecals_ppc`.

## 2026-10-16 — Original map initialization reaches full game_tick on both G4s

Final27 loads the actual globals weapon-list closure (343 bitmap owners;
59,894,380 registry-owned bytes Intel, 93,419,548 PPC). Original `game_load`
and `game_initialize_for_new_map` return on MacBook i386 and both G4s.
The local-player test enters original `game_tick` and `objects_update`, then
aborts in `structure_decals_update` on missing actual scenario decal
`e7a1062d` (`levels\b30\decals-environment\decal fore symb red`).
This establishes map activation and entry into the real tick, not a completed
tick, player spawning, movement, or playability. Final28 connects the existing
validated scenario decal palette and native decal resource loader.

Evidence: `build/mac-audit/session-globalweapons-{intel,powerbook,ibook}.txt`;
Intel includes debugger stack. Exact PPC binary `session_globalweapons_ppc`.

## 2026-10-16 — Lazy native textures advance to game-engine resource prediction

Final26 loads 326 native bitmap owners on all three Macs. Registry-owned bytes
are 57,105,013 on MacBook i386 and 89,449,077 on both G4s; these are allocation
accounting totals, not process RSS or minimum-spec validation. Encoded/native
texture samples remain resident; RGBA views are allocated on demand.
Original `game_load` succeeds and original map initialization passes model-link
preparation, then stops in game-engine resource prediction because the globals
weapon list references regular Needler `e7f0067c`, absent from scenario pickups.
Intel debugger confirms `object_definition_predict` from
`game_engine_initialize_for_new_map`; both G4 runs reproduce the missing tag.
No completed map activation or full game tick is claimed.

Evidence: `build/mac-audit/session-lazytextures-{intel,powerbook,ibook}.txt`;
exact PPC binary `session_lazytextures_ppc`. The next integrated batch loads
all actual globals weapon-list roots and their resource dependencies before
model shader closure, including equipment entries without substituting roots.

## 2026-10-16 — Full shader textures reach A4R4G4B4 support

Final25 advances beyond the 128 MiB shader budget boundary on all three Macs,
then rejects `e72305af`, the plasma cannon first-person multipurpose bitmap.
Its real metadata is 256x256, depth 1, type 0, format 9 (A4R4G4B4), flags 385,
and eight mipmaps. This is a missing supported format, not an endian mismatch.
All three report 13 checks and one failure with normal teardown. Logs:
`build/mac-audit/session-shaderbudget256-{intel,powerbook,ibook}.txt`; exact PPC
`session_shaderbudget256_ppc`.

The portable decoder now expands all four 4-bit channels exactly as original
`bitmap_get_pixel`, using explicit little-endian reads. Independent unaligned
pixel fixtures and invalid-length tests pass 288 checks on MacBook i386 and
both G4 PPC hosts. Evidence: `build/mac-audit/textureformat9-{intel,powerbook,ibook}.txt`,
exact PPC `textureformat9_ppc`. Native bitmap ownership/sampling integration is
the next step; this focused result alone does not establish session loading.

## 2026-10-16 — Complete shader dependency walk reaches measured memory ceiling

Final23 exposed a missing new-family reference-visitor dispatch, independently
caught by the focused shader test and reproduced on all three integrated
hosts. Final24 adds that dispatch and resolves model shader texture dependencies
until the 128 MiB native ownership ceiling. Intel stops at shotgun bitmap
`e5950421` (133,256,080 bytes owned plus 1,575,648 requested); both G4s stop at
Scorpion body bitmap `e45a02e6` (134,015,432 plus 1,750,424). All runs fail cleanly
before binding, reporting 13 checks and one failure.

Evidence: `build/mac-audit/session-modelshaders-*` (final23),
`session-shaderclosure-*` (final24), and exact PPC binaries
`session_modelshaders_ppc` / `session_shaderclosure_ppc`.
Final25 tests a temporary bounded 256 MiB bring-up ceiling. This is explicitly
not the minimum-spec target. A separate reduction will retain RGBA mip views
only on demand, preserving pointer lifetime and charging each retained
allocation to the registry before allocation; encoded/native sampling bytes
remain real owned resources. No reduction is claimed before its tests pass.

## 2026-10-16 — Original game_load succeeds; model shader closure next

Final22 passes initial `game_load` on Intel, PowerBook G4 and iBook G4 after
correcting the premature reconnect callbacks. All three enter original
`game_initialize_for_new_map`, then abort on unconverted shader `e1b60042`:
`scenery\trees\tree_leafy_stump\shaders\tree_leafy_stump_edges` (`soso`).
This is new-map initialization, not yet completed activation or gameplay.
Evidence: `build/mac-audit/session-firstload-{intel,powerbook,ibook}.txt` and
exact PPC `session_firstload_ppc`.
MacBook debugger evidence (`session-firstload-gdb-intel.txt`) locates the
demand in `models_fix_transparent_part_links` called by
`render_initialize_for_new_map`, before object placement.

The native tag decoder now converts `soso` model shaders and the `sgla` and
`smet` families from the source-backed render schema. It derives the runtime
shader discriminator and exposes the actual bitmap dependencies through the
native reference visitor. The focused Blood Gulch probe decodes 117 soso, six
sgla and four smet roots; the failing `e1b60042` stump shader has two bitmap
references. Malformed detail enums and a wrong bitmap group are rejected with
empty output. PPC and i386 builds pass, and all three Macs report 394 checks
with zero failures. This is decoder/dependency evidence; the integrated map
initializer still needs a fresh run with these providers.

A read-only inventory of every model in Blood Gulch reports 155 soso, 51 schi,
14 scex, 8 sgla, 8 smet and 7 senv references (duplicates included). The
focused decoder covers all observed soso/sgla/smet roots; it does not establish
all-map readiness for the other shader families.

The focused bitmap probe now reports retained bytes by representation without
changing ownership or decode behavior. On the 128×128 cubemap `e24900d5`, the
PowerPC owner uses 1,574,276 bytes: 524,288 mapped encoded bytes, 524,280
host-endian sample bytes, 524,280 RGBA mip bytes and 1,428 bytes of metadata.
The Intel owner uses 1,049,996 bytes because it does not need the PPC sample
copy. On bitmap `e2a1012d`, the corresponding totals are 11,680 bytes on PPC
and 10,984 on Intel. These examples show distinct retained views; they are not
whole-session totals or process RSS. The focused probe's 186 checks pass on
Intel and both G4s. Evidence: `build/mac-audit/native-bitmap-{build,intel,
powerbook,ibook}.txt`.

## 2026-10-16 — Native map binding reaches original map-loading lifecycle

Final21 binds the complete current resident bitmap set on Intel and both G4s:
134 owners, 69,129,321 native-owned bytes on Intel and 82,532,761 on PPC.
These are registry accounting totals, not process RSS or final memory targets.
The explicit resident texture-service startup succeeds, as do the bound tag,
precache and texture checks. All three enter original `game_load` and stop at
`data.c` line 303 (`data->valid`). No map activation or full tick occurs.

Source comparison identified a native transition ordering error: initial
scenario loading must not run BSP reconnect callbacks. The source only runs
those when replacing an old BSP; object arrays become valid later during
new-map initialization. The next batch removes those premature callbacks and
explicitly limits the native adapter to first loading. It does not manually
mark arrays valid or substitute object updates.

Evidence: `build/mac-audit/session-resident-{intel,powerbook,ibook}.txt`, exact
PPC `session_resident_ppc`. The tests halt on the assertion; normal teardown
is not established for this run. Both G4 runtimes are Leopard.

## 2026-10-16 — Full current session asset closure loads on all three Macs

Final20 with the bounded 128 MiB ownership ceiling completes session asset
loading on MacBook i386 and both G4s. It then fails to bind the resident
texture service: that early provider only had 64 owner slots. All three
report 20 checks and three downstream binding/scope failures, with normal
teardown. Evidence: `build/mac-audit/session-budget128-{intel,powerbook,ibook}.txt`
and exact `session_budget128_ppc`.

The next integrated batch replaces the fixed array with dynamically sized
owner pointers, bounded by the cache's 65536 possible tag slots. It also
starts/stops the real resident texture-cache service explicitly in the
headless probe; this service ordinarily starts inside rasterizer startup,
which the current probe does not execute. Neither change claims a complete
renderer, map activation or gameplay. The probe now prints the loaded tag
bytes and bitmap owner count to make memory evidence directly reviewable.

## 2026-10-16 — Complete HUD ownership and measured integrated texture budget

The focused full interface closure now passes 210 checks on MacBook i386,
PowerBook G4 and iBook G4: 144 HUD references resolve after source/resource
files close, 36 tags are published, and two registry ownership cycles dispose
cleanly. The dedicated HUD message destructor fixes the prior integrated
cleanup crash; final18 and final19 dispose cleanly on all three hosts.

Final19 adds actual vehicle effect dependencies before object construction.
All three now reach the bounded aggregate native-tag ownership limit. Both
G4s fail identically on `eabb0945` with 65,195,899 bytes already owned and
2,101,328 requested against 67,108,864. Intel advances farther and fails on
`e54e03da` at 67,039,653 plus 95,564 bytes. This is measured resource budgeting,
not an endian conversion failure. A bounded 128 MiB bring-up limit is the
next experiment; minimum-spec memory use remains an optimization requirement.
Eager decoded mip views and PPC sample copies require further accounting.

Evidence: `build/mac-audit/session-vehicle-effects-{intel,powerbook,ibook}.txt`,
exact PPC `session_vehicle_effects_ppc`; all report 12 checks and one failure.
The original sound manager is active and full game initialization completes,
but session opening still fails before map activation/full game ticks.
No playable session is established. G4 runtime evidence remains Leopard only.

## 2026-10-16 — Native input and actual sound-manager startup on all three Macs

The final16 integrated session build now calls the existing native input
initializer and the original sound-manager initializer before full game
initialization. `sound_is_active()` is observed true on the Intel control,
PowerBook G4 and iBook G4. All three complete `game_initialize`, then fail
while loading HUD message text. Sound/input teardown and physical-memory
cleanup complete normally. This establishes manager startup, not audible
playback, Xbox DSP emulation, or a playable session.

The HUD failure is a caller initialization defect: the dependency walker
passes an uninitialized output tag to the new message decoder, which checks
for an existing root. The focused message converter itself passes all three
machines, including UTF-16 run bounds, terminators and native alignment.
The next build initializes the output before every dependency decode.

The integrated probe now contains the next real lifecycle gate: stock Slayer
selection, map load/activation, `player_new`, initial game pulse, and actions
through the actual local queues and full `game_tick`. These stages have not
yet been reached; no successful spawning or movement is inferred from them.

Evidence: `build/mac-audit/session-full-tick-{intel,powerbook,ibook}.txt` and
`session_full_tick_ppc` (final16). Each reports 12 checks, one failure. Both
G4 runs use Leopard; Tiger compatibility is still compile-target evidence.

## 2026-10-16 — Integrated cube/lens loading reaches placed-object animation validation

The combined native session now passes its former flashlight cube-map and
lens-flare failures on the MacBook Intel control, PowerBook G4 and iBook G4.
The original full `game_initialize` still completes. Session ownership now
loads the validated scenario scenery, sound-scenery and vehicle palettes,
using dynamic native palette counts and existing bounded asset converters,
and loads the BSP sound palettes before binding engine services. It does not
replace the original engine object-placement routine.

The final12 build stops consistently during session opening at native
animation conversion line 176 (node-table validation). This remains an
unresolved asset dependency; map activation and full `game_tick` execution
have not been reached. The prior final10 attempt accidentally used stale
bitmap sources; final11 refreshed those providers and final12 added the
actual physics decoder needed by vehicle loading. Both architectures link.

Observed logs: `build/mac-audit/session-placed-assets-{intel,powerbook,ibook}.txt`;
exact PPC executable: `build/mac-audit/session_placed_assets_ppc`. Each runtime
reports 10 checks and one failure, with cleanup completing normally. Both G4s
are running their installed Leopard systems; this is not Tiger runtime proof.
Next: identify and correctly support the failing animation graph, finish HUD
resource closure, then exercise the actual map lifecycle and player tick.

## 2026-10-16 — Effect light ownership reaches flashlight cube-map dependency

The native bitmap owner now decodes six-face cube maps with independently
owned RGBA views for every face and mip. Source and retail-cache witnesses
confirm the PC map's mip-major, six-faces-per-mip order; the GPU hardware
resource still retains its original encoded cache bytes and is not exposed as
a face-major cube payload. GPU cube sampling remains unsupported.

The bitmap probe checked the flashlight spotlight, Warthog headlight gel and
Scorpion headlight gel references (`e24900d5`, `e3f70283`, `e46802f4`). It
compared every face/mip against an independent retail texture decode, checked
the encoded bytes remain intact, and exercised malformed dimension/type
mutations. Intel, PowerBook G4 and iBook G4 each pass 400 checks; their
face/mip byte counts are respectively 524280, 262140 and 65616. The device
probe also passes 129 checks on all three machines, including the actual
2D retail texture draw/readback; it deliberately refuses cube sampling.

Evidence: `build/mac-audit/texture-bitmap/native_bitmap_{i386,ppc}` and
`native-bitmap-{powerbook,ibook}.txt`; `build/mac-audit/texture-device/device_state_{i386,ppc}`
and `device-state-{powerbook,ibook}.txt` under
`$PROBES/20261016/texture/`. The 2D draw uses a real owned bitmap
and checks its decoded texel against GL readback. This is bounded texture
sampling, not general retail pixel-shader composition or cube-map rendering.

## 2026-10-16 — Effect light ownership reaches flashlight cube-map dependency

The effect graph now uses the existing complete native light decoder and
publishes owned light roots. A new typed visitor follows both gel bitmap
references and the lens-flare reference, including when another owner already
published the light. It propagates failed callbacks and rejects wrong-family
roots. No referenced bitmap or lens flare is silently skipped.

The focused flashlight visitor passes 29 checks on Intel and both G4s,
including both exact references, callback refusal, wrong-family rejection,
and enumeration after source storage is freed. Digest remains `e50e95ff`.
The integrated session now advances beyond the previous unsupported-light
failure on all three machines and stops at bitmap `e24900d5`,
`characters\cyborg\bitmaps\spotlight cyborg`: a 128x128 cube map, format 10.
This is a genuine unsupported bitmap family, not a light endian failure.
The native bitmap owner is being extended to own all six faces; the next
known dependency is its lens flare `e24a00d6`.

Evidence: `build/mac-audit/light-references-{build,powerbook,ibook}.txt`,
`light_references_ppc`, and `session-light-startup-{intel,powerbook,ibook}.txt` /
`session_light_startup_ppc` (final7 integrated binaries). Startup still reaches
the same real game initializer and cleans up after the failed map load;
no successful new-map lifecycle or playability is claimed.

## 2026-10-16 — Full game startup executes on Intel and both G4s

The integrated session executable now links on PPC and i386 with the genuine
`game_tick` retained, the complete game initializer, and the full UI callback
table. All three machines execute original `game_initialize` through its last
progress-bar initializer, then attempt the owned Blood Gulch session load.
The observed next failure is `effect light dependency unsupported`. All three
then complete `game_dispose` and physical-memory cleanup, reporting 10 checks
and one failure. This is real complete global initialization, not successful
map initialization or a playable session.

The initial Intel run lacked the native data-root environment and stopped in
`cache_files_map_directory`, called by `progress_bar_initialize` (confirmed
with GDB). Selecting the actual installed GameData directory fixes that setup.
The iBook test uses scratch `maps` symlinks to its existing v7 files; both tests
use an isolated save root. Native filesystem behavior is unchanged.

Evidence: `build/mac-audit/session-full-startup-{intel,powerbook,ibook}.txt`,
`session-full-startup-gdb-intel.txt`, and `session_full_startup_ppc` (MacBook
`build/mac-game-tick-session-final5` binaries). The debugger log records the
earlier missing-data-root failure; the run logs record the corrected setup.
The next change adds actual owned light dependencies to the effect graph.

## 2026-10-16 — All 67 retail vertex shaders on legacy GL drivers

The GLSL 1.20 adapter now translates the NV2A generator's exact bounded
relative constant expression into floating-point `clamp` plus an integer
array index. GLSL 1.20 lacks the original integer clamp overload. The
translation preserves the `[0,191]` bounds and accepts only the generator's
8-bit constant offset; unsupported expression forms still fail closed.

On Intel, PowerBook G4 and iBook G4, all 67 original retail vertex programs
were generated, translated, driver-compiled and linked against the actual
NV2A-generated untextured pixel shader, with zero rejections. The probe also
renders VS0 and checks its color. A separate hidden-CGL test executes relative
reads below/at/inside/at/above the bounds, checking red/green/blue output for
five addresses: 34 checks pass on all three machines. The converter's unit checks pass 18 checks on all three machines. The retail GPU gate now requires all 67 programs
to compile/link rather than allowing a partial inventory to pass.

Evidence: `build/mac-audit/nv2a-relative-{build,powerbook,ibook}.txt`,
`nv2a_relative_ppc`, and `glsl-relative-address-{build,powerbook,ibook}.txt` /
`glsl_relative_address_ppc`. This proves legacy-driver vertex shader support
for these programs and bounded constant lookup, not all textured pixel
combiners, complete engine rendering, or a playable session.

## 2026-10-16 — Owned native interface text and HUD roots

Added `native_interface_assets` to publish complete existing HUD globals and
owned ASCII/UTF-16 string-list roots. The text reader checks source spans,
short-sized counts and payload limits, even UTF-16 byte lengths, and a 16 MiB
per-list allocation bound. UTF-16 words are converted from little endian;
terminators follow the original text getter's convention. No font or other
HUD dependency is implied by this loader.

MacBook PPC/i386 builds succeeded. Intel, PowerBook G4 and iBook G4 each pass
24,909 checks over 55 string lists and 915 strings, with digest `c379d612`.
The probe compares characters against original little-endian bytes, rejects a
malformed counts, addresses, oversized payloads and odd UTF-16 lengths, frees source map storage, then rereads every owned string
and verifies the same digest. Evidence: `build/mac-audit/interface-assets-build.txt`,
`interface-assets-{powerbook,ibook}.txt`, `native_interface_ppc`, and
`interface-assets-sha256.txt`. This supplies map-initialization data; it is not
a complete HUD renderer or playable session.

## 2026-10-16 — Native interpolation setting service

Added the platform interpolation query to the native shared-settings adapter.
It reads the existing `display.interpolation` setting, so menu changes and
existing environment precedence remain owned by the unchanged settings parser.
The MacBook compiled PPC and i386 versions; Intel and both G4s each passed
nine checks, including toggling interpolation twice and reading the saved
settings file. Evidence: `build/mac-audit/interpolation-config-build.txt`,
`interpolation-config-{powerbook,ibook}.txt`, and `interpolation_config_ppc`.
This closes one full-session link dependency; it does not establish rendered
interpolation or a playable session. Full startup and native drawing work
continue in parallel.

## 2026-10-16 — Player-queued rifle controls on both G4s

The firing probe now delivers selection, trigger and release as actual
`player_action` records through `update_client_queue` and
`players_update_before_game`, with real distributed tick advancement. The
original player update supplies unit control; original unit and weapon updates
ready the rifle and fire it. It no longer directly injects weapon control flags.
Ten neutral updates after release verify that ammunition stops decreasing.

Intel and both G4s each pass 270 outer checks plus two sets of 87 queue/PVS
checks. Per cycle, 30 firing ticks consume 15 rounds and create 15 correctly
owned rifle projectiles. Flight, impact/decal, shield damage and natural
particle retirement remain unchanged. Connection, queue ownership and game
time are restored; final arena teardown succeeds. The preceding direct unit
control/release version separately passed 268 checks on all three machines.

Evidence: `build/mac-audit/rifle-player-queue-runtime-{build,powerbook,ibook}.txt`,
`rifle_player_queue_runtime_ppc`, and `rifle-player-queue-runtime-sha256.txt`.
The direct-unit-control evidence uses the same filenames with
`rifle-unit-control-runtime` / `rifle_unit_control_runtime_ppc`.
These remain focused incremental executables with the documented reused source
object baseline. They establish queued player-to-weapon control, not physical
keyboard/controller input, a complete `game_tick`, engine drawing, or a playable
session. The summary now labels its original 120 steps `base_steps`, separate
from the logged 841 retirement updates per cycle.

## 2026-10-16 — Real equipped rifle trigger firing on both G4s

The native effect loader now traverses all source-derived weapon/equipment/
projectile effect, sound and damage references, including firing, misfire and
empty-trigger records. Model, HUD and other object families retain their own
asset owners. This replaces the narrow explicit projectile-only dependency
list while preserving checked tag traversal and ownership.

The player/effect runtime selects the actual inventory rifle through
`unit_control`, gives the original `unit_update` and `weapon_update` paths 60
updates to ready and attach it, then drives the real `weapon_owner_update` /
`weapon_update` trigger path for 30 updates. Intel, PowerBook G4 and iBook G4
all passed 266 outer checks plus two sets of 87 queued-player/PVS checks over
two map cycles. Each cycle consumes 15 rounds (`60 -> 45`) and creates exactly
15 rifle projectiles; each definition and owner exclusion is verified.
Ground impact/decal, direct flight, shield damage and final arena cleanup pass.

The first trigger attempt exercised an unequipped inventory weapon and thus
had no biped projectile owner. Actual unit selection/readiness fixes that
setup; no parent pointers are fabricated. Spent casings last longer than the
original two-second probe: all three machines need 841 additional real effect/
particle/contrail update steps per cycle for natural retirement. The summary's
`steps=120` counts only the two original 60-step phases; retirement is separately
logged (1,802 total effect/particle update steps across both cycles).
`mac_engine_arena_stop` now reports the name and count of any still-live data
records when refusing teardown, without weakening that check.

Evidence: `build/mac-audit/rifle-trigger-runtime-build5.txt`,
`rifle-trigger-runtime-powerbook.txt`, `rifle-trigger-runtime-ibook.txt`,
`rifle_trigger_runtime_ppc`, and `rifle-trigger-runtime-sha256.txt`.
This is an incremental native-runtime link retaining the documented source
object baseline. It proves equipped weapon firing, not live physical input,
full `game_tick`, visual weapon drawing or a playable session. Full map startup
and authentic GPU draw integration remain active work.

## 2026-10-16 — Real rifle ground impact and decal creation on both G4s

The focused runtime now creates a second real rifle projectile aimed at the
Blood Gulch ground, then calls the original object-type update dispatch. Intel,
PowerBook G4 and iBook G4 each passed 194 outer checks plus two sets of 87
queued-player/PVS checks over two map cycles. All three observed material 7,
projectile action 2 (disappear), impact height `1.70348573`, and a nonempty real
decal datum pool. The earlier matching flight step and shield damage checks
still pass, as do 120 particle/contrail updates and final arena cleanup.

The test now uses the actual decal startup/map/disposal lifecycle. An initial
attempt reached decal creation without its vertex-cache initializer and failed;
another cleanup attempt deleted decal datums before the cache purge callback.
The final path lets `decals_dispose_from_old_map` flush its cache and delete the
datums in the original order, then verifies the pool is empty. No engine logic
or callbacks were replaced.

Evidence: `build/mac-audit/rifle-impact-runtime-build3.txt`,
`rifle-impact-runtime-powerbook.txt`, `rifle-impact-runtime-ibook.txt`,
`rifle_impact_runtime_ppc`, and `rifle-impact-runtime-sha256.txt`. The binary is
an incremental relink of the full focused flight build with the latest native
effect/tag providers and probe. Its documented source-object baseline remains;
this does not demonstrate a full current-tree engine build or a playable
session. Trigger-driven weapon firing, full native map startup and engine GPU
drawing remain the immediate integration work.

## 2026-10-16 — Rifle flight matches Intel on both G4s

A fresh full focused-checker build includes the latest native GPU device-owner
implementation while running this test in its explicit headless mode. Intel,
PowerBook G4 and iBook G4 each pass 184 outer checks plus two sets of 87
queued-player/PVS checks, two map cycles, 120 particle/contrail update steps,
and complete arena cleanup. Each cycle creates a real rifle projectile and
calls `object_type_update`, which dispatches the original projectile simulation.
All three machines observed the identical first flight step:
`(98.4934006, -157.639008, 2.70473003)` to
`(109.293404, -157.639008, 2.70294738)`.

Evidence: `build/mac-audit/rifle-flight-runtime-full.txt`,
`rifle-flight-runtime-powerbook.txt`, `rifle-flight-runtime-ibook.txt`,
`rifle_flight_runtime_ppc`, and `rifle-flight-runtime-sha256.txt`.
The checker refreshes its native providers but still reuses the documented
scenery source-object baseline. This proves projectile creation and one flight
step, not collision impact, trigger-driven firing, a full current-tree engine
build, or a playable session. The full startup/map-transition and GPU drawing
work continues separately with the lower-cost agents.

## 2026-10-16 — Actual rifle projectile creation on both G4s

The focused player/effect probe now optionally takes `sounds.map` as its third
asset argument, loads the rifle projectile's real effect/damage resources, and
creates the projectile through `object_new` with the spawned biped as owner.
Intel and both G4s passed 180 outer checks plus two sets of 87 queued-player/PVS
checks: projectile velocity `(10.8000002, 0, 0)`, cluster 3, and correct owner
exclusion. Shield damage remains `1 -> 0.833333313`, body `1 -> 1`.

Actual contrail initialization and updates were needed for the bullet's tracer.
The initial creation-only run left a detached trail alive at arena shutdown;
normal contrail updates now retire it, and arena teardown passes. Evidence:
`build/mac-audit/rifle-projectile-runtime-build3.txt`,
`rifle-projectile-runtime-{powerbook,ibook}.txt`, and
`rifle_projectile_runtime_ppc`. This incremental executable retains the earlier
verified headless device/source-object baseline; it does not yet include the
concurrent new GPU device owner. Projectile flight/impact and trigger-driven
weapon firing remain separate verification steps, as does a playable session.

## 2026-10-16 — Impact and rifle-projectile resource graphs verified

The owned effect loader now follows projectile impact, attached-detonation,
super-detonation, flyby-sound and per-material response references. All five
Blood Gulch material-impact roots loaded with their transitive resources on
Intel and both G4s: 664,218 checks, 220 tags, 40 bitmaps, pixel digest `d3fd7f9a`.
Adding the assault-rifle projectile dependency graph passed 766,769 checks on
all three Macs: 288 tags, 53 bitmaps, pixel digest `2a748945`.

Evidence: `build/mac-audit/material-effect-assets-{build,powerbook,ibook}.txt`
and `projectile-effect-assets-{build,powerbook,ibook}.txt`; corresponding PPC
binaries are retained alongside them. Most checks validate decoded pixel words,
not gameplay frames. This proves resource ownership; the next focused runtime
check creates the actual projectile through `object_new`. Trigger-driven firing
and collision-impact simulation are not yet demonstrated by these results.

## 2026-10-16 — Native material-impact table conversion

Added source-derived `foot` roots, their effect rows and material rows to the
owned native decoder. Limits (13 effects, 33 materials) and effect/sound
reference families come from the existing clean shared schema. The asset queue
can now follow this family instead of rejecting it outright.

MacBook Intel, PowerBook G4 and iBook G4 each passed 1,196 checks for all five
Blood Gulch roots and 188 references; semantic digest `062c2824` matches.
Checks compare nested native records to little-endian source values, reject
oversized counts and invalid block pointers, then traverse owned references
after releasing the source map. Evidence is `build/mac-audit/material-effects-`
`{build,powerbook,ibook}.txt` and `material_effects_ppc`.

This proves storage and reference traversal, not impact execution. The full
transitive effect-resource graph is being checked separately and can still
reject unsupported downstream families. Rifle damage runtime also passed a
repeat full focused-checker build (`damage-player-runtime-full.txt`); it retains
the documented reused source-object baseline.

## 2026-10-16 — Real rifle damage reaches spawned players on both G4s

The native particle/player probe now loads the assault-rifle bullet damage tag
and calls the original `object_cause_damage` on the actual spawned cyborg.
MacBook Intel and both G4s each passed 172 outer checks plus two sets of 87
queued-player/PVS checks over two map cycles. Every run observed shield vitality
`1 -> 0.833333313`, body vitality `1 -> 1`, alongside native particle emission.
This is damage execution, not just successful decoding of the damage tag.

The first attempts exposed missing real rumble and allegiance initialization.
The probe now initializes and disposes the actual allegiance, rumble, player
effects, input, and player-profile services in their engine lifecycle. Keyboard
and mouse input does not imply a physical rumble device. No gameplay callback
was substituted. The full checker includes the native input and profile owners
and links Carbon without duplicating the input backend in view builds.

Evidence: `build/mac-audit/damage-player-runtime-relink3.txt`,
`damage-player-runtime-powerbook.txt`, `damage-player-runtime-ibook.txt`, and
`damage_player_runtime_ppc`. These focused executables still reuse the verified
scenery source-object baseline; changed native providers and this probe were
rebuilt on the MacBook. This is neither a full current-tree build nor a playable
session: trigger-driven projectile firing, complete native map startup, and
real engine rendering remain to be integrated. G4 evidence is Leopard, not a
Tiger runtime test.

# PPC Mac port progress

## 2026-10-16 — native damage definitions join effect resource loading

- Added native `jpt!` damage conversion and reference visitation. Inline nested
  structure arrays now decode, hash, and visit every element, including both
  vibration channels. The source-native root is exactly 672 bytes (`0x2a0`),
  matching the independently inspected spacing of all 81 Blood Gulch damage
  roots. Static assertions witness root size, damage payload `0x1c4`, and sound
  reference `0x114`; no PC-to-Xbox size expansion is needed.
- All 81 damage roots pass 920 checks on Intel and both G4s, including known
  assault-rifle bullet category/damage values, second-channel endian conversion,
  malformed enum/nonfinite float/reference rejection, and reference visitation
  after freeing map bytes. Twelve sound references and field digest `2cd3a821`
  match on all hosts. Evidence: `build/mac-audit/damage-effects-build.txt` and
  `damage-tags-{powerbook,ibook}.txt`. A `cdmg` storage decoder is compiled but
  Blood Gulch contains no such roots; its runtime use is not validated or enabled
  in the effect resource queue.
- The bounded effect queue now loads damage roots and their sound dependencies.
  The resource probe adds actual assault-rifle bullet and cyborg melee damage
  alongside the rocket graphs, selects by group as well as name (projectile and
  damage tags can share a name), and checks repeat loading/owned references.
  All three hosts pass 532,572 assertions: 14 published tags, four bitmap roots,
  pixel digest `053d014e`. Most assertions are pixel comparisons, not gameplay
  coverage. Evidence: `build/mac-audit/damage-effects-assets-build.txt` and
  `damage-effect-assets-{powerbook,ibook}.txt`.
- Damage storage/resource closure does not yet prove weapon firing or vitality
  changes. Next: exercise `damage_data_new/object_cause_damage` on a real spawned
  biped with normal engine damage/player initialization, then connect projectile
  and firing dependencies. Use a fully refreshed native runtime build after the
  actor bitmap owner ABI change; earlier focused particle binary evidence remains
  valid only for its recorded source/object baseline.

## 2026-10-16 — real player visibility now drives effect emission

- Extended the particle runtime to create the actual multiplayer biped and its
  starting weapons, allocate and associate a local player, and feed an action
  through the real queue/player update. Native BSP PVS is computed by the
  engine: spawn cluster 3, 30 clusters, four hidden. No visibility bits or
  private active-game flags are written by the probe.
- The ordinary rocket-exhaust effect is first tested without an observer and
  correctly culled. After player PVS computation, the same actual effect emits
  an ordinary particle. Its zero-duration effect record can finish immediately;
  the probe checks emitted particle storage, not merely a returned effect index.
  The separate actual particle system peaks at three particles.
- Visible emission exposed the missing random-direction geosphere table. The
  runtime now calls the real `random_math_initialize/dispose` pair. Added the
  missing `system_seconds` platform provider with the source API's Unix-time
  semantics; it is distinct from the monotonic millisecond clock. The clock
  suite passes 217 checks on Intel and both G4s.
- Observed on all three Macs: two complete map cycles, 120 effect/particle
  updates, 165 outer assertions and two sets of 87 queued-player/PVS assertions,
  zero failures, one emitted ordinary particle. Logs:
  `build/mac-audit/player-effect-runtime-{build,powerbook,ibook}.txt` and
  `clock-seconds-{intel,powerbook,ibook}.txt`.
- Build provenance: MacBook GCC 4.0/Tiger SDK, freshly compiled native providers
  and probes plus the verified `mac-scenery-view-fixed` engine object baseline.
  This focused binary does not prove a full current-tree frontend rebuild or
  incorporate unrelated later source changes. Refresh all source objects for
  the final integrated build. Effects are simulated, not drawn; live input,
  stock pickup/game-engine initialization, and full session rendering remain.

## 2026-10-16 — native BSP shader storage and real particle updates

- Generated complete `senv`, `schi`, and `scex` native storage layouts from the
  shared render validator. The decoder owns block/reference names, validates
  bounds and finite fields, checks shader/animation table indices, and derives
  the runtime shader discriminator. All 69 relevant Blood Gulch tags pass 735
  assertions on the MacBook and both G4s, including malformed-enum/reference
  rejection and reference walks after releasing source bytes. Reference count
  157 and typed-field digest `1a21ffd9` match across architectures. Evidence:
  `build/mac-audit/environment-chicago-{build,powerbook,ibook}.txt`.
- Added a native-world material shader accessor and explicit lighting resource
  loader. It publishes complete material shader storage plus the environment
  base-map and BSP lightmap bitmaps used by the real `light_particle` function.
  Chicago map/layer/lens rendering dependencies remain separate; preserved
  extended map storage still needs renderer normalization before drawing.
- The first real particle creation reached `light_particle` and exposed missing
  breakable-surface initialization, confirmed in the Intel debugger through
  `breakable_surface_flags_get` and `collision_test_vector`. The runtime probe
  now uses the real breakable-surface and collision-log lifecycle.
- Real rocket-exhaust particle-system creation, lighting, 120 updates, and
  disposal across two map cycles passed on Intel and both G4s (147 assertions,
  peak three particles). Source map/resource mappings are released before
  runtime. Evidence: `build/mac-audit/particle-runtime-relink.txt` and
  `particle-runtime-{powerbook,ibook}.txt`. Assertions check lighting
  ranges and require particle creation independently in each cycle.
- Extended the same scope with the real player, ordinary particle, and effect
  pool lifecycles. The actual non-looping rocket exhaust effect is correctly
  culled because the scope has no local observer; the probe checks its effect
  and location allocations are reclaimed rather than claiming visible emission.
  The separate particle system still emits and updates. Combined results are
  153 checks, zero failures on all three Macs, peak three particles, two map
  cycles and 120 updates. Evidence:
  `build/mac-audit/effect-particle-runtime-{relink,powerbook,ibook}.txt`.
  Next effect milestone: use the existing actual biped/player/queued-action path
  to compute player PVS and verify visible effect emission, without writing
  visibility bits or private game flags.
- This is observed native simulation and lighting, not particle drawing or a
  playable session. Complete input/game-engine integration remains outstanding.

## 2026-10-16 — effect validation connected to item resource loading

- Added `mac_actor_assets_load_item_collection_with_resources`: it resolves
  weapon/equipment effect attachments and weapon-trigger projectile attachments
  before the existing actor/model/animation loader publishes each item. Native
  actor attachment handling now accepts an already loaded matching `effe` or
  `pctl` root; absent resources still fail explicitly. No attachment is deleted.
- The integrated Intel collection run advances beyond the rocket launcher and
  stops at `item collections\single weapons\flame thrower` with `animation
  seat fields`. This collection is actually referenced by Blood Gulch's netgame
  equipment, so it must be resolved before claiming the full pickup asset batch.
  Evidence: `build/mac-audit/item-effects-build.txt`. The full collection batch
  has not passed and has not been claimed as hardware-validated.
- Particle systems now validate loaded bitmap sprite sequences after their
  dependency queue closes. Negative times/radius multipliers and zero-time loops
  receive the shared render validator's corrections; an unusable active sprite
  sequence fails loading explicitly. A corrupted sequence is rejected, restored,
  and successfully reloaded in the rocket resource probe. All three Macs pass
  532,562 assertions; pixel digest remains `053d014e`. Evidence:
  `build/mac-audit/effect-assets-validated-{build,powerbook,ibook}.txt`.
- Particle storage also bounds sprite orientation and applies the shared
  validator's radius/frame-rate/contact corrections. All 468 effect/particle
  roots pass 13,103 storage assertions on Intel and both G4s, with malformed
  orientation rejection. Evidence:
  `build/mac-audit/effect-storage-orientation-{build,powerbook,ibook}.txt`.
- Effects are still not executed in a full native `game_tick`; this is resource
  and consumer-boundary validation. Runtime particle-system initialization,
  weapon firing, full cache startup and interactive input remain separate work.

## 2026-10-16 — rocket exhaust resource graph on Intel and both G4s

- Added `native_effect_assets`, a bounded work queue over actual referenced
  tags. It visits cyclic references once, validates handle/family pairs, loads
  owned effect/particle/particle-system/contrail/decal/sound payloads plus bitmap
  pixels and point physics, and fails explicitly on unsupported families.
  Failed batches retain their owned allocations and must be disposed. No
  unsupported attachment is removed, and no raw tag bytes become engine roots.
- Blood Gulch's rocket exhaust effect, smoke particle system and exhaust
  contrail now load together: 12 tags and four bitmaps. The MacBook Intel run
  and both G4 runs pass 532,559 assertions with identical pixel digest
  `053d014e`. Most assertions compare individual decoded pixels with the native
  CPU sampling view after the original map/resource mappings are released;
  this count is not a count of gameplay tests. Evidence:
  `build/mac-audit/effect-assets-all-{build,powerbook,ibook}.txt`.
- The flash bitmap is actual A1R5G5B5, previously unsupported. Added its bounded
  RGBA conversion and native 16-bit PPC sampling; added native X8R8G8B8 and
  A8R8G8B8 ownership using the existing RGBA decoder, including the real rocket
  contrail's A8R8G8B8 pixels. Encoded GPU bytes remain unchanged; CPU words are
  converted to host order. Independent texture fixtures pass 281 checks locally.
- Particle-system function-table and sprite-orientation indices now reject
  out-of-range values before publication. The all-effect storage probe passes
  12,891 checks on Intel and both G4s, still digest `109d79b5`.
- This closes the tested rocket attachment resource graph, not weapon firing or
  particle simulation/rendering. Runtime bitmap sequence/loop validation and
  integration into the weapon/collection loader remain next; the full game
  startup/cache/UI/input path is still under construction. Tiger remains SDK
  compile coverage; observed G4 runs use their installed Leopard systems.

## 2026-10-16 — native equipment assets and effect storage

- The equipment asset loader resolves real model, collision, animation and
  supported attachment/sound dependencies. All 14 Blood Gulch equipment roots
  pass 43 ownership checks on Intel and both G4s (49 published tags, 1,081,486
  owned bytes). Evidence: `build/mac-audit/equipment-assets-{build,powerbook,ibook}.txt`.
  Collection loading now resolves actual weapon/equipment permutations, but the
  full batch correctly fails at the rocket launcher projectile's `effe` and
  `pctl` attachments. The equipment-only result does not establish full item
  closure or pickup execution.
- Added bounded native conversion for all 350 effect roots, 12 particle systems
  and 106 particle roots in Blood Gulch. Exact engine declarations provide the
  nested layouts; typed references, owned names, finite floats and block limits
  are checked. Effect loop indices are bounded and effect-part dispatch classes
  are derived from actual referenced tag ancestry, matching the shared validator.
- The MacBook built PPC and i386 against the Tiger SDK. The Intel control and
  both actual G4s each pass 12,867 checks with 2,943 visited references and digest
  `109d79b5`. The probe releases the source map before walking the owned native
  references. It also rejects malformed counts, addresses, loop indices, NaNs
  and mistyped particle bitmap references. Full logs:
  `build/mac-audit/effect-particle-tags-{build,powerbook,ibook}.txt`.
- This verifies storage conversion, not effect execution. Next is bounded
  dependency loading for particle bitmaps/point physics and effect references,
  plus runtime particle-state validation before registry publication. No effect
  attachment is silently removed to make the rocket launcher load. Full native
  cache/scenario lifecycle, UI/input integration and `game_tick` remain required
  before Blood Gulch is playable. Hardware runs are Leopard; Tiger is compile
  coverage only.

## 2026-10-16 — Carbon text input and native window owner hooks

- Carbon raw key-down and repeat events now pass their Mac character byte
  through the bounded input snapshot into the original engine keystroke queue.
  The real edit-text path can therefore receive ordinary typed characters;
  `platform_text_field` now shares the existing text-typing lifecycle so those
  keystrokes do not also drive player actions. Unicode composition and an
  on-screen keyboard are not provided.
- The native window owner now exposes its live drawable width and dimensions
  through the engine-facing window queries. `platform_request_quit` ends the
  owning frame loop, and a frame attempted after quit is not drawn. With no
  active window, size queries return zero.
- The extended input regression passes 57 checks and the prepare/action
  boundary passes 779 on Intel and both G4s. The hidden CGL window probe
  verifies actual 64x48 owner dimensions, two rendered frames and orderly
  quit/close on all three Macs; it creates no desktop window. G4 window logs
  are `build/mac-audit/mac-window-owner-{powerbook,ibook}.txt`; renderer names
  account for their different digests. Input logs are in
  `build/mac-audit/engine-input-text-{powerbook,ibook}.txt`,
  `engine-input-prepare-text-{powerbook,ibook}.txt`. This validates host
  input/window services, not the full game tick or a visible game session.

## 2026-10-16 — scenario compass and empty-reference correctness

- An inventory using compiled 32-bit engine field offsets found that Blood
  Gulch's `local_north` is pi, while the native scenario had left it zero.
  The native world now converts that finite value and preserves scenario flags.
  This matters to `objects.c`'s actual compass-heading function input.
- The six empty root tag references now retain their source group and explicit
  `NONE` handle. Zero-initialized handles incorrectly named tag index zero.
  Nonempty root references are refused until their separate native dependencies
  are supported; no raw pointer fallback is used.
- The combined world graph, script ownership, sky-reference and scalar/root
  reference probe passes **13,031 checks on Intel and both G4s**, including
  nonfinite north and malformed empty-reference rejection. Complete runtime
  logs are clean: `build/mac-audit/world-north-refs-{build,powerbook,ibook}.txt`.
- The source inventory is saved in `build/mac-audit/scenario-field-inventory.txt`.
  Editor scenario data (506 bytes) and eight scenario editor decals remain
  unconverted metadata; actual runtime BSP decals already have native storage.
  Sky payloads, dependency publication and the full map/game lifecycle remain
  outstanding. This is still component verification, not a playable session.

## 2026-10-16 — shared narrow-format linkage on PPC

- Exposed the shared port's `halo_linux_snprintf`/`halo_linux_vsnprintf` names
  through the existing native format translator. This closes the wide-format
  dependency without reinterpreting PPC `va_list` or importing x86 varargs.
- **21 checks pass on the MacBook and both G4s**, covering mixed integer,
  64-bit and floating arguments, width/precision arguments, truncation,
  length-only formatting and escaped modifiers. Runtime logs are clean:
  `build/mac-audit/format-shared-{build,powerbook,ibook}.txt`.
- The full startup census must rebuild this provider rather than reuse its
  old object. This is a formatting service, not game-loop validation.

## 2026-10-16 — native multiplayer item collections

- Follow-on batch adds complete native `eqip` roots and inherited object/item
  fields, preserving typed dependency handles and validating powerup/grenade
  enums. All **14 equipment tags** plus the collection tests pass **527 checks
  on Intel and both G4s**; every equipment field digest matches across all
  three. Evidence: `build/mac-audit/item-equipment-{build,powerbook,ibook}.txt`.
  This does not yet resolve all equipment dependencies or execute pickups.

- Added complete owned `itmc` root/permutation conversion using the actual
  engine declarations and shared schema limits. Native references retain their
  salted handles and copied names; referenced weapon/equipment definitions
  still require their own dependency loading before normal spawning.
- All **14 Blood Gulch collections / 15 permutations** decode on Intel and
  both G4s: **372 checks, zero failures**, identical digest `a83e0aa0`. The
  probe compares raw weight bits, spawn times and item handles, rejects bad
  counts/spans/reference types/weights, and walks owned references after the
  source map has been freed. Full runtime logs are clean.
- Selection weights are checked against the actual `random_item` signed-short
  range before its integer conversions; empty/zero-total collections are
  refused by this supported gameplay decoder. No weapon/equipment spawn or
  game-rule execution is claimed by these data checks.
- Evidence: `build/mac-audit/item-collections-{build,powerbook,ibook}.txt`.

## 2026-10-16 — owned native script storage and cache membership

- Blood Gulch has no compiled script/global/reference blocks or occupied syntax
  nodes, but its retail cache does hold a 19,001-node syntax arena and 1,024
  bytes of console-string storage. The native converter validates those exact
  supported conditions, owns both allocations, fixes the data-array pointer,
  and converts the node/header fields. Active PC scripts are explicitly refused
  until their function/global indices can be translated.
- The engine cache-membership provider now uses a scoped lease of these actual
  allocations. It rejects nonpositive lengths, outside spans, nested bindings
  and mismatched release; it never treats a raw PC map pointer as native memory.
  Other tag families need their own allocation metadata if future callers query
  their membership.
- **76,071 checks pass on Intel and both G4s**, using the unchanged engine HS
  validators and real datum allocation/deletion after the source map is freed.
  The console bytes match digest `2358ea6d`; complete runtime logs are clean.
  Evidence: `build/mac-audit/scenario-scripts-lease-{build,powerbook,ibook}.txt`.
- The world now owns and attaches these blocks. Its scoped callback binds the
  production cache-membership provider before engine code runs and releases it
  afterward; nested scopes remain refused. The world probe confirms the engine
  accepts only the native owned spans and rejects the original raw map pointers.
- This validates storage and membership, not `hs_initialize`, compiled scripts,
  `game_load` or a full game tick. Builds target the Tiger SDK; G4 observations
  are on their installed Leopard systems.

## 2026-10-16 — native scenario sky-reference block

- The world now converts Blood Gulch's bounded scenario `sky_references`
  block into owned native tag references, validates the `sky ` group, salted
  handles and source names, and copies referenced names before cache unmapping.
  A world accessor preserves each exact handle for a later sky asset loader.
  The sky payload, bitmaps, atmosphere and renderer remain unconverted.
- The integrated world-graph probe passes **12,018 checks on Intel and both
  G4s**, including malformed sky count/group/handle rejection and the existing
  owned-script cache-scope tests. All used the same map SHA-1
  `94867bcb456a24363ca6763832d030b9bb3fe687`; the PPC binary SHA-256 is
  `2d699e15a88b1fd0cf1d57a5aa992c3edd332c41833ec3bfd4197cb89afbb246`.
  Evidence: `build/mac-audit/scenario-scripts-world/world-graph-{build-intel,powerbook,ibook}.txt`.
- This attaches scenario records only; it does not load sky definitions or
  complete `scenario_load` / `scenario_switch_structure_bsp`.

## 2026-10-16 — native BSP and material lighting

- Converted actual default BSP lighting, material lighting, centroids and planes
  instead of leaving these fields zero. Float values must be finite; distant
  light counts are bounded and cached point-light counts must be zero, matching
  the shared engine schema. Nonempty breakable material references are refused
  until their native owner is converted, rather than silently changing them.
- The surface probe compares every native lighting field and material
  centroid/plane to independently saved little-endian words, then releases the
  source map before actual engine surface lookup. **3,329 checks pass on the
  MacBook and both G4s**, including malformed lighting/plane rejection and a
  successful real ground-surface sample. Complete logs are clean.
- Graph/world malformed default-lighting cases bring that probe to **10,926
  checks on all three Macs**. Evidence: `world-lighting-{build,powerbook,ibook}.txt`
  and `world-sound-{build,powerbook,ibook}.txt` in `build/mac-audit`.
- This supplies native numeric world fields. It does not complete GPU resource
  ownership, original material shaders, live input or the full game/map loop.

## 2026-10-16 — real vehicle and scenario-object update lifecycle

- Extended the real Warthog asset closure to decode its direct `snd!`
  references and sample payloads from `sounds.map`; the native sound registry
  owns copied bytes before the mapped file closes. The scenario runtime creates
  the actual palette-0 vehicle, updates it through `objects_update`, and checks
  that its native type, definition, world cluster and finite transform survive.
  Its Z origin is placement height plus half the loaded definition's bounding
  radius, as `vehicle_new` specifies. The constructor also creates the genuine
  antenna widget.
- Each of two map lifecycles runs 120 genuine object updates with queued
  movement, including jump and landing; each cycle passes 1,227 movement checks.
  The same bounded run creates and deletes all 49 Blood Gulch scenery and
  sound-scenery placements, retaining and restoring the three map-owned
  looping-sound records. The textured third-person biped, held weapon and
  scenery render in the hidden GPU diagnostic view. Runtime counts pass 428
  scenario checks and 144 overall object-runtime checks on the MacBook Intel,
  PowerBook G4 and iBook G4. Complete logs contain no failure, assertion,
  allocator, abort or missing-tag diagnostics.
- Evidence: `build/mac-audit/object-renderer-schicago-vehicle120/{intel,pbg4,ibook}.log`
  and the matching actor/object PNG captures. The two G4 PPM captures are
  byte-identical for both actor and scenery frames. The renderer exposes opaque
  base maps only; full shader alpha/light/animation composition is unsupported.
  These tests exercise queued movement and actual `objects_update`, not the full
  game tick or native event-to-input path. Sound definitions and samples are
  loaded and lifecycle-managed, but playback and the full sound update remain
  unverified.

## 2026-10-16 — native BSP sound palettes and listener environment

- Native world loading now owns BSP background-loop and sound-environment
  palettes, converts their references, and validates every cluster's palette
  index. Labels/reserved bytes stay opaque; names are owned, disk pointers and
  block definition pointers are never retained. Invalid spans, groups, salted
  handles and negative/out-of-range cluster indices are rejected.
- The same world batch now converts finite vehicle floor/ceiling limits and
  ordered world bounds, consumed by aircraft physics and visibility. Tests
  compare their exact float bits and reject NaNs and reversed ranges.
- The native `snde` decoder converts the actual 0x48-byte engine definition,
  including finite reverb parameters and priority. The owned reference API
  exposes both palette families to the asset loader; it does not itself load
  audio samples or initialize/apply listener state.
- Extended graph/palette checks pass **10,926 on Intel and both G4s**. The sound
  decoder regression passes **8,415 on all three**, including all 333 sound
  roots/951 permutations plus Blood Gulch's environment and a malformed NaN.
  All 334 sample/environment digests match across architectures; full runtime
  logs contain no allocator/assertion/tag diagnostics.
- Evidence: `build/mac-audit/world-sound-{build,powerbook,ibook}.txt` and
  `sound-environment-{build,powerbook,ibook}.txt`. Full game sound listener update,
  audible playback and a playable session remain unverified.

## 2026-10-16 — normal game-state array ownership and repeated startup

- Normal `game_state_initialize` now registers its borrowed physical arena with
  the native allocator, just as diagnostic arena startup already did. This
  prevents real `data_dispose` from freeing an interior game-state pointer.
  Normal shutdown calls the original body, unregisters ownership, closes the
  allocation log and clears allocation tracking for a subsequent startup.
  Diagnostic reservations remain separately owned.
- Two complete physical/startup/array/create-delete/disposal/write/shutdown
  cycles pass **35 checks on the MacBook and both G4s**, with full logs free of
  runtime diagnostics. Each cycle uses the actual game-state and data functions.
  This verifies storage ownership and reinitialization, not game/map startup.
- Evidence: `build/mac-audit/game-state-array-{build,powerbook,ibook}.txt`;
  driver `tools/mac_game_state_lifecycle_probe.c`, native wrapper
  `port/mac/engine_arena.c`. The checker freshly rebuilds the wrapper instead
  of reusing the earlier census object.

## 2026-10-16 — genuine game-state startup and storage ownership

- `game_state_initialize`, an actual CPU arena allocation, `game_state_write_to_file`,
  `game_state_dispose` and physical teardown pass 10 checks on Intel and both G4s.
  All three produced a 20 MiB file in private test save roots, and full logs are
  free of allocator/assertion/ownership diagnostics. This is a blank-state I/O
  test, not a playable-map save or validated revert.
- The first run exposed a double owner: `game_state_allocate_buffer` borrows the
  physical-map allocation, but its free routine released that allocation before
  `physical_memory_free` did. `mac_game_state_storage_view.py` now generates a
  checked native view that validates the borrowed owner and leaves final freeing
  to the physical map. Original source is unchanged.
- Evidence: `game-state-owned-{build,powerbook,ibook}.txt`; the initial rejected
  lifecycle log is `game-state-lifecycle-build.txt`. The PowerBook allocation log
  is preserved in `$PROBES/20261015/game-state-owned-allocation.txt`.

## 2026-10-16 — verified cache ownership fix

- The checked native LRU source view preserves verification/clearing of the
  embedded data array and frees only its parent allocation. Fresh Intel and
  both G4 runs pass 13,167 sound-cache checks, with complete logs checked for
  malloc/assertion/tag errors and none found. All 333 sample digests match.
  Logs: `sound-cache-owned-{build,powerbook,ibook}.txt`. Earlier assertion-only
  passes remain documented below because their invalid frees were real failures.
- Real physical allocation, 951 sound-cache loads, hardware/software reference
  lifetimes, close/delete and freeing all physical allocations are now observed.
  Game audio output and a complete playable game loop remain unverified.

## 2026-10-16 — original sound cache and native memory budgets

- Original physical-memory allocation and 951 sound loads pass 13,167 assertions
  on all three Macs, but full logs reveal an invalid interior free in the
  original `lruv_delete`. Cleanup is not accepted as verified until the native
  ownership fix passes a fresh run with stderr checked. No audio playback claim.
- The first real allocation attempt exposed the desktop 256 MiB texture cache
  exceeding the Mac allocator’s 128 MiB window. Native-only cache budgets now
  use the original 22 MiB textures / 4 MiB sounds, leaving gameplay pools intact.
  See `ppc-mac-audio.md` and `sound-cache-*.txt` for evidence and build requirements.

## 2026-10-16 — owned sound definitions and sample reads

- All 333 Blood Gulch sound roots and 951 permutations now decode to native owned
  storage. All three Macs pass 8,410 checks, including full byte-for-byte sample
  readbacks through the real native cache API and malformed data rejection.
  External offsets use the native sounds-file space, not raw map offsets.
  See `ppc-mac-audio.md`; game audio playback remains unverified.
- The player-render capture exposed distorted skinning despite successful
  pixel-digest checks. Rendering is not accepted as correct; investigation now
  targets live-node versus inverse-bind transforms before further visual claims.

## 2026-10-16 — PCM conversion and native host services

- Bounded PCM16 little-endian decoding passes 33 checks on the MacBook and both
  G4s. It validates complete mono/stereo frames, output capacity, alignment and
  overlap before writes. Logs: `audio-pcm-{build,powerbook,ibook}.txt`.
- Real OS random bytes and diagnostic output now have native Tiger providers.
  Both G4s and Intel pass buffer-boundary and multi-read checks. Failure to
  obtain OS randomness aborts instead of supplying predictable keys. Logs:
  `host-misc-{build,powerbook,ibook}.txt`. This is not a network-session test.

## 2026-10-16 — textured live scenery and native audio output

- The actual first Blood Gulch boulder now draws its live engine node matrices
  and region selection with its real base bitmap. Both G4 captures match
  byte-for-byte; the inspected PNG is
  `build/mac-audit/object-renderer-texture/boulder-textured-pbg4.png`.
  Each target reports one part, 42 vertices, zero unsupported base shaders, and
  a changed BSP-versus-object pixel digest. This focused run uses 8 movement
  steps (99 movement checks), 406 scenery checks per cycle and 138 runtime checks;
  it does not re-establish the earlier 120-step jump run.
- Bounded soso base-map lookup now covers all 117 such shader roots in Blood
  Gulch, with 705 checks on Intel and both G4s, including malformed UV scales
  and bitmap handles. Full model lighting, blending and shader animation remain
  outside this diagnostic renderer. Logs and source hashes are in
  `build/mac-audit/object-renderer-texture/`.
- A genuine default-output CoreAudio boundary now runs on the MacBook and both
  G4s through Tiger APIs. Each passes 29 checks, observing real render callbacks,
  native-endian float32 stereo, silence fallback and stop/close behavior. Tests
  emit only silence. DirectSound mixing, native sound resources and game audio
  are not yet integrated. See `ppc-mac-audio.md`.

## 2026-10-16 — combined initialization link and debug storage

- Cycle5 is the first correctly staged combined provider build after the remote
  staging error. Both architectures compile 393 objects without source failures
  or duplicate symbols. Their engine object manifests pass the wide-import gate;
  `ui_widget` now imports the shared UTF-16 functions. PPC still has 197 unresolved
  symbols, predominantly audio, optional video, network/UI host services and
  cache/texture lifecycle. This is a link census, not executed startup. Observed
  linker evidence: `build/mac-audit/game-tick-cycle5-link-ppc.txt`.
- A further 26 debug commons now have exact native boolean/long storage generated
  from their actual consumer declarations, with per-symbol ABI assertions.
  `engine_debug_globals.c` compiles for PPC/i386 using the Tiger SDK. The real
  initialized collision-debug flags remain in the original source. The module
  is queued for the next combined link; it does not initialize gameplay.

## 2026-10-16 — vehicle roots and native host ABI checks

- All five Blood Gulch vehicle palette definitions now convert to owned native
  storage, covering the references used by 28 scenario placements. The layout
  is extracted from the actual vehicle source. All three machines pass 138
  checks, including malformed type, salted model handle, enums and seat counts.
  PPC reference digests match Intel. This is definition conversion only; physics
  and other dependencies still gate actual vehicle spawning. See
  `ppc-mac-vehicle-tags.md` and `build/mac-audit/*vehicle*.txt`.
- Native physical-memory backing passes 38 checks on the MacBook and both G4s,
  including non-clobbering allocations at the actual game-state and tag-cache
  addresses. These are CPU virtual mappings, not Xbox GPU bus addresses.
- The input backend now passes 55 callback checks and 779 pure adapter checks
  on both G4s after correcting Carbon modifiers and left/right Command mapping.
  Live player-control integration remains gated by full runtime initialization.
- The expanded full-tick link exposed direct Darwin wchar32 imports in
  `ui_widget`. `tools/mac_wide_import_check.sh` rejects such imports from engine
  object manifests. The previous scenery manifest passes; the affected widget
  object correctly fails. Fresh full-tick provider work must include UTF-16
  routing. Older cycle2/3 remote link batches used a stale script due to flattened
  staging; their unresolved counts are not evidence of the latest changes.

## 2026-10-16 — heap and direct CRT boundaries

- The native debug allocator now supports reallocating owned heap buffers,
  preserves contents while growing/shrinking, returns NULL on a zero-size free,
  and refuses to move/free a registered game-state arena pointer. Native
  `system_malloc`/`system_free` provide the platform heap without mixing in
  the original debug allocator's private headers. The MacBook i386 and both
  G4 PPC runs pass 84 checks. Logs: `build/mac-audit/heap-*.txt`.
- Direct `_strnicmp` and `_strdup` imports now have native providers. Direct
  `_wcsnicmp` routes to the existing shared UTF-16 implementation, never Darwin's
  wchar32 ABI. Both explicitly routed and redirect-header builds pass on Intel;
  the PPC boundary passes on both G4s. Each run passes 34 checks including
  non-ASCII UTF-16 ordering and bounded comparisons. Logs:
  `build/mac-audit/crt-extended-*.txt`.
- A separate full initialization link remains incomplete. These native service
  checks do not establish a running `game_tick`, live input, audio, or playability.

## 2026-10-16 — complete scenery population and additional native files

- The combined hidden GPU diagnostic now loads and creates all 47 scenery
  placements and both sound-scenery placements through actual scenario
  constructors. All 49 coexist, survive a genuine object update, then delete
  cleanly with the two looping-sound records released. Two map cycles pass
  399 scenery checks each on the MacBook and both G4s. The same process passes
  1,227 movement checks per cycle and 138 runtime checks, rendering 240 frames
  from the genuine biped camera. Logs: `build/mac-audit/scenery-view-fixed-build.txt`
  and `build/mac-audit/scenery-view-{powerbook,ibook}.txt`. This is not audio
  playback, full game initialization, a full game tick, or a playable session.
- Native file services now implement copying through leased file handles,
  truncating/extending at the current file position, read-only/normal attributes,
  and real filesystem capacity via Tiger statvfs. Same-file copies are refused
  before truncation by the source deny-write lease. Unsupported attributes fail.
  GCC 4.0/Tiger SDK PPC and i386 builds pass; the MacBook and both G4s each pass
  119 file checks, including copied contents, sparse extension, permissions,
  invalid handles and disk-space consistency. Logs:
  `build/mac-audit/files-extended-{build,powerbook,ibook}.txt`.

## 2026-10-16 — looping-sound storage and typed service handles

- All 17 Blood Gulch looping-sound roots now own converted tracks and details,
  with runtime scripting ownership reset through the documented native schema.
  The probe visits 27 salted sound references after freeing the map, rejects
  malformed counts/pointers/salts, and passes 243 checks on Intel and both G4s.
  Logs: `build/mac-audit/looping-sound-{build,powerbook,ibook}.txt`. Audio sample
  resources and actual playback are still separate dependencies.
- Importing sound-schema metadata initially also tightened unrelated globals
  references to Xbox `mode`, incorrectly refusing retail `mod2` hands. The
  generator now scopes the new metadata import to the looping-sound family;
  existing retail reference rules remain unchanged. This was caught by the
  integrated scenery batch before any runtime execution.
- Handle ownership now distinguishes file enumeration, save enumeration,
  nicknames and signature state before exposing payloads. Cross-kind acquire
  and close attempts fail, and outstanding leases preserve storage after close.
  All three machines pass 166 handle checks. Logs:
  `build/mac-audit/service-handles-{build,powerbook,ibook}.txt`.

## 2026-10-16 — integrated real movement, GPU frames and scenario spawning

- `MAC_RUNTIME_VIEW=1 HALO_MAC_SCENERY=1 HALO_MAC_QUEUED_MOVEMENT=1
  HALO_MAC_OBJECT_STEPS=120` builds a combined native diagnostic. Each genuine
  `objects_update` is followed by a GPU frame using `unit_get_camera_position`
  and `unit_get_aiming_vector`. Two map cycles produce 240 frames in one process.
  The retail map mapping remains alive for the diagnostic renderer and closes
  only after renderer disposal; ordinary non-view probes still unmap first.
- MacBook Intel, PowerBook G4 and iBook G4 each pass **1,227 movement checks per
  cycle and 154 runtime checks**, zero failures. All agree on the earlier final
  position, jump height and camera. Frame 240 contains 307,114 scene pixels on
  each native GPU. No visible desktop window was opened. Logs:
  `build/mac-audit/integrated-view-{build,powerbook,ibook}.txt`.
- After movement, the probe calls genuine `object_new_from_scenario` on the
  first boulder placement using the attached native palette. Both map cycles
  create a scenery object in cluster 19 at `(37.9339,-88.6921,0.0875717)`, verify
  its definition/type/position and delete it through the real object lifecycle.
  This is one actual scenery placement, not all 47 scenery objects or vehicles.
- The renderer remains the bounded BSP diagnostic (30 textures, two unsupported
  materials); it does not yet draw the spawned actors, weapons, HUD or scenery.
  Input snapshots are currently checked as neutral while queued test actions
  drive movement. Full `game_tick`, live controls, audio and gameplay remain
  incomplete. These results do not establish a playable session.

## 2026-10-16 — real player camera and accelerated hidden rendering

- The queued 120-step movement test now reads `unit_get_camera_position` and
  `unit_get_aiming_vector` from the real biped after movement. Intel and both
  G4s pass 1,107 movement checks per map cycle, twice, and agree on camera
  `(101.614693,-156.074493,0.648732722)`, yaw `0.75000006`, pitch `-0.124999993`.
  Runtime checks remain 133 with zero failures. Logs:
  `build/mac-audit/object-camera-{build,powerbook,ibook}.txt`.
- Hidden native rendering now uses accelerated CGL with an attached depth
  buffer. Near/far occlusion and readback pass on all three Macs. The standard
  BSP-view build passes with warnings treated as errors and renders this camera
  pose on the GeForce 9400M, Radeon 9600 XT and Radeon 9550. Both G4 captures
  match byte-for-byte. Evidence: `build/mac-audit/bsp-view-final/`; details in
  `docs/ppc-mac-bsp-view.md`. No visible desktop window was opened.
- All 19 scenery roots and the sound-scenery root now decode through the same
  bounded native object schema, including nested blocks and references. The
  object probe passes 139 checks and 28 rejection checks on Intel and both G4s;
  field digests agree. Logs: `build/mac-audit/scenery-tags-{build,powerbook,ibook}.txt`.
  This converts root storage; their model, sound and other dependencies remain
  separately required before constructors can use them.
- Native memory reporting passes 11 checks on all three Macs, reporting their
  observed physical RAM while preserving the engine’s 64 MiB physical budget.
  Unknown virtual/commit values remain zero; this is not an allocation promise.
  Logs: `build/mac-audit/memory-status-{build,powerbook,ibook}.txt`.
- These are verified components, not a playable session. Full `game_tick` still
  has unresolved platform providers; visible presentation, live controls and
  complete gameplay/rendering integration remain unverified.

## 2026-10-16 — queued walking and jumping agree on all three Macs

- `HALO_MAC_QUEUED_MOVEMENT=1 HALO_MAC_OBJECT_STEPS=120` now creates the real
  local action queue, sends a fresh action each step through
  `players_update_before_game`, and runs genuine `objects_update`. A single
  queued jump occurs at step 16. No biped movement or collision callbacks
  are substituted and no private game-active flag is forced.
- Intel, PowerBook G4 and iBook G4 each pass **1,103 movement checks per cycle,
  two map cycles**, plus the runtime's 133 checks and existing queue/HUD gates.
  All three agree on jump height `2.356102` from initial height `1.70473`,
  final position `(101.61469,-156.07449,0.028732702)`, dirt support surface
  1590 and eight decals. The queue owner is deleted and prior time/connection
  state restored before map cleanup. Evidence:
  `build/mac-audit/object-queued-walk-{build,powerbook,ibook}.txt`.
- This is genuine queued player movement plus full object updates, still a
  component lifecycle rather than the complete `game_tick` or playable app.
  Full HS/UI/game-engine source integration currently compiles on both legacy
  architectures but its latest link exposes 303 missing providers. That census
  is under active reduction; initial small counts were not full closure.
- Native file metadata, deletion, directory operations and typed enumeration
  now pass 92 checks on each machine. Access/truncate checks, share-mode
  reservations, offset bounds, mixed separators and enumeration aliasing were
  corrected during review. Creation time uses the documented Tiger ctime
  approximation. Asynchronous file flags remain explicitly unsupported; an
  OVERLAPPED offset structure is supported only for immediate synchronous I/O.
  Evidence: `build/mac-audit/mac-file-{intel,powerbook,ibook}.txt`.

## 2026-10-16 — sustained object movement across the base boundary

- A fresh MacBook PPC/i386 runtime batch includes the reviewed DXT sampler,
  current native thread/event/mutex/socket/clock boundaries and real device
  transform state. The latter separately passes 96 state/reset/invalid-enum
  checks on all three machines (`device-transform-*.txt`).
- With `HALO_MAC_OBJECT_STEPS=120`, Intel and both G4s complete 120 genuine
  `objects_update()` calls in **each of two map cycles**. Each cycle passes
  982 movement checks; the runtime retains 133 checks plus queue/PVS/HUD
  checks. The actor leaves the metal base and reaches dirt surface 1590
  (material 1), retaining eight decals. All three match position
  `(101.61469,-156.07449,0.028732911)` and displacement
  `(3.1212845,1.5645142,-1.6759971)`. Evidence:
  `build/mac-audit/object-walk-{build,powerbook,ibook}.txt`.
- This test retains one applied action while advancing object callbacks; it
  still does not run a full game tick. A separate queued-input/jump variant
  subsequently passed as recorded above.
- Typed directory-enumeration leases now distinguish find handles from file
  handles. Native condition waits use Mach monotonic deadlines and Tiger's
  relative condition wait, avoiding wall-clock adjustment sensitivity.
  Combined handle tests pass 125 checks on Intel and both G4s:
  `build/mac-audit/monotonic-find-handles-{build,powerbook,ibook}.txt`.

## 2026-10-16 — native host, synchronization and socket boundaries

- The Carbon/AGL host builds PPC+i386 with the MacBook's GCC 4.0/Tiger SDK.
  Two offscreen frames with pixel readback pass on Intel and both G4s:
  `build/mac-audit/mac-window-{build,powerbook,ibook}.txt`. Review corrected the
  visible handler-install return check (`OSStatus` succeeds at zero); that
  correction also compiles (`mac-window-reviewed-build.txt`). No visible
  window has been opened; visible input, resize and swap remain unverified.
- Native `GetTickCount` uses the existing monotonic counter; `Sleep` yields
  at zero and resumes its remaining delay after signals. Clock/ASCII tests
  including an interrupted sleep pass 216 checks on all three machines:
  `build/mac-audit/clock-sleep-{build,powerbook,ibook}.txt`.
- The handle owner now supports file-operation leases (closing a public handle
  cannot destroy a descriptor still used by a leased operation), plus recursive
  mutexes, distinct handles for the same process-local name, timed contention,
  foreign-owner rejection and abandoned-owner recovery through pthread TLS.
  Names share within this process; this is not a cross-process namespace.
  Combined thread/event/file-lease/mutex tests pass 114 checks on all three
  machines: `build/mac-audit/mutex-handles-{build,powerbook,ibook}.txt`.
- Winsock/Darwin translation passes 76 TCP/UDP loopback checks on each machine,
  including the incompatible address and fd-set layouts, nonblocking I/O,
  descriptor indices above 64, errors and oversized UDP datagrams. Unsupported
  socket options are rejected. Evidence: `build/mac-audit/socket-{build-intel,intel,powerbook,ibook}.txt`.
- Native file operations have a first successful all-machine probe, but review
  found access/disposition and unsupported-flag handling that must be corrected
  before integration. Full game-loop linkage still needs genuine providers;
  duplicate-object link errors must be cleared before a new unresolved-symbol
  count can be considered current. A passing component probe is not game startup.

## 2026-10-16 — native texture scalar order and event services

- Real RGB565 lightmap sampling now passes on the MacBook Intel control and
  both G4s: 15 images, 124,992 base pixels, 125,054 checks. The cache owner is
  bound and source mappings are closed before sampling. Evidence:
  `build/mac-audit/lightmap-sampling-{build,powerbook,ibook}.txt`.
- World surface tests pass 129 checks on all three machines, including rejected
  invalid surface references/vertex spans and a real structure collision query
  after the map mapping is released. Evidence:
  `build/mac-audit/world-surfaces-{build,powerbook,ibook}.txt`.
- The shared S3TC sampler writes BGRA bytes through a scalar alias. PPC therefore
  returned the wrong numeric pixel value despite native compressed-word conversion.
  A checked native source view now repacks that return to the engine's numeric
  AARRGGBB convention; shared source is unchanged. Installed DXT1 and synthetic
  DXT3/DXT5 tests pass 19 checks on Intel and both G4s. The 227 DXT1 interpolation
  differences are at most one RGB unit, matching the original sampler's rounding;
  alpha and the synthetic fixtures are exact. Evidence:
  `build/mac-audit/dxt-sampling-fixed-{build,powerbook,ibook}.txt`.
- Added owned unnamed manual/auto-reset events to the native thread handle
  service. Waits consume auto-reset state once, manual-reset state persists,
  invalid/wrong-kind handles are rejected, and waiters retain storage. Named
  events/security attributes return unsupported rather than false success.
  Thread/event concurrency and clean-XDK ABI probes built on the MacBook;
  80 checks pass there and on both G4s. Evidence:
  `build/mac-audit/event-handles-{build,powerbook,ibook}.txt`.
- These are component milestones, not a playable session. Full game lifecycle,
  complete loaded scenario and game tick remain under integration. The initial
  19 missing symbols expanded into hundreds through actual HS/UI provider
  tables; the latest link logs, not that initial number, describe closure.

## 2026-10-16 — full object updates and real lightmap sampling on both G4s

- Native surface conversion now owns 14,150 surface references, actual surface
  triangles, 16 lightmap records and their materials/vertex buffers. It calls
  the real rasterizer vertex compressor on bounded, decoded retail geometry.
  This is the illumination subset; material render state is not complete.
- Loaded the actual 15-image RGB565 lightmap group eab3093d. Bitmap CPU sampling
  is native-endian on PPC while resource bytes retain their original encoding;
  retail-only packed flags are removed from the runtime bitmap flags.
- The MacBook-built PPC binary and Intel control complete **eight genuine
  objects_update calls in each of two map cycles**. PowerBook G4 and iBook G4
  both match Intel spawn illumination `(1, 0.93725497, 0.83921576)`, movement
  delta `(0.186203, 0.092727661, -0.0012443066)`, support surface 1412 and eight
  decal datums. **133 runtime checks, 87 movement checks per cycle, 87 queue/PVS
  checks and 13 HUD checks pass**. Cleanup/arena teardown pass as well.
  Evidence: `build/mac-audit/object-update-lightmap-{build,powerbook,ibook}.txt`.
- This closes the prior surface-reference assertion. It does not establish a
  full game tick, renderer, physical input or playable session. Further work
  remains active at the user's request. A real-game-tick link census exposed
  19 missing providers; a separate batch is integrating their genuine source
  units and typed native platform services. Full lightmap pixel-sampling and
  malformed world-data tests subsequently passed as recorded above.


## 2026-10-15 — native decals verified; full updates reach BSP lighting

- Converted the actual eight Blood Gulch runtime decal placements, four palette
  references, cluster ranges, complete `deca` definitions and referenced bitmap
  assets. Positions are endian-correct; references/names remain owned after
  map unmapping. The separate decoder passes 23 checks on Intel and both G4s,
  including malformed fields and a cyclic chain.
- Real decal initialization, PVS-driven creation/removal, map reset and LRUV
  disposal now pass on the MacBook, PowerBook G4 and iBook G4: eight decals per
  map cycle, two cycles, **137 runtime checks with zero failures**, plus the
  existing HUD and player queue checks. This is the explicit
  `--decal-lifecycle` diagnostic, not a completed `objects_update` call.
  Logs: `build/mac-audit/decal-lifecycle-{build,powerbook,ibook}.txt`.
- Connected the native headless device identity to engine initialization and
  registered arena ownership with native `debug_free`. Genuine LRUV deletion
  zeros its embedded array/cache without trying to libc-free arena memory;
  successful arena teardown remains checked. No gameplay callback was replaced.
- Added weapon first-person models/animation graphs and the biped's complete
  dialogue tag. Dialogue enum gaps follow the shared validator's documented
  zero-padding rule; the installed player dialogue has no live sound refs.
- Full `objects_update` now enters real unit/weapon updates and creates eight
  decal datums, observed in the Intel debugger. It stops at native BSP
  `surface_references`, currently empty, during player illumination. Intel's
  exact request is reference 2168. Both G4 debugger backtraces confirm the same illumination/surface lookup;
  logs: `build/mac-audit/object-update-surface-*.txt`. Do not report full object
  updates, simulation, drawing, Tiger runtime testing or playability as passed.
- Regression: eight real biped callbacks still match positions on all three
  Macs; **77 movement checks** (including exact decoded decal coordinates),
  **131 runtime checks**, **87 queue/PVS checks** and **13 HUD checks** per
  applicable scope pass. Logs: `build/mac-audit/decal-movement-*.txt`.
  The 22 local cache/geometry reader tests also pass.
- Next batch: native surface references, surfaces, lightmap/material/vertex
  records and actual lightmap bitmap dependencies required by
  `structure_test_vector` and `lights_illumination_at_point`. See
  `ppc-mac-render-surface-next.md`; preserve actual lightmap references rather
  than bypassing illumination. Blood Gulch remains unplayable.


## 2026-10-15 — collision materials verified; real decal dependency identified

- Expanded the native BSP world with all 11 real collision-material records,
  using the witnessed 20-byte layout and owned shader-reference names. Checked
  surface indices, shader handles/groups/name pointers, type range and agreement
  with each referenced shader. Source mappings can still be released safely.
- Intel and both G4s pass 130 runtime checks plus 72 movement, 87 action/PVS
  and 13 HUD checks per map cycle across two cycles. A genuine downward
  collision query resolves support surface 1412 to material type 7 (thick
  metal), and shader names remain readable after unmapping. Movement stays
  numerically unchanged. Evidence: `build/mac-audit/movement-material-
  {build,powerbook,ibook}.txt`. Local cache/geometry tests pass all 22 checks.
- Broader object-update linking now includes the real HUD shield and structure
  decal providers. Map inspection found 8 actual runtime decals, 4 scenario
  decal palette entries, and decal references in clusters 3 and 19. These are
  not yet converted/loaded. The new `--object-update` diagnostic refuses this
  partial world (expected exit 3 on all three Macs), instead of letting zeroed
  fields silently omit the decals. Fog blocks and flagged fog references are
  absent in this map. Evidence: `object-update-refusal-{intel,powerbook,ibook}.txt`.
- Next: native runtime decals, scenario palette and actual referenced decal
  definitions/resources, then the complete object update. Full game ticks,
  rendering integration and playability remain unverified.

## 2026-10-15 — eight consecutive biped updates verified on all three Macs

- Extended the successful movement gate to eight actual biped callbacks per
  map. Intel and both G4s each pass 66 movement checks per cycle, with ground
  support and collision context preserved throughout, finite position/velocity,
  and valid cluster membership. Two map cycles still pass 129 harness checks
  plus 87 action/PVS and 13 HUD checks per cycle. All have zero failures.
- Final position `(98.679604, -157.54628, 1.7034857)` and displacement
  `(0.186203, 0.092727661, -0.0012443066)` agree in the recorded output on
  Intel and PPC. Ground support remains surface 1412 in cluster 3. This is
  observed numeric agreement, not exhaustive cross-platform determinism.
- Evidence: `build/mac-audit/player-movement-eight-{build,powerbook,ibook}.txt`.
  Run `--player-movement` using the runtime check script; regenerate/stage
  `mac_game_state_body.h` with `tools/mac_game_state_boundary.py` first.
- Next: broaden to genuine object updates and required service lifecycles,
  then full game/map initialization and rendered camera/input integration.
  Blood Gulch is not yet playable; these callbacks are not a full game tick.

## 2026-10-15 — first actual biped movement on Intel and both G4s

- The native game compilation view now shares the exact original core-state
  allocation/zeroing prefix with full startup. It owns the real declared game
  variant, refuses duplicate ownership, and restores state before arena release.
  No active/map-loaded flags or getters are substituted.
- Genuine cinematic, breakable-surface and collision-log initialization let
  the unchanged biped callback complete. All three Macs pass 129 harness
  checks plus 14 movement, 87 action/PVS and 13 HUD checks per map cycle
  across two cycles. The callback moves horizontally, resolves support surface
  1412 and stays in cluster 3. Observed deltas agree across Intel and PPC:
  `(0.0095443726, 0.0047607422, -0.0012443066)`.
- Evidence: `build/mac-audit/player-movement-collision-build.txt` and
  `player-movement-runtime-{powerbook,ibook}.txt`. This is a bounded actual
  callback test, not full object/game simulation or a playable session.
- Full-startup linking still needs a broader service closure; its initial
  29-symbol census and provider inventory are in `ppc-mac-game-startup.md`.
  Lower-cost agents audited the initialization seam and collision dependencies.

## 2026-10-15 — applied input, visibility and HUD messaging verified

- Real `players_update_before_game` now consumes queued controls, sets unit
  throttle and composes both visibility arrays from native Blood Gulch data.
  All 30 cluster bits agree, including four hidden clusters. Whole genuine
  `hud.c` supplies the pickup callbacks; no empty handlers were introduced.
- Complete native HUD globals storage passes 23 checks on Intel and both G4s
  with digest `eb225dce`. Existing globals/object regression checks also pass.
- Actual HUD messaging allocation and map initialization now consume that
  owned root after source mappings close. Real weapon/ammunition pickup calls
  create two messages and accumulate ammunition quantity to 12. On all three
  Macs: 125 runtime checks, plus 87 action/visibility and 13 HUD checks per
  map cycle, across two cycles, zero failures. Evidence:
  `build/mac-audit/player-hud-runtime-{build,powerbook,ibook}.txt`.
- Actual biped callback execution was attempted on Intel. The first crash
  was a missing cinematic lifecycle; genuine initialization fixed it. The
  next crash is `game_players_are_double_speed` reading uninitialized private
  game state. See `player-movement-cinematic-debug.txt`. No G4 movement pass
  is claimed, and the diagnostic `--player-movement` mode remains failing.
  Next work is the genuine game initialization dependency closure, followed
  by further movement/collision/animation validation. HUD drawing, a full
  simulation tick and playable gameplay remain unverified.

## 2026-10-15 — real player-control initialization verified

- Complete native globals now feed actual player-control map initialization
  after source mappings are released. Intel and both G4s pass 123 runtime
  checks plus 20 queue checks per map, across two complete spawn/map cycles.
  Genuine player association, both weapons and four lights remain intact.
- Evidence: `build/mac-audit/player-control-runtime-{build,powerbook,ibook}.txt`.
  Local cache/geometry regression tests also pass all 22 checks.
- Next: genuine HUD message providers retained by player update, then queued
  actions applied to unit control and BSP visibility. That opt-in extension
  is currently excluded from verified builds. Physics and a playable rendered
  session remain unverified; no gameplay callbacks were substituted.

## 2026-10-15 — native globals and actual player queues verified

- Complete owned globals (`matg`) storage converts all 15 root blocks and
  nested values. Intel and both G4s pass 25 checks with identical digest
  `4467fdfb`, including malformed-input rejection. The existing biped/weapon
  decoder retains its 37 native and 28 mutation checks on all three machines.
  Referenced handles are validated but are not all loaded dependencies.
- Actual player action queues now pass 20 checks per map across two map
  lifecycles on each Mac, alongside 91 spawning/player runtime checks.
  This verifies delivery, button/trigger accumulation and one-shot rearming;
  it does not advance biped physics. Evidence:
  `build/mac-audit/player-queue-runtime-{build,powerbook,ibook}.txt`.
- Next integration: genuine player-control initialization using the complete
  globals root, then queued actions through player update and native BSP
  visibility. Complete simulation startup and visual gameplay remain pending.

## 2026-10-15 — local player association and legacy font drawing verified

- Genuine player creation, local slot assignment and biped/control association
  pass 85 checks on Intel and both G4s, across two spawn/map cycles. Ownership
  flags are set by the actual engine; detach and record cleanup also execute.
  Evidence: `build/mac-audit/player-runtime-{build,powerbook,ibook}.txt`.
- Optional legacy GL font upload uses the actual shared atlas implementation
  with an explicit current-context/invalidation boundary. Intel and both G4s
  pass 37 checks including full texture readback, dirty uploads, scale reset,
  lifetime and an offscreen alpha-blended glyph draw. GPU alpha digest
  `2a30d4dd` matches. Evidence: `build/mac-audit/text-gl-{build,powerbook,ibook}.txt`.
- No simulation movement, complete engine rendering or playable session is
  established by these gates. Font GPU memory still needs minimum-spec review.

## 2026-10-15 — full biped and weapon spawning verified on both G4s

- MacBook Intel, PowerBook G4 and iBook G4 each pass 49 checks across two
  native Blood Gulch map lifecycles. Actual object_new invokes unit/biped
  constructors, recursively creates both starting weapons, populates their
  inventory slots and creates four attachment lights. Each observes three
  object datums and the biped in cluster 3, after source mappings are closed.
- Genuine object/player/render/time/AI-bootstrap services support the test;
  all callback tables remain intact. Immediate deletion and explicit cleanup
  of inventory objects leave zero objects/lights and permit arena release.
  Ordinary object_delete is deferred and is unsuitable for a no-tick teardown.
- Evidence: `build/mac-audit/object-spawn-{build,powerbook,ibook}.txt`.
  The reproducible command is `sh tools/mac_object_runtime_checks.sh
  <bloodgulch.map> <bitmaps.map> --spawn` on the staged MacBook checkout.
- This is full constructor execution, not playability: no controlling player,
  simulation tick, GPU display or audio playback yet. AI remains genuinely
  initialized but not activated for a map. Next: real player association and
  bounded input/movement ticks, required runtime tags/visibility/camera, then
  legacy graphics integration.

## 2026-10-15 — dependency link and object lifecycle verified

- Actual object_new now links on both architectures with real device, text and
  cache services. Headless device tests pass 76 checks, native font tests 56
  (identical glyph digest `843122b2`), and bounded cache reads 53 on Intel and
  both G4s. No GPU drawing or sound playback is established.
- Runtime testing found and fixed a native forced-header recursion in the
  real cseries memory wrappers. After rebuilding that implementation, actual
  object initialization, two map lifecycles and teardown pass 21 checks on
  all three Macs. Evidence: `build/mac-audit/object-runtime-{build,powerbook,ibook}.txt`.
- Complete biped/weapon creation is now the next runtime gate, with genuine
  player, game-time and render-object storage initialization.

## 2026-10-15 — mapped bitmap input and real CPU device state verified

- Replaced the external bitmap probes’ 317-MB heap read with a checked,
  read-only private file mapping. The input mapping is released before owned
  resources and actual enhanced weapon constructors are exercised. Intel and
  both G4s pass 19 mapping, 52 bitmap and 151 enhanced constructor checks.
  Virtual address space and touched pages still count; minimum-RAM operation
  remains unproven. Evidence: `build/mac-audit/mapped-assets-{build,powerbook,ibook}.txt`.
- Added a native CPU device-state subset using unchanged shared renderer
  function bodies: state/defaults, bindings, constants, pixel shader state and
  actual synchronous callbacks. Intel and both G4s pass 40 checks, including
  signed constant registers, dirty serials and lifecycle reset. Evidence:
  `build/mac-audit/device-state-{build,powerbook,ibook}.txt`.
- The unchanged shared CPU resource implementation now builds through a native
  declaration boundary, with genuine Mach allocations and explicit write
  generations. Intel and both G4s pass 44 buffer/texture/layout/lifetime checks;
  all allocations are released. Unannounced stores are not tracked. Evidence:
  `build/mac-audit/cpu-resources-{build,powerbook,ibook}.txt`.
- With those verified services, 41 additional genuine renderer/support units
  compile on both architectures. The expanded link now has 24 identical
  missing symbols (19 device APIs, four text services and cache file reads).
  The detailed closure entry below records the ABI correction and evidence.
- Drawing, immediate mode, complete device startup, actual full biped spawning
  and a playable session remain separate gates. No substitute gameplay
  callbacks or successful drawing stubs were introduced.

## 2026-10-15 — corrected resources and enhanced weapon constructors verified

- Corrected native texture resources to the shared renderer's actual memory
  contract: encoded mip data uses non-clobbering Mach high-window pages and the
  header stores its physical address. Texture, model vertex and index headers
  are owned by the native tag registry and no longer claim D3D-created ownership.
  Their owner alone releases backing memory and header allocations.
- Tests extract the actual shared `D3DResource_Release` implementation and reject
  any attempt to deallocate a tag-owned resource. Data remains readable after
  release. MacBook Intel and both G4s pass 51 bitmap checks (10008 owned bytes,
  unchanged pixel digest `68578307`) and 262302 model checks (unchanged digest
  `38f8ff76`). Hardware logs: `native-bitmap-owner-{powerbook,ibook}.txt` and
  `native-model-owner-{powerbook,ibook}.txt` under `build/mac-audit/`.
- Rebuilt the integrated enhanced weapon test using these corrected resources.
  Intel and both G4s again pass 150 checks with 18 native roots, now 2141586
  owned bytes after page accounting. Actual stock-engine `item_new`/`weapon_new`
  compiled with enhanced network limits perform three observed AR trail-cache
  requests. Both source files are freed before constructors. Registry-owned
  resources, cache binding/unbinding and late-failure rollback are checked.
  Evidence: `build/mac-audit/actor-enhanced-resource-{build,powerbook,ibook}.txt`.
- This supersedes the earlier resource address/ownership limitation. It does
  not establish GPU upload/drawing, `object_new`, biped spawning, shooting or a
  playable session. Enhanced-network capacity is a build configuration here,
  not a 128-player interoperability/performance result.
- A separate graphics audit compiles nine actual Xbox renderer units on both
  architectures with zero compile failures. Adding them to the current source
  set exposes 90 unresolved symbols, including the same 48 D3D API/global
  names on each architecture. The smaller 34-symbol baseline is not a complete
  count of all remaining implementation work. Evidence lives in
  `build/mac-graphics-audit/census_{ppc,i386}.json` and its link logs.
  `docs/ppc-mac-renderer-spawn-boundary.md` records the next concrete approach:
  reuse actual shared CPU resource code with native contiguous allocation and
  an explicit headless device preserving real state/resource semantics, while
  completing renderer providers. The 48-name set does not cover all startup.
- Next: genuine graphics backend/platform closure, complete runtime startup,
  then actual biped and starting-weapon spawning. A headless milestone may
  validate constructors but cannot satisfy playable Blood Gulch. Preserve the
  current tested CPU resource contracts when connecting the legacy renderer.

## 2026-10-15 — unattended continuation: native platform and weapon dependencies

- Updated the existing hourly heartbeat to continue while the user is away,
  choose recommended reversible options and, after verified playable Blood
  Gulch, continue to menus/another installed map and the broader roadmap.
- The full source link expanded to 203 actual engine/native units, compiling on
  MacBook for both PPC and Intel with no source failures. It reaches genuine
  AI, camera, game/main, scripting and recorded-animation providers. At this
  checkpoint 38 unresolved symbols match across architectures: 34 renderer/cache
  and four native socket/handle services. Logs/reports:
  `build/mac-audit/spawn-link-203-*` and `spawn-providers-203-*`.
- Replaced the unsupported GCC 4 file-publication intrinsic only at errors.c's
  call boundary with a typed native FILE-pointer compare/exchange using Tiger
  OSAtomic full barriers. Intel and both G4s pass 1000 checks over 100 concurrent
  eight-thread publication rounds; exactly one thread wins each round.
- Added actual unsigned 32-bit elapsed milliseconds from the native monotonic
  clock. Native clock/CRT checks now pass 212 on Intel and both G4s. Evidence:
  `build/mac-audit/atomic-clock-{build,powerbook,ibook}.txt`.
- Native narrow MS formatting preserves PPC va_list and handles I64/I32,
  truncation, dynamic width/precision and long format strings. All three Macs
  pass 15 checks: `build/mac-audit/format-native-{build,powerbook,ibook}.txt`.
- Native stack diagnostics use real bounded compiler unwinding, not a fabricated
  stack. Intel and both G4s pass five checks with actual observed frames.
  GCC 4 requires `-fexceptions` for these probe units; `-funwind-tables` alone
  caused duplicate assembly metadata. Existing caller units without unwind
  metadata have best-effort coverage, not complete crash-context support.
  Evidence: `build/mac-audit/engine-stack-{build,powerbook,ibook}.txt`.
- Complete supported projectile/contrail native conversion passes 36 checks on
  Intel and both G4s, including malformed inputs and source-map lifetime;
  digest `f4d099e2`. The AR chain is projectile e29f012b -> contrail e2a0012c ->
  bitmap e2a1012d; the pistol projectile e3750201 has no attachments. Existing
  biped/weapon digests remain unchanged. These are converted roots, not spawned
  projectiles. Logs: `build/mac-audit/projectile-tags-*.txt`.
- Native tracer bitmap CPU residency passes 45 checks on all three Macs: six
  DXT1 mip levels, digest `68578307`, 6604 owned bytes. Actual D3D texture
  records and owned encoded/decoded pixels back the resident cache lookup.
  GPU drawing remains unimplemented. Current probes read the entire external
  bitmaps file; this does not meet the eventual 256 MiB minimum-memory goal.
  See `docs/ppc-mac-renderer-spawn-boundary.md` for constructor/render separation.
- After integrating the observed socket/handle boundary, the 203-unit baseline
  has 34 renderer/cache unresolved symbols, identical on PPC and Intel, with
  zero compile failures. Reports: `spawn-providers-203-native-{ppc,i386}.json`;
  build evidence: `spawn-link-203-native-build.txt`. Socket hardware logs are
  `socket-{powerbook,ibook}.txt`; thread logs are `handle-{powerbook,ibook}.txt`.
  This closes the current nongraphics link gaps, not all possible runtime
  services of a complete game.
- The first integrated enhanced weapon batch passed 150 checks on Intel and
  both G4s with 18 roots and three actual AR trail-cache lookups after freeing
  both input files. Evidence: `build/mac-audit/actor-enhanced-*.txt`. Subsequent
  resource review identified that the bitmap header's address/ownership flags
  needed adjustment before shared D3D resource use; the initial successful
  lookup test does not establish that stronger contract. The corrected resource
  batch and repeat integration evidence are recorded below when complete.
- Native BSD send/close and pthread WSA-error storage pass 24 checks on Intel
  and both G4s. Native owned-thread handles pass 23, including suspended start,
  resume, wait/exit status and closing a running handle without killing its
  worker. Invalid handles are rejected. These do not implement XNet/P2P or
  general file/event/mutex handle services. Exact clean-XDK signatures compile
  on both architectures. Evidence: native socket and handle logs in
  `build/mac-audit/`.
- Full `object_new` / biped creation and playable Blood Gulch remain unverified.
  Runtime dependency integration continues; successful standalone components
  are not evidence of full spawning or Tiger runtime compatibility.

## 2026-10-15 — native runtime bootstrap and actor map initialization

- Added a non-clobbering Mach arena around whole real `game_state.c`, preserving
  its allocator and allocation registration. The MacBook builds both native
  architectures with GCC 4.0/Tiger SDK; Intel and both G4s pass 39 checks,
  including occupied-address protection, real data/pool allocation, compaction,
  live-owner teardown refusal and repeated arena start/stop. Start this arena
  before loading model resources. Logs: `engine-arena-build.txt` and
  `engine-arena-{powerbook,ibook}.txt` under `build/mac-audit/`.
- Actual `lights_initialize` and map lifecycle routines now execute with real
  game-state allocations and cluster partitions: 25 checks pass on all three
  Macs across two map cycles. This does not call `light_new`. Original
  `cluster_partition_delete` retains its port side arrays/registry entry, so
  this does not establish leak-free whole-engine restart. Logs:
  `build/mac-audit/light-runtime-{build,powerbook,ibook}.txt`.
- Native tag iteration now returns only published roots, supports inherited
  groups and finalized runtime model groups, and remains exhausted safely.
  Real engine `tag_iterator_new/next` and `tag_index_is_group` use this registry.
  Including the authoritative cache header exposed and fixed the old signed
  `tag_get_group_tag` return declaration. Registry checks: 7273 pass on Intel
  and both G4s, plus host ASan/UBSan.
- The 14-root actor batch now calls actual `weapons_initialize_for_new_map`
  before constructor routines. Its 123 checks pass on Intel and both G4s,
  including exact iteration counts, group/stale-handle checks, source-map
  release and real stock weapon allocation/constructor behavior. No full
  `object_new`, `unit_new` or biped spawning is claimed. Hardware evidence:
  `build/mac-audit/runtime-tags-{powerbook,ibook}.txt`; Intel:
  `actor-map-init-build.txt`.
- Native Mach performance counters and ASCII CRT comparison pass 211 checks on
  all three Macs. Counter units are nanoseconds with a matching fixed frequency;
  integer conversion avoids multiplying the entire uptime by the timebase.
- Reused actual shared settings and unchanged vendored TOML implementation with
  a generated native file/path/log compilation boundary. Defaults, persistence,
  environment overrides and invalid-type fallback pass 22 checks on all three
  Macs using isolated directories. No constant settings substitutes. Evidence:
  `build/mac-audit/config-native-{control,powerbook,ibook}.txt`.
- Reused the actual shared UTF-16 implementation with native file boundaries
  and pthread tokenizer TLS (Tiger cannot use its compiler TLS declaration).
  Intel and both G4s pass 22 checks, including mixed/64-bit formatting,
  Unicode/surrogates, truncation and thread isolation. No Darwin wide-character
  imports remain in that probe. Logs: `build/mac-audit/wide-native-build.txt`
  and `wide-native-{powerbook,ibook}.txt`. The SDK-header redirect variant also passes 22 checks on all three Macs.
  The broad gate applies routing to unicode.c, the sole compiled unit with
  wide-character imports in its object audit; future added units need the same
  audit. Global engine_prefix.h remains unchanged.
- Added actual typed common storage and a whole-source game-engine wrapper for
  its private global type. Null subsystem pointers remain uninitialized until
  their real initializers run; these are not opaque weak buffers. The full
  constructor dependency build is continuing with actual provider units.
- Expanded the full constructor gate to 130 actual engine/native translation
  units plus real settings/UTF-16 support objects. All compile for PPC and
  Intel. The repaired full link still has 118 unresolved symbols on both
  architectures, down from 205 at the previous milestone. This now reaches
  genuine renderer backend, AI encounter/perception, camera and sound-service
  boundaries. Linking alone would still require complete runtime initialization.
  Evidence: `build/mac-audit/spawn-link-130-{ppc,i386}.txt` and matching
  `spawn-providers-130-*.json`. Earlier duplicate-symbol attempts do not provide
  a valid undefined-symbol census; the report generator now marks that case
  unavailable instead of incorrectly implying zero remaining dependencies.
- Both G4s still run modified Leopard 10.5.9; Tiger SDK compilation is not Tiger
  runtime validation. Local retail unittest suite passes 22 checks. Blood Gulch
  remains unplayable pending complete engine linkage and runtime integration.

## 2026-10-15 — integrated actor dependency batch passes on both G4s

- Built with MacBook GCC 4.0 and the Tiger SDK, ran the Intel controls, then
  deployed the same PPC binaries to PowerBook and iBook. Both report Leopard
  10.5.9; this is not a Tiger runtime test. Hardware logs:
  `build/mac-audit/spawning-batch-powerbook.txt` and
  `build/mac-audit/spawning-batch-ibook.txt`.
- Complete supported native model roots now include nodes, regions, markers,
  shader references and geometry, with actual engine vertex compression and
  owned CPU-backed D3D resource headers. Retail `mod2` publishes explicitly as
  engine `mode`; no raw retail root fallback. Both G4s pass 262178 checks,
  digest `38f8ff76`. This validates CPU resources, not GPU upload or drawing.
  Bitwise deterministic geometry rounding passes 30 checks on all three Macs.
- Native cyborg animation graph contains all 254 clips plus seats, weapon
  classes/types, nodes and owned streams. Pistol world and both first-person
  graphs also convert. Both G4s pass 3255 checks; cyborg digest `1fe96678`,
  pistol world `8db82016`, AR first-person `89aaa7b1`, pistol first-person
  `16af5f24`. Unsupported nonempty graph families still fail explicitly.
- Actual native biped/rifle/pistol collision publication now passes 8456 checks
  on both G4s, digest `707180ca`, with all three owners released. This supersedes
  the Intel-only limitation in the preceding entry.
- Native portal geometry, reciprocal cluster links and exact PVS rows now
  support actual structure sphere traversal and real cluster-partition
  reconnect/list/disconnect. Both G4s pass 9596 checks across 72 starts and 30
  clusters. Rendering, AI, attachment services and full object-to-world
  reconnection remain outside this scoped world test.
- The atomic actor loader publishes 14 owned roots: biped, two weapons, three
  models, three collision models, two world animation graphs and three lights.
  It owns 2115313 bytes, deduplicates references, validates hierarchy/checksum
  compatibility and rolls back on late failure. After releasing source-map
  memory, actual stock `item_new`/`weapon_new`, allocation, compaction and salt
  reuse pass 93 checks on Intel and both G4s.
- Expanded the genuine constructor link gate from 42 to 63 whole source units.
  All 63 compile on both PPC and i386; the full link still fails with the same
  205 unresolved symbols on each architecture (161 prior dependencies resolved,
  56 newly exposed). Candidate census: 181 engine/shared, 23 platform and one
  actual header inline requiring native emission. Full callback tables remain
  intact; no gameplay stubs. Logs `build/mac-audit/spawn-link-63-{ppc,i386}.txt`
  and `spawn-providers-63-{ppc,i386}.json`. Local retail unittest suite: 22 pass.
- This is NOT full spawning: `object_new`, `unit_new` and biped creation have
  not executed in this batch. Complete real object callback tables retain a
  broad engine dependency closure; the next gate adds actual provider source
  files rather than substituting gameplay callbacks. Enhanced-network weapon
  precaching, effect/bitmap/lens/HUD/dialogue payloads, platform initialization
  and a working player update loop remain work. Blood Gulch is not yet playable.

## 2026-10-15 — next spawning batch in progress (Intel control only)

- Adopted the agreed batching cadence: implement related spawning dependencies
  together; use local checks and a shared MacBook compile/control gate; defer
  G4 deployment until an integrated milestone or a specific PPC failure needs
  isolation. Both G4s still need the next endian/ABI milestone confirmation.
- Added an actual native collision_model adapter for the biped, rifle and pistol.
  It owns resistance/effect names, materials, regions/permutations, pathfinding
  spheres, nodes and all eight native BSP arrays. Registry publication uses the
  actual engine collision-model lookup. No portable audit structure is exposed
  as an engine root. Obsolete nonempty modifiers still fail explicitly.
- MacBook GCC 4.0/Tiger SDK builds both i386 and PPC; Intel control passes 8456
  checks, digest 707180ca, with all three owners released. Roots are 664 bytes;
  total ownership is 27689 / 4768 / 4256 bytes for biped/rifle/pistol collision.
  Logs: build/mac-audit/native-collision-build.txt. **These new adapter changes
  have not been run on either G4.** Earlier portable-decoder G4 evidence does
  not establish this adapter's runtime correctness.
- Added direct dependency enumeration to the native biped/weapon descriptor
  walker, including nested references and predicted resources, cancellation and
  map-buffer-independent ownership checks. It inventories loading work, not
  transitive readiness. MacBook Intel passes 37 checks plus 28 malformed/empty
  block checks; the biped has 18 direct references, digest a201de01. Native
  biped/rifle/pistol field digests are unchanged. PPC also compiles, with runtime
  deferred to the next integrated G4 batch. Log:
  build/mac-audit/native-object-dependencies-build.txt.
- Next batch: complete actual model/animation roots and their group/layout
  translation, then the remaining constructor services and map linkage. Do not
  install the location-only world as a complete gameplay world. Full biped
  spawning and playable Blood Gulch remain unverified.
- Revisited an earlier emulation project's Tiger/ATI evidence and a controller
  project's status as references only; no changes to either project. Graphics/controller work remains separate from
  this native-tag batch. No new source/ edits in this increment.

## 2026-10-15 — owned native tags, collision models and real world locations

- Native registry inventories all 2408 retail tags, checks salted handles and
  canonical inheritance, owns converted roots and rejects unconverted data.
  Real engine tag_get/tag_get_name now use this boundary. Registry tests pass
  7250 checks on MacBook Intel and both G4s, plus host ASan/UBSan; failed or
  duplicate publication leaves ownership with the caller.
- Complete native multiplayer biped storage (actual biped_definition plus every
  nested block/name) publishes through real engine lookup. Final combined
  biped/weapon test passes 31 checks plus 28 mutation/empty-block checks on all
  three Macs. Biped: 1268-byte root, 3054 owned bytes, 271 values, digest
  618158e7. Source map memory is freed before retained-data checks. This does
  not yet validate every gameplay index or provide the referenced tag closure.
- Complete native assault-rifle and pistol storage now includes every inherited
  item/object field, first-person references, magazines, triggers and firing
  effects. Combined generated schema: 28 structures / 324 typed fields. Rifle/pistol
  digests a18ee21a / 0ea985ea match all three Macs.
  Constructor-sensitive magazine/trigger domains are checked; broader firing
  enum validation and referenced dependency conversion remain outstanding.
- Actual stock-engine item_new and weapon_new now execute on real engine object
  header/data/pool allocations: 63 checks pass on MacBook and both G4s. Checks
  cover AR 60 loaded/180 spare, pistol 12/48, low initial ammunition, item
  lighting flags, actual game-time 123, pool compaction, stale salts and slot
  reuse, with zero engine errors. This is isolated constructor-parts execution,
  not object_new or full weapon spawning. Enhanced-network projectile-trail
  precaching is outside this explicit stock configuration; no fake callback.
- Bounded portable collision-model decoder handles the biped, assault rifle and
  pistol: respectively 16/1/1 node BSPs, 206/26/18 surfaces. All three Macs match
  digest 41cd426e and pass 36 malformed/synthetic checks. Host sanitizers pass.
  These are owned portable structures; actual engine collision_model publication
  still needs a native adapter. Nonempty obsolete modifiers are rejected.
- Whole scenario.c and bsp3d.c execute actual scenario_location_from_point with
  scoped native world data. MacBook and both G4s pass 11033 checks, zero engine
  errors: 5971 leaves, 30 clusters, all 72 starts open, digest 9a36dc52. Includes
  a 2646-point grid, malformed leaf metadata and restoration of prior globals.
  Retail player starts at 0x374 are translated to native scenario.players at
  0x354. This view supports location queries only, not full object map linkage.
- Builds use MacBook GCC 4.0 with the Tiger SDK, then execute on real PowerBook
  and iBook G4 Leopard installations. No current Tiger runtime claim. Logs are
  ignored build/mac-audit/{native-tags,native-object,collision-model,engine-world}-*.
  Existing reader/geometry regression suite: 22 tests pass. Installed Halo data
  remains unchanged. No new source/ changes in this increment.
- Constructor dependencies and next integration boundaries are recorded in
  docs/ppc-mac-native-spawning.md. Full object_new/unit_new and a playable
  Blood Gulch session remain unverified.

## 2026-10-15 — real engine selection/seat mutation and constructor prerequisites

- Actual engine name lookup, unit_animation_get_fitting and unit_set_seat now
  execute with scoped native selection data and a diagnostic unit/header.
  All three Macs pass 533 checks: 254 names plus uppercase variants, rifle/pistol
  idle/front slots, real seat state mutation and invalid-input rejection. One
  expected engine error tests undersized model capacity. No spawned biped claim.
- Bounded native animation selection graph decodes 24 seats / 58 classes / 127 types,
  3899 indices / 44 IK points / 23 sounds / 254 clips. Digest 0a472803 matches MacBook and
  both G4s;12 malformed-map checks pass everywhere, plus host ASan/UBSan.
- Native biped/starting-weapon input bundle matches digest 7ea7d0c1 on all three
  Macs, with 30 synthetic checks passing. AR expected 60 loaded / 180 spare and pistol
  12 loaded / 48 spare are decoded expectations, not weapon_new execution.
- Actual data.c/memory_pool.c allocation+relocation tests pass 20 checks on all
  three Macs using real biped/weapon/header sizes 1152 / 636 / 12. No fake constructors.
- Added clean native XDK declaration prefix after researching units.c compiler
  blocker. Whole units.c, bipeds.c, weapons.c compile PPC+i386 on MacBook without
  source edits, TLS erasure or x86 impersonation. 72 native constructor-layout
  witnesses pass Intel and both G4s. No full Xbox service implementation claim.
- Detailed evidence and remaining creation dependencies:
  docs/ppc-mac-selection-spawning.md. Actual object_new/unit_new/weapon_new and
  playable Blood Gulch remain unverified; installed Halo resources unchanged.

## 2026-10-15 — real engine base-animation sampling on both G4s

- Added bounded native multiplayer-model skeleton decoding: 19 nodes, digest
  32e86515 on MacBook and both G4s. Native hierarchy/transform checks reject
  cycles, invalid indices and nonfinite data; 28 synthetic checks pass.
- Added uncompressed animation decoding with mixed packed16-bit quaternion and
  float fields. All 254 clips/7292 frames decode, digest 1fe96678 matches all
  three Macs. Preserved observed unused mask bits and clamped loop/event marker
  conventions. Compressed streams remain explicitly unsupported.
- Whole engine animation/model/math units built on MacBook now sample decoded
  native model+animation data on both G4s: 174 base clips,5868 frames,111492 node
  poses,899215 checks,0 failures/engine errors. Every component matches an
  independent stream reference bitwise; all machines match digest 04c12069.
- 80 overlay/replacement clips decode but are not sampled by the base API.
  Added model count/checksum rejection and aligned empty-stream handling;
  synthetic all-default/all-animated clips pass actual engine sampling.
- Eight mixedstream checks and29 private-map mutation checks pass on all Macs.
  Host ASan/UBSan and existing22 reader/geometry tests pass. No source/ changes
  in this increment, so full web/Android rebuild was not required.
- Detailed scope, math-import caveats and reproduction: docs/ppc-mac-animation.md.
  Not a spawned biped or playable session. Next: native graph seat/weapon/node
  selection, overlay/replacement paths, remaining object/collision tags and
  actual biped+starting-weapon construction. Keep installed Halo data immutable.

## 2026-10-15 — actual engine collision, player records and native input

- MacBook GCC 4.0/Tiger SDK now builds whole engine bsp3d.c, players.c, data.c
  and unicode.c slices with native PPC/i386 ABI and dead stripping. No TLS
  erasure, architecture impersonation or replacement gameplay logic.
- Actual bsp3d_test_point matches all 72 starts and 5120 grid samples on both
  G4s and Intel: 5204 checks, no failures, digest 2c8542e6.
- Fixed source/memory/data.c identifier seeding to preserve Xbox little-endian
  salts on PPC. Actual player_new lifecycle passes 59 checks on all three Macs,
  including UTF-16 names, machine lists, stale handles and slot reuse. Original
  seed negative control fails two checks on both G4s and passes Intel.
- Added explicit UTF-16 CRT boundary; player binaries reject unsafe Darwin wide
  imports and unresolved weak imports. Unsupported Unicode paths remain unlinked.
- Carbon event decoder passes 49 checks on each Mac. Explicit native movement/
  action translation to actual player_action passes 779 checks on each. These
  use constructed events, not global input posting or a visible application.
- Full web build passes (725 steps); 22 retail-reader/geometry unit tests pass.
  Android target unavailable in existing graph; configuration preserved.
- Evidence/reproduction: docs/ppc-mac-engine-integration.md. Player records have
  no biped yet; no simulated movement, playable session or Tiger runtime claim.
  Next: multiplayer biped tag dependency conversion, real object creation,
  control queues/physics, and application/renderer lifecycle integration.
  Actual-map dependency audit: docs/ppc-mac-biped-plan.md; first conversion
  target is the 19-node skeleton and 254 uncompressed animation records.

## 2026-10-15 — collision, 72 starts, and textured Blood Gulch on both G4s

- Added bounded native collision arrays/point-leaf queries and diagnostic static
  grounding. All three Macs match collision digest 4cf01d47: 611/611 sampled ground
  hits classify open just above the surface. No capsule/object physics claim.
- Added native spawn decoding and source-equivalent mode filtering. All 72 starts
  classify open, all 72 have ground within the diagnostic range; same results on
  MacBook Intel and both G4s. Spawn 913 and collision 39 regression checks pass.
- Added bytewise RGB565/BGRA/DXT1/3/5 RGBA decoders; 276 regression checks pass.
  Offscreen base-texture/lightmap drawing loads 30 real images on each Mac,
  decoded digest ad223d21 matches, zero GL errors. Two teleporter materials remain
  unsupported/magenta; no claim of complete material rendering.
- iBook now has actual full bitmaps.map staged in probe storage, 317199907 bytes;
  both G4 texture runs read actual payloads. Original installations untouched.
- Retrieved/inspected PowerBook textured image. Details, tests and reproduction:
  `docs/ppc-mac-playfield.md`. No playable engine session yet. Next priority is
  native cache/accessor and actual engine player/object update integration,
  remaining tag coverage, platform input/audio and engine renderer work.

## 2026-10-15 — native Blood Gulch BSP and offscreen G4 terrain rendering

- Added bounded native BSP geometry visitor with full preflight before callbacks,
  engine count limits, float/vector checks and local triangle-index validation.
- Real-map MacBook Intel / PowerBook G4 / iBook G4 all decode 79 materials,
  5762 environment vertices,5506 light vertices,5503 triangles; native digest
  89aa9249 matches on all machines and the sanitized modern-host build.
- MacBook-built legacy OpenGL diagnostic renders actual terrain offscreen on
  both G4s with zero GL errors. Retrieved and inspected PowerBook image shows
  canyon and bases from above. Synthetic height colors; no textures/gameplay.
- Fifteen retail and seven new geometry asset-free tests pass. Independent
  research verified layouts/limits and reviewed decoder. No production loading
  changes and no existing installations/desktops modified.
- Full evidence, image location and reproduction: `docs/ppc-mac-bsp-geometry.md`.
  Next: collision/scenario placement decoding, texture/lightmap uploads and native
  cache integration. First playable-map milestone remains unverified.

## 2026-10-15 — real retail v7 Blood Gulch metadata on both G4s

- User explicitly selected version 7 maps from the MacBook/PowerBook installations.
  Added standalone bounded retail reader for native metadata, retaining immutable
  LE source bytes. Checks index/scenario/BSP/model ranges and nested resources.
- Confirmed 2408 tags, one BSP, 676 external bitmap payloads and 951 external sound
  payloads. Whole-tag external flags are all zero: CE's existing discovery would
  miss these dependencies, so do not simply relax its version check.
- Added wind/sound-environment complete scalar codecs and actual source layout
  witnesses. MacBook-built PPC tests pass on both G4s; i386 control matches.
  135308 endian checks; real 32 scalar tags and 19001 script slots round-trip.
- PowerBook used installed game files; iBook used staged MacBook map and explicit
  resource-size metadata, not actual resource payloads. Installations untouched.
- Three subagents supported format audit, scalar implementation and regression
  tests (the latter on an explicitly selected smaller model). Host 15 asset-free
  test cases and real-map sanitizer run pass; independent review incorporated.
- `docs/ppc-mac-retail-maps.md` is the current version 7 path, reproduction and
  next-work reference. Next: native-cache/accessor separation, scenario/BSP and
  geometry conversion, remaining tag coverage and retail script mapping. No
  playable map or full-engine loader integration yet.

## 2026-10-15 — cache codecs and first complete numeric tag

- Added standalone bounded native/LE codecs for Xbox headers/tag index records,
  BSP metadata, GPU descriptors, entire script allocations and point physics.
  All runtime coefficients are converted; unused bytes and strings preserved.
- MacBook GCC 4.0/Tiger SDK builds pass on both G4s and i386 control:
  135210 synthetic checks, plus seven existing-map-header checks where present.
  Actual engine header witnesses verify layouts and decoded point-physics floats.
  Host sanitizer run also passes; independent research review checked layouts
  and identified a virtual-address-wrap edge now fixed and regression-tested.
- Located installed Blood Gulch on PowerBook/MacBook, but it is retail version 7,
  not Xbox version 5. No version checks bypassed and installations untouched.
- New codec is deliberately not connected to loading: validator schemas omit
  numeric fields, so complete tag/GPU coverage is still required before loading.
- Details, data locations, reproduction and integration boundaries:
  `docs/ppc-mac-map-endian.md`. Next: typed wind/sound-environment coverage,
  scenario/BSP traversal and additional game-data inventory. No playable map yet.

## 2026-10-15 — native scripting endian patch verified

- Fixed runtime conversions, equality/logical readers/results, sleep defaults,
  narrow external-global reads, and typed evaluator/syntax aliases together.
  Added `source/hs/hs_value.h`; syntax remains 20 bytes. Parser and CE script
  accessors benefit from the shared corrected aliases.
- Expanded actual-source extraction tests to 318 checks. MacBook-built PPC code
  passes on both G4s; Intel i386 control passes. Restoring old layout/store
  behavior fails 45/36 checks on both G4s, with exact exit status 1.
- Independent research-agent review found no actionable correctness defects.
- Details/reproduction/limits: `docs/ppc-mac-scripting.md`. The previous failing
  scalar conversion is now fixed; do not repeat the old diagnosis as a blocker.
- Full Android check attempted but unavailable (unknown android target).
  Full engine/map execution remains unverified. Next: map conversion coverage,
  engine linkage/TLS and actual GLSL translator work toward Blood Gulch.

## 2026-10-15 — compiler/ABI and GLSL follow-up

- Added reproducible `tools/mac_compiler_probes.sh`: real_math.c compiles at
  -O2 on the Intel build Mac for PPC Tiger; emitted assembly has no fused multiply-add family
  instructions. This is compile-only, not linked engine execution.
- Engine-header layout/anonymous-union probe passed on both G4s. GLSL 1.20
  texture sampling and dynamic 192-vec4 bank probes passed on both G4s, including
  a final-bank-entry displacement check and zero GL errors.
- Corrected the previous scripting union-return claim: current converters
  return long. Generic PPC aggregate-return behavior was measured separately.
- Reproduced the actual script scalar endianness bug with extracted engine
  functions: -7/7 short-to-real chains fail on both G4s and pass on the Intel build Mac i386.
  Do not retry or classify those intentional nonzero exits as SSH failures.
- Research agent confirmed coordinated slot-boundary work is required; changing
  converters alone would break other consumers. No scripting production edits.
- Full results and next implementation steps: `docs/ppc-mac-compiler-shaders.md`.
  Raw logs remain in build/mac-audit. The first playable map is still pending.

## 2026-10-15 — network hardware and first PPC engine execution

- Read saved SSH configuration and project connection notes. Existing aliases
  resolved the earlier identity issue; do not repeat bare-IP attempts.
- Inventoried PowerBook G4, iBook G4, Core 2 Duo MacBook and real Tiger G3.
  See `docs/ppc-mac-hardware.md` for actual OS/tool/GPU data and limitations.
- Extended graphics probe: both G4s report GLSL 1.20 and pass offscreen triangle,
  depth and blend checks. Exact blend values and tolerance are documented.
- Made the engine's wide byte-swap masks portable without changing their value.
  Added a harness linking the actual engine unit and a reproducible build script.
- GCC 4.0 on `intel` built the PPC Tiger executable; all swap tests passed on
  PowerBook G4, iBook G4 and Tiger G3. This is the first real engine-unit proof,
  not a complete game build. GCC 4.2/10.4 SDK and xcodebuild problems recorded.
- Compiler research agent identified anonymous structs, inline linkage, TLS and
  scripting union-return ABI as next investigations. No existing Mac software
  was changed; remote probes are in `$PROBES/20261015`.
- Next: representative math compilation, ABI/layout checks and GLSL texture
  tests. Use the hardware report's roles; unavailable VMs do not block progress.

## 2026-10-15 — initial audit

- Cloned upstream into the requested directory, commit
  `8102e711c353068d22d62efe6bf67ebc8a90a22e`.
- Read AGENTS.md; audited build, ABI, map loading, virtual memory, platform,
  renderer, shaders, texture conversion and math integration.
- Reviewed the emulation and controller projects' evidence without modifying either.
- Research subagent investigated the legacy-renderer blocker. Recommendation:
  probe real hardware, use legacy shaders if supported for the first map,
  retain fixed-function minimum-GPU work as a separate required milestone.
- Added `tools/mac_capabilities.c`; compiled and ran on the modern host only.
  Output in ignored `build/mac-audit/host-capabilities.txt`. No PPC validation.
- Created active hourly thread automation `advance-opence-powerpc-port`.
  It chooses recommended reversible options and uses research agents when stuck.
- Remote inspection is unavailable until a trusted PowerBook host identity is
  established; two bounded attempts made, no remote modifications.

## Initial next-work list (superseded by live hardware entry above)

1. Build a compile-only representative PPC ABI smoke harness and inventory
   required SDL services. Investigate available cross/native compiler choices.
2. Extend the standalone graphics diagnostic with deterministic offscreen
   texture/depth/blend readback. Measure the actual PowerBook when trusted access
   is available. Do not use modern host results as PPC capability evidence.
3. Inventory Xbox cache fields and existing byte-swap metadata for complete
   conversion coverage; the current tag-validator schema is insufficient.
4. Locate user-provided Xbox game data within explicitly relevant project
   locations or request its path when needed; no game data is in this checkout.

Read `docs/ppc-mac-audit.md` for priorities and acceptance criteria. Keep each
iteration reviewable, preserve upstream behavior, record actual checks and
update this log so scheduled runs do not repeat unchanged blockers.

## 2026-10-15 — expanded genuine renderer provider closure

- The isolated `tools/mac_spawn_graphics_checks.sh` gate now compiles 41 real
  renderer/support units on the MacBook for PPC and i386, over the existing
  203-unit spawning baseline. Both architectures report zero compile failures.
  Incremental runs reuse only explicitly unchanged baseline/graphics objects.
- Preserved the original frame-statistics layout witnesses. Its two private
  64-bit counters require eight-byte alignment; the checked generator
  `tools/mac_frame_statistics_boundary.py` adds that alignment to an otherwise
  complete compilation view, without changing engine source or global packing.
  All original offset and size assertions pass on both architectures.
- Added actual shader preprocessors, rasterizer support, loose-sound reader,
  shield-color logic and whole player-profile color provider. Renderer globals
  use their authoritative declared types. The native scheduler yield follows
  the shared kernel's real `sched_yield` behavior.
- Linked the separately verified native CPU device-state and shared CPU resource
  objects. Final linker census is **24 identical unresolved symbols** on PPC and
  i386: 19 Direct3D immediate/draw, render-target, visibility and vertex-shader
  APIs; four high-resolution text services; and `cache_file_read` for the actual
  sound-cache path. No API has been satisfied by a fake gameplay callback.
- Evidence: `build/mac-audit/spawn-graphics-41-build.txt`,
  `build/mac-graphics-audit/link_{ppc,i386}.txt`, and
  `build/mac-graphics-audit/providers-41-{ppc,i386}.json`. Compilation/link
  evidence only: this does not execute `object_new`, establish initialized
  rendering or audio, or make Blood Gulch playable. Next work is an explicit
  native drawing-disabled device lifecycle, genuine text/retail cache services,
  then the complete engine initialization and spawning test.

## 2026-10-15 — complete spawning link and real native text atlas

- The MacBook now **links the actual `object_new` dependency closure for both
  PPC and i386**, including genuine object dispatch tables and the separately
  verified native device, resource, text and cache services. Both architecture
  gates report zero compilation failures. This is a link milestone, not proof
  that `object_new` has executed or that Blood Gulch is playable.
- Reused the exact shared `text_hires.c` CPU font/atlas functions and vendored
  stb implementation. The generated native boundary leaves GL atlas upload
  unimplemented, rather than reporting fake GPU success. Fonts are the existing
  licensed Overpass assets; native embedding preserves their byte order in
  aligned byte arrays, avoiding the x86 embedding's word-order assumption.
- Actual font lookup, rasterization, cached glyphs, scale-triggered atlas reset
  and native teardown pass 56 checks on Intel and both G4s. All three report
  atlas digest `843122b2` and 4,558 covered pixels. Tests do not claim onscreen
  text drawing. Evidence: `build/mac-audit/text-{build,powerbook,ibook}.txt`.
- Link evidence: `build/mac-audit/spawn-graphics-complete-build.txt` and
  `build/mac-graphics-audit/link_{ppc,i386}.txt`. Complete object manifests are
  `build/mac-graphics-audit/objects_{ppc,i386}.txt`; portable C inputs are listed
  separately in `source_inputs.txt`. The diagnostic weapon probe's main must
  be excluded when linking a new engine initialization/spawning executable.
- Next: execute actual object-system allocation and map initialization against
  the native Blood Gulch registry/world, then call full biped/weapon creation.

## 2026-10-16 — bounded scenario multiplayer blocks on both G4s

- Added owned, endian-correct conversion for Blood Gulch's actual scenario
  netgame flags (191), netgame equipment (37) and starting equipment (2), then
  attached those arrays to the scoped native scenario. The decoder checks the
  source block limits/record sizes, finite positions and facings, known flag
  and game-type enums, team bounds, and typed `itmc` references. It owns copied
  tag names and frees all arrays on world teardown. The already-converted 72
  player starts remain on the existing retail-spawn path.
- The PPC and i386 builds pass; the focused probe passes 445 checks on Intel,
  PowerBook G4 and iBook G4, including malformed count, enum and reference
  rejection. Evidence: `build/mac-audit/scenario-gameplay/`.
- This is scenario data conversion and scoped attachment, not a successful
  `scenario_load`/BSP-switch lifecycle or playable multiplayer session. Skies,
  triggers and other scenario blocks remain separate dependencies.

## 2026-10-16 — bounded NV2A immediate draw and native screen clock

- The native GPU device now accepts deliberately narrow real draws: the
  immediate triangle path with a live retail vertex program, an NV2A-generated
  untextured pixel program, full color writes, and no blend, fog, stencil,
  or alpha test. It translates both
  generated programs to GLSL 1.20, compiles/links them in the current native
  framebuffer, uploads the D3D constant banks and vertex registers, and issues
  `glDrawArrays`. A second bounded path reads live index and stream data and
  expands it through the same shader path. It accepts only stream zero, float1
  through float4, `NORMPACKED3`, `SHORT2`, `SHORT1` and `PBYTE2` declarations, and supported triangle/line
  primitives; it rejects unregistered, freed, overrun, non-finite, and
  unsupported inputs. Packed vectors are CPU-decoded with the source
  `uncompress_int32_to_real_vector3d` mapping before entering the shader path.
  Indexed expansion currently uses GL arrays rather than GL element buffers.
- `mac_device_state_probe` runs retail vertex-shader table entry 0 through the
  actual D3D `Begin`/`SetVertexData4f`/`End` path and checks framebuffer pixel
  readback (RGBA 63–64, 127–128, 191–192, 255). Vertex register 0 commits an
  immediate vertex, so the test writes color register 9 first for each vertex.
  The D3D channel-write mask is mapped from the Xbox bit positions.
- The suite passes **191 checks on MacBook i386/PPC and both G4 PPC hosts**.
  It draws a selected real Blood Gulch BSP triangle using the 32-byte
  environment-compressed layout (float3 position, three packed vectors,
  float2 UV) through the indexed GPU path. Decoded normal, binormal, and tangent
  components match the source geometry floats within its 0.01 compression
  tolerance; framebuffer readback confirms pixels changed. It also checks an
  out-of-range index rejection.
  The allocation query recognizes only live ranges registered by their owner;
  native model vertex/index buffer creation registers its exact readable spans
  and unregisters before release. The focused GPU probe uses separately owned
  contiguous test buffers. A second shader/readback case covers the source
  model-compressed 32-byte field layout: float3 position, three packed vectors,
  signed-short2 UV, byte2 node values, and signed-short weight. Separately, the
  native model probe checks Blood Gulch's real compressed biped and weapon
  vertices against the engine decompressor: positions, UV codec, node-triplet
  divisibility and node-index mapping all agree. It passes 303,937 checks on
  MacBook i386/PPC and both G4s with digest `38f8ff76`. The GPU fixture does not
  yet submit retail model buffers; actual retail model rendering remains on the
  separate diagnostic GL path.
  Full BSP lighting/shaders remain unsupported. Logs are
  `build/mac-audit/device-model-compressed-i386.txt`,
  `device-model-compressed-powerbook.txt`, and `device-model-compressed-ibook.txt`;
  actual model-layout runs are in `device-model-compressed-native-i386-ppc.txt`,
  `model-compressed-native-powerbook.txt`, and `model-compressed-native-ibook.txt`.
  The GPU-probe PPC executable SHA-256:
  `c2f52ee7bc46b317d6727710ecc949b557635829e44316ac41d8de953abcc143`.
- Native screen commit currently reports a fixed logical 640x480 surface and
  zero UI offset; it avoids mixing physical window width with the game's
  480-line coordinates until uniform physical/logical scaling is implemented.
  The native vblank service uses Mach absolute time for Xbox-style 60 Hz frame
  pacing; this is synthetic timing, not measured monitor-vblank synchronization.
  Callback replacement and device teardown quiesce the callback and stop/join
  the clock thread. These checks establish neither a complete Direct3D device
  nor a full Xbox renderer; unsupported shader/state cases still fail closed.

## 2026-10-16 — live model geometry over the terrain camera

- Corrected the live-model GPU diagnostic projection to select the same
  coordinate components as its largest model axes. The earlier biped failure
  came from projecting world X/Y while the diagnostic had selected Z/Y; this
  placed its vertices around clip X=219. The corrected path uses the actual
  selected skinned position components and retains the live node matrices,
  owned compressed model buffers and real strip indices.
- The actual biped plus both starting weapons pass **63 checks with zero
  failures** on MacBook i386 and PPC and on PowerBook G4 and iBook G4 PPC.
  Fresh GPU results: biped 4 parts/2256 vertices/1147–1149 pixels; weapon 0
  8/819/1137; weapon 1 2/1033/869. Binaries are SHA-1
  `5be819c8967af474d0774846a0ec898e3d95bc70` (i386) and
  `7a16aaf86ba4febf890cbcbfac6599d47267336f` (PPC).
- Added an opt-in overlay for the actual unit camera. After each bounded
  `objects_update`, the hidden runtime view draws the BSP with the camera from
  `unit_get_camera_position` and the live aiming vector, then draws the biped
  and current weapon through their live object node matrices and active model
  permutations. All three hosts pass **136 runtime checks, zero failures**;
  each overlay changes the frame digest. The overlay draws base bitmap layers
  where supported; unsupported base shaders and omitted alpha/light/detail
  composition are counted and reported. This is a geometry/base-texture
  diagnostic, not full retail shader parity or a complete game renderer.
- Hidden captures are `build/mac-audit/terrain-object-overlay-i386.png`,
  `terrain-object-overlay-pbg4.png`, and `terrain-object-overlay-ibook.png`;
  the PPC captures have matching SHA-1 `382b75be6a3d1a76a772a77f5d990beb8ccd3354`.
  Per-host run logs are `terrain-overlay-i386.log`,
  `terrain-overlay-powerbook.log`, and `terrain-overlay-ibook.log`; the final
  63-check logs are `model-gpu-final63-i386.log`,
  `model-gpu-final63-powerbook.log`, and `model-gpu-final63-ibook.log`.

## 2026-10-16 — native BSP sound-distance matrix ownership

- The native world loader now converts the BSP `sound_cluster_data` header,
  bounds-checks the complete payload against its BSP mapping and the required
  triangular cluster-pair size, and copies the encoded bytes without endian
  transformation. The world owns that copy through teardown, so the original
  mapped cache may be released while AI and weapon sound propagation still use
  the actual `structure_bsp_get_cluster_encoded_sound_data` path.
- The focused Blood Gulch world probe passes **11,058 checks with zero
  failures** on MacBook i386/PPC and both G4 PPC hosts. It verifies all 30
  clusters are represented by a 435-byte sound matrix, and checks the matrix
  remains readable after releasing the source map buffer. The payload digest is
  `fc9a4c00`; engine service errors remain zero. Logs are
  `build/mac-audit/world-sound-cluster/intel-ppc.txt`,
  `powerbook.txt`, and `ibook.txt`; i386 output is in
  `build/mac-audit/world-sound-cluster/intel-i386.txt`.

## 2026-10-16 — original main-game render link boundary

- Added `tools/mac_main_game_render_view.py`, which compiles the unchanged
  `main_game_render` body from `source/main/main.c` and appends only a public
  wrapper. `tools/mac_main_game_render_checks.sh` combines that wrapper with
  the genuine session manifest and the source units reachable through
  `render_frame`, emitting reusable per-architecture object manifests and
  source hashes.
- The original render graph links for i386 and PPC on the MacBook against the
  final36 session manifest. The test did not execute `render_frame` and does
  not yet claim original rasterizer initialization or image output. Binaries
  and exact link/source provenance are in
  `build/mac-audit/render-frame-link/`; SHA-256 is
  `8693af675eb62024240d4f681ee00de0b2c6931d523747db0db5776a8f164cdd`
  (i386) and
  `f0f0b29925495de98c844d095cc4660e6f2921d9bac4529583bd1ae64b4fc2f0`
  (PPC).
- Before any window exists, `halo_screen_width()` now reports the game's
  logical 640-pixel width; the independent platform window-size query remains
  zero. This matches the rasterizer's source order, which creates its device
  before opening a host window. The hidden CGL window probe passes on the
  MacBook's NVIDIA GeForce 9400M and checks both pre-window and post-teardown
  behavior; no visible window is opened.
- The opt-in rendered-session checker links the final38 session objects with
  genuine `rasterizer_initialize`, the source `main_game_render` wrapper, and
  original present. On the first real render, the initial i386 run stopped at
  the pre-stencil framebuffer check; that binary is superseded. A rebuild from
  the validated stencil/dual-stream device source then completed genuine
  rasterizer/game startup, Blood Gulch map load, 31 game frames, movement,
  observer mouse look, and seven fired projectile frames before reaching the
  first original draw. It correctly refused the draw because vertex declaration
  element 4 is `D3DVSDT_NORMSHORT2` (`type=0x21`, stream 0, register 4, 4 bytes
  at offset 24 in a 32-byte stride), which was not supported by the bounded
  native stream decoder at that point. No rendered frame or capture is claimed. The exact
  diagnostic run, i386 binary, object list, source provenance, and link output
  are in `build/mac-audit/rendered-session/declaration-diagnostic/`; diagnostic
  device-state source SHA-256 is
  `f07ce1325a437c92365eea442cbe19e9736f873603823edd36521ec340fc56ed` and the
  i386 binary SHA-256 is
  `58bc7e8d52dd662cc9e515eb91016b26ced0221634575ff8720f489f6083c98e`.

## 2026-10-16 — nonindexed dual-stream QUADLIST and stencil

- The nonindexed QUADLIST path passes its dedicated GPU readback test using
  stream 0 positions and stream 1 colors, with the first four vertices
  beginning at stream index 2. The test also checks constant vertex
  attributes and confirms the indexed-draw counter is unchanged. Stencil
  write/read and reject-color modes run in the same probe.
- The exact canonical device-state source
  (`111bd4ec5e4e4c2fc3ddffe4618d163f64bc1852c5f1d11977b3b1a8023ca238`)
  and current probe
  (`3bbfc029fec76f8d39c5f5ad47db120f352c74d28c91766da96c6ab0a6fafd10`)
  pass **233 checks, zero failures** with Blood Gulch and
  `bitmaps.map` on MacBook i386, PowerBook G4 PPC, and iBook G4 PPC. The earlier
  163-check report did not execute mode 13 and is superseded. A separate
  malformed-range run exits through the intended 16-bit index-range guard.
  The checker hash is
  `e6cc932c3dd2898bfd16f29fa78878b7eac0fd0790d04212fa99685421562ba6`.
  Logs and PPC/i386 binaries (SHA-256 `a5af260fbdc39d0705302b3f6500ace415710942aba6be2a7e4334d24d51c048` and
  `f8de11a8f06cbe8ee0be8cbf82bedee349870b14c8708b21af8dac31de406187`)
  are in `build/mac-audit/quadlist-cli/`;
  this synthetic two-stream test does not claim that the original map renderer
  has completed a frame.

## 2026-10-16 — Default texture generator on the build MacBook

The source-checked default A4R4G4B4 texture generator now runs with the
MacBook's installed Python 2.6, without a pre-generated file or Python 3.
Its output was generated on that host and compared with the prior Python 3
output: both SHA-256 hashes are
`29a9dfe690bcd139828a22c1bde943935b286bc993e8aa5529856f94c898e1ab`.
Only generator compatibility changed; the generated renderer remains identical.

## 2026-10-16 — Explicit interactive device selection

The native device factory now accepts a frontend visibility choice before
creating a device. It defaults to hidden CGL, rejects changes while a device
is alive, and can select the existing Carbon/AGL window for a future explicit
interactive launch. No automated run opens a desktop window.
The factory probe passes **45 checks / zero failures** on native MacBook i386
and both G4s. Logs: `build/mac-audit/factory-visible-{build,powerbook,ibook}.txt`;
PPC binary `factory-visible-ppc`, SHA-256
`6c59f92a8734d6e1c12081ee9f179ef4fd8c1a354ce99d33c6e4e2b4cd91cfee`.
This validates hidden creation and selection lifecycle, not visible gameplay.

The interactive path also requests an 8-bit AGL stencil buffer and makes
its context current before device initialization. The frame guard distinguishes
a window drawable (framebuffer zero, actual depth/stencil capability checks)
from hidden CGL framebuffer objects. Visible rendering remains unverified;
automated validation continues on hidden contexts.

## 2026-10-16 — Source declaration vertex formats

The bounded stream decoder now covers the actual Xbox declaration formats
used by `rasterizer_xbox_vertex_shaders_initialize.c`: signed raw SHORT1/2,
normalized NORMSHORT1/2, packed D3DCOLOR, and normalized PBYTE3. The conversion
matches the shared D3D-to-GL mappings; signed normalized shorts clamp the
minimum value to -1. Probe mode 13 checks these formats through a real
nonindexed quad draw, including both normalized-short endpoints, raw signed
values, packed color channels, and byte normalization. Mode 6 exercises the
model declaration's normalized UVs and weight.

The exact focused source (`engine_device_state.c` SHA-256
`e32ec58037198bf93e33095caf0f880678d819601e941130f0f9f569801d753e`) and probe
(`mac_device_state_probe.c` SHA-256
`18733b9dda78f69e99ec2cbba11d35ca1292fe528b2df160222900c50020b06f`) pass
**261 checks, zero failures** on MacBook i386 and PPC, PowerBook G4 PPC, and
iBook G4 PPC with the Blood Gulch and bitmap-cache inputs. Logs are in
`build/mac-audit/streamdecl-types/`; PPC and i386 focused binaries have SHA-256
`b15e9cb458d0d9540a1bfe89597af47cf441eb20482bf6c9df015ad521de0e8a` and
`5c2f9535727c25991682b1c706391fe2663009c4dfd23d765e6f65a9c095876c`.
This closes the first reported declaration failure for the integrated renderer;
it does not claim that the original renderer has completed a frame.

## 2026-10-16 — Renderer checker dependencies and session header drift

The checker scripts that compile `engine_device_state.c` now generate the
XGPU texture decode include and supply the new texture-upload and visibility
query units where their link mode needs them. The affected non-owned scripts
are `mac_device_resources_checks.sh`, `mac_particle_runtime_checks.sh`,
`mac_game_tick_checks.sh`, `mac_cache_vertex_checks.sh`, and
`mac_object_runtime_checks.sh`. All five pass `sh -n`; they were not rerun as
part of this change.

The rendered-session manifest also exposed a potential stale-object ABI split.
Local `engine_cache_files.h` adds a trailing 1024-byte `map_directory` field;
The Intel build Mac copy used by the rendered build does not. Because `mac_engine_session`
embeds `mac_cache_files` before `mac_actor_assets`, mixing objects compiled
against these versions moves `assets.tags` by 1024 bytes. The rendered object
list reuses `engine_cache_files_i386.o` from `build/mac-game-tick` and
`engine_session_i386.o` from `build/mac-game-tick-session-final37`, while
rebuilding the probe. Rebuild the cache-files, session, and probe objects from
one header snapshot before interpreting the `tags.state` value; this audit has
not itself rerun that session.

The subsequent hidden-frame regression includes the drawable/FBO guard,
current four-stage binder, and vertex-format batch: **261 checks / zero
failures on native Intel and both G4s**. Logs are
`build/mac-audit/framebuffer-kind-{build,powerbook,ibook}.txt`; source hashes
are in `framebuffer-kind-sources.sha256`. PPC binary SHA-256:
`ea9db9ef84fbd43f9ccd41331b2aaab917aff4ba214e61fb2fae13680ee0b1f8`.
The new visible branch has compile coverage only.


## 2026-10-16 — Point primitive validation

The native draw boundary used zero as its unsupported-primitive sentinel,
which also equals OpenGL's valid `GL_POINTS`. It now uses a distinct invalid
value for immediate, indexed and nonindexed validation. A real one-point
GPU draw checks exactly one colored pixel and its RGB values. The combined
suite passes **298 checks / zero failures on native MacBook Intel, PowerBook
G4 and iBook G4**. This includes the previously verified volume upload and
visibility paths; it does not establish a complete game frame.

Evidence: `build/mac-audit/point-draw-{build,powerbook,ibook}.txt`.
PPC binary `point-draw-ppc` SHA-256:
`edc824076d2aed0aea0dd2f2a742c5662613009ade483963d4583ce8182a6aa9`.


## 2026-10-16 — Immediate quads and linked shader reuse

The original renderer's next fail-closed stop was primitive type 8, an immediate
`D3DPT_QUADLIST`. The native boundary now draws each quad as triangles
(0,1,2) and (0,2,3), matching the shared renderer. Counts must be divisible by
four, and generated 16-bit indices stay within the bounded vertex batch.
The temporary element-buffer binding is restored after drawing. The GPU test
reads pixels from both halves of the quad.

A context-owned 128-entry linked-program cache now keys the exact translated
vertex/pixel source and attribute mask, including all generated lighting and
debug settings. Per-draw uniforms remain dynamic. Failed programs are never
inserted, LRU eviction deletes the old program, and device shutdown disposes
all programs before closing the GL context. The standalone cache suite passes
**287 checks / zero failures on native MacBook Intel and both G4s**; logs are
`build/mac-audit/gl-program-cache-focused/mac_gl_program_cache_final_*.log`.

The combined device suite passes **304 checks / zero failures on all three
machines**. Each reports seven cache misses and thirteen hits, and the expected
visibility results. Logs: `build/mac-audit/quad-cache-{build,powerbook,ibook}.txt`.
PPC binary SHA-256:
`d3f1ff0e46e7f1ec02193a5b4100d18bd4a0f3742476e914a07a056dd1cf39eb`.
State source SHA-256:
`e5f7ece227d766e71760448dcd4f29e8f8ff700dd8ab8225b6a4d7776cafae9b`.
These are focused tests; a complete original game frame is still the next gate.


## 2026-10-16 — Compact engine tag indices and integrated render progress

The original renderer now passes the first-person lighting shader (`vec3`
varyings), immediate quad drawing, and the particle tag lookup on native
MacBook Intel, PowerBook G4 and iBook G4. `render_particles.c` intentionally
stores a definition index in a signed short; the native engine adapter now
resolves nonnegative compact indices through the published native registry.
Full salted handles still require exact identity, and the registry's loader
APIs remain strict. The bound-session regression checks full/compact root,
name and group equivalence plus invalid/stale index refusal. It selects an
already published root because the scenario is installed later by game_load.

Registry tests pass **7,282 checks / zero failures on all three Macs**, with
all 2,408 raw cache tags refused until publication. Evidence:
`build/mac-audit/native-tags-index-{intel,powerbook,ibook}.txt`.

All three integrated runs reach the same subsequent guard in the original
transparent-geometry draw: its dynamic index buffer was allocated by shared
`D3DDevice_CreateIndexBuffer` but was not registered with the native span
owner. The guard has not been weakened. This remains a failed full-frame run,
not a completed frame or playable session. Exact binaries/provenance and G4
logs are in `build/mac-audit/rendered-session/compact-index/`; Intel runtime
log is `compact-index-runtime2-i386.txt` in its parent directory. PPC SHA-256:
`79d17d7c95a0b55fbb642c15ee10ac4880543f34775e72c2fd7da1e73c6e3261`.

The probe also has opt-in `--gpu-interactive` and bounded
`HALO_MAC_RENDER_FRAMES` support through a native 30-Hz input/simulation/render
loop. The default remains a hidden single-frame run. These paths compile for
PPC/i386, but sustained rendering and the visible window are not yet validated.

## 2026-10-16 — Dynamic index ownership and remaining frame blockers

Native-created dynamic index buffers now register their exact allocated span
and unregister it on final release. The focused CPU resource suite passes
**49 checks / zero failures on MacBook Intel and both G4s**, including bounded
12-byte ownership, rejection of a 13-byte access, retained ownership across an
extra reference, and removal at final release. Logs are
`build/mac-audit/index-owner-final2-build.txt` and
`build/mac-audit/index-owner-cpu-{powerbook,ibook}.txt`. PPC test SHA-256:
`f564a55af101ece866fe7eb983ffc067f7ad170f9d4dfdeec0e070eb84f006ff`.

The integrated Intel run passes that original transparent-geometry draw and
reaches HUD text, where the strict native texture cache rejects the runtime
font placeholder. The two G4 runs instead halt on an invalid real RGB color
(-429492160, -429492160, 0) in bitmaps_inlines.h:244. These are distinct observed
blockers; neither run completed a frame. Exact binaries, provenance and G4
logs are in `build/mac-audit/rendered-session/index-owner/`; the Intel log is
`index-owner-runtime-i386.txt` in its parent directory. Full PPC SHA-256:
`c3e47469288ba249906c4c8df972fd3dbd4de5d6e95712bce40aa0ceb1fdeef2`;
Intel SHA-256:
`3355b8b9636110e8954c2ed7fd6c9f7561158f3cf83695b501607159f81ef426`.

The full graph now also compiles and links with the existing real GL font atlas
uploader in place of the CPU-only service, using a scoped borrowed-texture
resolver and the placeholder's original UV dimensions. Runtime validation of
this change and the PPC color diagnosis remain pending.

The runtime bitmap fallback now passes **65,756 checks / zero failures on
native Intel and both G4s**. It admits only a noncached, allocated 2D A4R4G4B4
bitmap whose dimensions/mip chain match a genuine D3D-created texture and whose
entire data span is live in the native allocator registry. Cached map resources
still use the existing owner lookup. Focused fixtures exercise actual texture
creation, lock/readback and final release. G4 evidence and the exact PPC binary
are in `build/mac-audit/font-runtime-cache/`; PPC SHA-256:
`6a93caa21945a8a68584a33fa94496bd06c38484836600d8065869e917d26050`.

## 2026-10-16 — First complete original frame on native Intel

The combined runtime texture and borrowed high-resolution font atlas path
completed `main_game_render` and one present on the MacBook. Captured image:
`build/mac-audit/rendered-session/font-atlas/font-atlas-i386.png` (original PPM
alongside it). Observed present count 1, nonblack pixels 307200, RGB digest
`f455f6b17f08c053`. The image contains sky, first-person weapon, HUD and text,
but has major rendering defects: oversized/incorrect weapon geometry and
missing terrain. This is a full-frame integration milestone, **not a playable
build**. Repeated-frame behavior remains unverified. The G4 color failure is
still unresolved; this milestone is Intel only.

Exact combined binaries/provenance and runtime log are in that same directory.
Intel SHA-256:
`e95aa802add858fefe51af6b722cecb4b584f405128eb9cbe4255b08b58b95db`;
PPC SHA-256 (compiled, not yet run with this batch):
`02e845ea1b0560f49ca9e4ee633d820d4bd9d751cf88e2ef8d650b9445d01843`.

The single-frame Intel run completed both session cycles: **798 checks / zero
failures**, with 34 linked shader programs and 183 cache hits. The subsequent
30-frame run instead stopped when simulation requested `globals\\falling`
(`e8d1075b`, `jpt!`), which had not been published. It did not complete 30 frames.
Log: `font-atlas/runtime-loop-i386.txt` in the same evidence directory.

Diagnosis: the schema generator read vehicles.c's partial falling-damage
layout after the complete native layout, silently treating the first 44 bytes
as unused. The complete declaration now wins, adding the three distance floats
and primary damage-effect reference to conversion/visitation. The session
loader also follows globals damage/effect references through its existing
bounded effect dependency loader. Focused globals tests pass **38 checks /
zero failures on all three Macs**, with identical field digest `4467fdfb` and
165 references; tests compare converted float bits to the raw little-endian
fields, confirm primary damage visitation and reject invalid floats/handles.
Evidence: `build/mac-audit/falling-globals-build.txt` and
`build/mac-audit/falling-globals/{powerbook,ibook}.txt`; PPC SHA-256:
`5af4e920c6ba1df0021adc211de3a3d37cef9a81cbde0c0160a62d11527b3db6`.
Full integration of that fix remains pending the next related renderer batch.

## 2026-10-16 — Depth mapping, meter endian diagnosis and collision effects

The Mac GLSL 1.20 adapter now converts D3D clip Z (0..w) into legacy OpenGL
clip Z (-w..w). The desktop shared generator otherwise assumes modern clip
control, unavailable here. Focused GPU depth reads for 0, 0.5 and 1 pass on
all three Macs; the 67 retail vertex shaders also generate, translate, compile
and link on all three. CPU translator: 31/0; ordinary color draw: 12/0;
relative-index draw: 34/0; depth draw: 14/0. Logs are in
`build/mac-audit/glsl-depth/`. This corrects a depth bug, not a claim that
world/weapon rendering is now correct.

PPC GDB traced the invalid RGB to the transparent meter's background color at
offset 0x94. `CDCCCC3E` is the unswapped representation of little-endian 0.4f.
The previous native layout preserved the meter's semantic fields as reserved
bytes. Its generator now extracts the complete renderer declaration and asserts
all color, alpha and source-index offsets. The native schema prioritizes that
complete layout over the shared pointer validator's partial declaration.
Focused tests compare all 15 color floats, two alpha floats, five source indices
and flags for all four retail meter roots: **97/0 on native Intel and both
G4s**. Logs: `build/mac-audit/native-meter-shaders-{intel,powerbook,ibook}.txt`.

The depth/falling integrated 30-frame Intel run passes the former falling-tag
stop but reaches an unpublished `characters\\cyborg\\cyborg shield depletion`
effect (`e24100cd`). Collision-model resistance/region effects sit outside the
object tag's ordinary reference graph. The object effect loader now walks their
validated portable collision representation and loads all eight resistance
variants plus region effects through the existing bounded effect loader.
The full-session probe checks the player's collision-effect dependencies before
simulation. Full validation of this latest batch is pending; the failed run is
`build/mac-audit/rendered-session/depth-falling/runtime-i386.txt`.

The borrowed-atlas boundary passes **37/0 on native MacBook Intel and both
G4s**, including live atlas readback, placeholder dimensions, saved GL bindings,
pack/unpack settings, borrowed sampler restoration, rejection paths and
owner-only deletion. Logs and exact binaries are in
`build/mac-audit/font-resolver/`; PPC SHA-256:
`946e723fc276dfc9eecbcd2e0fff142731e81b81854484e66f5dcd086de7d3fa`.
The separate Rosetta run is not counted as native hardware evidence.

Six environment-shader specular/reflection values previously hidden in reserved
ranges are now converted, with source-layout witnesses. All three Macs pass
**735/0** across 69 environment/Chicago tags with digest `1a21ffd9`; evidence
is in `build/mac-audit/senv-six-fields-low/`.

The combined shader batch runs past the meter-color assertion on **both G4s**,
then stops at unconverted grenade HUD tag `ui\\hud\\default` (`e7a90635`,
`grhi`). No complete PPC frame yet. Exact binaries and runtime logs are in
`build/mac-audit/rendered-session/complete-shaders/`. The preceding integrated
Intel 30-frame test gets through collision effects but stops on missing voice
sound `sound\\dialog\\chief\\deathviolent` (`e25d00e9`); its log is in
`rendered-session/meter-collision/`. The native unit schema already exposes each dialogue variant as a reference,
but the object effect loader previously skipped the dialogue family. It now
follows each validated dialogue tag into its complete sound references.
That latest dialogue change is pending full runtime verification.


## 2026-10-16 — Grenade HUD and dialogue integration batch

The complete-shaders Intel integration run completed **808 checks / zero
failures**, including two map lifecycle cycles and an original rendered frame.
Its capture still has major weapon/world visual defects. Both G4s passed the
previous meter color stop and reached the grenade HUD dependency instead.

The new grenade HUD conversion uses the original consumer layout, including
nested overlay items at 0x15C; an initially incorrect 0x158 assertion was caught
by the PPC compile and corrected before runtime. The unused warning-sound and
messaging-icon fields remain opaque, matching their unused status in the
consumer. The integrated grenade HUD, dialogue sound closure and opt-in
simulation trace batch now links for PPC and i386 on the MacBook.

Exact binaries and source provenance are preserved in
`build/mac-audit/rendered-session/grenade-dialogue/`:

- PPC SHA-256: `87a27ca6be98828b48dccd81fffa9f4f32343af10e1f8de7ba8b0addcf16bb82`.
- Intel SHA-256: `92655ca4ffa8cb3498463c6855156e57a25e399ecd01f0ebb20dc1e134cc5302`.

Native Intel is testing 30 rendered frames with simulation state tracing;
both G4s are testing their first complete engine frame with this exact PPC
binary. These runs were still loading at this entry; linking is not runtime
validation. No playable-session milestone is claimed.


The focused grenade HUD test now passes **27/0 on native Intel and both G4s**:
2 roots, 26 bitmaps and 6 references. PPC test SHA-256:
`6b3ff86f2ad92a3fc2aaa9b2912e59482ace90f772d130a6204978535544967a`.
G4 logs: `build/mac-audit/native-grenade-hud-runtime-{powerbook,ibook}.txt`.

**First complete original-engine frame on both G4s:** the exact integrated
PPC binary above completed render/present and old-map disposal on the
PowerBook and iBook. Captures are in the batch directory (`powerbook.ppm/png`,
`ibook.ppm`). The PowerBook image visibly contains sky, weapon/arm, HUD and
objects, but weapon projection and world rendering remain substantially
incorrect; this is not playable validation. Both machines then entered their
second lifecycle load.

Intel completed 30 rendered frames / 88 simulation ticks in its first cycle.
The player remained grounded on surface 1382 with full normalized shield/body
vitality through tick 87; tick 88 has no player unit. The final capture is a
postgame carnage report. Investigating match termination is therefore more
appropriate than assuming this is falling damage. Second lifecycle results
are pending at this entry.


Final results for the grenade/dialogue integrated batch: **832/0 on each G4**,
**834/0 on native Intel**, all completing two lifecycle cycles. G4 captures
come from one original frame; Intel rendered 30 frames. GL program caches:
34 entries on the G4 runs; 36 entries on Intel's longer run. The latest audio
snapshot has no callback/PCM evidence and is not counted as audio validation.
The trace confirms no loss of player vitality before match termination.

An opt-in `HALO_MAC_RENDER_TRACE` diagnostic now logs one indexed draw per
vertex-shader ID (bounded at 64), including viewport/depth state, vertex bounds,
declarations and projection constants. It builds for PPC and i386; the native
Intel diagnostic run is pending under `rendered-session/projection-trace/`.
Normal rendering remains unchanged with this environment variable absent.


## 2026-10-16 — Correct stock Slayer selection

The postgame transition was caused by `engine_game_variant.c` constructing a
zero-filled variant with only its engine index populated. Slayer's original
player update ends the match when score reaches the target, and a zero score
already meets that zero target. The helper now calls the original
`build_game_variant_slayer`, retaining its 15-kill target and other authored
preset settings, and verifies the selected target. The focused variant probe
also checks 15. No original gameplay rule was bypassed.

The full rendered probe now additionally requires a hidden multi-frame run to
remain out of postgame with a local player unit. A postgame image alone can no
longer pass the sustained-session check. The batch builds on the MacBook for
both architectures; 30-frame native tests on all three machines are pending.
Exact artifacts: `build/mac-audit/rendered-session/stock-slayer/`.
PPC SHA-256: `774f9602f13e3eeaf9ee779f801e3f14c830c70661d0cdcc2039dd49f25730a4`.
Intel SHA-256: `2682534f0c6da32508bd751ffe1769231405088b015f013e28eaf9f4c137e600`.


The first stock-Slayer integration attempt did **not** include the new helper:
its provenance still named the old baseline `engine_game_variant` object.
The newly added live-game assertion correctly failed the Intel run (431 checks,
2 failures including the enclosing scope check). Both G4 runs of that stale
binary were deliberately terminated after confirming the provenance mistake;
they do not count as successful runtime validation.

The checker now explicitly replaces `engine_game_variant`, records its actual
source/object hashes, and rejects unrecognized incremental unit requests instead
of silently retaining baseline code. The corrected binaries are preserved in
`rendered-session/stock-slayer-fixed/` and are running 30-frame tests on all
three Macs. PPC SHA-256:
`fcde5dd2a774388e973c8479db7e9694a222b45b9484d38c09a987b711154a32`;
Intel SHA-256:
`978142543508a980958b739c9cbfc8311bb6f4b53dd2cc77aa305b5834d32194`.

Renderer research found that native cluster surface lists are empty. The
original legacy visibility path consumes those lists to select world geometry;
this explains missing terrain independently of texture appearance. Bounded
construction from real BSP leaf surface references is in progress. First-person
trace shows reasonable local vertex magnitudes; animated node constants still
need inspection before attributing the oversized weapon to projection/scaling.


The correctly linked Slayer batch completed its first **30-frame Intel loop
while remaining in gameplay**, satisfying the new postgame/local-unit checks.
Its capture (`stock-slayer-fixed/intel.png`) shows a normally sized weapon after
those frames. This weakens the earlier suspicion of a persistent first-person
projection/scaling defect; the first captured pose may have been transient
animation. No speculative projection change has been made. Terrain is still
absent and remains the confirmed visibility integration issue. Intel's second
load and the slower G4 multi-frame runs were still in progress at this entry.


The correctly linked stock Slayer batch now passes **836 checks / zero failures
on native Intel and both G4s**, including 30 rendered frames / 88 simulation
ticks in the first cycle and a second map lifecycle. Both G4 traces retain a
live, healthy, grounded player through tick 88. This verifies the premature
match-end fix on actual PPC hardware, but does not certify complete rendering.

The cluster surface-list batch builds for PPC and Intel. Its native Intel test
completes 832/0 under the earlier one-frame assertions, but still emits only
sky/model/effect shader traces; no terrain milestone is claimed. Artifacts are
`rendered-session/world-surfaces/`; PPC SHA-256:
`af977267c97a4796fe78c72f73dfa560f56cff63a95806cf078518c238f6d233`;
Intel SHA-256:
`6c2999c17eccc63b9ddb94c4a3534061e7fa1004687c57229365a192db9dc1ee`.
The recorded source hash for the cluster-list implementation is
`5be5f0190714c91b5af796ec8820f62ca635ef4314ec00f01c9e91c8bcd6843e`.

The next probe revision logs actual visible-cluster/environment-surface counts
and requires nonzero terrain for the fixed hidden view. BSP compressed streams
also need real rasterizer vertex-buffer lifecycles; that related change is in
progress in a separate native renderer helper, preserving CPU world tooling.


The direct visibility diagnostic confirms the new lists are effective:
**cluster 3, 15 rendered clusters, 2,968 visible environment surfaces** on
native Intel. Thus the lack of terrain is downstream of visibility, not
continued empty cluster lists. Evidence: `rendered-session/world-visibility/`.

A separate `native_world_vertex_buffers` helper now creates/deletes the actual
rasterizer buffers for compressed environment and lightmap streams. Session
startup prepares them only for a live GPU device; CPU-only world tooling keeps
its original ownership. Cleanup releases GPU resources before world data,
including failure paths. The combined buffer/visibility batch links for PPC
and Intel; first-frame native runtime checks are in progress on all three.
Artifacts: `rendered-session/world-buffers/`. PPC SHA-256:
`e8ad02fddb1cfda6460691370ec3ec814eb491e846a43687432e3f6c6afc5055`;
Intel SHA-256:
`137a22e7d42c4f8b64b8dd78ab281e71137511a5509f5eac18b6d48e07055956`.


The combined world-buffer batch passes **833/0 on native Intel and both G4s**,
including two complete map lifecycles, a rendered frame, nonzero visible terrain,
and cleanup. Captures in `world-buffers/{intel,powerbook,ibook}.png` now show
textured terrain and base geometry. The shader trace includes the additional
environment/lightmap passes (18 distinct indexed-draw shader IDs, rather than
11 before the buffer lifecycle). This is the first observed complete BSP draw
on the G4s, not yet a sustained-play claim. The exact same binaries are now
running 30-frame simulation/render loops on all three Macs; loop logs are
`world-buffers/loop-{intel,powerbook,ibook}.txt`.


### Clean play entry and measured rendering cost

The world-buffer binaries above complete the 30-frame/two-cycle run with
**837 checks, 0 failures on native Intel, PowerBook and iBook**. Captures show
textured terrain/base geometry, sky, weapon and HUD. Player state remains
live and grounded. Evidence: `rendered-session/world-buffers/loop-*.txt`.

A separate `--gpu-play` entry now creates the real player with neutral startup
input and skips scripted movement/fire. `--gpu-play-check` exercises that
startup invisibly. The latest exact binaries pass **142/0, one cycle on all
three Macs**. PPC SHA-256:
`411f36aa4e503745e71072c9d42c86816b7b17bda1b62dd759437550e39541ef`;
Intel SHA-256:
`5e02eb24f9352cada700fb3acf69b731fdc8ee944ab7f9dc20151aeb999c4c33`.
Evidence: `rendered-session/play-timing/runtime-*.txt`. Five cold rendered
frames take 8,018 ms on Intel, 59,891 ms on PowerBook, and 67,060 ms on iBook.
These include warmup and are not steady-state FPS, but prevent a playable
performance claim. Retail bitmaps currently upload/delete per draw; a bounded
persistent GPU cache is the next optimization.

The fixed 640x480 visible window now supports explicit quit and foreground
activation; visible interaction has not been exercised. A launcher and
experimental package are prepared under `build/mac-playtest/ppc`, not launched
on the user's desktop. Menus, sustained interactive play, and Tiger parity
remain unverified.

The first bounded retail GPU texture-cache batch now builds for PPC and Intel.
It retains up to 8 MiB/128 entries, keys exact sampler state and map ownership
generation, pins active draw entries, and frees copies before GL teardown.
Font/render-target borrowing remains separate. Runtime comparison and focused
generation/sampler tests are pending. Artifacts:
`rendered-session/texture-cache/`; PPC SHA-256:
`00f93a21cce752a9a545cf4ad5c3ce2d8f83e7d030ec1a011e54e52dc69b8988`;
Intel SHA-256:
`f4d48fac6c090f107b50a41f266aa29428d91fa049bd09e86ccf6a123ab98900`.

The texture-cache five-frame clean-start check passes **142/0 on all three**.
PowerBook and iBook retain the exact prior pixel digest `49114adf54558095`.
Timings are mixed: Intel 5,674 ms (first 3,691), PowerBook 45,029 ms (first
24,877), iBook 84,299 ms (first 47,896). Thus no general speedup is claimed:
the iBook regressed in this sample. PowerBook reports 2,298 hits, 863 misses,
812 evictions, and 7,330,076 retained bytes: the small cache churns heavily.
Intel passes the separate scripted two-cycle lifecycle with **837/0**.
PowerBook's scripted run instead halts before rendering at tag_groups.c:3089,
index -31614 outside [0,32); this new failure is under investigation rather
than being counted as a renderer success. Log: `texture-cache/lifecycle-powerbook.txt`.

The next batch adds a bounded CPU cache of exact converted shader sources,
keeping bytecode, lighting, packed attributes, and full pixel keys distinct.
It retains the existing GL program cache and draw uniforms. Builds succeed:
PPC `bb93d0870efaeb0d495329f82c74b39f5947ae70428cd21ea60f227dc2bfa7a5`,
Intel `0a4f4ec8c06b7d6eae5e39b350b23fd4a1ee7ad41f9ace2f6506fb5b952685c9`.
Runtime comparison is pending in `rendered-session/source-cache/`.

The focused shader-source cache probe passes **83/0 on Intel and both G4s**,
including bytecode/key changes, pinning under eviction, and zero retained bytes
after disposal. PPC probe SHA-256:
`b4165a00c0c7f703c1b0b74279147af04c329e24b398eb2aab4df72f954e9e63`.
Evidence: `build/mac-audit/shader-source-cache/`. The integrated Intel startup
passes142/0 with the same image digest as the texture-only batch; five frames
take5,095ms, first3,585ms. G4 integrated results are pending.

Integrated shader caching now passes **142/0 on all three Macs**, with exact
unchanged per-architecture image digests. Five cold frames: Intel5,095ms
(first3,585), PowerBook41,650ms(first24,540), iBook49,235ms(first29,902).
On G4 the source cache records1,118 vertex hits/21 misses and692 pixel hits/
447 misses, using498,627 bytes. Rendering remains far below interactive speed.
The next batch removes redundant object-sampler queries for private cached
textures and includes opt-in failing tag-block caller diagnostics.

The private-sampler/diagnostic batch builds successfully:
PPC `2406f650c520f4ce093d4431d9023512c885d107aef5ae0ff21adc5daa39bae7`,
Intel `a90ed1b9dfe4d02a4efd1382c069f7d85825cfbd7e44c32dce883a11c16b7522`.
Both G4s reproduce the scripted movement failure before their first render.
The bad block has count32, element size48, index-31614 and caller0x000da9fc
in that exact PPC executable. Logs: `rendered-session/private-sampler/`.
The initial diagnostic printed valid accesses too; it is being narrowed to
failures before subsequent timing comparisons. Clean-start render success
does not establish that interactive movement is safe.

The diagnostic caller resolves to `material_effect_new`, not animation
selection: the invalid index addresses a 32-entry block of 48-byte material
effects. Ground/contact material propagation is now being traced. Intel
completes the same scripted private-sampler batch at **837/0, two cycles**.

Both G4s advertise S3TC. A subsequent unvalidated implementation is being
prepared to upload original retail DXT blocks directly and retain the current
RGBA path on older GPUs or driver refusal. The original little-endian encoded
payload is distinct from PPC's host-swapped CPU sampling buffer. This should
reduce retained GPU bytes (DXT1:8x, DXT3/5:4x for full blocks versus RGBA8),
but GPU decode and performance are not yet verified.

Root cause of the G4 footstep failure is confirmed: original
`material_effect_new_from_point` reads `*(long *)&collision.material_type`,
although that field is a short followed by padding. The narrowed big-endian
result selects padding (`0x8482`) instead of the actual material. The checked
`mac_material_effects_view.py` replaces only this access with the declared
signed field, retaining the original material-effect behavior. Both native
rendered-session and game-tick build graphs now include this view.

Focused persistent-texture lifecycle tests pass **171/0 on Intel and both G4s**,
covering real volume data, distinct sampler variants, reuse, generation change,
and zero cache storage after teardown. The combined DXT upload / footstep
fix build is now in progress; runtime results are pending.

The combined DXT/material batch links for both architectures. Exact hashes:
PPC `0550882c8e905c8c64d108de021b46146f27772715a808bfb79bc3e57b9fd6f0`,
Intel `44c05c3135899eca595aab4160e1ad5a9d882ecb4c00c41b2f0d5d35b3b8c002`.
The corrected tag-bound diagnostic now emits only for invalid indices.
Scripted movement/fire, five rendered frames, and two map lifecycles are
running on all three; evidence directory `rendered-session/dxt-material/`.

The DXT/material batch passes **837/0, two cycles on Intel and PowerBook**.
The prior footstep crash is absent; real movement/look/fire and map reload
complete. PowerBook five rendered frames take43,130ms(first28,926); iBook
first-cycle rendering takes51,519ms(first32,846), with reload still pending.
These scripted-fire views differ from clean-start timing and are not a
controlled speed comparison. PowerBook records523 compressed uploads,
2,625 cache hits/788 misses/696 evictions at8,140,848 retained bytes.
Cache churn remains high; targeted phase timing is the next measurement.
Capture: `rendered-session/dxt-material/powerbook.png` (actual G4 frame).
The encoded DXT CPU fixture passes **341/0 on all three**, with logs and
hashes under `build/mac-audit/native-dxt-cache/`; GPU readback comparison
is still pending and whole-frame pixel equality is not assumed across GPUs.

The iBook also completes **837/0, two cycles**, so the DXT/material batch
passes on all three. Both G4s execute native movement/look/fire before
rendering and repeat the lifecycle without the former footstep index failure.
This is a correctness milestone; frame rate and a sustained visible session
remain unverified. New opt-in render phase accounting is being built to
measure texture binding, shader conversion, program lookup, draw submission,
framebuffer readback, and visibility-result costs.

The source-derived footstep regression passes **12/0 on Intel and both G4s**,
including padding0x8482,0xffff, and0 with material types0,1,31,andNONE.
PPC probe SHA-256:
`6a405ed0b41f53ac227793da77a96086207437f91094b38b87d5ec4490f8010f`.
Logs: `build/mac-audit/material-effects-endian-*.txt` and the C2D build log.
No other matching short-plus-padding long alias was found in the source scan.

The opt-in timing batch links with exact hashes:
PPC `19b22d132520e69428d71c83e63502707c5532e1949138626f003ead81921048`,
Intel `2945575b85062a33a22a12ad9004a2001dbc62c19a7ea7f47a9acf31ad1a3da6`.
Five-frame clean-start phase measurements are running on all three.

The first phase-timing run passes **142/0 on all three**. Clean-start five
frames take4,718ms on Intel,35,116ms on PowerBook,35,447ms on iBook.
PowerBook totals: texture binding2.541s, shader sources1.355s, GL program
lookup4.823s, draw submission/restoration25.094s, framebuffer readback0.231s,
visibility results0.000076s. These include expensive first use; finer draw
phase timing and separate post-first-frame counters are being added.

The real32x32 DXT1 GPU readback passes **124/0 on all three Macs**:512-byte
compressed level, zero RGBA/alpha mismatches and max delta0 against decoded
reference. PPC probe SHA-256:
`ee65e1f36f8f4e55f6587310ceb6ebc3530c90775d65f6161a975d45eb16aa22`.
Evidence: `build/mac-audit/retail-gpu-cache-focus/dxt-*.log`.

The reduced private texture key retains only address/filter/mip/border
parameters that affect the texture object; per-stage LOD bias remains applied
and restored independently, and every bind still validates anisotropy and
other limits. Borrowed font/render-target handling is unchanged. The next
five-frame run also splits draw cost into setup, uniforms, draw calls, and
restoration, and reports first-frame startup separately from later frames.
Evidence directory: `rendered-session/sampler-key/`.

The narrowed-key/detailed-timing batch passes **142/0 on all three**.
Exact hashes: PPC `579fb4476991c25a76c896e8e705cef59b48da6e78369fae2b40e1e8d8f53cc3`,
Intel `743c97ab4b09e2164e37dea9a09a2f3e7197fa5b21a51f947f5d772bc3b4cda5`.
After excluding the first frame, four PowerBook frames spend7.364s in
attribute setup/draw calls,0.289s in state setup,0.102s uploading uniforms,
and0.043s restoring state. Intel's corresponding draw-call bucket is0.109s;
iBook's is8.067s. Texture binding remains1.668s over those PowerBook frames.
Thus redundant state queries are not the principal measured draw cost.
A separate16-frame PowerBook run is being sampled with the native `sample`
tool after first-frame initialization to identify CPU/driver hot stacks.
Its timing is diagnostic and not a controlled benchmark.

The post-startup PowerBook CPU sample is saved in
`build/mac-audit/rendered-session/sampler-key/driver-sample.txt`
(SHA-1 `21569843ea56c1f952847064df1186ca5aeb4e38`). It sampled only the
owned game process, PID 29373, for three seconds at 10 ms intervals after
the first-frame reset marker. The render thread includes
`gleLLVMVPTransform` and `glvmInterpretVPTransformFour`; a driver worker
spends its sampled time in vertex-transform LLVM/PPC code generation.
This establishes CPU-side driver vertex work, not a measurement of GPU time
or the precise cause of fallback. The diagnostic run completed 142/0.
Next: query CGL's per-draw GPU vertex/fragment processing flags and inspect
successful shader compiler logs, then test the implicated draw configuration.
Apple documents those flags in its OpenGL rendering-context guide:
https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_contexts/opengl_contexts.html

The focused narrowed texture-key probe passed 181/0 on Intel and both G4s;
the DXT1 readback probe passed 124/0 on all three. Per-stage LOD biases reuse
one cached texture while remaining distinct and restoring correctly, and
unsupported anisotropy still fails on a cache hit. Probe PPC SHA-256:
`7a8d21b41bbfda85e89634d0be0e646df3b4ba17be98744a8dea0f54ed101c96`.
The newest G4 output was observed in remote tool sessions, not redirected
to files; older 171-check saved logs must not be cited as 181-check evidence.

A reversible compact-vertex experiment is being prepared. The source-level
attribute mask includes all 16 declarations, even when the linked shader
optimizes most away. The program cache now records the actual linked inputs
once per link and exposes a checked mask lookup without per-draw GL queries.
The standalone upload helper packs those inputs from the existing fully
initialized D3D vertex records into a bounded VBO. Intel focused checks pass
41/0 for compact/client-array pixel equality and buffer cleanup, and 291/0
for the linked-program cache including mask lookup and disposal. G4 runs and
full-engine integration remain pending; these results do not establish a
performance improvement.

The PowerBook diagnostic completes 142/0: two frames in 25.603 seconds,
first frame 22.611 seconds. Its CGL snapshots report vertex processing off
GPU for 21 of 39 first program draws, with fragment processing on GPU;
other sampled programs report both stages on GPU. All query error codes
are zero. The diagnostic is a current-state snapshot, not a full-frame
hardware execution certificate. Intel reports both stages on GPU.
Logs: `build/mac-audit/gl-diagnostics/runtime-powerbook.txt`.

A source-token comparison finds a precise correlation in this sample:
vertex shader IDs 9, 30, 31, 32, 36, 39, 47 and 50 each have six relative
constant-address instructions and report vertex_gpu=0. Every sampled ID
reporting vertex_gpu=1 has no relative-address instruction. The comparison
uses word 3 bit 1, as the shared NV2A translator does, and table-order shader
IDs from native creation. Machine-readable evidence:
`build/mac-audit/gl-diagnostics/relative-address-correlation.json`.
Next diagnostic: isolate bounded relative constant lookup forms on both G4s.
Do not remove index bounds to force acceleration. The compact VBO experiment
remains useful as a transport comparison, but this evidence prioritizes the
relative-address shader path over unused-attribute pruning.

The matching iBook diagnostic also passes 142/0, with 21 software-vertex and
18 hardware-vertex first-draw snapshots; all 39 fragment snapshots use GPU
processing and all queries succeed. Two frames take 27.005 seconds, first
frame 24.251 seconds. Its image digest matches PowerBook. Saved log:
`build/mac-audit/gl-diagnostics/runtime-ibook.txt`, SHA-256
`aa5b670197853b2f0468b26120ce2366e89e01675d3020e976d3ef1533ef17ba`.
The focused compact VBO helper now additionally rejects short input spans;
43 checks pass on Intel and PowerBook. The full opt-in integration builds
for PPC and Intel, with runtime comparisons queued. Its default remains the
existing client-array path until a hardware benefit is demonstrated.

The final focused upload batch passes 43/0 on Intel and both G4s. The PPC
binaries fetched back from both G4s match SHA-256
`64594fa7e83fa54cb92dc9440b8724d564f0574a0ab55b0f4fd7fde94ae98bcb`.
Durable logs and binaries are in `build/mac-audit/vertex-upload/`.

Full PowerBook compact-VBO comparison completes 142/0 with five frames.
PPC binary SHA-256:
`2379bb9af22592e9b5655f2fb3e526965cee52d248da1a98dbe2c6afae96be62`.
Frame digest `2b7c6a7cafa0bf42` matches the same five-frame client-array
baseline exactly. Total 35.779 seconds, first frame 22.781 seconds; four
later draw-call buckets total 8.276 seconds versus the earlier 7.364 seconds.
Software-vertex snapshots remain on the same relative-address shader IDs.
This sample demonstrates correctness, not a speedup. Keep the experiment
opt-in and prioritize the bounded relative constant lookup investigation.
No extra full iBook VBO benchmark is needed absent a PowerBook benefit;
both G4s already passed the focused upload correctness milestone.
Intel compact-VBO comparison also completes 142/0. Five-frame digest
`2ae20f599d1dbbd7` matches its own baseline; total 4.623 seconds and first
frame 3.517 seconds. Cross-architecture pixel equality is not claimed.
Comparison caveat: the compact run has GL diagnostics enabled, so the timing
comparison is not a controlled microbenchmark. The saved old PowerBook PPM
in `sampler-key/` was overwritten by the later 16-frame sampling run (its
recomputed digest is `05063f78d869c844`). Use the matching five-frame runtime
log digest for comparison; do not present that old PPM as a five-frame image.

The bounded relative-lookup fixture passes 949/0 on native Intel and both
G4s, including first/warm draw snapshots and exact boundary/rounding colors.
All simple lookup forms remain on GPU. Thus dynamic indexing alone does not
reproduce the full game shader fallback; the six-read production shader and
its surrounding instructions need comparison. Logs:
`build/mac-audit/relative-constant-gpu/runtime-*.txt`.

The GLSL adapter now adds the relative offset after converting the address
to float. This avoids integer overflow before clamping. With offsets 0..255,
any address producing an interior index lies in -255..191, where float
represents the integer exactly; other defined integer results clamp to the
same endpoint. The local conversion regression passes 31/0. Full real-shader
hardware comparison is in progress, with compact VBO mode disabled.

An isolated ARB vertex program plus fragment-only GLSL probe passes 22/0
on Intel and both G4s, including exact bounded-relative-lookup pixels and
positive GPU processing snapshots. It establishes a possible fallback stage
combination, not a production NV2A backend. Virtual limits of 1024 parameters
and 262144 instructions must not be treated as physical hardware limits.
Logs and the probe binary are in `build/mac-audit/arb-vertex/`.


### Real skinned-shader GPU fallback isolated (2026-10-16)

The complete generated shader 39 reproduces the G4 fallback that the simple
relative-address fixtures did not. Keeping the floored ARL result in float
storage, while retaining the generator's rounding expression and bounded
constant reads, moves both first and warm draws onto hardware vertex
processing on both G4s. Integer storage with either old or safer float-offset
addition stays on software vertex processing. All variants produce the same
expected pixel (64,127,0,255); Intel processes every variant on hardware.
Removing 12 unused input/copy pairs offers no additional change to the GPU
status. The expanded focused batch passes 1221/0 on Intel and both G4s.
Evidence: `build/mac-audit/relative-constant-pruned/runtime-*.txt`.
PPC binary SHA-256:
`8da852a5913f75442bd11f3965d7d1257598383257be4a07a6a9f4f34f1eaf5b`.
This isolated test used converter SHA-256
`06fc9b8322b9295b03a487217c547728278f2428003330953932fcb5906bea26`;
the production converter promotion is a subsequent change.

The GLSL 1.20 adapter now promotes only the exact generated declaration,
assignment and bounded-read grammar after accounting for every address-token
use. Unknown uses retain the previous integer storage. Floor, the 0.001 bias,
and index bounds remain intact. Local conversion fixtures pass 38/0,
including conservative refusal to promote unknown address uses. Independent
review found no memory or semantic blocker for validated finite model indices;
NaN/infinite model indices are outside that premise. Full-engine hardware and
image verification of this production change is pending.

The pixel source-cache key now ignores the 18 state words supplied solely as
uniforms; GLSL generation still receives the original state, and draw-time
uniform uploads are unchanged. Focused generated-source equality and cache-hit
checks pass 140/0 on Intel and both G4s. This is batched with the address-register
change for the next full-engine build.

The ARB feasibility probe now passes 23/0 on all three machines and distinguishes
native limits: both G4s report 256 native instructions, 256 native parameters,
32 native temporaries and 40 varying floats. No ARB production backend or CPU
gather fallback was added; the simpler GLSL storage correction is preferred.


### Original Mac Halo as a reference

The user suggested using the shipped PC/Mac version to guide the port. A
read-only inventory on the MacBook and PowerBook identifies installed MacSoft
Halo 2.0.4, universal PPC750/i386, linked to AGL/OpenGL and Carbon. Exported
symbols include Mac Direct3D/effect/buffer wrappers; imports include
`glProgramStringARB`, ARB query APIs and NV register-combiner APIs. Imports alone
do not establish which renderer path executes on Radeon hardware. No useful
exported game/rasterizer entrypoints were found, so the app is a reference,
not a drop-in runtime service for OpenCE.

Binary strings include profile activation/dump/frame timing commands and
`rasterizer_fps`; their availability in an actual retail session remains
unverified. No original app was launched, no user desktop was disturbed, and
no proprietary SDK headers were inspected. A future controlled original-app
Blood Gulch run at matching resolution/settings should establish loading time,
resident memory, frame-time distribution and a visual reference. Compare
measured resource and renderer behavior to OpenCE, then investigate specific
differences; avoid making a second full decompilation the critical path.
Current policy still forbids unattended visible desktop tests.


### Combined address-storage/source-cache engine verification

The combined build passes the five-frame clean-start batch, 142/0, on native
Intel and both G4s. Evidence: `build/mac-audit/rendered-session/float-relative-final/`.
PPC SHA-256 `225f76bbc3553adcad723e5bec168ae4c3cc71ef4b07f42e2d668259b0c5be93`;
Intel SHA-256 `60a01c20f82802edfaba78a230702fc070c99885c42ef7bf874224e6ebc5c5b5`.
G4 five-frame digest remains `2b7c6a7cafa0bf42`; Intel remains
`2ae20f599d1dbbd7`, matching each host's five-frame baseline.

Shader 39 now reports hardware vertex processing in the actual engine on both
G4s. This is a partial fix: IDs 9,30,31,32,36,47,50 still report software vertex
processing. Exactly 20/40 program snapshots are hardware vertex on each G4
(previously 19/40), and all 40 are hardware fragment. Intel reports hardware
vertex/fragment for all 40. Do not confuse program snapshot counts with the
21 distinct vertex shader IDs, or infer failed source promotion from a software
GPU status: other limitations can still trigger fallback.

Pixel source misses drop from 447 to 47; G4 hits rise from 692 to 1092.
PowerBook five frames take 32.132 seconds, first frame 21.720 seconds;
iBook 35.519/23.576 seconds; Intel 4.354/3.504 seconds. Four warm draw buckets
total 6.883 seconds on PowerBook and 7.657 on iBook. These diagnostics show a
modest improvement, not playable performance or a controlled benchmark.
The next targeted shader investigation is IDs 9/47 and their differences from
39. Full movement/fire/unload/reload regression of this build is still pending.


The full retail shader-table regression now passes on Intel and both G4s:
67 generated, 67 accepted and driver compiled/linked, zero rejects. All 23
canonical relative-address shaders verify successful float-register promotion.
Thus the seven remaining software-rendered types are not missed promotions.
Evidence: `build/mac-audit/relative-constant-pruned/nv2a-*.log`.
PPC shader-probe SHA-256:
`94a559c2db9fc04152048255b8cee6871a4b2d1abeab7395c96176d1417a5f16`.
Shader 39 is the shortest of the affected group (21 NV2A instructions);
30/32 have 37/38 and are useful next controls, versus 9/47 at 45/55.
Instruction count alone does not establish the driver's fallback cause, and
native ARB limits must not be treated as equivalent GLSL limits.


The combined build also completes the full scripted movement, firing,
projectiles, rendering and unload/reload regression on all three machines:
837 checks, zero failures, two cycles each. Logs and captures are
`build/mac-audit/rendered-session/float-relative-final/full-*`.
The longer scene exercises 47 program snapshots and 22 distinct vertex IDs;
Intel reports GPU vertex processing for all 47, while each G4 reports 25 GPU
and 22 software snapshots (14 GPU and eight software distinct vertex IDs).
The PowerBook capture was visually inspected: textured base, terrain, sky,
weapon and HUD are present. Dark HUD backing rectangles remain a visual defect.
This is automated correctness evidence, not an interactive playable-session
verification. Full-scene digests differ between machines and are recorded in
the logs; no cross-machine pixel-equivalence claim is made for scripted runs.


Read-only HUD comparison found no demonstrated mismatch in GL blend-factor
mapping, decoded RGBA uploads, viewport scissoring or 640-wide UI placement
against the shared desktop renderer. The dark panel remains unexplained;
trace its actual quad, texture and blend state before changing alpha semantics.
The WebGL-only shader blend workaround is not evidence for a Mac desktop fix.

An isolated 16 MiB texture-cache variant passes Intel's five-frame test with
an identical capture; production remains 8 MiB. Intel warm texture binding
falls from 198.4 to 140.2 ms, and total elapsed from 4.354 to 4.062 seconds.
Its 128-entry limit is reached at about 13.7 MiB, so byte capacity alone is not
the entire constraint. Evidence: `build/mac-audit/rendered-session/cache16/`.
G4 comparison remains pending. In parallel, investigate reusing unpinned
texture storage when only sampler parameters change; simultaneous conflicting
stage samplers must retain distinct objects and failed parameter changes must
not leave stale cache keys. This may avoid extra copies without raising the
minimum-hardware memory budget.


### Bounded bone-row gathering experiment

A diagnostic rewrite of real shaders 9 and 30 replaces their six bounded
relative constant reads with six otherwise unused vertex attributes. On both
G4s the original float-address shader remains software vertex, while the
pre-gathered version reports hardware vertex on first and warm draws. Entire
32x32 images match, with nonzero coverage required per shader; some deliberately
clamped extreme cases have no coverage and are not relied on alone. Intel and
both G4s pass 1367/0. Evidence: `build/mac-audit/gather-ab/runtime-*.txt`.
PPC binary SHA-256:
`8fa125623108efde4fe52ae2e091ddf9c6e5b752c2425ddb4d5455951d5cee18`.
This uses handcrafted transform inputs and a simple fragment shader, not the
full engine's material combinations or gameplay input distribution.

An opt-in integration (`HALO_MAC_BONE_GATHER=1`) is in development, default off.
It caches a recognized gather plan with each shader variant, preserves the
original decoded vertex stream by gathering into an owned copy, and records
its CPU cost separately. Unknown source patterns retain ordinary GLSL. Review
requires exact address dataflow/order validation, checked finite arithmetic,
bounded rows and transactional output; recognition fixtures and full-engine
hardware/image verification are not complete yet. Do not use the focused
experiment as evidence that this integrated path is ready or playable.

The isolated 16 MiB cache comparison also passes 142/0 on both G4s with
unchanged images. PowerBook five frames: 31.677 seconds versus 32.132 at 8 MiB;
iBook: 31.931 versus 35.519. G4 evictions fall from 586 to 414 and compressed
uploads from 452 to 375. Warm texture binding drops from 1755 to 1548 ms on
PowerBook and 1915 to 1597 ms on iBook. Production remains 8 MiB while unpinned
sampler-storage reuse is validated as a lower-memory alternative.


Sampler-storage reuse now passes 220 focused device-state checks on Intel and
both G4s. Exact sampler-key matches take priority; on a miss, an unpinned copy
of the same owned texture may receive validated new sampler parameters. A
failed GL mutation drops the candidate rather than retaining a stale key.
Pinned copies remain immutable, preserving simultaneous incompatible stage
samplers. Tests verify the same GL name across valid parameter changes, no
extra upload/miss, rejection without cache mutation for invalid settings, and
distinct pinned incompatible copies. The default remains 8 MiB. Evidence:
`build/mac-audit/sampler-reuse-20261016/`; PPC binary SHA-256
`bb45fe844c14539510a24542a0503553daee49a52f55d881184ef6317a6c1d4b`.
Full-engine performance and image verification of this change is pending.


The gather source-cache integration now passes 234 checks on Intel and both
G4s. All 67 retail shader entries are exercised; 17 match the conservative
source grammar: 5,9,10,13,14,15,17,30,31,32,36,39,45,47,50,60,62.
This covers all eight software-vertex IDs observed in the longer Blood Gulch
run. Cache-mode changes preserve separate original/gather entries, and cached
plans survive hits. The native row-preparation probe passes 188 checks on
Intel and both G4s; the modern-host AddressSanitizer/UndefinedBehaviorSanitizer
run also passes 188/0. Cases include adjacent float representations around
rounding boundaries, negative floor, large finite saturation, invalid input
and unchanged output when a later vertex fails validation.

The combined renderer compiles for PPC and i386 with the gather helper built
as a native provider without the game's forced XDK headers. Its first full-map
opt-in run is pending. The source recognizer was tightened after initial
source-cache activation failures; only the current passing snapshot should
be used. No default activation or playable-performance claim is made.


### First integrated bone-gather hardware result

With `HALO_MAC_BONE_GATHER=1`, native Intel and both G4s complete the five-frame
clean-start test at 142/0. This includes the 8 MiB sampler-reuse change.
Evidence: `build/mac-audit/rendered-session/gather/`.
PPC SHA-256 `ed5c7c8f47943555c3b377812abdb4cf6d5e0f3ab039816caeb7e2bdb021be8e`;
Intel SHA-256 `7ef394fdb2e2d411738d9069a21eae39809538fe557bb6071efe5de95e0fcd26`.

Both G4s now report hardware vertex processing for 38/40 program snapshots;
only shader IDs 31 and 50 remain software. All fragment snapshots are hardware.
Intel remains 40/40 hardware vertex. PowerBook five rendered frames take
13.327 seconds (8.098 first frame), versus 32.132/21.720 in the prior
float-address/source-cache build. iBook takes 15.278/8.885 seconds; Intel
3.931/3.147. These frame intervals exclude the earlier map-load phase.
Warm PowerBook draw-call time falls from 6.883 to 1.632 seconds over four
frames. Bone preparation adds 336 ms on PowerBook and 297 ms on iBook over
404 warm draw calls; texture binding remains 1.529 seconds on PowerBook.
Warm frames are still much too slow to claim playable performance.

Intel's capture is byte-identical to its prior five-frame baseline. Both G4s
produce digest `a68f29f0bc324e2b` and match each other, but differ from their
prior software-rendered baseline: 3,963 of 307,200 pixels change; mean absolute
channel difference 0.0131, maximum 214. Only 15 pixels have maximum channel
difference >=64. Root inspected the frame and difference image: changes are
sparse around the weapon, with terrain/base/sky/HUD unchanged. GPU arithmetic
and edge rasterization are plausible explanations, not proven root causes.
Do not claim byte-identical G4 rendering. The gather path remains opt-in.

One PowerBook launch failed before map loading because of incorrect paths;
its log is retained separately as `pbg4-wrong-map-path-first-attempt.txt`.
The successful retry used the installed app's maps. Full movement/fire/reload
regression of this exact gather build is pending; no manual playability,
Tiger runtime, minimum-spec performance or audio-output claim follows from
these five-frame captures.

Texture accounting clarification: the 16 MiB experiment's G4 resident value
was 13,247,884 bytes (about 12.63 MiB), not 13.2 MiB. Earlier rounded labels
used decimal MB loosely; use exact byte counters in logs. Cache byte counters
cover encoded/RGBA payload, not driver alignment or allocation overhead.


### Gather full regression and reduced CPU row preparation

The exact first integrated gather build now completes 837 checks with zero
failures on native MacBook Intel, PowerBook G4 and iBook G4. Logs and captures
are in `build/mac-audit/rendered-session/gather/*-full-runtime.txt` and
`*-full-frame.ppm`. These runs render 30 frames in cycle 1 and validate teardown
and reopening in cycle 2; they do not render 30 frames in each cycle.
Observed rendered intervals (total / first frame) are Intel 9.205 / 3.551 s,
PowerBook 35.794 / 8.452 s, iBook 41.781 / 10.077 s. Earlier iBook numbers
15.278 / 8.885 refer to the five-frame run, not this full run. Warm PowerBook
frames average about 0.943 s, still far below playable performance. No manual
input, audible output or Tiger runtime acceptance follows from these probes.

A subsequent CPU preparation change floors each bone address once for its three
rows and copies contiguous matrices together, retaining per-row copies at clamp
boundaries. It preserves separate multiply/add rounding, finite checks, bounds,
and validation of every vertex before any destination is modified. The focused
probe now compares full records over 2,463 additional cases spanning positive
and negative scales, fractional addresses and both clamp boundaries.
MacBook GCC 4 Intel and the same PPC executable on both G4s pass 2,651 checks;
modern-host AddressSanitizer/UndefinedBehaviorSanitizer also pass 2,651 checks.
Evidence: `build/mac-audit/bone-apply-optimized/`.
Module SHA-256 `abae653535ae44fa4fbcd25118171f83bb8a2329e43c9fab58eb7a54ca9a6a1f`;
PPC probe SHA-256 `5e2f7708ff50380bb8fb50e74d4da000ad8bbaa80c8afce1a0207a00f9937088`.
This optimized preparation has not yet been measured in the integrated engine;
no speedup is inferred from the focused checks. The next integrated batch also
includes the pending per-program uniform-location cache, after focused hardware
validation. A separate same-source 16 MiB/256-entry texture experiment keeps
cache-capacity effects separate; production remains 8 MiB and gather remains
opt-in pending broader validation.


### Uniform lookup cache and remaining graphics investigation

The bounded per-linked-program uniform-location cache passes 304 checks on
MacBook Intel and PowerBook G4, with persisted logs in
`build/mac-audit/uniform-cache-20261016/{intel,pbg4}.log`. The testing agent also
observed 304/0 directly on iBook G4; that terminal output was not saved as a
local artifact. Coverage includes absent uniforms, eviction, replacement
programs and disposal. The cache owns locations with the live program entry,
including -1, and the draw upload helpers use it. The redundant second
`glUseProgram(program)` was removed; GL state save/restore remains intact.
Module SHA-256 `6fd6f52df1f096eefcd0c1dd466a74689789864571accc4469804f338b374a70`.
Integrated performance validation with optimized bone preparation is pending.

Focused PowerBook diagnostics still show shader 31/50 software fallback after
bone gathering, even with only the fog scalar output removed. That diagnostic
is not a production change: actual outputs remain intact. More precise
output-chain and expanded-operation comparisons are in progress.

Source review finds exact retail texture-cache hits avoid bitmap decode and
RGBA expansion. However, each bind scope snapshots all four stages' three
texture bindings and LOD bias, including unused stages, in addition to global
active-texture and pack/unpack state. A focused selective-stage preservation
change is being tested separately from cache capacity and bone/uniform work.
The current full PowerBook log attributes 7.979 seconds of 29 warm frames to
texture binding and 8.592 seconds to draw calls. Present readback adds 1.055
seconds; it currently synchronizes, reads RGBA and packs every pixel into the
native CPU back buffer. Readback cannot simply be removed: screenshot/CPU
surface consumers and render-primary aliases require a defined synchronization
policy. No readback behavior was changed in this investigation.


### Larger texture working set and packed readback feasibility

The isolated prior gather source with 16 MiB / 256 texture entries passes
142/0 on all three Macs. Evidence is
`build/mac-audit/rendered-session/gather/cache16x256-*-runtime.txt`.
PowerBook five-frame elapsed/first frame: 10.721/8.095 seconds, leaving
2.626 seconds for four warm frames (about 0.657 s/frame). Its cache retains
132 entries / 13,969,520 payload bytes (13.32 MiB), with zero evictions;
the previous 8 MiB test evicted 560 entries. Both G4 final captures match
their own 8 MiB baseline exactly. Intel final capture differs around the
weapon/reticle; logs show 12 simulation ticks versus 13 in the baseline.
This supports a timing-dependent explanation but does not prove every changed
pixel is explained. Do not claim Intel pixel equivalence. Production capacity
is unchanged while integrated changes are tested separately.

A new hidden-CGL diagnostic compares `GL_BGRA` /
`GL_UNSIGNED_INT_8_8_8_8_REV` readback against the existing RGBA byte conversion.
It checks native numeric ARGB words, varied alpha/color channels, vertical row
reversal and destination pitch padding. GCC 4 builds on the MacBook pass
173/0 natively on Intel and both G4s. Logs/binaries are in
`build/mac-audit/packed-readback/`; sources are
`tools/mac_packed_readback_probe.c` and `tools/mac_packed_readback_checks.sh`.
The readback works without an additional explicit glFinish in this probe.
This establishes format correctness for these drivers, not an integrated
speedup; production Present has not yet changed.


Following the packed-format probe, working-tree `D3DDevice_Present` now reads
native packed ARGB words and reverses rows with memcpy instead of repacking
individual channels. It preserves/restores pack alignment and byte-swap state,
and relies on synchronous ReadPixels rather than a preceding glFinish. This
production edit still needs integrated compilation and image/timing verification.
The bone/uniform baseline uses the preserved pre-edit device source at
`build/mac-audit/rendered-session/bone-uniform-baseline/engine_device_state.c`
(SHA-256 `3133207129e34988a3a78c0520244781c9ac1caac218fedca3060c4faaea2fb2`)
so this readback change can be measured independently. No speedup is claimed yet.


### Per-shader submission timing and selective-state review

Optional render timing now attributes the existing CPU draw-submission interval
to each bounded retail vertex-shader ID, using the same end timestamp as the
aggregate draw bucket. It does not add a GPU wait or claim GPU execution time.
Counters reset with the other first-frame/warm-frame counters. This is intended
to quantify IDs 31/50 before selecting further compiler work. The native timing
module compiles cleanly on the modern host with warnings as errors; the next
MacBook/G4 integrated readback batch must compile and exercise it.

Review of the pending selective texture-state patch found that borrowed-texture
sampler cleanup binds objects on the currently active texture stage. If a caller
switches to an uncaptured stage between bind/end, restoring only captured stages
would leave that inactive stage's binding changed. The texture agent has been
asked to select a captured stage explicitly for borrowed cleanup and add this
regression case before integration. No selective-state performance claim is made.


### Combined-run configuration correction and image-comparison scheduling

The first optimized-bone/uniform binary's five-frame invocations omitted
`HALO_MAC_BONE_GATHER=1`. Their 142/0 results validate the uniform-cache path
with gathering disabled; zero bone-gather timing is a configuration error,
not evidence that no skinned draws occurred. Preserve these runs separately.
Corrected gather-enabled runs are underway with explicit settings recorded.
Do not use the mistaken runs to claim optimized gather integration or speedup.

The hidden bounded interactive probe now accepts exact
`HALO_MAC_RENDER_FIXED_STEP=1` to advance one real engine tick per frame without
wall-clock catch-up or sleeping. Visible play ignores it and retains the normal
scheduler; default hidden behavior is unchanged. This test-only option will
allow same-tick image comparisons when optimizations change frame duration.
It has not yet been compiled/run in the integrated engine. Correctness pairs
must record the mode and assert frames == ticks; existing wall-clock timing
runs must not be silently compared against fixed-step performance numbers.


### Corrected bone/uniform integration verified

Corrected gather-enabled five-frame runs now pass 142/0 on native MacBook
Intel and both G4s. Evidence:
`build/mac-audit/rendered-session/bone-uniform8-gather/`.
Root compared complete PPM bytes against the previous gather captures: each
host is byte-identical to its own baseline. PPC binary SHA-256
`8dc783894acec49274e1847941a2148f2482e270c3401f0f66f1ff1f5f27d49d`;
Intel `7caf3978430103cac235649a41042fd9d64343c9a1b27fbde88b9d7bb6b1c308`.
Warm bone preparation over 404 calls is Intel 18,579 us, PowerBook 232,255 us,
iBook 230,041 us; prior values were 24,629 / 336,266 / 297,251 us.
PowerBook retains the 8 MiB cache: 78 entries, 8,053,124 payload bytes,
560 evictions. Uniform cache reports 680 driver lookups / 18,683 cache hits.
Total/first-frame times are Intel 3.873/3.119 s, PowerBook 13.117/7.985 s,
iBook 14.356/8.237 s. This modest aggregate improvement does not meet playable
performance; the larger-cache benefit was measured separately, not in this run.

The upcoming packed-readback build logs actual renderer settings at GPU startup
when timing is enabled, including bone gathering, compact upload, readback mode,
visibility and dimensions. This makes missing experiment flags evident from
runtime evidence rather than relying on the intended invocation.


### RSQ alternatives rejected; packed batch compiled

The isolated shader 31/50 diagnostic completed 1,995 checks with zero failures
on both G4s. Replacing all three reciprocal-square-root expressions with either
reciprocal sqrt or exp/log forms leaves vertex processing in software, first
and warm draws alike. Valid non-clipped input cases match the original GLSL
images exactly, but these diagnostics are not comprehensive equivalence proofs
and none of the algebra changes were applied to production. Logs:
`build/mac-audit/bone-gather-final/subset-diagnostic/runtime-rsq-*.txt`.
Together with reduced-output tests, this redirects investigation toward native
instruction/register pressure and an isolated direct ARB vertex-program path,
rather than further speculative GLSL spelling changes.

Packed readback, per-shader submission timing and the hidden fixed-step option
now compile/link on MacBook GCC 4 for PPC and Intel. Final binary hashes:
PPC `42fc88971e96c4e02c091607a3f65cdea91537fffdf8fd3bccafd9972e580348`;
Intel `ec713889039f10fdb8401d431ed4644e490353467dd9fbdc673e9b3de7f93465`.
Wall-clock five-frame hardware runs are in progress with gathering explicitly
on, compact VBO/fixed-step unset, and the preserved 8 MiB texture baseline.
An unrelated Objective-C build was observed on the Intel build Mac; leave it alone and treat
Intel timing as potentially contended. Hardware correctness results are pending.


### Packed readback verified; measured priority shifts toward shader 9

All three packed-readback runs pass 142/0 with images byte-identical to each
host's previous gather-enabled capture. Evidence and exact provenance:
`build/mac-audit/rendered-session/packed-readback-gather/`.
Readback over four warm frames changes from 160.4 to 116.8 ms on PowerBook,
304.6 to 245.4 ms on iBook and 45.8 to 41.3 ms on Intel (Intel may be contended).
PowerBook CPU draw submission totals 1,813,699 us; shader 31 contributes
115,366 us and shader 50 6,062 us, together under 7%. Shader 9 instead accounts
for 719,662 us / 296 draws. These are CPU submission intervals including any
driver stalls, not independently measured GPU execution time. ARB31/50 remains
an isolated investigation, but eliminating those fallbacks alone cannot explain
away the whole measured bottleneck.

Source inspection identifies shader 9 as a model reflection permutation.
Mac's draw path passed lighting metadata to the vertex generator whenever the
shader supported it, even with `pixel_key.per_pixel_lighting == 0`. This emits
extra world-space normal/position outputs and calculations not consumed by the
stock fragment path. The shared renderer requests those only for enabled
per-pixel lighting. Working-tree device source now follows that condition,
retaining stock vertex lighting and selecting extra outputs only when the
fragment key requests them. Device SHA-256
`0501c946de1c2862201c23a687dedfb38db18c550b1818d32bb8c76f5ad5d63d`.
This change is pending integrated compilation, image comparison and timing;
unused outputs may already be optimized away, so no performance benefit is
assumed. The preserved packed baseline remains available for comparison.


### Selective texture state and checked program-source hints

Selective-stage texture preservation passes 69 checks on native Intel and both
G4s, including empty sampler sets, partially failed binds, and an intervening
switch to an inactive texture unit before borrowed-object cleanup. The bounded
large-cache option is now implemented as exact
`HALO_MAC_RETAIL_TEXTURE_CACHE_16M=1`: unset retains 8 MiB/128 entries; exactly
1 selects 16 MiB/256. Other values (including empty or 0) are rejected with an
explicit configuration error. Configuration is retained until cache disposal;
no live scope changes capacity. The borrowed/config probe passes default and
large modes at 76/0 on both G4s, with malformed/empty modes at 2/0. The broader retail upload/eviction/pin suite subsequently passed 220/0
in both default and opt-in modes on native Intel, PowerBook and iBook.
Logs are in `build/mac-audit/device-state-budget-{default,16m}-{pbg4,ibook}.log`
and `build/mac-audit/device-state-budget-16m-intel.log`.

Warm PowerBook program lookup costs 230,641 us over 914 draws in the packed
baseline. Inspection shows each lookup hashes the complete source strings.
A new fast path uses integer source addresses only as candidate hints and still
compares both complete strings against cache-owned copies before returning a
program; it never trusts pointer identity as source identity. Normal content
hash lookup remains the fallback and refreshes hints. This avoids repeated
hashing for pinned source-cache results without stale-code reuse from mutable
buffers or allocator address reuse. The focused cache probe now includes
same-address fragment mutation/restoration and equal text at another address;
it passes 310/0 on both native G4s. Logs:
`build/mac-audit/program-source-hint/{powerbook,ibook}.log`.
Native Intel also passes 310/0 in
`build/mac-audit/program-source-hint-intel.log`. Integrated timing remains
pending; no speedup is claimed yet.


### Integrated selective state and 16 MiB cache comparison

MacBook-built integrated binaries include packed readback, optimized gathering,
uniform/source-hint caches, the shader-lighting selection correction and selective
texture-stage state. PPC SHA-256
`319d1fe4508c3e2428f24fc3dfbd9e3fd6467d235b1a475d41154f5fd865cb4c`;
Intel `ac424010ab91c8f206d22768199b9f834e946defbf48e5df0fffde8b5a1e4716`.
Both cache modes pass 142/0 on native Intel, PowerBook and iBook. Evidence:
`build/mac-audit/rendered-session/selective-cache16/`. Gathering is enabled,
compact VBO is off, fixed-step is off. Actual runtime configuration and tick
counts confirm these modes; the original runner only printed two unset labels,
so the reusable runner was corrected to unset those variables explicitly.

PowerBook five-frame elapsed/first-frame time is 13.055/8.016 seconds at 8 MiB
and 11.890/9.393 at 16 MiB. Thus four warm frames average 1.260 versus
0.624 seconds each. Warm texture binding falls from 1.501 to 0.166 seconds;
draw submission/restoration falls from 2.151 to 0.798 seconds. Cache evictions
fall from 560 to zero; retained texture payload is 13,969,520 bytes (13.32 MiB),
132 entries out of 256. Both G4 captures match exactly across cache modes
(SHA-256 `6dad5500100e93f4295e9a0780e9ed20d50776db29a916d05153aa08b8a1a5a9`).
Intel's runs have different tick counts (13 versus 10), so their captures
require a fixed-step comparison before attributing differences to rendering.

In PowerBook 16 MiB warm frames, CPU draw submission is 661 ms total: shader
56 contributes 189 ms, shader 9 contributes 161 ms and shader 31 contributes
125 ms. Earlier percentages from the thrashing cache no longer describe this
configuration. Exclusive instrumented renderer buckets total approximately
1.495 seconds against 2.497 seconds of warm wall time; remaining CPU work
needs profiling. These measurements still do not establish interactive
playability. Fixed-step comparisons and longer lifecycle regression follow.

The next build batch adds a `stream_decode` timing bucket around native indexed
stream validation, allocation and expansion, ending before GPU draw submission.
It isolates previously unmeasured CPU work without changing decoded vertices
or inserting GPU waits; timing-disabled runs perform no clock reads. The timing
module passes local GNU89 warning-as-error compilation. Device integration and
PPC measurements are pending the next normal build batch, not a separate build
for instrumentation alone. The current paired binaries remain unchanged.


### Original Mac reference and remaining shader audit

Read-only inventories confirm the installed original Mac applications are Halo
2.0.4 universal PPC750/i386 executables with matching binaries and map files.
The executable imports ARB program APIs and NV combiner APIs, supporting a
legacy rendering reference investigation but not proving the selected runtime
path. No original app was launched and no preferences changed. See
`docs/ppc-mac-original-halo-baseline.md`.

The isolated ARB31/50 comparison exposed legacy secondary-color alpha/fog
transport differences; its revised bridge carries full vec4 outputs through
texture-coordinate varyings. Review additionally found a reversed sign select
in its RCC lowering: the positive reciprocal branch selected the negative
clamp. Identity-fixture output parity did not expose this because captured clip
coordinates bypassed the affected screen conversion. Correction and focused
signed RCC/nonidentity cases are required before wider claims. The prototype
is not part of the production renderer.

Integrated logs report all sixteen attributes active even on the four-vertex
shader56 draws. The shared source generator emits all sixteen local input
copies whether referenced by shader instructions or not. A conservative
Mac-only unused-input pruning experiment is being prepared; it must preserve
all genuine references (including gathered bone rows) and verify linked masks
and images on hardware. No performance improvement is assumed yet.


The integrated cache comparison now also passes fixed-step verification on
all three Macs: both cache modes complete 142/0 with exactly five frames and
five engine ticks. Per-host captures are byte-identical across the two cache
limits. Intel SHA-256 is
`592128b669d0dc840669577a63c7cacf6fa6bfc6b5dbacb8c17006a7b42c2016`;
both G4s are
`e5a5c2595f2f219e8397f0386f70e3f81569d66e9d12ef3bf31784800d09d630`.
Evidence: `build/mac-audit/rendered-session/selective-cache16/fixedstep/`.
This resolves the uncontrolled Intel animation-tick comparison for cache
capacity, but does not establish cross-architecture image equality. The frozen
combined build proceeds to its longer lifecycle regression.


Shader56 call-site audit corrects a misleading translation-table reading: the
row is a table offset, not a vertex-type index. Shader56 selects
`_rasterizer_vertex_type_dynamic_unlit` (type6). Direct `0x38` callers include
internal widget occlusion tests (`rasterizer_xbox_widgets.c`) and lens-flare
occlusion (`rasterizer_xbox_lights.c`), consistent with the observed four-vertex
draws. Its expensive CPU submission intervals may include synchronization on
previously queued GPU work; they do not establish expensive shader arithmetic
and do not justify removing visibility tests.

The conservative unused-input adapter and boundary fixtures now pass 49/0
on the modern host with AddressSanitizer and UndefinedBehaviorSanitizer.
GNU89 warnings are treated as errors, except existing modern-SDK deprecation
warnings for the older adapter's sprintf calls. The positive fixture originally
used spaces while the recognized generator grammar uses tabs; this mismatch
was corrected, while a separate changed-grammar fixture verifies retention.
Native GCC4 compilation, GPU linked-location and integrated-image verification
remain pending.


The ARB diagnostic agent reported exact final varying-image comparisons on
both G4s but failed its standalone RCC point readback. It also acknowledged
starting those GPU runs without the coordinator's explicit release; the runs
may overlap the integrated regression. Preserve their logs as provisional,
exclude their timings from performance conclusions, fix/verify point coverage,
and rerun in an explicitly granted GPU slot. The coordinator is checking
overlap against integrated run times. A process-name search is not a substitute
for a release because executables use varied names.


### Combined 16 MiB build completes longer lifecycle regression

The frozen selective-cache16 binaries complete 837/0 on native Intel, PowerBook
and iBook, including 30 rendered frames in cycle1 and a second map open and
teardown (not 30 rendered frames in each cycle). Root inspected the final
results in `build/mac-audit/rendered-session/full-cache16/`. Both G4s finish
with 145/256 cache entries, 14,188,096 bytes retained, zero evictions and 102
compressed uploads. This establishes the longer runtime/lifetime check for
the combined cache/readback/gather changes, not manual playability.

PowerBook logs 23.952 seconds including an 8.437-second first frame; iBook
28.330/10.000 seconds; Intel 5.065/3.222. These timing observations are
provisional until the coordinator resolves possible overlap with the isolated
ARB probe. Preserve the logs without using these figures as a clean benchmark.
Next GPU allocation is explicit: linked-input focused checks, final ARB
checks, then a separate sampled PowerBook renderer run. The source-tree
stream-decode timing and unused-input pruning are not yet in these binaries.


Pending next batch: client-memory attribute submission now uses the retained
compatibility-source `input_mask`, enabling and supplying only those arrays
and explicitly disabling absent ones. The compact-VBO branch is unchanged.
This makes unused-input pruning effective at the submission boundary too;
all source-retained inputs, including gather rows, keep their original index
and full-record stride. Native compilation and image/runtime checks remain
pending alongside the adapter pruning and stream-decode timing.

The final isolated ARB finite matrix comparisons match on both G4s, but the
signed-zero RCC control shows positive saturation for -0 on both. Keep this
case recorded and compare against the actual GLSL implementation on the same
host before interpreting it as a translator discrepancy or baseline-driver
behavior. A negative near-zero case can supplement, not replace, that edge
case. No ARB code has been enabled in the engine.


### Scoped ARB shader feasibility verified on native hosts

Final isolated probe passes 773/0 on native Intel, PowerBook and iBook. Root
inspected `build/mac-audit/arb-retail-probe/runtime-{i386,powerbook,ibook}.txt`.
Shaders 31/50 match the GLSL reference at framebuffer precision for the baseline
and one nonidentity finite matrix case, comparing all eight vec4 outputs by
component plus fog-x. Both report ARB GPU vertex processing on the G4s.
Signed-zero control is retained: -0 maps to positive saturation in both GLSL
and ARB on both G4s, and to negative saturation in both paths on Intel.
Thus this edge result is host-matched rather than evidence of an ARB-only bug.

G4 native resources: shader 31 uses 122 instructions/43 parameters/14 temporaries;
shader 50 uses 104/42/14, both within native limits. PPC probe SHA-256
`56516bbc423a9f62d57c8c3310fff4d6dafd5c64d00c4f6e4cc972ecc41c59ac`;
probe source `64f77b62f1e3dbc8fbbe85ebcee6f78f5ee33dc3f3baef3f700ac75fd0170497`.
This remains a diagnostic only. Production still needs strict supported-program
recognition, dynamic viewport parameters, zero/NaN-w semantics and engine
program/fragment lifecycle integration. No gameplay or general shader parity
claim follows from the finite fixtures.


### Unused-input batch reaches integrated native builds

Focused native validation passes on Intel and both G4s: all 67 retail programs
compile/link at 864/0, retaining 286 input locations and removing 786 unused slots
across the table. The explicit v0/v10 draw passes 15/0 with unused v1 absent
and expected RGBA 64,127,191,255; converter fixture passes 49/0. Intel source-cache
checks pass 252/0, including each of 17 gathered shaders retaining all six bone
inputs. Evidence: `build/mac-audit/glsl-input-pruning/`. This is input/link
validation, not a measured renderer speedup.

MacBook incremental builds of pruning, client-array mask submission and stream
decode timing link successfully. PPC SHA-256
`d430e3d41bb354e4bdc9926fa79280588f2602e67d5cb68c72f9df34e72ce5a8`;
i386 `453301123df6ba4962a8d6b5a863dfe7f157cba538c15936fef5caa71da502d9`.
Source clone/output: `engine-pruned-inputs` and `volume-render-pruned-inputs`
under MacBook `$PROBES/20261016/rendered-session/`. The build
command accidentally omitted BUILD_ONLY and started a default-config hidden
Intel run; its exact owned process was terminated at session_open, partial
log preserved, neither runtime pass nor renderer failure claimed. Intended
fixed-step/gather/16MiB runs now follow with explicit configuration.


### Pruned-input integrated images and PowerBook CPU sample

Root inspected the three fixed-step five-frame runtime logs: native Intel,
PowerBook and iBook each pass 142 checks with zero failures. Captures match
the prior fixed-step baseline on each host: Intel SHA-256
`592128b669d0dc840669577a63c7cacf6fa6bfc6b5dbacb8c17006a7b42c2016`;
both G4s `e5a5c2595f2f219e8397f0386f70e3f81569d66e9d12ef3bf31784800d09d630`.
Artifacts are in `build/mac-audit/rendered-session/pruned-inputs/`.

PowerBook elapsed/first-frame time is 10.246/7.881 seconds, leaving 2.365
seconds for four warm frames. The prior matched run was 2.320 seconds; this
does not demonstrate a pruning speedup. New stream-decode instrumentation
accounts for 463,630 microseconds across 788 calls in those four frames
(Intel 55,964; iBook 473,228). The current decoder expands every index,
including repeats, into a full 16-vec4 record before drawing. Reusing decoded
records within one draw is a candidate; it must retain stream bounds checks,
validation of first occurrences, default attributes and exact output order.

The coordinator sampled the owned PowerBook process for three seconds after
the first-frame counter reset during the subsequent 30-frame run. Root read
`pbg4-cpu-sample.txt`: 268 total samples, 194 in the interactive frame and
173 in main_game_render. This confirms rendering remains the main sampled
CPU workload; the short sample is not a GPU-time measurement. The sampled
run must not be used as an undisturbed timing comparison. Full lifecycle
results are pending collection. No interactive playability claim follows.

The full pruned-input runs subsequently completed: root inspected all three
`full/*/runtime.txt` logs, each reporting 837 checks, zero failures, two
lifecycle cycles and 30 frames/30 ticks in the rendered first cycle. The
second cycle reloads and disposes the world. All three retain zero texture
cache evictions. This proves the bounded lifecycle regression, not responsive
interactive gameplay. The coordinator released the GPU slots for the next
source-identity cache checks after all three processes terminated.


### Immutable shader identity and repeated-index decode batches

Focused GL cache identity tests pass 318 checks on native Intel and both G4s.
The source cache issues non-reused epoch/entry tokens only for pinned immutable
source pairs; generic or incomplete identities retain content comparison. The
engine now passes the pinned token through the draw and reports identity hits.
The separate MacBook identity clone has built successfully; integrated
five-frame comparisons are in progress. Evidence is in
`build/mac-audit/identity-cache/`; source-cache CPU coverage is being separately
confirmed rather than inferred from the GL tests.

The next local batch introduces exact within-draw index reuse. A bounded
unsigned-short slot table is enabled only when the index count exceeds its
min/max span; allocation failure and sparse draws retain ordinary decoding.
First occurrences pass the original full declaration/finite-value conversion,
then duplicates copy the full decoded 16-vec4 record in original draw order.
All index/stream bounds checks remain before decoding, and bone gathering
remains downstream. Table contents are reset each draw and freed at device
teardown. Counters report decoded and reused occurrences.

The standalone helper passes 32 checks locally (also ASan/UBSan as reported
by its author); root reran the normal probe. Eight engine-device build scripts
include the new native helper and all nine affected checkers pass shell syntax
validation. Native helper tests, actual mixed-format device-probe comparisons
and integrated images/timing are pending. No speedup is claimed yet. The
identity snapshot remains frozen and separate from these newer local edits.

Source-cache coverage correction is now resolved with observed native PPC
runs: PowerBook and iBook each pass 325/0, with `bytes=0` after disposal.
Root read `identity-cache/source-cache-confirm/{powerbook-ppc,ibook-ppc}.log`.
The matching PPC probe SHA-256 is
`0ea9a6216a40b4abcc2648c7a25ff9515d4b615f78fe9b74cec5557305102ddb`.
Independent review found the engine keeps the shader result pinned through
lookup and draw and releases it at common cleanup. A second review found no
ordering, default-attribute, base-index or gather-lifetime issue in the new
index reuse integration. Its Mac cross-builds are deliberately held until
the separate identity timing runs finish, avoiding benchmark contention.


### Shader identity integrated images verified

The frozen shader-identity batch passes 142/0 on Intel, PowerBook and iBook,
five frames/five ticks. Captures match the pruned-input baseline byte for byte
on each host. Each G4 records 1,022 identity hits and zero program evictions;
Intel records 1,002. Root inspected the runtime logs in
`build/mac-audit/rendered-session/shader-identity/`.

PowerBook program-lookup time over 905 warm draw calls falls from 182,492 to
142,803 microseconds in the paired observations. Warm total is 2.253 seconds
for four frames versus 2.365 before. Intel and iBook warm totals do not improve
in these short observations, so this is not a general frame-rate claim.
Remaining program misses can include shader compilation in the measured
phase. Images and hit counters establish that the integrated fast path is
used without changing this captured sequence. Decode-reuse native tests follow.


### Decoder equivalence fixture and native build

Mode 6 in the actual device probe now compares all 64 decoded floats per
vertex between three-index draws and reordered/repeated six-index draws. It
uses base vertex 3, packed normals, normalized shorts and byte attributes,
then changes a source position and repeats the comparison on the next draw.
Root corrected the initial fixture's assumption that immediate_count remains
nonzero after submission; the renderer intentionally resets it to zero.
Draw-count expectations include the three extra submissions.

The fixture reports 355/0 on Intel and both G4s in
`build/mac-audit/mode6-index-equivalence/`. Its linkage to the new optimized
decoder is being checked separately: the assertions can also pass against the
old decoder, so these results alone do not prove cache reuse. The separate
integrated decoder clone has linked PPC and i386 successfully, and its
explicit decoded/reused counters and image comparisons will establish which
path actually ran. Root reran helper ASan/UBSan at 32/0.

The linkage audit caught that the first 355/0 fixture runs used an older
engine-device source (`313320...`), not the integrated reuse source
(`8e115c2e...`). They are baseline fixture evidence only. The agent is rebuilding
against the current decoder and helper, preserving full logs and provenance.
The separate integrated build already used the current source.

Standalone helper native verification is complete: 32/0 on Intel and both
G4s. Root inspected both archived PPC logs under
`build/mac-audit/index-decode-cache-native/`. PPC binary SHA-256
`215c1943758716755c688563b4f1523152c73518ae07f3adaced9acdb79f1b4e`.
This establishes helper behavior on PPC, not yet full renderer correctness
or an observed performance gain.


### Pending driver-side indexed reuse experiment

The local device renderer now has an off-by-default
`HALO_MAC_INDEXED_VERTEX_REUSE=1` experiment. It retains the expanded decoded
records and existing bone gathering, but records first-occurrence WORD slots
for duplicate indices and submits them through glDrawElements. Primitive
order remains unchanged. Quad lists apply that mapping to the existing
two-triangle diagonal. Client index submission binds element buffer zero and
restores the prior binding; allocation failure falls back to the existing
expanded submission. The remap is bounded by the existing 65,535-vertex limit
and is discarded as active state after each draw, with storage freed at
device teardown. It reports an explicit enabled flag and remapped draw count.

This is not compiled or enabled in the current decode-only hardware runs.
Their frozen engine source is `8e115c2e...`, also saved locally with matching
cache dependencies in `index-decode-cache-native/frozen-integration/`.
The next experiment targets driver vertex reuse only; it does not reduce
expanded upload bytes. Focused primitive/image/state-restoration checks and
integrated capture/timing verification are required before enabling it.

### MacBook workspace relocation requested

The MacBook's workspace was moved to a second, larger volume, and the move
was limited to OpenCE. Read-only inventory found about
9.5 GiB in `$PROBES`; the system disk had about 21 GiB
free and the second volume about 91 GiB free. It is writable, journaled HFS+
with ownership enabled on SATA. Other project snapshots and user files are
outside this move and remain untouched.

All agents paused MacBook writes/builds; the completed index-decode runtimes
are already archived locally. The entire OpenCE probe workspace is being
copied with symlinks and hard links preserved to
the new volume. Source deletion/cutover will occur
only after verification, retaining one compatibility symlink at the original
home path so historical absolute build paths resolve to the new volume. Migration is
not complete at this entry.

While the MacBook workspace copy was running, root inspected the completed
index-decode integrated results: 142/0 on all three hosts, five frames/five
ticks, with exact prior per-host capture hashes. Each host reports 136,610
decoded and 139,295 reused occurrences. The frozen current-source integration
therefore exercised reuse independently of the stale focused fixture.
PowerBook warm decode time is 381,355 microseconds over 788 calls versus
478,804 in the preceding identity batch; warm total is 2.170 seconds over four
frames versus 2.253. These are short observations, not interactive acceptance.
Artifacts: `build/mac-audit/rendered-session/index-decode/`.

The separate indexed-submit prototype passes 122/0 on the local ARM Mac's
hidden CGL context, with exact full-frame RGBA parity for points, lines,
line strip, triangles, strip, fan and quad triangulation. Each indexed draw
restores a prebound nonzero element buffer. This is modern local-driver
API evidence only; native Intel/PPC legacy-driver runs remain pending the
MacBook storage migration. No MacBook/G4 activity occurred for this local
probe. Evidence: `build/mac-audit/indexed-submit-local/runtime-local.txt`.

After storage migration, consider a matched fixed-step compact-VBO toggle on
the already built index-decode binary before another large renderer rewrite.
The earlier VBO comparison preceded the current gathered/pruned-input batch;
its result cannot establish performance for the current predominantly GPU
shader path. Keep the current no-VBO capture as the reference and require
exact per-host images, actual configuration logs and exclusive timing slots.

### MacBook relocation complete

The migration is complete. `rsync -aEHnci --delete` exited 0; its
318,526 itemized entries were all unchanged (309,665 files, 8,215 directories,
646 links). The old home directory was renamed temporarily, replaced by the
compatibility symlink, and checked with a fresh native i386 helper build on
the new volume (32/0). Only then was the checksum-verified old directory removed.
A second helper run through the compatibility path after removal passed 32/0.

Physical workspace: `$PROBES` on the new volume.
Compatibility path: the same folder name in the MacBook's home folder.
The shared `20261015/engine/build` and current source-clone links resolve.
About 9.5 GiB of OpenCE disk usage moved off the system volume; final observed
system free space is approximately 45 GiB and the new volume's free space 66 GiB. Other
activity also changed disk usage during the move, so the total free-space
increase must not all be attributed to this cleanup. Other project trees,
game installations and user files were left alone as requested.

Use the new volume for all future remote OpenCE builds and snapshots; preserve the
compatibility symlink. Evidence: `build/mac-audit/macbook-storage/`, including
verification summary, full itemization, build smoke log and before/after disk
readings. The storage hold can now be lifted. Pending port work remains the
current-source focused decode fixture, then indexed-submit/VBO comparisons.


After migration, work resumed with remote mount/path verification through
The Intel build Mac was reached over SSH. The corrected frozen decoder fixture is rebuilding against source
8e115c2e; its older focused checkout required the matching timing header/source
and generated semantics header. These are staged from the validated snapshot,
not inferred from successful compilation against older dependencies.

The next local device fixture also binds a nonzero caller-owned element buffer
around repeated draws, checks that it is restored and checks GL errors. This
local fixture is for the opt-in indexed-submit batch, separate from the frozen
355-check decode-only fixture. Native tests for this newer fixture are pending.

### Corrected decode fixture and legacy indexed-submit parity

Retrieved and inspected the complete corrected frozen mode-6 logs: native
Intel, PowerBook and iBook each pass 355 checks with zero failures, and each
records `decoded=25 reused=8`. This supersedes the older focused result that
did not exercise the current decoder. Device source is `8e115c2e`, probe
`c8bb8a58`; the transferred PPC binary MD5 is
`0537748bb906bb610b2a2e1cc35f9547` on both G4s. Full provenance and logs are
in `build/mac-audit/index-decode-cache-native/mode6-corrected/`.

The separate indexed driver-submission prototype also passes 122/0 on native
Intel and both G4s, with exact full-frame parity across seven primitive cases
and restoration of a nonzero caller-owned element buffer. Logs and binaries
are archived in `build/mac-audit/indexed-submit-native/`. This validates the
legacy driver operation, not yet the engine's opt-in remap integration.
The new engine fixture adds binding/error checks and a repeated-index quad;
its default and enabled runs are pending. The coordinator is scheduling a
matched compact-VBO comparison on the existing integrated binary before
further builds to avoid contaminating timings.

The manual-launch script now defaults to the tested bone-gather/16 MiB texture
cache combination, while preserving explicit environment overrides. It also
clears the bounded-probe fixed-step setting along with existing frame limits.
Visible play already enforces wall-clock scheduling in the native loop; the
launcher cleanup makes its intended configuration explicit. Shell syntax
passes. No visible session was launched, and this is not playable acceptance.

The next remap fixture was strengthened before its build: review found that
its vertex/binding assertions alone would miss wrong rendered pixels. Mode 6
now clears color/depth before each draw, disables blending, and compares the
entire 16x12 RGBA framebuffer for the repeated-index triangles and indexed
quad against a non-remapped triangle reference. It repeats the comparison
after source mutation and requires nonzero coverage in both references.
The quad's second triangle is deliberately degenerate, so its expected image
is the reference triangle. This adds four assertions; hardware results for
this updated fixture remain pending. The separate 122-check primitive test
continues to cover nondegenerate quad triangulation.

### Matched compact-upload comparison: G4 results

Using the same frozen index-decode executable, both G4s pass 142/0 in each
five-frame fixed-step configuration. On/off configuration lines were checked;
decoded/reused counts remain unchanged and each capture exactly matches its
prior host reference. These runs use gather=1 and cache16=1, with no concurrent
builds. Root retrieved the full logs and captures to
`build/mac-audit/rendered-session/index-decode-vbo/{pbg4,ibook}/{off,on}/`.

PowerBook: off 10,008 ms total / 7,839 first frame; on 10,216 / 7,919.
Warm four-frame totals are therefore 2,169 ms off versus 2,297 ms on.
iBook: off 12,006 / 9,095; on 10,975 / 7,901. Warm totals are 2,911 ms off
versus 3,074 ms on. The lower iBook total with VBO enabled comes from its
first frame and does not indicate a warm-frame improvement. On this short
matched sample compact uploads are roughly 6% slower warm on both G4s;
retain the ordinary upload default. Intel enabled comparison is still running
at this entry. Proceed with indexed driver submission as the next candidate.

Intel enabled run is now complete too: 142/0, same capture, 5,105 ms total /
4,516 first frame (147.25 ms per warm frame versus 95.5 off). Root checked all
six full logs, explicit configuration values and SHA-256 capture equality;
`build/mac-audit/rendered-session/index-decode-vbo/comparison.txt` summarizes
the evidence. The compact-upload option remains off. The coordinator released
all hosts to the updated focused indexed-remap test batch.

The first attempted focused remap build used stale symlinked files and is not
validation of the candidate. The focused checkout's three source links were
replaced with regular copies without changing their old targets. The corrected
build-only run verifies engine `b9e0e9c2`, probe `56047ab7`, checker `aee9b5c4`
before compiling; it completed on the MacBook. Native default/enabled tests
are now pending from those binaries. Preserve the previous 355-check evidence
as a separate frozen baseline.

Read-only review of the residual program lookup cost found that exact-content
hits overwrite a single immutable-source identity per GL entry. Distinct source
entries containing equivalent GLSL may therefore alternate between identities
and repeatedly take the hashing path. The 68 non-identity cache hits in the
five-frame result justify a bounded alias experiment, but do not establish
how much of the warm lookup time comes from hashing. An isolated scratch
candidate is being prepared without modifying the current remap batch.

### Indexed remap focused native validation

The corrected focused binary passes 373 checks with zero failures on Intel,
PowerBook and iBook, both with the opt-in unset and enabled. Root retrieved
all six logs and verified the expected counters: default `enabled=0 draws=0`,
opt-in `enabled=1 draws=4`, and `decoded=28 reused=9` in every run. These
include the new full-frame pixel comparisons, source mutation and element
buffer restoration checks. Evidence: `build/mac-audit/index-remap-native/`.
Both G4 transfers matched PPC MD5 `9f656574a065c1704d29fdaca9509ca1`.

This establishes the focused implementation's behavior, not its full-scene
performance or playability. The next slot is the incremental integrated
candidate and matched default/enabled five-frame engine runs on all hosts;
the existing index-decode baseline stays immutable. The test agent had already
started an additional log-preservation build when asked to stop redundant
work; it will finish once, check binary identity, and release the CPU slot.

Follow-up decode audit: do not assume every stream is immutable. Static BSP
CPU data is copied into D3D vertex buffers during world preparation and those
buffers are disposed before the packed CPU owner. The draw uses the hardware
buffer address, not the original malloc-backed tag data. Existing lock/write
notifications advance generation tracking for contiguous resources, but model
buffers registered through the separate owned-range path lack equivalent
write coverage. Any cross-draw decoded cache must explicitly opt in only
certified BSP environment/lightmap buffers, key their non-reused lifetime and
write generation plus exact declaration/stride, preserve per-draw bounds
checks and current default attributes, and bypass dynamic/unmarked storage.
No cross-draw cache has been enabled by this audit.

The separate shader-identity alias scratch candidate passes 348/0 on the local
ARM Mac's hidden CGL context. Its eight-slot ring scans only occupied aliases;
all token fields and both source pointers must match, and insertion follows
proven source equality. Evidence and a review patch are under
`build/mac-audit/program-alias-candidate/`. This is not PPC/Intel validation
and the candidate has not replaced production cache code.

Alias regression test review found the initial alternating-identity assertions
checked only returned program equality, which the old slow path also satisfies.
The scratch fixture now checks the identity-hit deltas for alternation and
for adoption of distinct source pointers. Candidate result is 351/0; running
that strengthened fixture against unchanged production code gives 351 checks
with exactly one failure at the alternation assertion. This negative control
establishes that the test detects the proposed optimization. Updated patch
SHA-256: `8f10d9a6654cf83c94d9ad63a6e5b4b3f1afc9d139f6f719c24017a690c85520`.
Evidence remains local ARM hidden CGL only.

The integrated remap build's dependency issue also exposed a stale full
checkout (`rendered-session/engine`). The candidate is returning to the
coherent `engine-index-decode` sources with only the new state source and
explicitly required matching dependencies staged. Do not switch source trees
merely to resolve one missing include: restore that dependency in the coherent
checkout and verify hashes before compiling. No candidate runtime result is
claimed yet.

### Integrated remap candidate links

The coherent `engine-index-decode` source checkout now builds both integrated
architectures. Root retrieved successful link logs and provenance under
`build/mac-audit/rendered-session/index-remap/`; it records device source
`b9e0e9c2`, index helper `483b09a8` and unchanged production program cache
`bf9f202f`. Candidate binary SHA-256:

- i386: `0abd0443b3e0be2ddb38c15fc9c39f6327c8626cd4a9222583e457cb3145b166`
- PPC: `78d73a881216b37fa8e1684edd069628020f16225a2ed703dba043be0d979f49`

The builder checked all 114 saved baseline object/binary hashes unchanged.
An initial 8 MiB runtime attempt was stopped before gameplay and is invalid
for comparison; a subsequent `env -u` command was rejected before launch by
the old host. All hosts were released to the existing runtime coordinator,
which will use its proven shell-unset harness and 16 MiB configuration for
both candidate-off and candidate-on. No integrated remap runtime pass is
claimed at this entry.

The cross-draw decode prototype now has a v2 stream-bundle design: one exact
multi-stream key selected per draw, direct indexed values/valid bits, bounded
LRU at bundle level, and current-default attribute overlay. This avoids v1's
per-vertex hash/heap overhead. Review found a partial-store bug when replacing
an already valid record with a later non-finite component. V2 now validates
all values before committing. Root strengthened the regression to change an
earlier component before the late NaN (otherwise a partial write would look
unchanged); the ASan/UBSan build passes 81/0. V1 remains an obsolete reference,
not an integration candidate. Neither version is in production.

Before integrating certification, resolve a write-tracking boundary:
`D3DVertexBuffer_Lock` forwards a zero size unchanged to the Mac watcher,
which currently returns immediately. Such a whole-buffer lock must not leave
cached vertices valid if a caller writes through it. Initial BSP uploads use
an explicit size, but that does not prove the generic mutation boundary safe.
The audit is checking a conservative Mac-only fix before enabling cross-draw
reuse.

### Integrated indexed submission: observed results

Root retrieved and checked all six candidate logs/captures. Intel and both
G4s pass 142/0, five fixed-step frames/ticks, with the 16 MiB cache and compact
uploads off. Off/on captures exactly match each other and the prior per-host
baseline; reuse counters are 0 versus 895 draws. Warm mean milliseconds per
frame: Intel 95.50 off / 93.75 on; PowerBook 555.75 / 528.50; iBook 736.25 /
732.50. This short sample shows a modest PowerBook gain, little iBook change,
and no evidence of responsive play. Keep the option experimental pending a
longer lifecycle run. Full evidence and calculated summary:
`build/mac-audit/rendered-session/index-remap/`.

Root also implemented the Mac-only zero-length write-notification fix in
`native_resource_memory.c`: resolve only the remaining requested bytes of an
exact live contiguous allocation. One-past pointers, page padding and unowned
pointers do not advance generation. The CPU resource probe now exercises
whole-buffer zero locks, interior read-only zero locks and those exclusions.
Native validation is queued after the shader-alias probe; this change was not
in the integrated remap binaries above.

### Immutable shader aliases promoted after native checks

The alias candidate passes 351/0 on native Intel and both G4s. Root inspected
all three full logs and source hashes in `build/mac-audit/program-alias-native/`
and promoted exactly the tested cache source (`bf192e18`) and strengthened
probe (`778ade9a`) into production. The public header is unchanged. PPC
binary SHA-256 is
`88226ca066ed8ff67d94de928ec133e3662a5b9d89ddd2d465f3f27fe4f49302`;
both transferred copies matched MD5 `ea2a60c8b18aa2788df4ca72e66d3d21`.
This expands the existing immutable-identity fast path to a bounded eight-alias
ring per program while retaining full-token/pointer checks and exact-source
fallback for mutable callers. Full-scene performance remains unmeasured.

Next integrated batch will combine this tested alias cache with the zero-lock
write-tracking fix after its native CPU probe passes. Recompile both owners
into a separate output and compare with the existing indexed-reuse-on result;
retain the 16 MiB/gather/fixed-step configuration and compact uploads off.
The BSP certificate/decoded-cache work remains isolated scratch preparation.

### Zero-length resource writes: native verification

Root retrieved all three runtime logs and the build log into
`build/mac-audit/zero-lock-native/`. The Intel control, PowerBook and iBook
each report 58 checks with zero failures. This verifies the real buffer
lock/write-generation path, including zero-length whole-buffer and interior
locks, and rejects one-past/padding notifications. The integrated alias batch
now includes this tested resource owner; scene checks are still in progress.

Review of the next BSP certificate prototype found that certificate allocation
failure would delete a successfully uploaded real vertex buffer and fail map
loading. Certification is an optimization prerequisite, so failure must instead
leave the buffer usable through ordinary decoding. The scratch implementation
is being corrected before integration, with certificate lifecycle tests added.

The BSP cache v2 checker now has a reusable host/legacy-Mac runner in
`build/mac-audit/bsp-decode-candidate/v2/checks.sh`; the reviewed source and
strengthened partial-store regression again pass 81/0 under ASan/UBSan.
Eight build-checker overlays in the sibling `build-integration` directory
link the helper without the engine prefix; their shell syntax passes. These
remain staged integration work, not production or native runtime evidence.
The certificate failure fallback is corrected in scratch, and focused real
resource tests are being prepared for one native CPU validation batch after
the renderer coordinator releases the hosts.

### Alias and zero-lock integrated lifecycle pass; long capture unresolved

The combined candidate completes 30 rendered frames/30 engine ticks and two
map lifecycles on Intel and both G4s, with 837 checks and zero failures each.
Root inspected the archived logs in `build/mac-audit/program-alias-integrated/`.
This is lifecycle evidence, not interactive acceptance or full image parity.
The 30-frame captures differ from the older pruned-input baseline despite
matching fixed-step settings. Root measured 70,486 changed Intel pixels,
71,703 PowerBook pixels and 65,452 iBook pixels. The PowerBook images visibly
align in geometry/HUD/weapon but differ around plasma particles.

A concrete reproducibility lead: `random_math_initialize` initializes the
local effects seed from system time and rand(), and particle systems use that
local generator. Fixed engine ticks alone therefore do not establish identical
effect seeds. This is a hypothesis for the image difference, not a parity
pass. Investigate and log/control the hidden probe's actual seed before making
a matched long-capture claim. Short five-frame captures still match per host.

### BSP cache helpers verified on native targets

Root inspected the corrected certificate probe logs (97/0) and v2 decode
helper logs (81/0) from Intel and both G4s. Source/binary hashes and the
MacBook build location are recorded in
`build/mac-audit/zero-lock-native/current/PROVENANCE.md`. The real world-buffer
provider also compiled for PPC and i386; that is compile-only evidence.
Renderer integration remains scratch until its focused GPU checks pass.
Review strengthened those checks to change default attributes between a cache
miss and hit, use counter deltas, and explicitly verify second-stream mutation.

The session probe now supports decimal `HALO_MAC_RENDER_SEED` for hidden GPU
runs only. It sets the real local random seed after math initialization, logs
it, and rejects malformed/overflow values before engine side effects. Root
checked uint32 boundary parsing using the extracted source function. The
assignment is guarded for non-GPU builds; visible play retains its normal seed.
The visible launcher clears this test variable alongside fixed-step settings.
Native replay with this control is still pending; no long-capture parity claim
is made yet.

### BSP renderer seam: first native integration results

The scratch renderer links for PPC/i386. Root retrieved the Intel focused
logs into `build/mac-audit/bsp-decode-native/`: cache off passes 379/0, cache
on passes 384/0. The on-path assertions require actual cache misses/hits,
current undeclared defaults, and invalidation after writes to either stream;
both modes retain indexed submission and EBO/image checks. Decode counts fall
from 37 to 28 with nine cached vertices, and no store rejections. G4 focused
runs are pending; production integration is not yet promoted.

The next CPU target is bone gathering, after BSP reuse. Correct PowerBook
warm timing is 12.457 seconds over 29 frames: stream decoding 2.472 s and
bone gathering 1.250 s together account for about 29.9% of that interval.
An earlier subagent report mistakenly labeled iBook timing as PowerBook; it
was corrected against the raw per-host logs. ARB shader31 is a smaller
measured opportunity (~14 ms per warm frame), not the next priority.

Root has now inspected the iBook focused logs in
`build/mac-audit/bsp-decode-native/`: cache off 379/0, cache on 384/0,
with nine real cached vertices and no store rejections. PowerBook validation
is queued after its seeded replay.

The two PowerBook candidate captures using seed 1835103336 are byte-identical:
SHA-256 `9e485f53cb824891fa2f58deee897800f0b902ae10108fd1dba8f5c975c89fba`.
Root copied and compared the completed frames in `build/mac-audit/seed-replay/`;
the second process was still completing its second lifecycle when checked.
This establishes repeatability for those captures, not baseline/candidate parity.

A separate compact bone-gather scratch experiment initially clamped the first
row before adding subsequent row offsets, which disagrees with the original
at the lower boundary. Root review and expanded reference comparisons caught
this (4235 checks/193 failures in the intermediate prototype). It remains out
of production while the row calculation and late-failure test are corrected.

The exact native-verified v2 helper is now in `port/mac/bsp_decode_cache.c/.h`,
with `tools/mac_bsp_decode_cache_probe.c` and a host/legacy-Mac checker.
The promoted helper passes 81/0 under host ASan/UBSan. Renderer wiring and
certificate owners remain staged pending the PowerBook focused seam check;
adding the standalone module does not yet enable scene caching.

The compact gather prototype's corrected per-row clamping and strengthened
late-invalid tests now pass 4262/0 under host ASan/UBSan. Its separate renderer
overlay preserves original decoded records and falls back to the full-record
gather path when compact VBO upload is enabled. It remains uncompiled scratch
with no performance claim.

### BSP renderer promoted after three-host focused validation

PowerBook focused off/on runs also pass 379/0 and 384/0; root inspected the
archived logs and the three-host provenance at
`build/mac-audit/bsp-decode-native/PROVENANCE.md`. All cache-on runs report
three draw hits, three misses, nine vertex hits, nine misses and zero rejected
stores. Root promoted the exact reviewed renderer, certificate owner, world
VB adapter, session teardown hooks and tests, plus the eight checker link
updates. Previous production versions are preserved in the `pre-promotion`
subdirectory of that evidence folder.

`HALO_MAC_BSP_DECODE_CACHE=1` remains opt-in. The integrated Blood Gulch build
and off/on comparison are next; focused tests do not establish full-map
performance or playability. The matched seeded baseline replay is separately
running on PowerBook; both same-seed candidate replays finished 837/0.

### Seeded PowerBook baseline comparison

Root retrieved the matched remap-baseline 30-frame capture and compared it
byte-for-byte with both alias/zero-lock candidate replays. All three PPMs
have SHA-256 `9e485f53cb824891fa2f58deee897800f0b902ae10108fd1dba8f5c975c89fba`.
They use hidden fixed-step rendering, seed 1835103336, gather/cache16/indexed
reuse enabled, and compact VBO upload disabled. This resolves the long-capture
comparison for this seed and configuration: the alias/zero-lock candidate
preserves the baseline image. The earlier unseeded effect differences are not
evidence of a renderer regression. This comparison does not cover the newly
integrated BSP cache or establish interactive playability. The baseline process
was still in its second map lifecycle when the completed capture was checked.
Artifacts: `build/mac-audit/seed-replay/{baseline,candidate-1,candidate-2}/`.

### Integrated BSP cache: correct on all three Macs, no measurable speedup

Claude Code staged the promoted BSP batch into the remote `engine-index-decode`
checkout (hashes verified) and built it incrementally into the
`bsp-decode-integrated` output on the Intel build Mac (PPC `f89d9925…`, i386 `15325147…`).
The copied pre-BSP runs there were moved to `copied-baseline-not-bsp/`.
Evidence: `build/mac-audit/bsp-decode-integrated/PROVENANCE.md`.

Seeded 5-frame off/on runs pass 142/0 on the Intel build Mac, PowerBook and iBook with equal
off/on captures per host. Seeded 30-frame two-lifecycle runs (off, on, on, off
on each host) all pass 837/0. Cache on reports 2205 draw hits, 45 misses,
about 4300 bypasses, 372237 vertex hits and zero store rejections. PowerBook
cache-off and cache-on captures each reproduce the earlier seeded baseline
`9e485f53…` byte for byte.

Warm means do not improve: PowerBook 432.8/455.7 ms off against 451.3/434.8
ms on; iBook 592.7/573.4 against 591.9/575.2; the Intel build Mac 62.7/62.1 against
63.7/63.6. The warm `stream_decode` bucket does not fall. Two thirds of draws
bypass (model buffers are not certified), and a hit still writes the full
64-float record per vertex, so it costs about what decoding did. The option
stays opt-in and off by default.

Seeded G4 captures are not fully repeatable: PowerBook r2 (on) and r4 (off)
and iBook r3 (on) differ from the other runs, only in the plasma impact
particles and smoke. Because a cache-off run diverges too, this is a
remaining PPC particle nondeterminism, not a cache effect. The Intel build Mac was identical
in all four runs. Single seeded G4 captures need repeats until this is found.

### Seeded PPC captures made repeatable: sound shared the local random seed

The plasma-particle variation came from sound, not rendering. An opt-in trace
(`HALO_MAC_RNG_TRACE=1`, hidden runs) showed diverging runs match after each
tick and then differ after one render by exactly two local-seed steps. That is
`sound_definition_next_permutation`, which the looping-sound update calls
when the real-time mixer finishes a buffer; the sound manager also schedules
detail sounds from `system_milliseconds()`. The fog screen was ruled out (Blood
Gulch never activates one; disabling it changed nothing).

Seeded hidden runs (`HALO_MAC_RENDER_SEED`) now call the engine's
`sound_enable(FALSE)` after `sound_initialize`; normal, visible and unseeded
runs keep sound. All eight traced G4 runs then match byte for byte, captures
and full traces, across both hosts. With final binaries (PPC `35f15fe3…`,
i386 `dc943dfd…`), seeded BSP off/on/on/off runs pass 837/0 everywhere and
all eight G4 captures are identical, so the BSP cache is pixel-identical to
the uncached path on PPC. It still gives no measurable speedup. Seeded
captures from before this change (`9e485f53…`) were made with sound and are
not comparable to new ones.

Observation to measure properly: G4 warm frames were lower with sound off
(PowerBook about 440 to 410 ms, iBook about 583 to 525 ms), suggesting the
mixer thread's voice mixing competes with the game on single-core G4s.
Evidence: `build/mac-audit/rng-trace/PROVENANCE.md`.

### Audio mixing costs the PowerBook about 15% of a frame

Same-seed A/B with sound off and on (`HALO_MAC_RENDER_SEED_SOUND=1` keeps it):
PowerBook warm frames 397 -> 459 ms (+62 ms, +15.5%), iBook 532 -> 572 ms
(+40 ms). The mixer itself takes about 41 ms/frame on the PowerBook (timed
inside `mix()` by the Mac mixer view): the scalar 48-tap resampler at 48 kHz,
about 1.7 us per voice output frame. MacSoft's Halo instead hands voices to the
OS (Carbon Sound Manager, or OpenAL loaded at run time). An OpenAL output path
or a cheaper PPC resampler is the indicated fix.

An apparent "silent output" was a stale `mac_audio_output` object reused from
the shared graph, which predates its nonzero counter; the mixer produces real
audio at every stage. Evidence: `build/mac-audit/audio-cost/PROVENANCE.md`.

### OpenAL voice output recovers the cost of sound on G4

Apple's OpenAL mixes a voice for 0.2-0.6% of a PowerBook's CPU against about
8% for the port's software resampler (`tools/mac_openal_bench.c`). An opt-in
backend (`HALO_MAC_AUDIO_OPENAL=1`, `port/mac/mac_dsound_openal.inc`, loaded
with `dlopen`) plays the DirectSound streams through OpenAL sources. Seeded
sound-on runs, software/OpenAL/OpenAL/software: all 837/0, no fallbacks or
refused buffers; PowerBook warm frames 440 -> 401 ms, iBook 567 -> 522 ms.
Dry voices only so far, and not yet listened to. Evidence:
`build/mac-audit/openal/PROVENANCE.md`.

### OpenAL voices gain Apple's reverb and obstruction

The OpenAL backend now uses Apple's ASA extension where present: reverb with
a room type from the game's I3DL2 decay time and a per-source send equal to
the software mixer's room gain, and obstruction from the direct path's high
frequency loss. `audio.reverb` still turns the reverb off. In a same-binary
OpenAL comparison the reverb costs 2.8 ms a frame on the PowerBook and 8.6 ms
on the iBook (about 1%); all sixteen runs 837/0, reverb on with large-room
type for Blood Gulch, no fallbacks. Obstruction was not triggered in the
scripted scene. Evidence: `build/mac-audit/openal-asa/PROVENANCE.md`.

### First attended play: window fix, then unconverted tag families

The visible launcher failed on the PowerBook: a bare executable from Terminal
is background-only to Carbon (`SetFrontProcess` -606), so `mac_window.c` now
transforms the process into a foreground application first. Attended play
then reached Blood Gulch with working audio in all three audio modes, and
halted on the landing beacon's lens flare and the plasma pistol bolt's light
volume: tag families never converted to native memory. A load-time inventory
(`HALO_MAC_TAG_REPORT=1`) listed 523 unconverted tags; light volumes, scenery
lens flares, grenades, projectile models and camera tracks are now converted
(470 left, mostly menus, the unloaded gun turret, the CTF flag and AI seat
actors). 837/0 after each step. Evidence:
`build/mac-audit/tag-fix3/PROVENANCE.md`.

### GPU-resident world geometry: -45 ms a frame on the PowerBook

Profiling the PowerBook's 417 ms frame showed ~270 ms spent decoding vertices
into 256-byte records and resubmitting them every draw as client arrays (~15 MB
a frame, the static world included). `HALO_MAC_WORLD_VBO=1` decodes each
certified vertex buffer once into a GL buffer of only its declared attributes
and draws the game's own indices from it. Seeded runs are pixel-identical
(`bf3e22f6…`) and 837/0 on both G4s; PowerBook 400 -> 355 ms/frame, iBook
510 -> 472 ms. Model buffers are now certifiable (owned ranges gained write
generations), but skinned models still need CPU bone gathering: with gather off,
eight shaders fall to Apple's software vertex path (1465 ms/frame). Evidence:
`build/mac-audit/world-vbo/PROVENANCE.md`.

### Lean per-draw state: about -35 ms a frame

`mac_gpu_draw_immediate` queried about 20 pieces of GL state before each draw,
restored them afterwards, and looked up uniforms by name. With
`HALO_MAC_LEAN_DRAW=1` (opt-in) it does neither: uniform locations are resolved
once per program, unchanged small uniforms are skipped, and vertex constants are
uploaded as changed ranges. In seeded runs (lean off/on/on/off, two batches per
host) the PowerBook went 384 -> 341 ms/frame and the iBook 480 -> 447, with
`draw_state_setup`, `draw_uniforms` and `draw_restore` falling by 21 to 29 ms a
frame. Run-to-run noise is 20 to 40 ms, so the gain is about 30 to 40 ms. The
texture scope no longer saving bindings made no measurable difference to
`texture_bind`. Captures are pixel-identical (`bf3e22f6…`) and 837/0. Evidence:
`build/mac-audit/lean-draw/PROVENANCE.md`.

### Skinned models from GPU buffers: about -70 ms a frame

A draw that needed bone gathering used to free its buffer's GL copy and decode
256-byte records per vertex every frame. Under `HALO_MAC_WORLD_VBO=1` such a
buffer now keeps a CPU copy of each vertex's address (attribute 5 x,y); the draw
computes the six gathered constant rows only for its index range, binds
attributes 0-9 from the GL buffer and 10-15 from a client array, and draws with
`glDrawRangeElements`. All 6060 buffer-eligible draws now come from buffers
(3210 gathered; 0 skips, 0 fallbacks). In seeded runs (old/new/new/old, with
`HALO_MAC_LEAN_DRAW=1`) the PowerBook went 337.9 -> 273.1 ms/frame (-65) and the
iBook 449.3 -> 369.4 (-80); `stream_decode` fell about 58/65 -> 30/28 ms and
`bone_gather` about 40/45 -> 10/11 ms. Captures are pixel-identical
(`bf3e22f6...`) and 837/0. Evidence: `build/mac-audit/skin-vbo/PROVENANCE.md`.

On the iBook the evening's steps read, in separate sessions: world VBO off about
510 ms/frame, world VBO on 462 to 480, lean draw on about 447, now about 369.
That is roughly 510 -> 370 ms, but the figures come from different sessions with
20 to 40 ms of run-to-run noise, so treat it as approximate; it is not one
continuous measurement.

### Apple's software vertex path costs about 110 ms a frame

Two vertex shaders, table entries 31 (transparent generic on models) and 50
(glass diffuse on models), run on Apple's software vertex path. A profile on the
iBook showed Apple's background thread (`glvmBuildVPTransformFunction`, LLVM)
compiling for the whole sample; the game's simulation is about 1% of main-thread
samples, `render_objects` 42%, transparent 22%, HUD 11%, lightmaps 7%.

With `HALO_MAC_DIAG_SKIP_VERTEX=31,50` (diagnostic: those draws, about 4 a frame,
are not issued) the seeded capture is unchanged and warm time goes from 262 to
156 ms/frame on the PowerBook and 371 to 259 on the iBook (-106 and -112).
Other experiments the same day: a 4 MiB streaming buffer for world draws made
`draw_call` more than double (PowerBook 263 -> 349, iBook 369 -> 529 ms/frame),
so it is opt-in (`HALO_MAC_STREAM_RING=1`); a texture-parameter shadow for
borrowed textures cut `texture_bind` by 67% and 78% (50 -> 17, 76 -> 17 ms) with
no change in frame time; keeping the two shaders off the buffers' layouts
(`HALO_MAC_VBO_EXCLUDE_VERTEX=31,50`) did not help.

On a single-core G4 the background LLVM thread preempts the main thread and
inflates whichever GL call it interrupts, so the cost showed up in
`texture_bind`, `draw_call` and `stream_decode`. It is the shaders being on the
software path at all, not layouts or texture parameter writes. The fix under way
is ARB vertex programs for them (translator in `port/mac/mac_nv2a_arb.c`, being
written). All runs 837/0 and pixel-identical (`bf3e22f6...`). Evidence:
`build/mac-audit/borrowed/PROVENANCE.md`.

## 2026-10-17 — ARB vertex programs for shaders 31 and 50

`HALO_MAC_ARB_VERTEX=31,50` translates retail vertex shaders 31 (transparent
generic on models) and 50 (glass diffuse) to ARB_vertex_program 1.0 with
`port/mac/mac_nv2a_arb.c`, paired with a fragment-only GLSL program, so they
leave Apple's software vertex path. A/B/B/A on both G4s, seeded 30-frame hidden
`--gpu-render` runs with `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1`, binary
`1c96f763...`: PowerBook 262.7 -> 154.3 ms/frame (-41%), iBook 375.2 -> 266.7
(-29%), about 108 ms saved on each. The gain is in `draw_call` and
`draw_submit_restore`; `present_readback` is unchanged. All runs 837/0, ARB runs
`draws=120 failures=0`, `vertex_gpu=1` for 31 and 50 (0 when off). Captures
reported as `bf3e22f6...`, but the capture frame does not visibly contain those
draws, so the flag stays opt-in until a GPU-level comparison of ARB against GLSL
outputs passes (`tools/mac_nv2a_arb_gpu_probe.c`, being done separately).
Evidence: `build/mac-audit/arb/PROVENANCE.md`.

## 2026-10-17 — Lazy present (no per-frame readback)

`HALO_MAC_LAZY_PRESENT=1` (opt-in) makes `D3DDevice_Present` call only `glFlush`
instead of reading the whole 640x480 frame back with `glReadPixels` every frame,
which also waits for the GPU. Readback is requested only for the last frame of a
bounded run (`mac_device_gpu_request_readback()`, from
`tools/mac_session_interactive.c` and `tools/mac_engine_session_probe.c`); visible
play never reads back. A/B/B/A on both G4s, seeded 30-frame hidden runs with
`HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_ARB_VERTEX=31,50`, binary
`31ea7934...`: PowerBook 152.3 -> 103.7 ms/frame (-32%), iBook 269.3 -> 215.5
(-20%). The PowerBook's `present_readback` falls from about 50 to about 2 ms a
frame; the iBook's falls only from about 110 to about 64 ms, so the iBook is
probably GPU- or VRAM-limited (32 MB). That is not established yet. All runs
837/0, ARB runs `draws=120 failures=0`, `vertex_gpu=1` for 31 and 50, captures
reported as `bf3e22f6...`. Opt-in, off by default. Evidence
`build/mac-audit/lazy/PROVENANCE.md`.

## 2026-10-17 — Rendering performance: five A/B experiments and the current best configuration

All are seeded hidden 30-frame `--gpu-render` runs of bloodgulch, A/B/B/A on
both G4s (PowerBook 1.5 GHz, Radeon 9700 Mobility, 64 MB; iBook 1.42 GHz,
Radeon 9550, 32 MB, the 30 fps target). Warm ms/frame is
`(elapsed_ms - first_frame_ms) / 29`. All runs 837/0.

- **Span cache** (`port/mac/native_resource_memory.c`): a small cache for
  `mac_resource_range_is_allocated` (invalidated on every allocation or owned
  range change) and per-64-page block maxima for `mac_resource_range_generation`.
  Binary `31ea7934...` -> `cf405a5d...`. PowerBook 105.0 -> 109.8 (noise, runs
  100.4 and 119.3), iBook 218.4 -> 208.4 (-4.5%, in `stream_decode`). Captures
  unchanged. Kept as harmless. Evidence `build/mac-audit/spans/PROVENANCE.md`.
- **Index cache** (`HALO_MAC_INDEX_CACHE=1`, binary `76a0edc4...`): world draws'
  indices in one 2 MiB element buffer, verified by `memcmp` against a CPU
  shadow. Hit rate about 93%, yet `draw_call` is +11.9 ms (PowerBook) and
  +11.3 ms (iBook) per frame: PowerBook 100.0 -> 114.1, iBook 210.4 -> 225.6.
  Apple's R300 driver handles client-memory indices better than element buffer
  objects here. Stays off. Evidence `build/mac-audit/indexcache/PROVENANCE.md`.
- **World VBO off, to test the iBook's `glFlush`** (binary `76a0edc4...`): the
  iBook's `glFlush` is 66 ms/frame with VBOs and 59 ms without, so the world
  VBOs' VRAM is not the cause. The VBOs are a large win: PowerBook 101.2 vs
  183.2, iBook 215.7 vs 289.9 ms/frame. Evidence
  `build/mac-audit/vbo-flush/PROVENANCE.md`.
- **Compact world VBOs** (only the components each element decodes; GL's 0,0,1
  defaults equal the decoder's; reports `peak=1339468`): `76a0edc4...` ->
  `f6a0c8d7...`, PowerBook 99.5 -> 101.4, iBook 210.9 -> 210.6, neutral, kept.
  `HALO_MAC_STREAM_RING=1` on the new binary is much slower (`draw_call` 2.7x on
  the PowerBook, 2.9x on the iBook; 98.9 -> 160.1 and 207.8 -> 328.9 ms/frame)
  and stays off. Evidence `build/mac-audit/compact-vbo/PROVENANCE.md`.
- **`HALO_MAC_ARB_VERTEX=all`** (binary `88292f5c...`, PPC MD5
  `88292f5ce0f8f6021ef004c192d57458`): every vertex shader is an ARBvp1.0 program
  with native relative addressing, so skinned draws need no CPU bone gather
  (0 calls) and about ten shaders Apple's GLSL ran in software run on the GPU;
  ARB environment parameters are sent only when changed. PowerBook 99.9 -> 88.6,
  iBook 225.0 -> 197.1 ms/frame; first frame about 6.1 s -> 3.1 s (PowerBook)
  and 7 s -> 3.5 s (iBook). The first PowerBook 31,50 run was an outlier (first
  frame 11.8 s, warm 209 ms). Also fixes a bug found in integration:
  `mac_gl_program_cache_uniform_location` returned -1 for programs the cache had
  not linked, so ARB fragment-only programs never got their sampler units, pixel
  constants or fog uniforms (the 31/50 ARB draws in the earlier ARB runs had
  wrong textures and constants, not visible in the capture); it now falls back
  to `glGetUniformLocation`. The ARB-all capture equals the GLSL capture exactly
  on the GeForce 9400M MacBook (`4580961108727e0a5c23a664f345501f`). On both
  G4s the ARB-all captures equal each other (`bfd5bcee6443bb336035bf82f572eae7`)
  but differ from the R300 GLSL capture `bf3e22f6...` (49436 pixels, 97% by fewer
  than 8 levels, about 1400 by more; faint noise on distant surfaces and the
  sand: Apple's R300 GLSL interpolates fog differently from GeForce for some
  shaders, ARB matches GeForce). **New G4 baseline capture for ARB-all:
  `bfd5bcee...`.** Frames in `build/mac-audit/arb-all/frames/`. Evidence
  `build/mac-audit/arb-all/PROVENANCE.md`.

**Current best configuration:** `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1
HALO_MAC_LAZY_PRESENT=1 HALO_MAC_ARB_VERTEX=all`: PowerBook about 89 ms/frame
(~11 fps), iBook about 197 ms/frame (~5 fps) in the seeded hidden benchmark,
against about 400 and 510 ms at the start of the night.

**Open question:** the iBook still spends about 60 ms/frame in `glFlush`
(`present_readback` with lazy present). It is not the world VBOs' VRAM. It is
being investigated with a `HALO_MAC_DIAG_TINY_SCISSOR` fragment-load diagnostic.

## 2026-10-17 — Evidence record: shadow, projective fetches, CPU cuts, fragment diagnosis

All are seeded hidden 30-frame `--gpu-render` runs of bloodgulch, A/B/B/A on both
G4s (PowerBook 1.5 GHz, driver reports Radeon 9600 XT class, 64 MB; iBook 1.42 GHz,
Radeon 9550, 32 MB, the 30 fps target). Warm ms/frame is `(elapsed_ms -
first_frame_ms) / 29`, recomputed from the logs. Capture MD5s are computed on the
G4s. `HALO_MAC_ARB_VERTEX=all` unless noted. All runs 837/0.

- **GL state shadow in lean mode** (`port/mac/engine_device_state.c`): caches
  viewport, scissor, depth range, masks, depth/stencil/cull/blend/dither state,
  array buffer binding, vertex attributes and the ARB vertex program binding;
  forgotten at frame start, by clears and before non-lean draws. Binary
  `88292f5c...` -> `7294627f...`. iBook 195.8 -> 189.6 ms (-3%; runs 196.4/195.2
  -> 189.9/189.2). Its `draw_submit_restore` fell 72.3 -> 59.4 ms a frame, but
  `present_readback` rose 61.2 -> 72.7, so the iBook stays flush-bound. PowerBook
  inconclusive: old 87.3 and 103.0, new 94.1 and 239.6 (the last ran 03:50, during
  another agent's GPU dump on powerbook). Captures `bfd5bcee...` in all eight runs.
  The Intel build Mac check (capture equal to GLSL, `4580961108727e0a5c23a664f345501f`) is as
  reported; it is not in these logs. Evidence `build/mac-audit/shadow/`.
- **Fragment-load diagnostic** (`HALO_MAC_DIAG_TINY_SCISSOR`, one-pixel scissor on
  every draw or on listed vertex shaders): the iBook is fragment-bound. Warm
  212 -> 142 ms and flush 64 -> 11 ms when every draw is clipped; PowerBook ~89 ->
  ~83. Clipping only shader 1 (HUD quad lists, about five draws a frame) gives
  191.6 -> 137 ms and flush 71 -> 11 (binary `2796d524...`). Logs on the G4s, numbers
  as reported. Evidence `build/mac-audit/tiny/PROVENANCE.md`.
- **Projective fetches** (`HALO_MAC_PROJ_FETCH=1`, binary `f73b68e0...`, off/on):
  2D fetches of stages with texture_scale 1 use `texture2DProj` in the ARB fragment
  rewrite. iBook 191.7 -> 170.2 ms (-11%); flush 69.1 -> 50.7 (run p1 vs p2). PowerBook
  off 79.3 (78.4, 80.2), on 89.4 (79.0, 99.7). The 99.7 run had `draw_submit_restore`
  39.2 ms a frame against 28.3 in the 79.0 run, so the PowerBook gain is not
  established. Captures: on `ffc280b8...` in all four runs; off `bfd5bcee...` on the
  iBook and in PowerBook p4, `7141d84d...` in PowerBook p1 (3115 pixels differ by at
  most 1 level in an 88x88 region at 436..523, 182..269; not seen in other runs).
  the Intel build Mac: differs from the original in 8840 pixels, at most 3/255 (as reported).
  Evidence `build/mac-audit/projfetch/`.
- **O(1) bitmap lookup and serial-driven ARB env upload** (`port/mac/native_bitmap_tags.c`,
  last-owner hint in `port/mac/engine_texture_cache.c`, only constants changed since
  the last upload), with PROJ_FETCH=1. Binary `f73b68e0...` -> `cef6a39d...`.
  PowerBook 78.4 -> 73.9 (-6%); iBook 171.3 -> 167.1 (-2%). Captures `ffc280b8...`
  in all eight runs. Evidence `build/mac-audit/cpu1/`.
- **Page-indexed span cache and hash hint** (`port/mac/native_resource_memory.c`,
  ahead of the 512-entry world-VBO search). Binary `cef6a39d...` -> `cc1520c2...`.
  PowerBook 76.2 -> 71.1 (-7%); iBook 169.2 -> 169.3 (GPU-bound, unchanged). Captures
  `ffc280b8...`. Evidence `build/mac-audit/cpu2/`.

Research already written (summarised from the reports, not re-run):

- `build/mac-audit/fragment-cost/FINDINGS.md`: the seeded frame has 46 pixel
  shaders, 253 draws and about 3.0M depth-passing samples; serial 37 is 64% of them.
  The generated GLSL (`port/linux/src/nv2a_psh.c`) emits dead combiner stages and a
  divide per fetch. Folding gives 25-35% and projective fetches a further 35-45%;
  hand-written ARB fragment programs help only alpha-test and cube-map shaders.
- `build/mac-audit/draw-overhead/FINDINGS.md`: per-draw cost on the G4s. A program
  switch costs 20-30 us (GLSL) or 14-24 us (ARB fragment). A GLSL fragment uniform
  change costs 24-47 us against 3.5-7 us for ARB env parameters. An FBO flip costs
  33-37 us, texture format costs nothing. A vertex program that reads more than
  about 100 env parameters costs 138-250 us per draw.

**Current best configuration:** `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1
HALO_MAC_LAZY_PRESENT=1 HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1`: PowerBook
~71 ms/frame (~14 fps; 71.8 and 70.3), iBook ~169 ms (~6 fps; 166-171 across the
runs above), which is fragment-bound. Next: an NV2A combiner -> ARB_fragment_program
translator (in progress), to cut the per-draw driver cost and the fragment ALU work.

## 2026-10-17 — Evidence record: texture scope, identity caches, next-fit allocation, sorted indexes

Seeded hidden 30-frame `--gpu-render` runs of bloodgulch, A/B/B/A (old, new, new, old)
on both G4s, with `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_LAZY_PRESENT=1
HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1`. Warm ms/frame is `(elapsed_ms -
first_frame_ms) / 29`, recomputed from the logs. All runs 837/0. Evidence:
`build/mac-audit/cpu{3,4,5,6}/PROVENANCE.md`.

- **Lean texture scope** (`port/mac/mac_gpu_textures.c`; no per-draw error drain or
  state queries, pixel store restored only after an upload). Binary `cc1520c2...` ->
  `86843258...`. PowerBook 71.9 -> 71.3; iBook 168.4 -> 166.7. Neutral. Captures
  `ffc280b8...` in all eight runs.
- **Identity-keyed ARB fragment cache, render-target check, sorted owner search**
  (`port/mac/engine_texture_cache.c`, `mac_engine_device_resources_texture_target`).
  Binary `86843258...` -> `99b75abe...`. PowerBook 71.0 -> 59.0 (-17%); iBook 168.9 ->
  162.5 (-4%). iBook captures unchanged. PowerBook new captures `8f29f40a...` (old
  `ffc280b8...`): 11872 pixels differ by at most 9, in two lens-flare glows whose
  brightness comes from occlusion queries polled without waiting (as reported; frames
  not archived). Evidence `build/mac-audit/cpu4/`.
- **Next-fit fixed-address allocation** (`mac_resource_vm_allocate_next`,
  `port/mac/native_resource_memory.c`, used by the contiguous allocator, model buffers
  and bitmap tags). Previously every allocation probed from the base one `vm_allocate`
  at a time. Binary `99b75abe...` -> `8f3b4672...`. Frame time unchanged (PowerBook
  57.6 -> 58.0; iBook 162.9 -> 164.3). Whole run, start to end: PowerBook about 2:47 ->
  18 s, iBook about 3:07 -> 23 s (about 8-9x). Logs run one map and 30 frames, not two
  loads and 60 frames. The Intel build Mac 174 -> 18 s is as reported, not in these logs. Evidence
  `build/mac-audit/cpu5/`.
- **Sorted address indexes** for owned ranges, allocations and certificates
  (`mac_resource_range_is_allocated`, `mac_resource_certificate_query`; binary search
  instead of linked-list walks per draw). Binary `8f3b4672...` -> `ad388054...`.
  PowerBook 57.9 -> 55.5 (-4%). The iBook runs were disturbed by another agent's probe
  for the whole A/B (first frames 6.6-7.6 s against 3.2 s); old 232.8, new 219.2 is
  inconclusive. Evidence `build/mac-audit/cpu6/`.
- **PowerBook captures are not repeatable.** One binary (`ad388054...`) gave
  `e5ae85d8...` and `95552659...` in two runs; the cpu5 old binary gave `8507b241...`
  once; the usual value is `8f29f40a...`. The lens-flare occlusion timing is the
  cause to fix before PowerBook captures can be compared exactly. iBook captures are
  `ffc280b8...` in every run.
- **Desktop kits** (as reported; not in these logs): "OpenCE Performance Test" on both
  G4s' Desktops with the cpu5 build; launchers 1 Fast (all optimisations, OpenAL),
  2 Fast software audio, 3 Previous renderer; a READ ME; on the iBook, GameData/Maps
  links to the v7 maps. Checked only in hidden runs.

**Standing:** PowerBook ~55 ms/frame (~18 fps; 55.4 and 55.6); iBook ~162 ms (~6 fps;
162.4 and 162.6, 164 in cpu5), GPU fragment-bound. Loading about 8x faster.
Configuration: `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_LAZY_PRESENT=1
HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1`.

## 2026-10-17 — ARB fragment programs (HALO_MAC_ARB_FRAGMENT=1)

Pixel shaders are translated from the NV2A combiner state to ARB_fragment_program
(`port/mac/mac_nv2a_arbfp.c`, validated on the GPU as in
`build/mac-audit/arbfp/PROVENANCE.md`) and drawn by `port/mac/engine_device_state.c`
instead of the generated GLSL. Programs are cached by translator state (the source
cache's pixel source first), the vertex program's fog slot and the projective
stages; constants are fragment environment parameters sent only when they change;
the GLSL program is unbound; bump environment parameters are zero, as the GLSL
path's never-set uniforms are. Needs `HALO_MAC_ARB_VERTEX`. Opt-in.

Seeded hidden 30-frame runs of bloodgulch, off/on/on/off on both G4s, binary
`7b123d84...`, with `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_LAZY_PRESENT=1
HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1`. Warm ms/frame is `(elapsed_ms -
first_frame_ms) / 29`, recomputed from the logs. All eight runs 837/0, exit 0.
Evidence: `build/mac-audit/arbfp-ab/PROVENANCE.md`.

- PowerBook 55.6 -> 41.9 ms (-25%, about 24 fps; off 55.8 and 55.4, on 40.9 and
  42.9). First frame about 3.0 s -> 1.4 s.
- iBook 167.1 -> 126.8 ms (-24%, about 8 fps; off 162.8 and 171.5, on 126.7 and
  126.9). First frame about 3.2 s -> 1.6 s.
- Every draw is an ARB program: `draws=7584 failures=0 programs=59 env_writes=5443`
  on both hosts. iBook `draw_call` 43-44 ms a frame (GLSL 47 and 54); PowerBook
  13-15 ms (GLSL 18.5). iBook `glFlush` 49-51 ms (GLSL 74-79).
- Captures: the ARB capture is `5500e4db...` on both G4s, identical. The iBook GLSL
  capture `ffc280b8...` differs from it in 360 pixels by 1/255 (as reported; the
  frames are not in this directory). The PowerBook's GLSL captures differ between two
  runs with the same settings (`633689437...`, `e5ae85d8...`, the lens-flare
  occlusion timing), so they are not a reference for PowerBook ARB captures.
- Tiny-scissor diagnostic on the iBook with ARB fragment programs (as reported): 94 ms
  with every draw clipped to one pixel, against 126.6 normal, so the GPU fragment
  excess is about 32 ms. The iBook is now bound by the draw calls and the readback
  more than by fragments.
- the Intel build Mac (GeForce 9400M, as reported): warm 40 -> 29 ms; capture `61cf8752...` (GLSL)
  vs `07edb9ab...` (ARB), 1 pixel by 1 level.

**Current best configuration:** `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1
HALO_MAC_LAZY_PRESENT=1 HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1
HALO_MAC_ARB_FRAGMENT=1`: PowerBook ~42 ms/frame (~24 fps), iBook ~127 ms (~8 fps),
from ~400/~510 at the start of the night. Desktop kits (`~/Desktop/OpenCE Performance
Test`, launchers 1 and 2) carry this binary with `HALO_MAC_ARB_FRAGMENT=1`.

Next: the iBook's `draw_call` (about 43 ms) and `glFlush` (about 50 ms) are the
costs to cut; the PowerBook's GLSL capture repeatability needs its occlusion timing
fixed before PowerBook captures can be compared exactly.

## 2026-10-17 (afternoon) — Detail settings, texture memory and the iBook's draw cost

Seeded hidden runs on both G4s (PowerBook: powerbook, PowerBook G4 1.5 GHz, 64 MB VRAM;
iBook: ibook, iBook G4 1.42 GHz, Mobility Radeon 9550, 32 MB VRAM). Warm ms/frame for
the 90-frame runs is `(elapsed_ms - first_frame_ms) / 89`. Evidence:
`build/mac-audit/detail-settings/PROVENANCE.md`.

**Video memory on the iBook.** The ATI driver exposes `PerformanceStatistics` in `ioreg`
(`texturePageIn`, `texturePageOut`, `gartMap`, `vramFreeBytes`, `texturePageInWaitTime`).
With full textures during play the driver paged about 750-835 MB/s of textures in, with
about 2.7 MB of VRAM free: the iBook's 32 MB overflowed. The PowerBook is not affected.

- **Texture skip** (`HALO_MAC_TEXTURE_SKIP_LEVELS=n`, opt-in, `port/mac/mac_gpu_textures.c`):
  mipmapped bitmaps are uploaded without their n largest levels, with MIN_LOD clamped.
  Non-mipmapped bitmaps (the HUD) are uploaded whole. Retail texture memory 14.2 MB ->
  6.1 MB (n=1) -> 3.7 MB (n=3).
- iBook, 30 frames: about 126.5 -> 103.5 ms (n=1). 90 frames: 127 -> 93 ms (n=1), and
  about 86-78 ms (n=3). Paging is mostly gone at n=3. PowerBook unchanged.
- Unscissored full clears (`HALO_MAC_FULL_CLEAR=1`) and static model index buffers
  (`HALO_MAC_INDEX_VBO=1`), re-tested with the texture skip: neutral to slightly slower on
  the iBook. Left off.

**Diagnostics** (`port/mac/engine_device_state.c`, all opt-in):

- `HALO_MAC_DIAG_DRAW_SAMPLES=<id|all>` (with `_AREA=1`): per-draw occlusion-query sample
  counts, GPU-processing status and ARBfp program sizes.
- `HALO_MAC_DIAG_VERTEX_MODE=<id>:<colormask|noblend|nodepth|tiny|nearest|nomip>`.
- `HALO_MAC_DIAG_DRAW_LOG=<id|all>`: the callers of each draw, for `atos`.
- `HALO_MAC_DIAG_TINY_SCISSOR=<list>`. The value `1` alone clips ALL draws, not shader 1.
  An earlier conclusion that shader 1 is the decals was wrong for that reason.
- `HALO_MAC_DIAG_ALPHAREF0`, `HALO_MAC_DIAG_NO_TEXTURES`, `HALO_MAC_DIAG_TARGETS` (only
  six render targets, 64-320 px), and program-switch counters.

**Where a frame goes.** About 263 draws a frame: models 91 (about 70 of them distant
scenery with about zero visible pixels), transparent groups 46, particles 30, environment
60 (lightmap 14, diffuse 14, fog 14, reflection 8 + 8, shadow 2), HUD about 22, decals and
others. On the iBook the GPU time goes mostly to particles (shader 65, about 1M pixels a
frame, 9-ALU program) and the environment lightmap and diffuse passes (shaders 16 and 40,
about 470k pixels). Render scale did not help much.

**Detail settings** (shared, in `port/linux/src/port_config.c`, documented in
`port/linux/README.md`). `port/linux/game/render_detail.c` applies them from a `port:` hook
in `source/render/render_objects.c`. The defaults draw as the original. The Mac build now
compiles `render_objects.c` and `render_detail.c` itself and uses a rebuilt settings table
(`tools/mac_rendered_session_checks.sh`).

- `display.scenery_minimum_pixels`: scenery smaller than N pixels across on screen is not
  drawn. Never units, vehicles or weapons.
- `display.specular`, `display.shadows`, `display.decals`: the PC version's Yes/No.
- `display.particles`: `high`; `low` (no particle systems or weather); `off` (no particles
  or contrails).
- Defaults are pixel-identical (the Intel build Mac capture `07edb9ab...`), 837/0 for every setting.
  Draws per frame 7417 -> 5495 with every setting low and scenery 16.

**Timing** (90-frame, warm ms):

| Host | Default | Low preset | Lowest |
| --- | --- | --- | --- |
| PowerBook | ~38 | ~30.7 (~32.5 fps) | ~28.1 (~35.6 fps) |
| iBook | ~100 | ~69 | ~59 (~17 fps) |

The low preset is texture skip 3, specular, shadows and decals off, particles low,
scenery 16. Lowest is the same with particles off. Scenery 16 and 32 px alone: PowerBook
37.4 -> ~33.5; iBook ~77.6 -> ~75.5.

**Texture-unit shadow** (the active unit, bound textures, LOD bias only on change;
`port/mac/mac_gpu_textures.c`): PowerBook default ~37.4 -> ~35.8. iBook neutral, GPU-bound.

**Render scale** (`HALO_MAC_RENDER_SCALE`, experimental, off: the back buffer is drawn at
a scale and stretched at present). iBook low preset 68.8 -> 67.9 at 0.75 and 63.8 at 0.6.
The iBook GPU is not pixel-bound.

**What the iBook's CPU spends.** With every draw clipped to one pixel on the low preset the
frame is 47.6 ms (the CPU floor). Self-time: game 41%, GL driver and kernel 35%, port glue
19%. Texture binding costs about 6.6 ms of draw time (no-binds diagnostic: floor 45.4 ->
32.7 ms). Program switches: vertex 22% of draws, fragment 37%.

**Desktop kits** (as reported; not in these logs): "OpenCE Performance Test" on both G4s,
binary `b1c9f1eb...`, with launchers 4 "Low detail" and 5 "Lowest detail".

**In progress.** A G4-tuned rebuild of the game objects (`-mtune=7450`), and a GL frame
capture and replay tool with ablations, to find what makes each of the iBook's draws cost
about 100 µs.

## 2026-10-17 (evening) — GL frame replay: the text atlas made the iBook page textures

A GL frame recorder and replay tool (`tools/mac_gl_record_interpose.c`, `tools/mac_gl_replay.c`, `tools/mac_gl_replay_gen.py`, `tools/mac_gl_replay_checks.sh`, `tools/mac_gl_record_run.sh`, `tools/mac_gl_replay_sweep*.sh`) records one warm iBook frame and replays it in a hidden context. Findings: `build/mac-audit/gl-replay/FINDINGS.md`.

| Replay of the recorded iBook frame (ibook, 100 frames) | Wall ms | Driver CPU ms |
| --- | --- | --- |
| as recorded | 92.4 | 27.4 |
| 2048x2048 RGBA8 text atlas (16.7 MB) allocated 4x4 | 54.3 | 9.1 |
| draw-call ablations (fixed constants, program sorting, index buffers, VBOs) | no gain | |

The 32 MB VRAM cannot hold the atlas (`port/linux/src/text_hires.c`), so the driver pages textures every frame. The atlas size is now a port choice: `text_hires.c` takes `ATLAS_SIZE` (`#ifndef`, default 2048, unchanged elsewhere), and `port/mac/engine_text.c` defines 1024 (4 MB). `build/mac-audit/mac_text_body.h` was regenerated with `tools/mac_text_boundary.py` (the old copy is `mac_text_body.h.before-atlas-20261017`; the only difference is the `#ifndef`). Binary `2574fa3100b36ccffdc5946aeeca76d9`; the Intel build Mac capture identical to the baseline (`07edb9ab...`), 837/0.

Before the fix, `display.high_res_text=false` alone took the iBook's low preset 69.3 -> 49.1 ms, and the default ~100 -> 56.4 ms. With the 1024 atlas (90 frames, warm ms, all renderer flags `HALO_MAC_WORLD_VBO=1 HALO_MAC_LEAN_DRAW=1 HALO_MAC_LAZY_PRESENT=1 HALO_MAC_ARB_VERTEX=all HALO_MAC_PROJ_FETCH=1 HALO_MAC_ARB_FRAGMENT=1`):

| iBook preset | Runs (ms) | fps |
| --- | --- | --- |
| default | 43.1 / 42.2 | ~23.5 |
| low (`display.specular`, `shadows`, `decals` off, particles low, `scenery_minimum_pixels` 16) | 34.7 / 34.3 / 35.5 | ~29 |
| lowest (low, particles off) | 32.3 / 31.8 | ~31 |

PowerBook (64 MB) is unchanged: default 35.6, low 30.1, lowest 27.2 ms. Texture skipping no longer helps on the iBook (default 43.7 with `HALO_MAC_TEXTURE_SKIP_LEVELS=1`, 42.6 with 3), but the scenery cutoff still does (low without it 38.5). The GPU wait at the frame flush is now about 1 ms, so the iBook is CPU-bound. Self time of the iBook default (`build/mac-audit/profile/ibook-p6`): game 41%, GL driver and kernel 32%, port glue 21%.

Earlier the same day, a `-mtune=7450` rebuild of 312 game objects was correct (identical captures) but within noise, so it was not adopted (`build/mac-audit/retune/PROVENANCE.md`). The desktop kits in `~/Desktop/OpenCE Performance Test` on both G4s carry `2574fa31...`; launchers 4 (low) and 5 (lowest) no longer skip texture levels.

Result: the iBook reaches about 30 fps in the seeded hidden benchmark on the lowest preset and about 29 fps on low. Visible play has not been observed. Evidence: `build/mac-audit/atlas/PROVENANCE.md`.

## 2026-10-17 (cpu8) — name lookup cache for native tags

`mac_native_tags_find` (`port/mac/native_tags.c`) now has a 128-entry name lookup cache, keyed on state, count, group and name hash. A hit is checked against its entry before it is returned, and only hits are stored, so a miss always takes the full lookup path.

Build: `$PROBES/20261016/rendered-session/cpu8`, PPC binary MD5 `9424083f4270c1d144f0344cc38343cd`. The Intel build Mac i386 capture is identical (`07edb9ab...`), and the checks read 837 checks, 0 failures.

iBook A/B, low preset, 90-frame warm ms: atlas build 34.5/33.6, cpu8 33.0/32.8.

Timing instrumentation (`HALO_MAC_RENDER_TIMING`) on vs off, low preset: 32.9/32.9 vs 33.1/31.7, so it is neutral. It was removed from the desktop kit launchers.

Both G4 desktop kits ("OpenCE Performance Test") now carry the cpu8 binary. The iBook's low preset in the READ ME is now about 33 ms.

Current numbers (90-frame warm ms): iBook default ~42, low ~33, lowest ~32. PowerBook default ~35.6, low ~30.1, lowest ~27.2.

Next lever under study: fewer ARB program switches. A replay estimate puts up to about 3 ms of CPU per frame in them.

## 2026-10-17 (afternoon): per-draw costs, sampler keys, full screen

Evidence: `build/mac-audit/per-draw/FINDINGS.md`. Every change below keeps the Intel build Mac i386 capture `07edb9ab...` and the checks at 837, 0 failures. Timings are the iBook G4's: 640x480, hidden seeded 90-frame runs, warm ms.

### Kept changes

1. Shader source cache (`port/mac/mac_shader_source_cache.c`, `.h`): the last entry is tried first, and an entry is compared only when its stored key hash matches. Probe: 234 checks, 0 failures.
2. Retail texture lookups (`port/mac/mac_gpu_textures.c`):
   - a 512-slot hint table, each hint verified in full;
   - no `glGetError` on lean cache hits;
   - textures outside the native bitmap cache skip the retail cache scan;
   - the sampler key of a 2D texture ignores `address_w`. The game alternates that field, which re-keyed the sampler and rewrote texture parameters about 14 times a frame. Re-keyings fell from 419 to 59 per 30 frames.
3. Unowned bitmaps (`port/mac/engine_texture_cache.c`): a 64-entry cache of bitmaps that no owner holds, keyed by owner generation and verified by `allocated_bitmap_hardware_format`.
4. Full screen (`port/mac/mac_window.c`): `display.mode = "fullscreen"` captures the main display and asks AGL for the game's 640x480 mode (`aglSetFullScreen`), which the panel stretches. If the capture or the mode fails, the log says so and the usual window opens. Esc quits and puts the display back. The menu pointer is not tracked in full screen. Not yet seen running.

| iBook build (warm ms) | Default | Low |
| --- | --- | --- |
| cpu8 (morning) | 42.1 | 33.6 |
| cand1 (items 1 to 4, with the full-screen code) | 40.6 | 33.4 |
| cand2 (cand1 plus the sampler key) | about 41, unchanged | 32.0 (one 38.3 run excluded; 33.2 before) |

Run-to-run noise is about ±0.7 ms, with machine-wide outliers to 46 to 47 ms.

### Diagnostics

All are off unless set. `HALO_MAC_DIAG_FROZEN_ENV`, `HALO_MAC_DIAG_NO_PROGRAM_SWITCH`, `HALO_MAC_DIAG_DRAW_TWICE`, `HALO_MAC_DIAG_DRAW_TIMES`, `HALO_MAC_DIAG_SEGMENTS` (`port/mac/engine_device_state.c`) and `HALO_MAC_ARB_DUMP` (`mac_arb_vertex_program`); `HALO_MAC_DIAG_SAMPLER_REUSE` (`port/mac/mac_gpu_textures.c`); `HALO_MAC_FLUSH_DRAWS` is experimental and slower.

### Where the driver's time goes

| Cost (iBook, low preset unless stated) | Figure |
| --- | --- |
| A draw that changes nothing | about 3 µs |
| A program switch | about 25 µs whatever the program's length; about 4 ms a frame |
| Texture binding (port and driver) | about 4 ms a frame |
| Vertex and fragment constants | about 1.5 ms a frame |
| Stalled draws | about six a frame, 0.2 to 1.7 ms each, about 4.3 ms in all |

The stalls are the driver submitting its command buffer. Extra `glFlush` calls make them worse: flushing every 16 draws gives 36 to 37 ms a frame, every 8 draws 43.8 ms. Shark works on both G4s (`/usr/bin/shark` with `-1 -i -T 8s -a <pid>`, then `shark -g` for a text report); it is installed with the developer tools.

### Tried and removed

No gain, so not kept:

- constants batched with `glProgramEnvParameters4fvEXT` (slower);
- `APPLE_vertex_array_object` for world draws;
- a narrowed relative-constant array;
- `display.sort_objects` (opaque objects grouped by definition; within noise);
- packed client arrays.

Element buffers, the index cache and the stream ring were tested again after the atlas fix: neutral or worse.

### Resolution and state

All of this is measured at 640x480. The playable build opens a fixed 640x480 window on the iBook's 1024x768 desktop. The Mac build sets no swap interval, so there is no vsync.

The desktop kits "OpenCE Performance Test" on both G4s carry PPC build `bc16f6fe...` (cand1 with full screen), with a sixth launcher, "6 Low detail, full screen 640x480", and an updated READ ME. cand2 (PPC `39028568...`, `rendered-session/cand2`, which also has the diagnostics) is not yet in the kits.

Before anything is committed, the shared files changed this week (`port/linux/game/render_detail.c`, `port/linux/src/port_config.c`, `port/linux/src/text_hires.c`, `source/render/render_objects.c`) must be built for Linux, Windows, Android and web, with ninja or CI. This Mac cannot cross-compile them, and that has not been done.

## Every retail multiplayer map (2026-10-17, night)

All 19 retail multiplayer maps now load, play their scripted ticks and
render on PPC. Timberland was the last; its static scripts are converted by
name (`native_scenario_scripts.c`). The details, per map, are in
`build/mac-audit/maps/FINDINGS.md`. The kits carry PPC `9a27a962`
(`rendered-session/maps28`), and the "7 Choose a map" launcher lists all 19.
Water ripples are still not drawn: their pass targets texture mip levels.

On the low preset, the iBook takes 17–33 ms a frame on every map except
Damnation (about 41 ms, GPU-bound by its dust particles). That is with
Longest, Hang 'Em High and Prisoner drawn one mip level smaller.
