# Native renderer boundary for object creation

This is a source audit of the 130-unit link gate recorded in
`build/mac-audit/spawn-providers-130-ppc.json` (118 unresolved symbols), not
an observed trace of `object_new`. That report contains 31 renderer/texture
entry points. Its linker references prove retention, not runtime execution.

## What construction actually needs

`source/objects/objects.c:3435` creates CPU object state, dispatches real type
constructors, computes model matrices, connects cluster references and creates
attachments. It initializes `cached_render_state_index` to NONE. The direct
construction body does not submit a model or draw geometry.

`source/units/units.c:2505` initializes animation state and calls
`unit_add_initial_weapons`. Consequently the complete standard-port constructor
path reaches `weapon_new`, including the port's projectile-trail preload in
`source/items/weapons.c:586`. With `HALO_PORT_MAXIMUM_NETWORK_PLAYERS` defined,
that preload reads each trigger's projectile, its contrail attachments, and the
contrail's bitmap group. It requests every image through
`_texture_cache_bitmap_get_hardware_format(bitmap, FALSE, TRUE)`. The pointer is
ignored, but loading is requested. The earlier stock weapon-only probe omitted
this conditional path; its success does not validate the standard-port preload.

`source/objects/object_lights.c:851` creates an attached light and calls
`light_reconnect_to_map` at line 1927. This needs real object markers and node
matrices, light bounds, world locations and light cluster references. It does
not fetch the gel bitmap or lens-flare pixels, or issue a GPU draw. The native
light definitions and real light/partition initialization are therefore useful
constructor prerequisites without claiming a rendering backend.

Existing `port/mac/native_model_buffers.c` owns native compressed vertices,
indices and compatible D3D resource headers. Actual model compression has been
validated on both G4s. This is CPU resource storage; no GPU upload is implied.

## Why rendering still prevents linking

The object and widget descriptor tables retain render/debug/update callbacks
alongside their constructors. The retained renderer graph leads to the model,
widget, decal and dynamic-geometry backend functions in the report.

The `_rasterizer_models_*`, `_rasterizer_model_*`, widget drawing/submission,
dynamic geometry drawing, debug line/text and frustum operations must eventually
have real implementations. Their presence in a failed link does not establish
that a single biped constructor invokes them. Similarly, flags allocate their
dynamic geometry in `flag_render_proper` (`flags.c:731`), and lightning's dynamic
vertices are used by its rendering path, rather than initial biped allocation.

The existing real Xbox dynamic-buffer implementation lives in
`source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c`. It creates D3D
vertex/index resources, allocates bounded slices, and locks/unlocks those
resources. Its per-slice delete functions are already empty because these are
frame allocations; this does not justify replacing other engine callbacks with
empty functions. Reusing the real implementation requires a genuine D3D device
resource boundary, initialization and frame reset, not a non-NULL sentinel
`global_d3d_device`.

## Recommended implementation sequence

1. Convert and own the actual projectile, contrail and bitmap dependencies for
   the two starting weapons. Keep the tag data immutable after native publication
   except fields explicitly owned by the runtime cache. Preserve salted handles,
   bitmap types/mips, dimensions and pixel storage bounds.
2. Provide a real native texture resource cache. A CPU-backed resource is a valid
   initial stage if it owns correctly decoded image/mip storage and a compatible
   texture header, records ready/failed state and has a defined release path.
   A preload may complete synchronously before entering the engine, but the
   engine-facing nonblocking request must not pretend an absent image loaded.
   GPU upload can then be deferred to an actual drawing backend. Allocation and
   decoding failures must remain visible.
3. First evaluate reusing the complete `source/cache/xbox_texture_cache.c` with
   real native file-read/resource services. Its cached path includes LRU state,
   read handles and completion, while its uncached path returns the bitmap's
   existing `hardware_format` (`:991`). Eager native residency is a possible
   explicit port policy, not permission to clear flags on arbitrary retail roots.
4. Implement real D3D resource creation, bounds-checked lock/unlock and release
   before retaining the real dynamic-geometry initializer. Share ownership rules
   with model/texture resources; reserve the native game-state arena first.
