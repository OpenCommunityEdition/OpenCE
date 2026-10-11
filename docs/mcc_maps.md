# Halo 1 MCC custom maps

This branch adds an independent loader for modern Halo 1 MCC version-13
caches and **MCC SINGLEPLAYER** / **MCC MULTIPLAYER** choices in the map
menus. It is an experimental
runtime adapter, with explicit format limits below. A successful menu
probe or file audit is not proof that a map plays correctly.

## Installation and menus

Put the map in the game data root's `mcc_maps` directory, beside `maps` and
`custom_maps`. For example:

```text
maps/                 Xbox game data
custom_maps/          Halo Custom Edition data
mcc_maps/a10.map       Halo 1 MCC custom map
mcc_maps/a10.txt       Optional plain-text description
mcc_maps/sounds.map    Matching MCC resources, when required by the package
mcc_maps/bitmaps.map   Matching MCC resources, when required by the package
```

The level identity is `mcc_maps\a10`. The filename is retained even when
the internal scenario name differs. The supplied Mercury Rising file is
an example: its filename is `a10.map`, while its header names
`mercury_falling`. The namespaces prevent it replacing either Xbox `a10`
or Custom Edition `custom_maps\a10`.

Some campaigns, including the supplied Combat Revolved package, keep their
audio and textures in separate resource files. Install the matching files
in `mcc_maps` alongside the levels. A filename alone does not establish a
match: the loader verifies each resource's indexed name and byte range.
It never searches `maps` or `custom_maps` for replacements. Packages that
require different resource files must use separate game-data installations.

Open the map-kind chooser and select **MCC SINGLEPLAYER** for campaign
maps or **MCC MULTIPLAYER** for multiplayer maps. The cache header's scenario
type determines the category, independently of the filename; both lists
read the same `mcc_maps` folder. New Game's campaign maps proceed to difficulty
selection, and multiplayer maps can be explored alone. The network host
map screen routes campaign maps to cooperative setup and multiplayer maps
to gametypes. Split-screen multiplayer skips **MCC SINGLEPLAYER**, just as
it skips the existing singleplayer categories. These
are menu routes in the implementation; multiplayer interoperability and
complete campaigns require gameplay testing.
Each selected MCC campaign map is treated as an independent scenario.
Automatic progression through a package's campaign map sequence is not
implemented; the MCC package manifest is not interpreted.

MCC checkpoints retain the canonical `mcc_maps\<filename>` scenario
identity, including when the original tag names an Xbox campaign level.
Continue and Load Game have MCC-specific routes with separate saved-name
storage; they do not turn an MCC checkpoint into a stock campaign level.

The catalog scans only `mcc_maps`, reads each candidate's 2,048-byte header,
and refreshes when a map menu opens. It accepts at most 1,024 entries, with
filenames of at most 54 characters before `.map`. A description is optional;
the default identifies the scenario type. MCC thumbnail sidecars, Workshop
downloads and MCC package manifests are not implemented. There is no
dependency on the Custom Edition setting or its resource directories.

The loader opens source maps read-only. A rejected map reports the failing
stage in `debug.txt` and returns a menu error. Matching the header checksum
can detect that a network host selected a different map version; it is not
a recomputation or authentication of the map's contents. Network support
uses GulchCE's game protocol, not the MCC game client or its matchmaking.

### In-game pause menus

MCC maps use a generated, stock-style pause menu for campaign, cooperative
and multiplayer games. The full, half and quarter viewport layouts follow
the local player arrangement. They use separate MCC-owned widgets, strings,
panels and selection highlights; the default pause route does not open the
map's embedded pause or Settings widgets. Loaded font tags are referenced
without changing their contents.

| Context | Available actions |
| --- | --- |
| Local campaign | Resume Game, Revert to Saved, Restart Level, Save and Quit. |
| Cooperative host | Resume Game, Revert to Saved, Restart Level, Leave Game. |
| Cooperative client | Resume and Leave Game. |
| Local multiplayer | Resume, Restart Game and Leave Game. |
| Multiplayer host | Resume, Restart Game, End Game and Leave Game; Choose Team in team games. |
| Multiplayer client | Resume and Leave Game; Choose Team in team games. |

All full-screen MCC pause menus offer **Settings**, including campaign,
network co-op and multiplayer, with either main-menu style. This opens
OpenCE’s native in-game audio, video and controls screens for the initiating
controller, using upstream’s option filtering and profile saving. Campaign
menus use 30-pixel row spacing and taller panels to accommodate the option. Split-screen players use Settings
from the main menu, since the native editor needs a full viewport.
Restart,
revert and leaving/end-game actions use confirmation
dialogs with Cancel selected initially. B returns from a confirmation to
the pause screen; Start closes the current player's menu and its navigation
history. At the main pause screen, either resumes play. Resume and team
selection also close only the initiating player's menu, preserving other
local players' screens. Each pause layout uses the native A/B button-icon
tokens in its footer, drawing the map's HUD button glyphs beside Select/Back.
Only local campaign pauses
the simulation; network games continue while a menu is open.

Save and Quit preserves the last valid local campaign checkpoint. Revert
requires a valid checkpoint, and cooperative restart/revert remain
host-controlled. A network restart retains the selected map, variant,
options and players through the native lobby/countdown route. End Game is
host-only and ends the round. Team changes remain subject to authenticated
player identity, host authority and balance checks.

The MCC Settings route uses the initiating controller's profile and widget
history. It does not replace another active profile editor. Its ownership
ends on native Save/Cancel, menu closure, replacement editing or map unload,
so a later native editor cannot be closed by stale MCC state.

Campaign layouts display the scenario's current mission objectives through
the native objective-text callback. Owned objective and confirmation text
boxes wrap their instance copies using the current font's metrics and clip
to their bounds. The map's objective data, custom HUD
and embedded widgets remain present. Scripts that explicitly open embedded
widgets can still use the MCC callback adapter described below. The new
pause labels and confirmation text are currently English; localization of
these generated strings is not implemented.

## Repository architecture and isolation

GulchCE is authoritative for this work. `source/` contains the reconstructed
Xbox game systems, including scenario/tag access, cache I/O, the script
interpreter, rendering, audio, objects, game variants and UI. The native
platform layers under `port/linux/`, `port/windows/` and `port/android/`
provide the host services used by those systems. The desktop renderer
implements the game's Xbox interfaces using the existing native graphics
backend. Menu XML and generated settings define the native map chooser.
The `tools/` tests and native harnesses exercise both portable utilities and
selected game subsystems.

Existing Custom Edition support has its own catalog, cache reader,
conversion, resource and behavior modules. Those modules and their state
are not an MCC implementation dependency. The independent components are:

