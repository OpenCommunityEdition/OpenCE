# Roadmap from native probes to a Blood Gulch session

Status note: this document records the early integration plan. Its historical
"Current verified pieces" table is superseded by
[ppc-mac-audit.md](ppc-mac-audit.md) and the evidence in
[ppc-mac-progress.md](ppc-mac-progress.md). Native engine sessions now exercise
real game ticks, player movement/fire, complete rendered frames and map reload
on Intel and both G4s. Responsive interactive play, Tiger runtime validation
and minimum-spec acceptance remain outstanding; the immediate focus is G4
rendering performance.

This roadmap connects the existing headless native object work, the standalone
Blood Gulch renderer and Carbon input into one visible, controllable session.
It separates verified providers from the engine lifecycle that is still missing.
A successful standalone probe or linker closure is not a playable game.

## Current verified pieces

| Piece | What is real and verified | Boundary that remains |
| --- | --- | --- |
| Retail map readers | `retail_cache`, `retail_geometry`, `retail_texture`, collision and spawn readers decode bounded little-endian map data into owned structures. | The full engine cache/tag ABI is not yet a native Mac runtime. |
| Native world | `engine_world` owns converted collision nodes, leaves and clusters; the actual engine point-in-BSP function and bounded biped movement run on Intel and both G4s. | Surface references, triangles, lightmap vertices, default/material lighting, centroids, planes, physical bounds and sound palettes are owned and tested. GPU resources and full material/render lifecycle remain incomplete. |
| Actors and controls | `mac_object_runtime_probe` creates a real biped and both starting weapons, associates a real local player, applies queued actions and runs 120 genuine object updates per map cycle, including a jump, collision support and both starting weapons. | This is an explicit component lifecycle, not `game_initialize`, `game_initialize_for_new_map`, `game_tick` or `main_new_map`. |
| Decals and object update | Real decal initialization/disposal and visibility-driven creation/removal pass. | 120 full `objects_update` calls per map cycle now pass on all three Macs, including illumination, movement and decals. Full `game_tick` remains unexecuted. |
| Native input | `mac_input` handles Carbon key/mouse/modifier/focus events. `engine_input` now implements real input callbacks plus the separate pure action adapter; 55 callback and 779 adapter checks pass on all three Macs. | The native window can pump snapshots, but no live loop yet feeds them through genuine input abstraction and player control into a running game. |
| Device and text | The shared CPU device-state/resource paths and real font atlas upload/draw run in an accelerated CGL context. The device identity used by object probes is explicitly headless. | Accelerated hidden CGL color/depth drawing is integrated. Visible Carbon/AGL mode remains untested; a full Xbox renderer and audio path remain incomplete. |
| BSP display | `mac_bsp_render` decodes the real Blood Gulch BSP and draws it through legacy OpenGL in an accelerated CGL framebuffer. Optional base textures and RGB565 lightmaps are uploaded as RGBA. | The integrated hidden view renders 240 GPU frames from actual biped camera/aim state during real object updates on all three Macs. Full engine materials, actor rendering and a playable loop remain unfinished. |

All 47 scenery and both sound-scenery placements also coexist through actual
scenario constructors, an object update and complete cleanup on all three Macs.
The two looping-sound attachment records are real; audio samples/playback are
not established. See the latest entries in `ppc-mac-progress.md` for exact logs.

The supporting evidence and limits are in `ppc-mac-native-spawning.md`,
`ppc-mac-runtime-decals.md`, `ppc-mac-engine-integration.md`,
`ppc-mac-playfield.md`, `ppc-mac-bsp-geometry.md` and
`ppc-mac-render-surface-next.md`.

The original physical-memory and sound-cache lifecycle now also passes on all
three Macs after two native ownership fixes: LRU embedded-array disposal and
the borrowed game-state buffer. Real `game_state_initialize` and private-file
writing are observed, with platform teardown clean. These are startup services;
full `game_initialize` and map initialization remain separate outstanding steps.
See the latest progress entries for the full-log evidence.

## What a genuine game tick requires