5. Connect the tested legacy OpenGL context and texture upload path to actual
   draw submission, preserving native-endian indices and vertex layouts. The
   existing terrain/GLSL probes are evidence of device capability, not this
   engine backend's correctness.

For the immediate spawn milestone, prioritize item 1 and the texture preload
boundary while continuing genuine link closure. Do not disable the preload
macro, remove attachments, null descriptor callbacks, or return success from
unimplemented draw functions. These would change the tested path or conceal a
missing service. Static splitting alone cannot remove address references held
by intact descriptor tables; a future operation-separated dispatch refactor
would need its own semantic-equivalence validation.

## Verification gates

- Intel and both PPC hosts: identical resource dimensions, mip counts and decoded
  hashes; pixel ownership remains valid after releasing the source map.
- Reject truncated/oversized payloads, stale handles and invalid resource ranges;
  test failure rollback and release without replacing occupied VM mappings.
- Standard-port `weapon_new`: observe real projectile-trail preload completing
  against the published graph. Do not substitute the stock conditional build.
- Full constructor: observe the returned biped handle, both weapon handles,
  inventory links, node matrices, live light attachments and cluster references.
  Rendering and interactive play remain separate milestones.

## Implemented native resident texture increment

`port/mac/native_bitmap_tags.c` now materializes the complete supported runtime
bitmap root, sequence/sprite records, encoded DXT1/3/5 data and decoded RGBA
for every mip of a 2D power-of-two image. It owns compatible D3D texture headers.
Other image formats/families are explicitly refused. Retail editor import size
and offset metadata are preserved without inventing an import buffer; addressed
editor data is refused. Resource ownership is capped at 32 MiB per tag.

`port/mac/engine_texture_cache.c` implements the engine's texture lookup for an
explicitly bound, eagerly resident native set. It returns the actual owned
resource for the exact registered bitmap address. Blocking/nonblocking queries
both find already-decoded resources, preserving the original bitmap flags.
Unknown bitmap requests fail loudly. This is CPU residency, not GPU upload or a
replacement drawing callback.

The actual assault-rifle tracer bitmap `e2a1012d` has one 32x32 DXT1 image and
six mip levels. MacBook Intel and both G4s passed 45 checks with the same decoded
pixel digest `68578307` and 6604 owned bytes. Checks include native header fields,
all mip dimensions, eight malformed input mutations, scope binding, and reading
resources after freeing both map and bitmap-file input buffers. Evidence:
`build/mac-audit/native-bitmap-build.txt`, `native-bitmap-powerbook.txt`, and
`native-bitmap-ibook.txt`. Integrating this lookup with standard-port weapon
constructors remains a distinct next gate; this probe did not call `weapon_new`.

## Shared-resource ownership correction

The shared D3D resource implementation (`port/linux/src/d3d8_resources.c`)
interprets texture/vertex `Data` as a physical address in the high memory window;
index buffers retain virtual addresses. Its Release frees only D3DCREATED
resources. Native bitmap encoded data now lives in non-clobbering Mach mappings
in that window, with the matching physical Data field. Bitmap and model/index
headers owned by native tags no longer set D3DCREATED: the tag owner releases
them, not the generic resource API. Bitmap accounting includes rounded mapped
pages, increasing the tracer's owned size to 10008 bytes without changing its
pixel digest.

The resource probes extract the exact shared Release function from the source,
with diagnostic deallocator guards, and verify that Release preserves tag-owned
headers and data until their actual owner is freed. This exercises source code,
not a rewritten Release. The updated Intel bitmap probe passes 51 checks and the
model probe 262302 checks (model digest `38f8ff76`). See the separate owner-test
hardware logs for the corresponding G4 observations.

The diagnostic bitmap executable currently reads the entire 317199907-byte
`bitmaps.map` before conversion. Its 10008-byte retained tracer resource is small,
but the whole-file reader is not suitable for the original 256-MiB minimum-RAM
machines. A bounded range-reader or mapped-file interface is required before
claiming that loading target. Existing tests ran on the larger-memory G4s.