| Component | Responsibility |
| --- | --- |
| `port/linux/game/mcc_maps.c` | Catalog, display identities, names and descriptions. |
| `port/linux/game/mcc_cache_format.c` | Portable little-endian version-13 reader and file-range audit. |
| `port/linux/game/mcc_cache.c` | MCC path admission, read-only file lifetime, conversion and streaming dispatch. |
| `port/linux/game/mcc_main.c` | MCC load-failure recovery through the native main-menu lifecycle. |
| `port/linux/game/mcc_ui.c` and `mcc_ui_network.inl` | MCC in-map widget actions, settings routing, checkpoint/quit behavior and synchronized network restarts. |
| `port/linux/game/mcc_pause.c` and `mcc_pause_runtime.c` | Generated stock-style pause layouts, objective display, independent UI art and atomic publication/lifetime of the appended tags. |
| `port/linux/game/mcc_ui_teams.c` | Authenticated team-only requests during MCC multiplayer rounds, with host balance checks and normal respawn replication. |
| `port/linux/src/mcc_memory.c` | Separate 64 MiB linked tag window; never replaces an existing allocation. |
| `port/linux/game/mcc_geometry.c` | New model descriptors, model vertex/index conversion, external BSP vertex streams and node palettes. |
| `port/linux/game/mcc_audio.c` | MCC sound decoding, resampling and Xbox ADPCM encoding into an MCC virtual stream. |
| `port/linux/game/mcc_resources.c` | MCC-only indexed sound/bitmap resource files, names, ranges and file lifetime. |
| `port/linux/game/mcc_vorbis.c` | Private namespaced Vorbis decoder, bounded Ogg validation and empty residue vector compatibility. |
| `port/linux/game/mcc_bitmaps.c` | MCC pixel layout, BC7 decoding and shader/HUD channel normalization into an MCC virtual stream. |
| `port/linux/game/mcc_texture_cache.c` and `port/linux/src/mcc_texture_bridge.c` | Direct bindings of checked MCC pixel allocations and lazy GPU textures, independent of the Xbox/CE staging cache. |
| `port/linux/game/mcc_tags.c` | MCC metadata, shader-type, HUD placement and widget normalization. |
| `port/linux/game/mcc_hud.c` | MCC bitmap/placement scaling, including nested weapon and grenade HUD items. |
| `port/linux/game/mcc_hud_draw.c` | MCC canvas, glyph geometry and text advances; neutral dispatch for Xbox/CE. |
| `port/linux/game/mcc_terminal.c` | MCC terminal-only font scaling; preserves the map's HUD fonts and all legacy rendering. |
| `port/linux/game/mcc_scripts.c` | Name-based MCC function/global linking and supported MCC native functions. |
| `port/linux/game/mcc_campaign.c` | Local-player/authority queries, gravity, campaign markers and scoped global permissions. |
| `port/linux/game/mcc_syntax.c` | MCC's 32,767-slot syntax validation and traversal workspace. |
| `port/linux/game/mcc_script_parameters.c` and `mcc_script_runtime.inl` | MCC parameter metadata/scopes and interpreter frames inside the existing HS stack. |
| `port/linux/game/mcc_objects.c` | MCC multiplayer vehicle placement masks. |
| `port/linux/game/mcc_grenades.c` | MCC slots 2/3, salted unit inventories, pickup/throw/drop behavior and HUD. |
| `port/linux/game/mcc_player.c` | MCC grenade selection/action checks and starting-profile counts. |
| `port/linux/game/mcc_network.c` | MCC host inventory and campaign-state messages over the existing distributed transport. |
| `port/linux/game/mcc_checkpoint.c` | MCC inventory/campaign snapshot inside proven-unused CPU save memory. |
| `port/linux/game/mcc_tag_validate.c` | Independent range/ownership state while applying stock tag schemas and their validators. |

Shared engine entry points have additive MCC dispatch for an MCC-namespaced
level or active MCC state. They route cache lifetime, raw resources, BSPs,
script extensions, validation callbacks, menu text and model palettes to
the new modules. For other levels those MCC checks do not select a loader.
The new code does not call the Custom Edition parser, converter, allocator,
script adapter, bitmap converter, sound encoder or behavior table. MCC
conversion writes only MCC-owned allocations, and unload releases them.

The pause builder appends its definitions and art to a copy of the live
tag index only after construction succeeds. It preserves the existing tag
records and restores the original index on unload. Its action IDs use the
separate `0x7000` range and require exact ownership of a generated widget;
an embedded map widget cannot acquire those permissions by copying an ID.
Allocation or graphics-resource failure releases the partial construction
without publishing it. Xbox and Custom Edition pause selection remains on
the existing route.

Maps with two through four grenade definitions are supported. The original
unit datum and player-action layouts are retained; the extra two counts
live in a separately owned table keyed by full salted unit handles.
Selection remains in the native unit fields. MCC profile, input, HUD,
pickup, throwing and inventory-network routes handle these slots.

MCC saves put the additional counts and campaign state into a 66,392-byte slot at the CPU
arena's end only if the allocator's current high-water mark leaves that
entire slot unused. No `game_state_malloc` call, pool capacity, native
allocation checksum or save-image size changes. Native checkpoint, core
and persistent writers already serialize the complete CPU/GPU arena;
the persistent image checksum therefore includes this snapshot. An MCC
footer adds version, canonical map identity, cache checksum and payload
CRC32. Before accepting the image, the adapter checks the payload and
remaps incoming object-header pointers to verify every salted unit record.
Unload restores the original unused bytes, including nonzero contents.
The version-2 payload includes gravity, navigation markers, dialogue gain
and the campaign's infinite-ammunition flag. Version-1 inventory snapshots
remain readable, with the new campaign state initialized to its baseline.
The footer remains at its original address. Images are validated before
restoring either state.

Campaign snapshots use the existing OpenCE host/client transport, authority
checks and stale-tick filtering. They are sent only in MCC games; the native
inventory and co-op message layouts are unchanged. Protocol version 27 is
required on every peer because this adds message type 84. Version 26 was
already used by the separately prepared OpenCE contribution branch.
This is an additional message in OpenCE's protocol, not an MCC networking
stack or compatibility with the retail MCC client. Client cheat restrictions
remain in force; only the host decides ammunition consumption.

This is an independently written implementation of documented wire formats,
informed by reading the existing engine's public interfaces and schemas.
No Custom Edition implementation was copied or renamed. Generic engine
operations, including vertex compression and GPU buffer construction, are
shared. MCC compiles a private namespaced instance of the unchanged generic
`stb_vorbis` library to decode Ogg; Xbox/CE retain their existing instance. The
MIT-licensed `port/third_party/bcdec` dependency decodes BC7 and DXT blocks;
its accompanying license and provenance are retained. This statement does
not claim the legal two-team meaning of formal clean-room reverse
engineering.

## Wire format and conversions

The reader admits little-endian `head`/`foot` caches with version 13 and
singleplayer or multiplayer scenario type. Tag memory uses the base
inferred from the tag-array pointer minus the 40-byte MCC tag header.
Each index entry is 32 bytes. Linked pointers are checked against the
loaded data before access. File and multiplication ranges use subtraction
or wider arithmetic to avoid wrapping.

Models use the MCC `mod2` part layout and uncompressed 68-byte vertices.
The adapter creates new 104-byte Xbox part descriptors, compresses vertices
to 32 bytes and uploads the index strips. Local node indices are mapped to
global nodes when possible; models requiring more engine palette slots
use an MCC-owned palette per part. No CE palette storage is consulted.

Version-13 BSP vertex data is external to the BSP tag allocation but can be
embedded in the same map file. The adapter follows the BSP header's vertex
byte count and file offset, reads the material's 56-byte environment and
20-byte lightmap streams, and compresses them into adjacent 32-byte and
8-byte streams. The converted streams have a separate tracked lifetime;
BSP changes release only the previous BSP's buffers.

Bitmap conversion supports the common Halo pixel formats, including P8,
DXT1/3/5 and MCC's BC7 format 18. BC7 is decoded to ARGB8888 for the existing
renderer. Pixel rows, Morton ordering and cube/mip layout are packed by
the independent MCC adapter. The MCC environment bitmap flag is metadata, so it is cleared
when constructing the Xbox descriptor. Compressed and palette flags are
derived from the resulting format. Model flag `0x40` and HUD meter flag
`0x20` identify resources already in Xbox channel order. Those channels
are preserved; references with the flags clear receive Gearbox-to-Xbox
channel conversion. A bitmap used by consumers
that require different channel layouts gets separate MCC-owned tag and
pixel copies, with the relevant consumer's reference redirected. The tag
index can grow into a separate checked allocation for those copies.

Unflagged MCC HUD placements use a 960p canvas, converted to native 480p
coordinates in MCC-owned tags. Legacy high-resolution placements (flag 4)
already carry 480p anchor offsets and retain them. Bitmap size conversion
is independent of that offset flag; the flag is consumed before native drawing. Bitmap half-scale flags are a separate factor, so high
resolution images retain their original texels. Anchors, number advances,
waypoint/damage margins and direct icon geometry are normalized; the
motion-sensor radius and font-relative icon offsets/advances already use
native units and are retained. Five native
HUD/UI files contain additive dispatch only: every pre-existing statement
is retained. Xbox/CE dispatch is neutral before any bitmap lookup or draw.
The pause A/B glyphs and pickup icons use their own bitmap flags, with
matching cursor advances so text remains beside the correctly sized icons.
Pause glyphs align to the active UI font's capitals instead of inheriting
a map's custom HUD icon baseline.

Gameplay A/B/X/Y prompts use OpenCE's embedded hand-drawn button artwork,
with visible bounds centered on the active font's capitals. Controller entries
are identified by their HUD-global entry, not their sprite sequence, so custom
weapon/pickup pictures remain map-authored. MCC owns and releases these bitmap
placeholders on unload; the pause-menu path is unchanged.