The public `game_tick()` in `source/game/game.c` asserts that the real
`game_globals->active` state is true. It then calls, in order, collision-log
setup and deterministic math reset; game-state integrity and light recovery;
cheat and player cleanup; allegiance, unit, AI/network actor, and player input
updates; effects, antennas, first-person weapons and game-engine rules; editor,
scripts and recorded animations; full object updates; post-player updates, HUD
updates and player effects; and final integrity/light recovery. These are
ordinary engine calls. A driver must retain their order and use genuine
initialization for each service instead of calling a hand-selected subset and
calling it a tick.

The supported state transition is likewise real engine code:

1. `game_initialize()` creates the private game globals and initializes the
   game-wide services.
2. `game_load()` succeeds through the actual scenario/cache path, making the
   real map-loaded state true.
3. `game_initialize_for_new_map()` runs each service's map lifecycle, marks
   the game active, places scenario objects and starts its normal placement
   work.
4. The ordinary local-player creation and `game_time_start()` path runs.
5. The fixed-step loop calls `game_tick()`; the frame loop builds cameras,
   calls `render_frame()` and presents the drawable.
6. Teardown follows `game_dispose_from_old_map()`, scenario unload and
   `game_dispose()` in the engine's order.

Do not set the private active/map-loaded flags directly. The probe's
`mac_game_core_start()` shares the genuine game.c core allocation/zeroing prefix
but intentionally does not claim the full service initialization or map-load
state. Likewise, `engine_world`'s partial scenario view and the probe's owned
actors cannot be fed to `game_initialize_for_new_map()` as if they were a
complete loaded scenario.

`game_initialize()` has a broad dependency closure: game engine, interface,
scenario, director/observer, render, objects/structures, decals and collision,
players, effects and particles, sound, AI, UI, scripting, transport, console,
network packets, cinematics, saves, events, input abstraction, player UI, Bink
and progress UI. `game_initialize_for_new_map()` adds the corresponding rasterizer,
game-state, scenario, render, structures, effects, sound, physics, networking,
player control, AI, console, cinematic and script map lifecycles. The startup
link census in `ppc-mac-game-startup.md` is historical, not a current complete
runtime link result. Close the link using real providers, then exercise each
initialization in order; a successful link alone proves none of those paths
runs correctly.

## Smallest credible integration architecture

Use one native app owner with four explicit lifetimes:

- **App and window:** Carbon event loop, one visible window, one AGL context
  attached to that window, and one `mac_input` instance. Install the existing
  handler and pump Carbon events on the context/game thread. Handle close,
  focus loss, resize and context teardown in this owner.
- **Platform services:** owned game-state arena and its registered free
  ownership predicate; native files/cache and tag access; real clock and fixed
  tick scheduling; native sound/input boundaries as they become available.
  Keep setup/teardown explicit and fail when a required provider is absent.
- **Engine session:** actual game-wide initialization, complete map load and
  map initialization, genuine local player creation, event-to-engine action
  conversion, actual fixed-step `game_tick`, and the matching engine teardown.
  This replaces the test probe's explicit subset only after full map state can
  be loaded.
- **Presentation:** a resizable drawable and swap/present path. First prove
  camera projection and a native legacy-GL world view using the verified
  geometry/texture decoders. Then route presentation through the real engine
  `render_frame`/Xbox rasterizer path when its resource, shader, surface and
  device dependencies are ready. Do not call a diagnostic GL callback a
  production material renderer.

A thin `mac_session` owner can sequence those layers, but should not become an
alternate game implementation. It should own resources and drive the public
engine lifecycle. Tests should expose each phase separately and fail at the
first missing real provider.

## Recommended integration gates

1. **Finish the world data needed by real object updates.** Follow
   `ppc-mac-render-surface-next.md`: decode and own the true surface references,
   surfaces, native lightmap/material records, material vertex positions,
   lightmap-group reference and bitmap assets. Preserve collision leaf ranges
   and actual handles. Then rerun the existing full `objects_update` diagnostic
   and diagnose its next real dependency. Do not empty or redirect the lookup.
2. **Prove full object and player updates.** Keep real callbacks and extend
   converted tag dependencies as demanded by the next observed fault. The
   existing biped callback proof is lower-level; it does not imply that unit,
   AI, script, object, weapon or HUD updates all finish in a tick.