## Actual backend compilation and a headless route

`tools/mac_spawn_graphics_checks.sh` extends the already built 203-unit native
baseline with nine real renderer units: Xbox models, decals, draw primitives,
dynamic geometry, widgets, profile, the Xbox rasterizer root, generic debug and
text. It does not execute the result or open a display. GCC 4.0/Tiger-SDK builds
compiled all nine for both PPC and i386. The only compiler accommodation is a
native force-include giving rasterizer_text.c's repeated compatible private
`draw_character_proc` typedef a distinct name, preserving its argument types.

Both resulting links retain 90 undefined symbols, including the same 48 D3D
API/global names. Evidence is `build/mac-graphics-audit/link_{ppc,i386}.txt` and
`census_{ppc,i386}.json`; this is a different, larger link boundary than the
34-symbol baseline, not a regression in that baseline. The census includes:

- Render-state storage and setters, texture-state changes, texture/stream/index
  bindings, pixel shader state and vertex shader constants.
- Immediate vertex attribute accumulation, Begin/End, indexed drawing and
  synchronous callback dispatch.
- Vertex-buffer locking, plus actual renderer state, text/font support,
  transparent groups, fog, vertex-shader selection, resource memory and caches.

A real headless D3D implementation is defensible for the constructor milestone.
The shared desktop implementation already creates CPU device state and targets
but skips window/context creation when `debug.null_renderer` is true
(`d3d8_gl.c:1954`). Its drawing gate then skips GL work; state setters still
store real values, immediate mode accumulates real attributes and callbacks are
actually invoked (`:2213`, `:2385`, `:5277`). A native implementation must preserve
those state/resource semantics and explicitly expose drawing-disabled mode,
not replace engine callbacks with silent success functions.

The best reuse starting point is `port/linux/src/d3d8_resources.c`, whose CPU
resource implementation has no direct GL calls. Its dependencies are native
contiguous allocation/free, texture-layout helpers, write-tracking and logging.
The existing native model/bitmap owners now use its address/ownership contract.
Texture layout helpers currently share `xgpu.h` with modern GL declarations, so
separating a clean resource-only declaration boundary is preferable to importing
SDL3 and OpenGL 4.5 into the Tiger build.

Next implementation batch should therefore expose the real shared CPU resource
unit through a native allocation/write-tracking boundary, add headless device
state with tests for state/readback, bindings, constants, immediate attributes,
callbacks and resource lifetime, and compile the remaining real renderer
providers. Keep every object/widget descriptor callback intact. The present
48-name D3D census is not a complete device-startup inventory: retaining actual
rasterizer initialization may introduce additional APIs. Only a subsequent
successful full link and observed constructors can establish spawning; neither
this compile gate nor drawing-disabled mode establishes playable rendering.

## Native CPU state and mapped asset readers (2026-10-15, continued)

The bitmap and enhanced actor probes now use `port/mac/mapped_file.c` for the
external bitmap file. It maps a checked regular file read-only/private, closes
its descriptor, and provides overflow-safe subspan checks. The 317199907-byte
file no longer requires a whole-file heap allocation/copy. The virtual mapping
still reserves that address range and touched pages consume RAM. This is not
proof of the original minimum-RAM target. Both probes unmap the source before
owned-resource lifetime/constructor checks; the writable 14-MiB map fixture
remains for malformed-input tests. Intel and both G4s pass 19 mapping checks,
52 bitmap checks and 151 enhanced actor checks. Pixel digest and retained owned
bytes are unchanged. Evidence: `build/mac-audit/mapped-assets-{build,powerbook,ibook}.txt`.

`port/mac/engine_device_state.c` provides a CPU state subset generated from
unchanged shared renderer function bodies by `tools/mac_device_state_view.py`.
It includes render/texture state, texture/palette bindings, stream/index
bindings, pixel-shader state, signed-index vertex constants with the original
64-bit dirty serials, and actual synchronous callback dispatch. Its explicit
start/stop scope initializes the shared defaults and resets borrowed bindings;
it owns no textures or buffers. No draw, presentation, Begin/End or full device
initialization entry point is supplied. Those dependencies remain unresolved
until implemented, rather than returning fake success.

