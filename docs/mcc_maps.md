# Halo 1 MCC custom maps

This implementation adds an independent loader for modern Halo 1 MCC version-13
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
```

The level identity is `mcc_maps\a10`. The filename is retained even when
the internal scenario name differs. The supplied Mercury Rising file is
an example: its filename is `a10.map`, while its header names
`mercury_falling`. The namespaces prevent it replacing either Xbox `a10`
or Custom Edition `custom_maps\a10`.

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
downloads and MCC package manifests are not implemented. Supported maps
contain their own resources; do not copy `bitmaps.map`,
`sounds.map` or `loc.map` from Custom Edition into `mcc_maps`.
There is no dependency on the Custom Edition setting or its resource
directories.

The loader opens source maps read-only. A rejected map reports the failing
stage in `debug.txt` and returns a menu error. Matching the header checksum
can detect that a network host selected a different map version; it is not
a recomputation or authentication of the map's contents. Network support
uses OpenCE's game protocol, not the MCC game client or its matchmaking.
This contribution uses network protocol 26 for the additional MCC inventory
message and grenade action values. Every participant needs a matching build
and map data; older builds are rejected even when playing non-MCC maps.

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

Multiplayer additionally offers **Settings** when `display.menus=pc` and
there is one local player. It opens the trusted native player Settings
screen for the initiating controller. Split-screen players use Settings
from the main menu, since the native editor needs a full viewport.
Campaign and cooperative pause menus do not offer Settings. Restart,
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

The existing OpenCE engine remains authoritative. `source/` contains the reconstructed
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
| `port/linux/game/mcc_vorbis.c` | Private namespaced Vorbis decoder, bounded Ogg validation and empty residue vector compatibility. |
| `port/linux/game/mcc_bitmaps.c` | MCC pixel layout, BC7 decoding and shader/HUD channel normalization into an MCC virtual stream. |
| `port/linux/game/mcc_texture_cache.c` and `port/linux/src/mcc_texture_bridge.c` | Direct bindings of checked MCC pixel allocations and lazy GPU textures, independent of the Xbox/CE staging cache. |
| `port/linux/game/mcc_tags.c` | MCC metadata, shader-type, HUD placement and widget normalization. |
| `port/linux/game/mcc_hud.c` | MCC bitmap/placement scaling, including nested weapon and grenade HUD items. |
| `port/linux/game/mcc_hud_draw.c` | MCC canvas, glyph geometry and text advances; neutral dispatch for Xbox/CE. |
| `port/linux/game/mcc_scripts.c` | Name-based MCC function/global linking and supported MCC native functions. |
| `port/linux/game/mcc_syntax.c` | MCC's 32,767-slot syntax validation and traversal workspace. |
| `port/linux/game/mcc_script_parameters.c` and `mcc_script_runtime.inl` | MCC parameter metadata/scopes and interpreter frames inside the existing HS stack. |
| `port/linux/game/mcc_objects.c` | MCC multiplayer vehicle placement masks. |
| `port/linux/game/mcc_grenades.c` | MCC slots 2/3, salted unit inventories, pickup/throw/drop behavior and HUD. |
| `port/linux/game/mcc_player.c` | MCC grenade selection/action checks and starting-profile counts. |
| `port/linux/game/mcc_network.c` | MCC-only host inventory messages and client reconciliation. |
| `port/linux/game/mcc_checkpoint.c` | MCC inventory snapshot inside proven-unused CPU save memory. |
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

MCC saves put the additional counts into a 65,640-byte slot at the CPU
arena's end only if the allocator's current high-water mark leaves that
entire slot unused. No `game_state_malloc` call, pool capacity, native
allocation checksum or save-image size changes. Native checkpoint, core
and persistent writers already serialize the complete CPU/GPU arena;
the persistent image checksum therefore includes this snapshot. An MCC
footer adds version, canonical map identity, cache checksum and payload
CRC32. Before accepting the image, the adapter checks the payload and
remaps incoming object-header pointers to verify every salted unit record.
Unload restores the original unused bytes, including nonzero contents.

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

MCC HUD placements use a 960p canvas, converted to the native 480p canvas
in MCC-owned tags. Bitmap half-scale flags are a separate factor, so high
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

The audio adapter accepts embedded PCM16, Xbox ADPCM and Ogg Vorbis
permutations. It uses independent bounds checks and codec state. Mono
output uses the game's 22,050 Hz format; stereo output follows the admitted
22,050/44,100 Hz tag rate. Necessary conversions produce an MCC-only ADPCM
stream. Unsupported or damaged audio rejects the map instead of silently
removing sound.

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

## Current boundaries

- Modern uncompressed version-13 maps are the target. Earlier Anniversary
  chunk-compressed caches, Halo 2 or later MCC maps, external indexed tags,
  and shared external bitmap/sound resource files are not supported.
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
- Non-volume bitmap dimensions are limited to 4,096 by 4,096 with depth one;
  volumes are limited to 512 by 512 by 256. There are at most 12
  mip levels after the base level. Individual raw/decoded allocations are
  capped at 128 MiB; the normalized bitmap stream is smaller than 512 MiB.
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
  Native Settings is available only in MCC multiplayer with
  `display.menus=pc` and one local player.
  HUD placement normalization and overlay handling
  still require visual comparison against the map's intended appearance.
- No claim is made here of complete campaign/gameplay fidelity or
  synchronization under all conditions, performance parity, or tested
  Android/Linux runtime behavior. See the bounded test status below.

## Validation and contribution scope

The contribution includes the MCC runtime modules, additive engine/menu
connections, the menu generator and its generated map-category strings,
the licensed texture decoder, and focused tests and inspection tools.
The MCC validation workflow runs synthetic-data checks without downloading
game assets. Existing build/release workflows and updater settings are not
replaced. No map, commercial font, save, binary or local playtest directory
is part of the contribution.

The compatibility layer is independently written, with the generic library
reuse described above. Research included public schemas and documentation,
existing OpenCE interfaces, and inspection of user-supplied maps. Shipped MCC
campaign scripts were consulted to understand the mission-segment call's
behavior (linked below); this is not a claim of formal clean-room certification
or a legal conclusion about the project's distribution.

The following checks were performed during development of the implementation:

| Area | Evidence and limits |
| --- | --- |
| Map formats | Mercury Rising and Nitra v2 pass parser and geometry checks. All ten Ruby's Rebalanced campaign caches were inspected for script capacity and audio compatibility. |
| Audio | 50,186 Vorbis references / 8,325 distinct streams from Ruby's ten maps decoded; PCM differed from the independent libvorbis comparison by at most one signed-16-bit sample unit. This does not prove an entire campaign plays correctly. |
| Windows gameplay | Bounded rendered loads, pause actions, checkpoint/core restoration, MCC-to-Xbox-to-CE-to-MCC transitions, Nitra multiplayer hits, and two-instance Mercury network co-op were exercised. These are not full campaign playthroughs or broad internet/cross-platform tests. |
| Platform builds | Windows, Linux and Android Debug/Release builds of the precursor implementation passed CI. Linux/Android gameplay was not established, and those runs predate this contribution's protocol-version and packaging changes. |
| Regression coverage | Generated cache data, malformed resources, codecs, HUD/shader conversions, menus, saved games, scripting, network messages and MCC unload/isolation have focused tests. Real-map checks are opt-in. |

Use a C compiler and Python/pytest for the tests below. The Windows CI job
sets up an x86 MSVC environment with Clang; Linux needs Clang and a 32-bit
C runtime (`gcc-multilib`). The existing `tools/harness` suite also checks
legacy engine behavior. A passing parser or build alone does not establish
map playability.

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
python -m pytest -q tools/test_mcc_lifecycle.py
```

Set `MCC_TEST_MAP` to an existing map path to opt into the real-fixture
checks. The parser tests are portable; the geometry harness currently
requires the x86 Windows game headers/toolchain and skips elsewhere.
`port/tools/mcc_cache_report.c` builds with `mcc_cache_format.c` and only
the C standard library, so files can also be audited outside the game.
None of these commands downloads or bundles a game map.

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