Weapon HUD static elements, meters, numbers and overlays also honor MCC's
per-element child anchors (0 inherits, 1..9 select an absolute anchor).
The field occupies the native header's padding at byte 6, as documented by
[Invader's weapon HUD format](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/weapon_hud_interface.json).
Each draw resolves a temporary placement without changing the parent tag.
This places A50_Revisited's visor at the screen edges; invalid child anchors
retain the parent placement. Xbox/CE returns the original parent untouched.

MCC textures bind their immutable converted allocations directly through an
MCC-owned hardware-header registry. Exact registered header identity grants
access to the pixels; a copied descriptor or arbitrary physical-address word
does not. GPU textures upload lazily and remain owned until MCC unload,
which unbinds and deletes them before freeing their source pixels. This
avoids another pixel copy and allocation in the Xbox/CE texture staging
cache, whose capacity and policy remain unchanged. BC7 decoding retains
the full decoded texels; there is no additional lossy recompression or
resolution reduction. Host memory and GPU texture capacity still bound
the maps the machine can render. The same owned pixels remain available
to CPU object-lighting samples. Two-dimensional DXT resources preserve
their complete 2-by-2 and 1-by-1 source mip blocks for those samples, while
the GPU descriptor retains its native mip limit. Unload clears only CPU
base pointers that still name the MCC-owned allocation.

The audio adapter accepts embedded or externally indexed PCM16, Xbox ADPCM and Ogg Vorbis
permutations. It uses independent bounds checks and codec state. Mono
output uses the game's 22,050 Hz format; stereo output follows the admitted
22,050/44,100 Hz tag rate. Necessary conversions produce an MCC-only ADPCM
stream. Unsupported or damaged audio rejects the map instead of silently
removing sound.
External ADPCM that already matches the native sample rate is validated
and retained byte-for-byte. The independent resource reader checks the
16-byte resource header, bounded names/index tables, resource type and
payload ranges. Sound names identify the tag, pitch range and permutation;
bitmap names identify the tag and image. Indexed whole-tag replacements
are still unsupported.

Compiled script function and engine-global indices are linked by the
names retained in the script strings. The adapter provides
`objects_distance_to_object` and `mcc_mission_segment`, and maps
`player_effect_set_max_vibrate` to the engine's rumble operation.
Other unresolved names reject the map. `mcc_mission_segment` evaluates its
string argument normally, records a bounded local diagnostic event and
returns true for a supplied string. This acknowledges the event locally;
it does not implement the MCC telemetry service. The boolean result retains
the conditional `sleep` calls used in shipped and Ruby campaign scripts.
It does not call `core_save_name`: that Xbox function writes a raw debug
save, and remains unchanged and unavailable to map scripts.

Additional MCC campaign operations include `local_players`,
`game_is_authoritative`, `objects_distance_to_position`, `sleep_forever`
(current or named script), gravity setting/reset and breadcrumb navigation
at positions, flags or objects. Local players excludes remote and dead
units. Sleep uses the existing interpreter stack and native wake semantics.
Navigation has eight independently owned team markers and uses the native
HUD drawing services; the host replicates the complete marker state and
gravity periodically, including after changes and late joins.

The recognized `debug_ice_cream_flavor_status_*` globals for IWHBYD,
Grunt Birthday Party and Bandanna report false. This does not implement
skull gameplay. A literal assignment of false is accepted because the
skull is already disabled; attempts to enable one or assign a computed
value are refused. MCC maps may
set `sound_gain_under_dialog` and `cheat_infinite_ammo`; the adapter captures
their previous values and restores them on unload. The native global table
and Xbox/CE write permissions remain unchanged.
Compiled MCC static/stub scripts support up to 16 typed parameters.
Signatures, scoped local references and call arguments are checked before
loading. The MCC interpreter extension keeps arguments, mutable local
values and the evaluation cursor in the existing thread stack; object-list
references survive sleeps and release on return or thread termination.
Nested and recursive calls retain separate scopes. The existing 512-byte
stack remains the recursion/depth bound. Dynamic console calls with
parameters are not supported by this extension; the cached map's calls
are the supported path. MCC syntax now retains its full 32,767-slot arena,
including unused slots for dynamic console expressions. This is the Halo 1
MCC format's signed 16-bit capacity, not 65,535. MCC's independent validator
checks the complete node span, occupied count, salted references and cycles
before the interpreter follows links. Its traversal and parameter workspaces
cover the full arena. Three additive dispatches in `hs.c` select MCC's
validation/allocation-admission path; Xbox and Custom Edition keep their
original 19,001-slot constants, validation storage and statements.
Syntax remains in MCC-owned tag memory, with unchanged datum handles and
thread/checkpoint layouts. No shared data-array structure or allocator changes.
The MCC tag walker also retains the full syntax byte span during its later
schema pass; the shared Xbox/CE schema still specifies 19,001 slots.

Ruby's Rebalanced was audited across all ten campaign maps: each reserves
32,767 slots. `a10` has a high-water count of 19,723, including 18,480 occupied
nodes; the other nine high-water counts are below 19,001. Its existing holes
are retained rather than compacting or renumbering the graph. Across these
maps, the only unsupported native name was `mcc_mission_segment` (225 calls,
42 in boolean contexts and 183 with a discarded return value).

Native Ruby loading exposed two additional MCC bitmap layouts. RGB565
lightmaps with flags `0x1281` are normalized only when their owning group is
a lightmap, they have no mipmaps, and the pixel range is complete and tightly
packed. Bit 12's broader meaning is not inferred. The existing descriptor
verifier still checks the normalized result. Four maps also contain a Wraith
HUD DXT1 texture whose tiny mip tail is sized using unrounded pixel counts.
For that exact full-chain size pattern, the converter retains every complete
mip level and logs the omitted tail; it never reads beyond the declared range.
Arbitrary truncation, missing base images and other unknown flags still fail.

Ruby's `sound\\music\\spooky1\\in` (including `b30` tag #3583) uses an
empty residue vector codebook. The generic decoder aborted the rest of the
residue after encountering it, producing an error and incorrect PCM. MCC's
private decoder normalizes only empty additive residue vector references
before the first overlap packet, matching Xiph's zero-contribution behavior.
Empty scalar floor/classification books are not covered by that rule.
Ogg checksums, page continuity, EOS, sample count and remaining decoder errors
are still checked. Decoder buffers return to their own allocator, independently
of the game's debug allocator and the later ADPCM format conversion.

An independent PCM comparison across all ten Ruby maps covered 50,186 Vorbis
references and 8,325 distinct streams: all decoded successfully and every
sample differed from libsndfile/libvorbis by at most one signed-16-bit unit.
These codec checks do not establish a complete campaign playthrough.

Solar Flare and Spasm Playground also use valid final Vorbis granule trims
across long/short window transitions. MCC decoding retains the requested
PCM frames while validating the remaining packets and a bounded overlap
tail. Page checksums, sequence, monotonic granules and decoder errors are
still checked. A comparison of 247 permutations from the two reported
sound tags matched libsndfile/libvorbis within one signed-16-bit unit.

Combat Revolved's material-less model part retains the native renderer's
skip behavior. Its 8192-pixel credits strip uses the MCC texture route;
the legacy texture cache's size policy is unchanged. Each bitmap tag has
its own virtual stream with exact tag-handle checks, allowing the total
decoded texture storage to exceed the former single 512 MiB offset range.

## Current boundaries

- Modern uncompressed version-13 maps are the target. Earlier Anniversary
  chunk-compressed caches, Halo 2 or later MCC maps and external indexed tags
  are not supported. Indexed MCC bitmap/sound payload resources are supported
  when matching files are installed as described above.
- The portable inspector accepts files up to `INT32_MAX`; runtime raw files
  must be smaller than `0x50000000` bytes because higher offsets identify
  MCC virtual resources. Runtime tag windows must be free, 64 KiB-aligned,
  64 MiB allocations based between `0x42000000` and `0x70000000` inclusive.
  The observed MCC base is `0x50000000`.
- There are at most 65,535 tags and 32 BSP references. Runtime conversion
  descriptors must fit below the lowest BSP in the tag window. Geometry
  currently admits at most 64 nodes per model, 22 palette nodes per part,
  256 geometries per model and 128 parts per geometry. Each part's vertex
  and triangle counts must be nonzero and at most 65,535.
- Non-volume bitmap dimensions are limited to 8,192 by 8,192 with depth one;
  volumes are limited to 512 by 512 by 256. There are at most 13
  mip levels after the base level. Individual raw/decoded allocations are
  capped at 128 MiB; aggregate normalized bitmap storage is capped at 1 GiB,
  with each tag's virtual stream smaller than 512 MiB. Actual allocation and
  GPU texture-size support remain required, especially on mobile devices.
  Linear textures must be two-dimensional, unmipped and uncompressed.
  Audio permutations are capped at 64 MiB of input and 16,777,216 decoded
  frames, with at most 256 MiB of converted audio.
- Reusing stock schemas and gameplay/render systems does not implement
  every MCC engine extension. MCC-only object behaviors, new shader
  semantics and UI callbacks require separate
  semantic work and representative maps. Embedded MCC widget event bytes
  are preserved: compatible callback IDs use the existing engine functions,
  while identified MCC-specific actions dispatch through the MCC adapter.
  This remains relevant to script-opened custom interfaces; the default
  pause menus use the separately generated definitions above. Unsupported
  embedded callbacks fail without performing the event's close/open actions.
  Native Settings is available in all MCC modes with one local player.
  HUD placement normalization and overlay handling
  still require visual comparison against the map's intended appearance.
- No claim is made here of complete campaign/gameplay fidelity or
  synchronization under all conditions, performance parity, or tested
  Android/Linux runtime behavior. See the bounded test status below.

## Validation status

The supplied `MercuryRising/halo1/maps/a10.map` was inspected without adding
its proprietary contents to the repository. Its observed inventory is:

| Field | Observed value |
| --- | --- |
| File size | 407,040,369 bytes |
| Internal name / build | `mercury_falling` / `01.06.66.HELL` |
| Version / scenario type / flags | 13 / 0 (singleplayer) / `0x0007` |
| Tag base / bytes / count | `0x50000000` / `0x012D661C` / 4,575 |
| Models / model parts | 153 / 842 |
| BSPs / BSP materials | 1 / 263 |
| Bitmap records / BC7 records | 1,122 / 38 |
| Sound permutations | 6,629, all embedded |
| Scripts / globals / parameterized scripts | 54 / 12 / 0 |
| Reserved syntax slots / active high-water count | 32,767 / 2,697 |

The parser suite passes 17 tests including this fixture. The native
geometry harness passes ten tests, including conversion of the fixture's
models and BSP, malformed vertex ranges/indices/nodes/normals, and partial
GPU allocation failure cleanup and local/global 64-node palettes. The harness uses the real engine vertex
compressor and mocked GPU buffers; it verifies that no buffers remain after
disposal. This establishes conversion and ownership behavior, not visual
correctness. Thirteen media tests pass for independent Xbox ADPCM decoding,
mono/stereo encoding and resampling, BC7 known-color decoding, DXT edge
clipping, channel transforms, independent bitmap-tag copies, HUD scaling,
Morton/cube/mip packing, row padding and MCC multiplayer text normalization,
virtual sound-stream bounds, exact owned pixel access above the legacy
texture-cache budget, and complete tiny DXT mip chains sampled using the
actual native CPU address function. Seven additional texture-bridge tests compile the
actual bridge and native descriptor/size routines with a mocked GPU. They
cover exact identity, copied-header rejection, upload failure/retry,
unload/re-registration, P8 palette changes, 2D/linear/cube/volume descriptors,
and repeated frames binding thirteen 2K ARGB mip chains (290,805,632 bytes)
without pixel staging allocations or repeated uploads. Live Mercury
audio conversion processed 4,676 permutations into 103,256,532 ADPCM bytes;
this is a conversion-stage result, not a listening test. Its bitmap stage
converts 1,268 original/copied resources into 522,484,864 bytes. The native
MCC schema validator has 30 passing tests. Thirty catalog/path tests cover
the real catalog and runtime admission routines, including same-name maps
in all three namespaces, version/scenario filters, rescan, case handling,
catalog capacity, filename limits, traversal rejection, scenario-type
filtering with stable identities (including empty lists) and checkpoint
identity allocation/failure. Nine checkpoint menu tests verify separate
MCC, Xbox and CE routes, mixed save lists, missing-map handling and bounded
difficulty values.
Twenty-one save-image tests cover the actual MCC snapshot module and
native save entry points, including checkpoint/core round trips, stock
bypass, high-water collisions, original-byte restoration, corruption,
map/checksum mismatch and prospective unit salt/type/pointer checks.
Twenty-five generated parameter tests cover wire bounds, local scope,
call signatures, pre-publication AI casts, mutable values, nested/recursive
argument lookup, object-list ownership, disabled scripts, stack overflow
and serialized frame resumption. Neither supplied real map uses parameters;
the parameter extension's runtime evidence is from the generated harness.
Three lifecycle tests exercise the actual scenario-load, BSP-switch and
main-menu recovery functions. They check balanced time/collision state,
nonmodal initial MCC rejection, preserved inactive Xbox/CE dispatch and
later-BSP rollback, released MCC ownership, and UI recovery before any
failed-map gameplay initialization.

Twenty-nine UI tests compile the actual widget event dispatcher and MCC
callback adapter. They cover mouse/controller confirmation, checkpoint
revert/save, restart permissions, campaign/multiplayer/cooperative quit,
red/blue team choice and balance feedback, New Game routing, trusted native
Settings, unsupported events and isolation of non-MCC widgets. Generated
actions also check exact widget ownership, stale or copied handles,
Settings across campaign and multiplayer and canonical map-name admission. Controller-local
closing tests retain another player's active menu, dispose the initiating
player's navigation history, and leave rejected team requests open. The
team-ingress tests exercise the actual server settings handler and MCC
adapter, including malformed lengths, wrong packet type, unjoined or wrong
machines, altered identity fields, live-player matching, campaign rejection,
balance rules and unchanged pregame/stock dispatch. These are action-routing
tests; they do not by themselves establish that every custom menu renders
or navigates correctly. The New Game lifetime
case synchronously frees the active menu and checks that event dispatch
does not navigate through or delete it again.

The generated-pause suites and UI action suite pass 63 focused tests:
fifteen builder, twelve runtime and thirty-six action tests. They compile the real
builder and runtime with native tag/bitmap structures and a mocked graphics
backend. Coverage includes all 256 selection contexts, 24 root layouts,
viewport bounds, objective callback 18, UTF-16 strings, Cancel-first
confirmations, mode-specific actions and Settings visibility. The actual
native text-wrapping helper and MCC renderer hook are checked with variable
font metrics, width boundaries, existing line breaks, clipping and ownership
isolation. Guarded heap
checks and injected allocation/art/graphics failures verify cleanup,
deduplication, atomic publication, preservation of the original tag table,
and restoration on unload. The combined MCC and existing cache-format suite
passes 462 tests with four optional-fixture skips on the Windows x86
toolchain, with the supplied Mercury fixture enabled.

Native validation of the generated Mercury campaign menu confirmed the
shortened stock labels, complete wrapped objective, separated panels and
A/B footer. Revert produced the saved-game restore log, Restart returned
to the level's beginning after the loading transition, and Save and Quit
returned to `ui.map`. In Nitra multiplayer, Settings opened the native
player Settings screen and Cancel returned to the pause menu; choosing
Blue produced a team-1 log and respawn; End Game reached the postgame
carnage report. Restart returned through the three-second lobby countdown
and reloaded Nitra; Leave Game returned to the main menu. Nineteen focused
restart tests cover the delayed host UI callback, retained settings,
one-shot ownership, stale/disposed sessions and unchanged ordinary postgame
behavior. These are bounded interaction checks; extended network and
split-screen playtesting remains useful.

After the Settings ownership correction, a further native Nitra check
opened the correct profile, returned through Cancel and Escape, reopened
Settings successfully, and used OK with no changes to return to the pause
menu. Seven additional action tests cover controller identity, overlapping
editors, cleanup, failed opens and replacement-editor ownership.

The footer's native A/B glyphs were visually checked in Mercury singleplayer,
Nitra multiplayer, and both host and client menus in a two-process Mercury
co-op session. The glyph fix changes only the MCC-owned label and alignment;
the existing icon renderer is unchanged. All 27 pause builder/runtime tests
passed after the correction.

A two-process local network co-op check displayed the same campaign layout
on both machines, with only Resume and Leave on the client. The host's
Revert restored the saved game and logged the co-op rewind. Restart carried
both players through the lobby and reloaded Mercury's opening cinematic on
both machines. The client then left to the main menu while the host kept
playing; the host's Leave Game also returned to the main menu. Both test
processes exited normally. These checks exercise the real renderer and
transport, but do not substitute for extended Internet or multi-controller
playtesting.

Deferred team-change deaths have their own records keyed by full player and
unit handles. Only the matching no-statistics death receives neutral kill
attribution before network replication; this preserves native respawn and
objective cleanup without a Slayer suicide penalty. Tests include ordinary
deaths, recycled handles, client authority and unloading the map.

The reported red/green Mercury cliff bands match the native allocation
failure texture: `DEFAULT_BITMAP_PIXEL0/1` are transparent red and opaque
green. The captured `stabbed.txt` identifies the 2K cliff normal map as a
failed 22,369,664-byte allocation. Its source is a 5,592,432-byte BC7 mip
chain, expanded to ARGB for the native renderer. About 189 MiB of the
256 MiB staging cache was locked in that frame; its largest available
contiguous span was only about 11 MiB. Repeated retries explain the
associated stalls. The direct MCC binding above removes this staging
constraint. The reported black cliff faces are consistent with a failed
bump/lightmap sample, but the screenshots alone do not prove their cause.
A bounded native comparison rendered twelve camera angles at the captured
failure location. Both builds showed normal cliff textures in that fresh
run: the intermittent baseline failure was not reproduced, so those frames
are a regression check rather than a before/after reproduction. The fixed
Debug renderer also completed Mercury -> Xbox UI -> Xbox A10 -> Nitra ->
Mercury -> Xbox UI in one process without assertions, texture allocation
failures or stale-resource errors. The later generated-pause interaction
checks are described above; broader multiplayer and cooperative playtesting
remains necessary.

An independent script audit compared 748 live function calls, using 103
distinct function names, against the Xbox definitions. It found no
parameter-type drift for functions present in both. The only missing
Xbox names were the four vibrate-alias calls and six
`objects_distance_to_object` calls addressed above. Eight nonuniform
uncompressed cubemaps in the fixture confirmed mip-major storage (all six
faces within each mip): lower mip pixels matched upper mip averages to
less than 0.26/255 mean RGB error. Face-major interpretation produced
37â€“80/255 error. BC7 follows the same layout in the adapter; that particular
layout comparison did not independently test BC7.

The second supplied fixture, `Nitra v2/dangercanyon.map`, also passes the
parser and geometry harness. It is a 102,888,276-byte multiplayer cache,
internally `nitra_v2` with build `01.03.43.0000`. It has 2,353 tags occupying
5,911,224 bytes at `0x50000000`, 55 models, one BSP with 139 materials, 731
bitmap records (no BC7), and 992 embedded sound permutations. Its 11 scripts
and four globals have no parameters. There are 244 high-water syntax slots,
173 live nodes and 56 function calls using 21 distinct names. The only name
missing from Xbox is the vibrate alias (four calls); no parameter-type
drift or missing engine globals were found. Geometry produces 1,112 buffers
and releases all of them.

Live Mercury loading now reaches the rendered world and runs its scripts
after passing media, tag and BSP validation. The validator corrects one
genuinely empty one-frame animation in
`vehicles\falcon_destroyed\left_wing\one frame empty`: both default-pose and
frame payloads are absent, so its node count becomes zero. This is not a
conversion that discards a present animation payload. A two-process Nitra
test ran 1,020 ticks and accepted ten remote hits with no rejected hits.
That run exposed a NULL-free assertion during geometry teardown; all
nullable geometry cleanup paths were guarded, and the harness now rejects
NULL frees like the native debug allocator. The corrected build subsequently
completed an MCC-to-Xbox-to-CE-to-MCC transition in one process without
rejection or assertions. Mercury save, revert and core reload accepted a
snapshot containing four nonzero extra grenade inventories. A two-process
Mercury cooperative run lasted about 1,320 ticks, ran opening scripts and
cinematics, and spawned both players alive at matching positions. Its host
core contained eight extra grenade records; the client's save request was
refused as host-authoritative. A final cooperative run lasted about 1,350
ticks and found the same eight nonzero type-2/type-3 inventories on host
and client, including exact salted handles, at 40, 50 and 60 seconds.
Both processes shut down cleanly and the client returned to the Xbox menu.

A read-only source fixture was copied into ignored test data and its first
BSP vertex was changed to NaN. The native loader explicitly rejected that
copy, released MCC state and returned to the Xbox UI. The same process then
loaded Xbox `a10` and valid MCC Nitra, executed post-load console markers,
initialized rendering and exited successfully without assertions.

Native keyboard/mouse navigation also verified the MCC menu routes and
campaign/multiplayer descriptions. A10 proceeded through difficulty
selection into rendered Mercury gameplay;
Dangercanyon proceeded through Slayer, server setup and the LAN lobby into
rendered Nitra multiplayer gameplay. These bounded checks do not establish
a complete campaign playthrough. Separate
15-second native regression runs loaded Xbox `a10` and Custom Edition
`hugeass` into gameplay/BSPs without assertions. The existing `hugeass`
capacity warning was also present in a prior log; these short checks are
not exhaustive playthroughs.

The initial combined MCC and existing cache-format suite passed 298 tests
with six optional-fixture skips, including the BSP lifecycle and score-hint
checks, both locally and in Windows CI. The Linux MCC CI subset passed
252 tests with five optional-fixture skips. The final native Windows build
and a further rendered Nitra load also passed.

The original Linux regression suite requires POSIX interfaces and could
not run fully on the local Windows/MSVC toolchain. GitHub CI subsequently
built Linux Debug and Release and ran that suite successfully: 236 tests
passed, four skipped, and the separate light-storage checks passed.
Windows Debug, Release and light-storage CI checks also passed, as did
Android Debug and Release. The [complete build run](https://github.com/chestahh/GulchCE/actions/runs/37975964199)
and [MCC validation run](https://github.com/chestahh/GulchCE/actions/runs/37975964090)
cover implementation commit `a196ae0c`. These build/test results do not
establish Linux or Android gameplay behavior.

The subsequent menu split passed 68 focused catalog, menu and saved-game
tests and a native Windows build. The tests cover header-based classification,
empty categories, both spinner directions, solo and host launch paths,
local multiplayer's singleplayer exclusion, returning from difficulty
selection, and switching categories without retaining stale selections.

### Solar Flare, Spasm and Combat Revolved compatibility checks

The initial October 10 load checks incorrectly classified all 16 supplied
maps as passing. They accepted the tag-loader's `mcc: loaded` message and
exit code zero, but missed `type is inconsistent with usage` during the
subsequent script initialization. The retained logs contained this fatal
error for Combat Revolved `a10`, `a50`, `b30`, `b40`, `c10` and `d20`.
Those results did not establish successful startup and are withdrawn.

B40's first failing expression called `core_save_name` as a boolean.
The legacy Xbox function with that name returns void and writes a debug
save. H1A repurposed it for campaign segments, as documented in the
[public scripting reference](https://c20.reclaimers.net/h1/scripting/#functions).
The MCC linker now gives this spelling its own boolean function definition
and the existing MCC segment evaluator. It logs the evaluated segment and
acknowledges it without writing a core file or contacting MCC telemetry.
The native Xbox/CE function table, compiler, validator and save functions
are unchanged. MCC console lookups use the same segment meaning; use the
existing `core_save`/`core_load` commands for an explicit debug core instead.

`tools/mcc_native_smoke.py` replaces the old private load-only checker.
It runs one hidden Windows instance at a time with a disposable data root,
rejects script failures in the logs, and requires a unique console marker
after a 150-tick sleep, followed by normal exit. An echoed command, a loader
message or a zero exit code alone cannot pass. Asset-free tests include
the previously missed fatal log and verify rejection of echoed markers.
For example, with privately supplied maps and any matching resource files:

```powershell
python tools/mcc_native_smoke.py --executable build/windows/halo.exe `
  --data-root build/private-mcc-data --output build/mcc-startup-check b40
```

The checker needs the Xbox UI maps and each selected MCC map in that test
data root. Its logs and map assets must remain outside version control.
These are bounded startup checks, not complete campaign playthroughs or
automatic level-progression tests.

Solar Flare M1 exposed a later runtime failure that a startup check cannot
cover. Its five pilot/dropship recordings contain `0xFF00` in their initial
weapon selection field. The Xbox playback code reads this as the signed
short `-256`, outside the four-slot unit inventory. Triggering `r_a_ds1` on
`a_pilot` in the published Windows release build 32 reproduced the reported
`units.c` assertions at original lines 1512 and 8371. The release's matching
symbols place the subsequent invalid object lookup in `unit_can_use_weapon`.
For a targeted reproduction in M1, create the pilot with
`(object_create_anew a_pilot)` and start `(recording_play a_pilot r_a_ds1)`
through the console. This exercises the control stream independently of the
mission trigger volumes.

`mcc_recordings.c` validates each recording stream before normalizing invalid
initial or event weapon selections to `NONE` (retain the unit's current
weapon). Valid slots 0 through 3 and the existing `NONE` sentinel are unchanged,
as are event timing, movement, flags and the remaining control fields. The
conversion writes only to the MCC loader's private tag buffer through
`mcc_tags.c`; it changes neither the Xbox playback interpreter nor Xbox/CE
loaders, unit code, inventory limits or network messages. Unsupported or
truncated streams are refused with a recording-specific diagnostic;
explicitly disabled empty recordings remain disabled.

Generated regression fixtures cover both playback codecs, all supported
control versions, selection events, truncation at every byte, unknown formats,
idempotence and checked scenario pointers/counts. A private audit of 377
recordings across the 17 supplied MCC maps found no parsing failures. It
normalized 23 invalid selections: five in each Solar Flare map, one in Combat
Revolved's credits and two in its D40. No map contents are included in tests.
The native startup checker now also treats `EXCEPTION assert` as a failure,
even if the process subsequently exits normally.

The recording fix passed a 65-second native M1 reproduction, followed by a
270-second Windows x86 run invoking all five recordings sequentially. Each
invocation was followed by a 1,290-tick wait (longer than the longest recording)
and a fresh completion marker. The run exited normally without assertions.
Only one hidden game instance ran at a time. These targeted checks do not
establish a full M1 playthrough. The final combined MCC/cache/BMP suite passed
821 tests with six optional-fixture skips, and the Windows release build
passed. Android could not be verified because this local configuration has
no Android build target; Linux was not built locally.

After the segment fix, all 16 supplied maps passed 45-second native Windows
x86 runs with rendering enabled: Combat Revolved `a10`, `a30`, `a50`, `b30`,
`b40`, `c10`, `c20`, `c40`, `d20`, `d40` and `credits`; Solar Flare `m0`
through `m3`; and Spasm Playground. Each produced its delayed marker and
exited normally without fatal script diagnostics. B40 also reached the
opening pilot/Cortana dialogue. Combat Revolved used its matching external
resources; no map assets are included in this contribution.

The production Vorbis decoder was compared with libvorbis through
libsndfile for all 158 permutations in Solar Flare's reported sound tag
and all 89 in Spasm's. All decoded, with a maximum difference of one
signed 16-bit PCM unit. Generated tests also cover legitimate granule
trimming, long/short block transitions, damaged granules, CRC errors and
truncated input; accepting these valid streams does not disable integrity
checks.

A single native process loaded Solar Flare, saved changed gravity,
dialogue gain, infinite-ammunition state and a breadcrumb marker, changed
them again, and restored the saved state through both checkpoint revert
and core load. Read-only inspection of that test process confirmed the
restored MCC snapshot and live gravity/gain values. The same process then
loaded Xbox `a10`, CE `hugeass`, and Solar Flare again. MCC state was cleared
on the legacy maps, their original gravity/gain restored, and the MCC-only
functions were unavailable there. The process exited normally.

A bounded two-process cooperative run used the existing host/join path
and confirmed equal gravity, dialogue gain and breadcrumb state on host
and client after host changes, followed by a synchronized gravity reset.
Both exited normally. This tests the new state message on loopback; it
does not establish Internet reliability, every campaign's co-op behavior,
or a full campaign playthrough.

Including the segment-alias and startup-checker regressions, the combined
MCC, existing cache-format and BMP suite passed 746 tests,
with six optional-fixture skips. The Windows x86 release build passed.
Linux and Android builds of these
particular changes have not been verified locally; the Windows build
configuration did not provide an Android target. Earlier CI results
above apply to their named commits, not these changes.

Run the isolated tests with a C compiler and Python/pytest available:

```text
python -m pytest -q tools/test_mcc_cache_format.py
python -m pytest -q tools/test_mcc_geometry.py
python -m pytest -q tools/test_mcc_media.py tools/test_mcc_tag_validation.py
python -m pytest -q tools/test_mcc_hud.py tools/test_mcc_shader_channels.py tools/test_mcc_vorbis.py
python -m pytest -q tools/test_mcc_texture_bridge.py
python -m pytest -q tools/test_mcc_ui.py tools/test_mcc_ui_teams.py tools/test_mcc_ui_network.py
python -m pytest -q tools/test_mcc_pause.py tools/test_mcc_pause_runtime.py
python -m pytest -q tools/test_mcc_maps.py
python -m pytest -q tools/test_mcc_menu.py
python -m pytest -q tools/test_mcc_saved_games.py
python -m pytest -q tools/test_mcc_checkpoint.py tools/test_mcc_grenades.py tools/test_mcc_network.py
python -m pytest -q tools/test_mcc_parameters.py tools/test_mcc_scripts.py tools/test_mcc_syntax.py
python -m pytest -q tools/test_mcc_campaign.py tools/test_mcc_resources.py
python -m pytest -q tools/test_mcc_lifecycle.py
python -m pytest -q tools/test_mcc_recordings.py tools/test_mcc_native_smoke.py
```

Set `MCC_TEST_MAP` to an existing map path to opt into the real-fixture
checks. The parser tests are portable; the geometry harness currently
requires the x86 Windows game headers/toolchain and skips elsewhere.
`port/tools/mcc_cache_report.c` builds with `mcc_cache_format.c` and only
the C standard library, so files can also be audited outside the game.
None of these commands downloads or bundles a game map.

## Additional script and HUD compatibility

The MCC function registry also supports `print_if`, `bitwise_and`,
`bitwise_or`, `bitwise_left_shift`, `bitwise_right_shift`, `bit_test`,
`bit_toggle`, `objects_distance_to_flag` and `sound_impulse_predict`.
The integer operators use defined unsigned 32-bit operations; right shift
fills with zero. Negative shift counts produce zero, while nonnegative
counts use the low five bits. Bit-test/toggle use a bit position, and
toggle's third argument selects set or clear. The prediction function
requests all sound permutations from the existing sound cache without
playing them or consuming random numbers. Its boolean selects a blocking
request. Public signatures and the supplied compiled scripts were checked;
this is not an exhaustive comparison with every MCC runtime edge case.

When referenced, `developer_mode` is linked to an MCC scenario-owned short
global, initially zero. Script reads, writes and native checkpoint storage
therefore work without adding a global to the Xbox interpreter or enabling
engine developer privileges. Loading a different scenario discards it.

MCC HUD anchors 5 through 8 (top, bottom, left and right center) retain
their authored values. MCC-only drawing handles their position, bitmap
bounds and number alignment using the current player's window. The native
anchor enum and its five existing cases are unchanged. Combat Revolved's
unit HUD uses anchor 5; previously the native schema clamped it to top left.

Oversized MCC terminal fonts are scaled at draw time to a 15-pixel line at
the engine's 480p reference resolution. The supplied Combat Revolved font
has a 33-pixel line. This affects only the console input/output, including
wrapping and line spacing; it does not resize HUD text or modify font tags.
Xbox/CE paths and MCC fonts at or below the nominal size remain unchanged.

### B30 Evolved capacity limitation

The supplied `b30_evolved.map` gets past its reported missing functions
with these adapters, but it is **not yet playable**. It contains 640 named
objects and a 101-entry scenery palette. The native object-name table and
co-op presence message support 512 names; the scenario palette schema
supports 100 entries. Earlier conversion truncated the names, followed by
`backpack_1_argl: this is not a valid object name` during script startup.
The MCC validator now refuses the oversized name list explicitly instead
of reporting a loaded map after discarding required script targets.
This check applies to every MCC map: 512 names are allowed, while 513 or
more are refused. It does not change Xbox or Custom Edition validation.

Supporting this map requires an MCC-owned name table with lifecycle,
checkpoint and co-op synchronization support, plus an audit of palette
consumers. Raising the shared Xbox/CE constants or dropping excess names
is not a safe fix. The changes here do not claim that this map runs.

### Checks for these changes (10 October 2026)

The Windows x86 release build passed. The MCC, cache-format and BMP suite
passed 876 tests with six optional-fixture skips. New tests cover typed
integer operations, conditional printing, sound-cache prediction, writable
scenario-local developer state, disabled-skull assignments, all four new
HUD anchors, terminal layout bounds and unchanged legacy dispatch. Name
capacity tests accept 512 and reject 513/640 without truncating the list;
the same fixture still follows the original legacy validator behavior.

The supplied Precipice and Combat Revolved B30 each passed a 70-second
hidden native run, including delayed output after 150 simulation ticks,
normal exit and no fatal script/assert diagnostics. Rendered Combat
Revolved frames confirm centered shields/health and smaller console text.
An initial transition test sent its command before map startup and was
inconclusive; rerunning after waiting for loading verified simulation in
MCC B30, Xbox A10, CE Hugeass and MCC B30 again in one process.

B30 Evolved was tested separately and is refused at its object-name limit;
it is not counted as a successful load. No full campaign playthrough or
network co-op test was performed for these changes. Android remains
unverified locally: `ninja android` reports an unknown target in this
Windows-only configuration.

## Solar Flare M0 cinematic and completion fixes

The supplied M0 has a 100-degree field of view in camera `cin_x4` (index 3).
The native scripted camera and observer accept at most 90 degrees; the
observer also clamps its output to that maximum. The reported assertions
were reproduced as that camera became active. MCC metadata conversion now
limits wider, valid cinematic lenses to 90 degrees and logs the camera
index. This narrows those shots; it does not implement wider cinematic
lenses. Zero still selects the native default, and already supported
angles remain unchanged. Invalid angles (including NaN and infinity) and
out-of-bounds camera blocks are refused. Only the in-memory MCC copy is
changed; the map file and Xbox/CE camera code are untouched.

M0's `cin_a` script calls `game_won` at the end of its cinematics. In local
play, an unknown campaign level previously fell through to the Xbox credits
movie immediately after loading and precaching the menu. A separate access
violation was reproduced through menu -> M0 -> menu, even after fixing the
camera: the cache worker was copying a pending read into protected cache
memory during that movie transition. The matching Windows symbols identify
`read_at` / `ReadFileEx` / `cached_map_issue_async_request` on the failing
thread. Direct startup into M0 did not reproduce this second failure.

MCC scenario completion now queues the ordinary return to the menu, without
starting the Xbox campaign outro. Its helper lives in `mcc_main.c`, selected
by one additive dispatch in `main_roll_credits`. When MCC is inactive the
original statements execute unchanged. The existing network co-op win
branch still returns to its lobby before this path. No network message or
compatibility-version change is involved. This is an MCC-specific avoidance
of the credits/cache interaction, not a general change to the movie player
or background I/O. Xbox and CE completion remain unchanged.

Automatic MCC campaign-package progression remains unsupported: after M0
finishes, select M1 manually. An unrelated saved-map identity rejection is
normal, and the map's equipment/grenade warnings are not suppressed here.

Regression tests cover camera bounds, default and supported lenses, wide
lenses, malformed floats, repeat conversion, and MCC versus legacy
completion dispatch. The Windows x86 release build and 889 automated tests
passed (six optional-fixture skips). Android could not be built locally:
this Windows configuration has no `android` target.

The final Windows native run started at the menu, loaded the supplied M0,
played its entire cinematic sequence, returned to the menu and remained
there for approximately another minute before a normal timed exit. This
220-second test used one hidden instance at 60 FPS with audio decoding
enabled and output muted. It produced neither camera assertions nor an
access violation. No new network co-op playtest was performed for this fix.

## OpenCE upstream integration through d03f6e27

The October 10 upstream merge includes independent Custom Edition directory
listing, profile creation from Settings, co-op elevator prediction, custom
HUD digits on separate bitmaps, and an optional first-person shield flare.
The shared menu, HUD and network changes retain the existing MCC dispatches;
MCC loaders, script adapters, rendering conversion and network message
formats are unchanged. The arm-shield setting defaults to off.

Upstream's directory fix revealed the same shared-iterator hazard in MCC's
catalog: startup profile cleanup can enumerate directories concurrently.
MCC discovery now owns and closes a separate platform directory handle,
without accessing the Xbox or CE catalogs. Catalog tests exercise filtering,
missing/empty directories, handle cleanup, sorting and rescan behavior.

The Windows x86 release build passed. B40 and Precipice each passed a
40-second hidden native startup/simulation check, run sequentially, with
normal exits and no detected assertions. These are bounded smoke checks,
not full campaign or live co-op playtests. Android remains unverified:
the local Windows configuration has no Android target. Upstream's
Linux-oriented harness checks were run with local Windows compiler/linker
flag adjustments; the test bodies and production code were not substituted.

## Additional campaign compatibility (October 2026)

The MCC function table additionally supports `list_count_not_dead`,
`script_finished`, `game_time_authoritative`, `sound_get_effects_gain`, and
`sound_set_effects_gain`. Living-object counts skip dead and stale objects.
`script_finished` checks the named script thread's completed state; dormant,
sleeping, and unknown scripts are not reported as completed. The authoritative
clock uses local/server game ticks, or the latest received host tick on clients.

Effects gain is an MCC-owned multiplier (default 1, accepted range 0â€“4).
It affects non-music sound classes without changing saved user volume settings.
The small additive connection in the common sound mixer returns a neutral
multiplier outside an active MCC campaign. The MCC writable-global whitelist
also permits `object_light_ambient_base`, `object_light_ambient_scale`,
`loud_dialog_hack`, and `ai_debug_ignore_player`. Original values are captured
and restored when MCC unloads. AI-ignore changes remain host-authoritative.
The existing Xbox/Custom Edition global tables and script implementations are
not extended or replaced.

These values accompany the existing MCC campaign checkpoint/network state.
The version-2 campaign snapshot is 764 bytes; version-1 752-byte checkpoints
are migrated with defaults for the added state. Network compatibility advances
from 27 to 28: **all peers must use matching versions**, including when playing
Xbox or Custom Edition maps. There is no separate MCC transport or matchmaking.

Boom, Sputnik, and Catch skull globals report disabled. Literal assignments
that keep them disabled are accepted; enabling them or computing an assignment
is refused rather than pretending their gameplay effects exist. Similarly,
`object_prediction` reports enabled and accepts a literal enable only, matching
the existing engine prediction policy. Converted constant expressions preserve
their expected result types, including a void result for a `set` statement.

Several data corrections are confined to MCC conversion:

- An animation graph's unused root sibling is cleared only after validating
  the reachable child tree and parent relationships. Orphans and child cycles
  still fail validation. This handles DP's brute graph.
- Invalid model tangent frames can be reconstructed from valid geometry and
  UV derivatives. Degenerate UVs use a deterministic perpendicular basis.
  Invalid positions, normals, UVs, or indices still fail; valid frames are
  retained. This repairs ML's pelican data without modifying the source map.
- Shared-resource lookup can resolve a relocated payload by a unique full
  resource name and exact size in the supplied cache's validated directory.
  Duplicate names, size mismatches, and invalid ranges are refused. This is
  structural validation, not a content hash: supply the correct package's
  resources. Mercury Falling needs this for a relocated footstep sample.
- Non-power-of-two 2D bitmaps retain their dimensions in a linear ARGB texture.
  Their declared source mip chain is validated, but only the base level is
  retained because the native linear texture path has no mipmaps. This handles
  Aerowalk's 1920Ã—1080 HUD bitmap.

Judgement's `game_time_authoritative` conversion is supported, but the supplied
Workshop package declares `InheritSharedFiles: FromMCC` and references 687
external bitmap descriptors. It does not bundle the shared resources. The
available Aerowalk and Mercury caches do not contain all required textures;
the loader correctly refuses missing data instead of substituting unrelated
assets. A complete Judgement load requires its matching MCC shared resources.

Regression tests exercise actual MCC handlers, script conversion, snapshot
migration, malformed resources, animation graphs, geometry, textures, and the
sound-mixer connection. Native smoke tests use an isolated Windows data root,
one hidden process at a time, and a console marker after 150 simulation ticks.
These bounded startup checks do not establish complete campaign playthroughs,
audible playback, online co-op, or Android compatibility.
The Windows x86 build and 929 regression checks passed (six optional skips).
Crashsite, DP, ML, Mercury Falling, Mercury 2, and Aerowalk passed bounded
native startup/simulation checks with normal timed exits. Aerowalk and
Mercury used their respective package resources in isolated data roots.
Android could not be built in this local configuration: `ninja android` has
no target. Crashsite advances simulation but also reports the existing
"too many transparent geometry groups" capacity warning; its rendering is
not certified by the successful load check.

## OpenCE upstream integration through 09597313

The October 11 integration includes upstream's optional first-person legs,
shield color/glow, campaign pause spacing, vehicle-home ownership, and stale
player-handle fixes. The visual options retain upstream's stock defaults
(legs/glow off, shield color default). MCC's pause ownership guards and
dedicated loading/conversion modules remain unchanged. Network compatibility
stays at GulchCE's version 28; this integration adds no wire format.

The new `object_mesh` reader needs one MCC connection: larger MCC models have
compressed vertices whose bone indices refer to a per-part palette. Mesh
extraction now resolves that palette through the existing MCC cache API before
the upstream bounds checks. Non-MCC buffers return no palette and retain the
upstream path. An executable regression test covers native indices, a
non-identity MCC palette including bone 40, and invalid secondary indices.

The Windows x86 build, 929 existing regression checks (six skips), the added
mesh test, and upstream's 22 stale-player lifetime checks passed. The upstream
test runner needed local Windows compiler/linker flag adjustments only;
the test bodies and production functions were unchanged. Android remains
unverified because the local Windows build has no `android` target.
Mercury 2 (70 seconds) and DP (65 seconds) passed sequential hidden native
startup/simulation checks with legs, shield glow/color and the viewmodel
shield enabled. Both produced the delayed simulation marker and exited
normally without detected assertions. These are bounded checks, not full
campaign, visual-fidelity, or live co-op verification.

## Public format evidence

These are wire-format references, not copied implementation code. Current
version-13 fields were checked against the supplied file where available;
older CEA documentation sometimes describes different compression and
memory limits and must not be applied indiscriminately.

| Fact | Primary source / observed check |
| --- | --- |
| Version 13, 40-byte PC tag header, 64 MiB tag capacity | [Invader map definitions](https://github.com/SnowyMouse/invader/blob/master/include/invader/hek/map.hpp), [engine configuration](https://github.com/SnowyMouse/invader/blob/master/src/hek/map.cpp); fixture tag pointer `0x50000028`. |
| Infer linked base from tag array; current v13 is uncompressed | [Invader map reader](https://github.com/SnowyMouse/invader/blob/master/src/map/map.cpp); header and index read directly from fixture. |
| MCC BSP external environment/lightmap vertex streams | [Invader BSP compiler](https://github.com/SnowyMouse/invader/blob/master/src/tag/parser/compile/scenario_structure_bsp.cpp), [build layout](https://github.com/SnowyMouse/invader/blob/master/src/build/build_workload.cpp), [BSP schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/scenario_structure_bsp.json); fixture header has `0x2B5D0C` bytes at file offset `0x800`. |
| BC7 format 18, environment flag bit 9 and external resource flag bit 8 | [Invader bitmap schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/bitmap.json); fixture contains 38 BC7 records and environment-flagged bitmaps. |
| MCC script parameters, expanded syntax capacity | [Invader scenario schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/scenario.json), [MCC editing kit change documentation](https://c20.reclaimers.net/h1/h1-ek/); fixture syntax is `56 + 32767 * 20` bytes. |
| MCC Halo 1 syntax limit is 32,767 | [Invader engine configuration](https://github.com/SnowyMouse/invader/blob/696830ff80af227e84e7237c2ef26eb2301ed110/src/hek/map.cpp#L94) uses `INT16_MAX`; all ten Ruby map headers reserve exactly this count. |
| Mission-segment boolean/string contract | [Sapien's generated function documentation](https://github.com/Sigmmma/c20/blob/master/src/data/hs_docs/h1/hs_doc_sapien.txt) and [shipped MCC c10 scripts](https://github.com/NervyDestroyer/Halo-MCC-Scripts/blob/main/H1/levels/c10/scripts/mission_c10.hsc); Ruby's 225 calls also take one string, with boolean conditions used before `sleep`. |
| Parameter wire records and local references | [Scenario schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/scenario.json) defines 36-byte records, maximum 16 and local-variable flag bit 4; [public script compiler](https://github.com/SnowyMouse/invader/blob/master/src/tag/parser/compile/scenario/pre_compile.cpp) confirms primitive/global/local flags and the parameter slot in node data. |
| Sound compression formats and cached sound data | [Invader sound compiler](https://github.com/SnowyMouse/invader/blob/master/src/tag/parser/compile/sound.cpp), existing Xbox sound definitions; fixture samples and tag fields checked directly. |
| MCC HUD canvas and independent bitmap half-scale | [Restored tagset author's format notes](https://github.com/Aerocatia/halopc-restored#hud-scale-is-still-480p); Ruby shield offsets and number advances are exactly twice their stock Xbox counterparts. |
| Xbox-order model and HUD meter flags | [Invader model shader schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/shader_model.json), [HUD meter schema](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/hud_interface_types.json); Ruby and Mercury contain mixed flag states. |
| Empty Vorbis residue vectors contribute zero | [Xiph Vorbis 1.3.7 codebook implementation](https://github.com/xiph/vorbis/blob/v1.3.7/lib/codebook.c), `vorbis_book_decodevs_add`, `vorbis_book_decodev_add`, `vorbis_book_decodevv_add`; synthetic multi-pass streams and independent Ruby PCM comparison. |
| Header and historical format differences | [Reclaimers map documentation](https://c20.reclaimers.net/h1/maps/), [SnowyMouse CEA format research](https://gist.github.com/SnowyMouse/39168bddd597549038a35d78aee39513); historical chunk compression is outside this implementation's scope. |

### Gameplay glyph and HUD placement follow-up

Gameplay A/B/X/Y use the embedded OpenCE redraws with compact text-fragment
spacing. The modern shoulder labels use Xbox white (LB) and black (RB),
matching `xinput_sdl.c`; the two neutral variants reuse the button redraw's
oval. Other controls and custom pickup artwork retain their existing path.
Pause menus are unchanged. All replacements are selected by controller-entry
identity, never by a shared sprite sequence number.

A50_Revisited mixes legacy high-resolution placements with unflagged visor
and number placements. Halving every anchor offset compressed the ammo
layout, moved the flashlight meter toward the centre, and overlapped the
shield panel with the visor. Preserving the flagged offsets corrects those
relationships without map-name checks or changing the Xbox/CE renderer.
MCC anchors also use a 2.5% title-safe inset, matching the supplied reference,
rather than Xbox's 7.5%; this keeps the flashlight above the bottom visor.
Centre-anchored crosshairs and the inner split-screen seam retain their
existing calculations.
The supplied [Workshop reference](https://images.steamusercontent.com/ugc/2013710564213466041/A0A23236FCCA6F4C077C15F6428B257FED2FEF8A/)
and a hidden native-renderer capture were used to compare the visor join and
ammo layout. A separate bounded capture enabled the flashlight and verified
its meter above the bottom visor. Regression cases cover split-screen seams,
zoom inset preservation, controller mappings, and neutral Xbox/CE dispatch.