MacBook Intel and both G4s pass 40 CPU-state checks: defaults, polygon bias,
bindings, bounds handling, clipped vertex-constant writes, unchanged-constant
serials, pixel-state transfer, callback context and lifecycle reset. Evidence:
`build/mac-audit/device-state-{build,powerbook,ibook}.txt`. This establishes a
verified subset suitable for the constructor link boundary; it is neither a
complete headless device nor graphics/playability validation.

## Shared CPU resource batch verified

The next boundary now compiles the entire unchanged
`port/linux/src/d3d8_resources.c` through
`port/mac/shared_d3d_resources.c`. `tools/mac_resource_boundary.py` extracts the
exact texture description declarations and pure layout section from the shared
`xgpu.h`/`xbox_textures.c`; `shared_texture_layout.c` compiles that section.
This avoids importing SDL3/modern GL declarations while retaining the actual
shared resource functions and format calculations.

`native_resource_memory.c` supplies real Mach allocation/free in the existing
128-MiB high window. It respects requested power-of-two/page alignment, rejects
unsupported protection/invalid sizes, supports exact physical placement, and
never replaces occupied mappings. Allocation records own the rounded pages;
unknown frees fail loudly. This boundary currently requires the target Macs'
4096-byte page size and engine-thread use.

Explicit resource lock/write notifications update per-page generations. This
is real bookkeeping for those writes, not protection-fault interception:
unannounced stores are not detected, and a future GL cache must not assume
otherwise. Counters fail explicitly rather than silently wrapping generations.

`tools/mac_cpu_resources_checks.sh` built both architectures on the MacBook.
Intel and both G4s passed **44 checks**: allocation rejection/alignment and
occupied-address preservation; actual shared vertex/index creation and locking;
DXT mip offsets and linear texture pitch; real shared resource release;
tag-owned-header preservation; and page-generation changes. Final allocation
count and retained bytes were zero. Logs:

- `build/mac-audit/cpu-resources-build.txt`
- `build/mac-audit/cpu-resources-powerbook.txt`
- `build/mac-audit/cpu-resources-ibook.txt`

These are CPU resources with no draw submission or GPU context. The shared
resource unit's existing API validation is unchanged; the test does not establish
safe arbitrary malformed D3D lock requests or complete renderer initialization.
The three linkable objects are `shared_d3d_resources`, `shared_texture_layout`
and `native_resource_memory` with suffix `_cpu_resources_{ppc,i386}.o`.

## Next file and sound boundary: read-only research

The following is **unimplemented and not hardware-validated**. No shared engine
or audio source was changed for this investigation.

- `source/cache/cache_files.h:179` declares `cache_file_read(tag, offset, size,
  buffer, completion, blocking)`. The Windows implementation at
  `source/cache/cache_files_windows.c:849` queues a read and rounds its size to
  a sector; its Custom Edition branch instead completes synchronously. The
  smallest useful native boundary is a synchronous, bounds-checked copy from
  explicitly bound map/resource-file views. Copy exactly the requested byte
  count, not rounded bytes into a destination of unknown capacity. Validate
  signed sizes/offsets, source range and completion pointer before copying;
  test completion and request-handle behavior against actual callers.
- Sound sample offsets are not necessarily offsets in the level map.
  `port/linux/game/cache_file_formats.c:1225` selects sounds.map using the
  sample-data resource bit and validates its range. Its conversion at line
  3859 adds a resource-file base and clears that bit. Follow the existing
  explicit offset-space convention (`custom_edition_cache.c:53` and `:1129`),
  with bounds checked separately for each bound file. A native sound adapter
  must also own and endian-convert sound roots, pitch ranges, permutations,
  references, and runtime cache indices before handing them to the engine.