3. **Close and run the actual lifecycle.** Resolve the current full startup link
   using genuine Mac file, socket, timing, input, renderer and audio providers.
   Initialize game-wide services, load Blood Gulch through the actual cache and
   scenario path, run map initialization and create the local player through
   existing engine paths. Capture a phase log and unwind in reverse engine
   order. Avoid hand-setting lifecycle flags or publishing partial roots as
   complete tags.
4. **Bring up the visible native window.** Reuse the existing Carbon event
   decoding and legacy CGL drawing knowledge, but replace the renderer probe's
   offscreen-only CGL setup with a window-backed drawable. First render the
   actual map's decoded geometry from a camera that follows the real observer;
   then display real actor model geometry and HUD. Add actual engine BSP
   visibility/material paths when their converted data is ready. Present by
   swapping the window's CGL buffers, not by writing a PPM.
5. **Drive input and simulation at engine time.** Pump events, snapshot input,
   apply engine inhibition rules and resolve player control through the real
   `player_control` path. Feed resulting actions into the real input queue.
   Advance ticks at the engine's fixed rate, preserve catch-up/pause rules, and
   render between ticks using the existing interpolation path where available.
   Test focus loss, pause, mouse look, movement, firing and weapon switching
   through the session rather than only the adapter.
6. **Add sound and cleanup.** Wire real sound-system lifecycle and playback,
   then verify one complete map load/unload and app close with no stale arena
   callback, cache entry, CGL resource or handler. Audio is part of a playable
   session, not covered by the existing silent/headless probes.

## Reusable legacy BSP display

`tools/mac_bsp_render.c` is a useful source of genuine rendering mechanics: it
creates an accelerated CGL context and FBO without touching the desktop, obtains
real map vertices/indices through `mac_retail_bsp_geometry`, decodes actual
base/lightmap pixels with `mac_retail_texture`, uploads RGBA bytes, and draws
with legacy multitexture calls. The textured run has 30 loaded images and the
same decoded-pixel digest on Intel and both G4s; two unsupported teleporter
materials are marked magenta. The implementation's callback is already
synchronous, so decoded buffers need not escape their map-reader lifetime.

Reuse the decoder, byte conversion, upload checks and a carefully separated
legacy GL draw helper. Replace only the diagnostic camera/FBO/output shell with
a window drawable and an observer camera. Add BSP switching, clipping, visible
cluster selection, actual material shader state and object/model rendering as
separate steps. The current orthographic top-down projection and two-stage
texture combine are not the game's renderer; retain the caveat for the two
multi-stage teleporter materials and all other unsupported material behavior.

The current `engine_device_state` is valuable for CPU-side D3D state and native
resource ownership probes, but its headless draw calls explicitly count skipped
work. It is not a source of pixels for a visible game. The existing shared
`d3d8_gl.c` implementation and Xbox rasterizer path are a separate route to
production rendering, with a much larger device/material closure. Do not wire
it to the headless identity and infer that rendering succeeded.

## Input reuse and remaining input work

`mac_input_install` registers the tested Carbon handler at application scope;
`mac_input_event` translates physical key positions, mouse deltas, buttons and
focus changes, while `mac_input_snapshot` consumes edge and motion state once.
`mac_engine_input_prepare` maps a snapshot to throttle/facing/action intent, and
`mac_engine_input_action` emits a real `player_action` after engine-resolved
control values are supplied.

The session owner must still map these fields through actual player-control
inhibition and queue code, keep all game mutation on the engine thread, install
and remove the handler with the app, and establish usable pointer-look behavior
without silently grabbing or warping the cursor. Adapter tests construct Carbon
events; they do not prove physical keyboard/mouse input in an app window.

## Acceptance evidence

A first working Blood Gulch session should show a visible resizable window with
actual Blood Gulch, create a real local player using the normal engine lifecycle,
respond to physical movement/look and at least one weapon action, run multiple
complete unmodified `game_tick()` calls, and close/unload cleanly. Record the
Mac model/OS, map/resource hashes, exact initialization path, tick count, input
results and rendering path. Until those checks pass, describe the work as
component probes or partial integration, not playability.