- Failure handling is a blocker, not a reason to report a successful empty
  read. `_cache_file_read_failed` is 2 (`cache_files.h:30`), but
  `xbox_sound_cache.c:703` and `xbox_texture_cache.c:1042` currently treat any
  nonzero loaded flag as success. Add explicit failure cleanup/tests before
  using that status in a native reader; leaving completion false would instead
  risk an endless blocking wait. Do not adopt the existing CE zero-fill-on-I/O-
  failure behavior as proof that a sound loaded correctly.
- `loose_sounds_read` (`port/linux/game/mod_sounds.c:1058`) really copies owned
  sample data only when the permutation belongs to a registered loose sound;
  otherwise it returns false so the normal cache read proceeds
  (`xbox_sound_cache.c:612`). Prefer compiling this genuine implementation and
  its owned registry over a constant-false replacement. An empty registry is
  the real default; optional loose-file loading/reloading still needs its
  actual lifecycle and settings support before claiming that feature works.
- Supported decode candidates already exist in
  `port/linux/game/custom_edition_sounds.c:399`: PCM, Xbox ADPCM and Ogg Vorbis.
  Start with a read-only census of the installed v7 sounds and reject unknown
  codecs, invalid channels/rates, partial blocks and cyclic permutation chains.
  Do not equate a file reader with audio playback. In particular,
  `port/linux/src/dsound_sdl.c:308` currently memcpy's PCM bytes into native
  shorts: little-endian PC samples will be wrong on PPC. The explicit PCM byte
  decoder in `custom_edition_sounds.c:308` provides the correct endian model;
  the ADPCM predictor at `dsound_sdl.c:283` already assembles bytes explicitly.
  The shared mixer still uses SDL3 audio, so a genuine Tiger/Leopard output
  backend and packet/completion handling remain separate work.

Recommended next batch: native sound metadata/range census, the synchronous
file-read boundary plus explicit failure propagation, and genuine loose-sound
lookup. Verify byte-for-byte reads and failure cleanup on Intel/PPC before any
sound playback claim. Reuse the read-only mapped-file interface for resource
sources; it avoids whole-file heap copies but does not prove minimum-RAM support.

## Native synchronous cache reader implemented

`port/mac/engine_cache_files.c` now supplies actual `cache_file_read`, promotion
of its completed request slot, and the synchronous idle boundary. This
implements the file-read portion of the preceding proposal; native sound-tag
conversion, loose-sound lifecycle, decoding and audio playback remain separate.

Explicit engine-thread scopes bind read-only mapped level/resource files. Offset
spaces match the existing CE convention: level below `0x30000000`, bitmap at
`0x40000000`, sound at `0x60000000`, upper limit `0x80000000`. The decoded-sound
region is rejected. Native loaders must normalize resource offsets before using
this interface; it does not infer file selection from unconverted tag data.
The resource files are optional, and absent sources fail when requested.

Reads copy exactly the requested bytes, with no sector rounding. The checked
native API validates source bounds and a supplied destination capacity, leaving
output untouched on failure. As with the original engine signature, the engine
entry requires its caller to provide an allocated destination large enough for
`size`; that signature cannot discover the allocation's capacity. Every accepted
engine read completes before returning, even when requested nonblocking, and
returns completed slot zero. Consequently no queued I/O needs promotion or
waiting. This provider must not be mixed with another asynchronous cache queue.

Because the current sound/texture consumers mistake failure status 2 for a
successful load, invalid engine-facing requests **abort with a diagnostic**.
They never publish successful zero-filled bytes or a pending flag that could
hang a blocking caller. This fail-closed bootstrap policy is not graceful map
error recovery; future shared caller changes must handle failed cache blocks
explicitly before returning recoverable read failures.

MacBook GCC 4.0/Tiger-SDK builds and execution on Intel plus both G4s passed
**53 checks each**. Tests cover exact unrounded copies and guard bytes; independent
map/bitmap/sound spaces using a temporary fixture; real installed map/bitmap
reads; unavailable sources, bad bounds and decoded-space rejection; scope
ownership and cleanup; and six child-process checks of the diagnostic abort
paths. Core dumps were disabled in those children. No installed sounds.map
metadata, codec, audio output, full object creation or minimum-RAM requirement
was validated. Logs: `build/mac-audit/cache-files-build.txt`,
`cache-files-powerbook.txt`, and `cache-files-ibook.txt`.

## Explicit headless device services (2026-10-15)

`engine_device_state.c` now closes the 19 remaining D3D entry points in the
41-provider graphics inventory. This is a constructor-test device, with GPU
drawing explicitly disabled. `mac_device_headless_start(width,height)` is the
opt-in and creates actual CPU back/depth surfaces through the unchanged shared
`d3d8_resources.c` implementation. The older `mac_device_state_start()` remains
a state-only fixture; attempting drawing/clear/visibility there aborts rather
than silently accepting an uninitialized renderer.

The generator includes unchanged shared immediate-vertex capture/setters,
shader selection/loading/size queries and declaration parsing, viewport
reserved-constant updates, and back-buffer reference handling. Native shader
creation retains copied instructions and the shared declaration interpretation,
adds allocation-failure handling, and retains a device-owned list so shutdown
can reclaim cached programs. External bindings remain borrowed. Native render
target selection preserves the shared CPU behavior (including resetting the
viewport), excluding only the optional graphics trace/statistics logger.

Draw, clear, End, and visibility specialize the existing shared renderer's
`!device.gl_ready` paths: drawing and clearing submit no GPU work, visibility
returns zero samples and timestamp, and immediate End still ends capture.
Skipped-work counters make this observable. Clear intentionally leaves CPU
pixels unchanged, exactly as the upstream null renderer does; this is not a
software rasterizer. No game/object callback table was replaced. Full device
creation/presentation, texture upload and visible rendering remain separate
work; this does not establish a playable session or actual `object_new`.

`tools/mac_device_state_checks.sh` built with GCC 4.0/Tiger SDK on the MacBook.
The Intel executable and the same PPC executable on PowerBook and iBook each
passed **76 checks, zero failures**. Checks include CPU surface ownership and
reference release, viewport constants, shader lifecycle and slot
selection, three captured immediate vertices with color/short/float inputs,
explicit skipped draw counts, unchanged pixels after headless clear, visibility
results, and repeated shutdown/start with zero remaining resource allocations.
Evidence is in `build/mac-audit/device-headless-{build,powerbook,ibook}.txt`.
The device object links against the three previously validated
`*_cpu_resources_{ppc,i386}.o` objects; the device test script expects those to
have been built by `tools/mac_cpu_resources_checks.sh` first.

## Real object initialization after dependency closure

The expanded graphics link now succeeds on both MacBook targets, retaining the
actual `object_new` and callback tables. Evidence:
`build/mac-audit/spawn-graphics-complete-build.txt`. Linking alone was not a
runtime success: the first integrated native-asset load hung in `csmemcpy`.
A MacBook debugger trace (`object-runtime-hang.txt`) showed the wrapper calling
itself. The native forced prefix had read `cseries.h` before cseries.c's own
`BUILDING_CSERIES` definition. The build now defines that macro before forced
includes for that implementation only, preserving its real CRT wrapper bodies.

With that boundary repaired, MacBook Intel and both G4s pass 21 checks through
actual `objects_initialize`, two complete object map-lifecycle cycles, and
object teardown/arena release using Blood Gulch's native assets/world graph.
The probe verifies that the source scenario has no device groups before using
the location-only scenario view; it does not pretend to convert nonempty groups.
The source files are unmapped before the engine initialization. Evidence:
`build/mac-audit/object-runtime-{build,powerbook,ibook}.txt`. This initial gate did
not call `object_new`; the following spawn-mode gate adds real players, time and
render-object storage before attempting the complete constructor.

## Complete native biped and weapon constructors verified

`tools/mac_object_runtime_checks.sh` now links a real lifecycle/spawn probe from
the verified graphics object manifest, excluding its earlier diagnostic main.
It adds actual render-object storage, first-person mesh-reset support and the
whole object-deletion callback table. The `--spawn` mode initializes actual AI
(pre-map only), players, render-object storage and game time, then the complete
object/type/widget/light subsystem. No callback table is pruned or replaced.

MacBook Intel, PowerBook G4 and iBook G4 each pass **49 checks**, across two map
lifecycles. Actual `object_new` creates Blood Gulch's multiplayer biped, invokes
its real unit/biped callbacks, recursively creates the assault rifle and pistol,
sets both inventory slots and produces four real lights. Each run observes
three object datums, biped handle `e26f0000`, cluster 3, and the real biped
constructor's initial support/jump state. Both source mappings have already
been closed. Evidence:

- `build/mac-audit/object-spawn-build.txt`
- `build/mac-audit/object-spawn-powerbook.txt`
- `build/mac-audit/object-spawn-ibook.txt`

Teardown uses actual `object_delete_immediately`, not ordinary `object_delete`
(which defers reclamation until a game tick). Inventory weapons not yet attached
to skeleton nodes before the first tick are explicitly deleted through that
same real API. Object and light datum counts reach zero, map teardown succeeds,
and the arena releases. The initial cleanup attempt exposed the real deletion
callback's dependency on `ai_globals`; genuine `ai_initialize`/`ai_dispose`
now establish and release its pre-map state. AI map activation is not faked.

This is the complete constructor milestone, **not a playable session**. There
is no player record controlling the biped, no simulation tick, no running AI,
no GPU draw/presentation and no audio playback in this probe. Player PVS retains
its actual initialized state, so construction does not establish active gameplay.
The native world still contains only the supported collision/location/portal
subset. Next is a real player association and first bounded movement/input tick,
with required runtime tags and genuine visibility/camera initialization, then
connecting the actual engine to the legacy graphics backend.

Next-path source audit (not yet implemented/tested): native `matg.player_control`
is an immediate dependency of `player_control_initialize_for_new_map` and
`handle_one_player_input` (`player_control.c:1903,623`). Actual `player_new`,
`local_player_set_player_index` and `players_set_local_player_unit`
(`players.c:1225,727,837`) provide record creation and biped/control association.
`players_update_before_game` and its private combined-PVS computation
(`players.c:4737,1789`) consume real queued actions and BSP cluster PVS. Preserve
that path rather than setting visibility bits manually. Director/observer and
input abstraction need their real lifecycles. Full `game_tick` additionally
requires active game globals and many effects/script/HUD services; it is not
safe to infer complete game startup from successful object construction.

## Legacy GL font upload and offscreen glyph drawing

`port/mac/engine_text_gl.c` is an optional GL variant of the actual native font
service, compiled instead of the CPU-only `engine_text.c`. The generator now
also extracts the existing shared atlas-upload function, substituting an
explicit native renderer-state invalidation callback. `mac_text_gl_start`
requires a current caller-owned CGL context and callback. Atlas access refuses
a different/no context; disposal deletes the owned GL texture before the CPU
atlas. The CPU-only engine/constructor build remains independent of OpenGL.

MacBook Intel and both G4s pass **37 checks** using real accelerated CGL contexts
with no windows. The test compares every uploaded RGBA texel against CPU glyph
coverage, checks unchanged-cache and dirty-glyph uploads, changes rasterization
scale, checks context/lifetime behavior, then draws a glyph area into an offscreen
framebuffer with texture sampling and alpha blending. Its RGB readback matches
the CPU coverage within one 8-bit unit. Final GPU alpha digest `2a30d4dd`
matches on all three Macs. Logs: `build/mac-audit/text-gl-{build,powerbook,ibook}.txt`.

This establishes native font upload and a real legacy GL draw, not integration
with the engine's object renderer or a visible menu. The shared 2048-square
atlas occupies 16 MiB as RGBA8 on the GPU plus 4 MiB CPU coverage, so suitability
for the minimum 32-MiB GPU remains unproven. A complete game renderer must supply
the invalidation callback and respect context/resource lifetime.
