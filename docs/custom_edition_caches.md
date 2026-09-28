# Halo Custom Edition and OpenSauce caches in the native builds

Experimental work on branch `experimental/custom-edition-yelo-loading`,
begun on `bnunu/halo-ce-universal` `main` at `f2fa457f`, and merged since
with that `main` at `223fa93f` and with `cybersecurity/halo-ce-universal`
`main` at `cd47170f`. It teaches the native builds (Windows, Linux,
Android) to recognize, load and, with the `game.custom_edition` setting
on, run Halo Custom Edition caches (`.map`, cache
version 609), OpenSauce caches (`.yelo`, and `.map` files with an OpenSauce
header), with the Custom Edition resource maps `bitmaps.map`, `sounds.map`
and `loc.map`. The byte-matching build is not affected: every game-source
change is under `#ifdef HALO_LINUX`, and all 621 matching objects compile to
the same bytes as before (see [Verification](#verification)).

**Three maps have been seen running, with limits.** In the Windows debug
build, the stock Custom Edition `bloodgulch.map`, the OpenSauce
`beavercreek_halo3.yelo` and the custom `hugeass.map` load, draw their
levels, models, sky and HUD with their textures and spawn a player:
`bloodgulch.map` in slayer, `beavercreek_halo3.yelo` in slayer, capture the
flag, king of the hill and oddball, `hugeass.map` (which runs scripts) in
slayer and capture the flag. Custom Edition's tags were made for Halo PC's renderer, and this
build draws with the Xbox's: where Halo PC wants data the Xbox does not (a
channel order, a bitmap resolution, a text placeholder), the data is
converted, and what was found of that so far is below; what is left is under
[Limits](#limits-and-remaining-work). That is what was observed, from
screenshots and `debug.txt`; [Tested](#tested) lists what was not (nobody
played either map). Running one map says nothing about the next: Custom
Edition maps can use features this build does not have, and OpenSauce ones
usually do.

## What "support" means

| Milestone | What it requires | Status |
| --- | --- | --- |
| **1. Recognize** | Tell the format and variant from the header alone, check every header field against the file, and name what the map needs: its resource maps, an OpenSauce mod set, memory upgrades | **Done**, in the loader, the report tool and the game (which, by default, names the format and refuses the map) |
| **2. Load** | Put the map's tag data where its pointers expect it (`0x40440000`), read the tags kept in resource maps and relocate their pointers, and check every tag instance, name and address, every structure BSP (file range, header, lightmap materials), every model part's geometry, every bitmap's pixels and every sound's samples (range in their file), and the header checksum | **Done** in the loader, the report tool (`cache_file_report`) and the game, with the limits under [Assumptions](#assumptions-not-verified) |
| **3. Run** | The game starts the map, draws it, plays its sounds and runs its scripts | **Reached for `bloodgulch.map`, `beavercreek_halo3.yelo` and `hugeass.map`, as far as observed** (below); not established for any other map, and not for Ogg Vorbis sounds or OpenSauce's own features |

## Running a map

Custom Edition maps run only when the `game.custom_edition` setting is on:
`custom_edition = true` under `[game]` in `config.toml`, which the desktop
builds keep next to the executable and write with every setting and its
default the first time they run (upstream's settings file,
`port/linux/src/port_config.c`), or, for one run, the environment variable
`HALO_CUSTOM_EDITION` set to anything. Without it they are refused as
before. The Android build's `config.toml` does not list the setting: that
build was never built with this work (below). Put the map and the stock
resource maps in the data root's `maps`
folder (an OpenSauce map built with a mod set needs its
`maps\data_files\<mod>-bitmaps.map` and so on instead), and start a
multiplayer map without the menus from `init.txt` in the data root:

```
game_variant slayer
map_name levels\test\bloodgulch\bloodgulch
```

`map_name` takes the scenario's tag path; only its last part names the file,
`bloodgulch.map`, or `bloodgulch.yelo` when there is no `.map`. A
multiplayer scenario needs a game variant, or no starting location
qualifies and no player spawns (`match_game_type` in `game_engine.c`
accepts none of a map's typed starting locations without a game engine).
`beavercreek_halo3.yelo`'s scenario is
`zteam\scenarios\multi_player\beavercreek\beavercreek_halo3`. The score
shows while the Xbox's BACK button is held: F1 on the keyboard, or the
controller's Back button.

## What the native builds do

- **With the setting off** (the default), a Custom Edition cache in
  `maps\` is named and refused, instead of being rejected as "an old
  version" of this build's caches:

  ```
  'd:\maps\ui.map' is a Halo Custom Edition cache (build 01.00.00.0609): this build recognizes it but cannot run it (docs/custom_edition_caches.md)
  ```

  The message is logged to `debug.txt` in every build, and debug builds stop
  on it as the original check stops on a bad cache
  (`port/linux/game/custom_edition_cache.c`, called from
  `cache_file_header_verify` in `source/cache/cache_files.c`).
- **An OpenSauce `.yelo` file** is found when there is no `.map` of that name,
  as OpenSauce looks for one (`cache_file_get_map_path` in
  `source/cache/cache_files_windows.c`), and refused the same way.
- **With the setting on**, the platform reserves the address window Custom
  Edition tag data is linked to, `0x40440000`–`0x426C0000`
  (`port/linux/src/xbox_memory.c`, which reads the setting at start-up,
  before anything else can map into the window; 36 MB of address space,
  backed as it is touched), and a Custom Edition map is loaded into it,
  converted as below, and run. It is read in place: it is never copied to
  the cache partition, and every read the game makes of it (structure BSPs,
  bitmap pixels, sound samples) is served from the map, `bitmaps.map` or
  `sounds.map` according to where the offset falls in their combined offset
  space (`custom_edition_cache_read`). A map that fails any check is logged
  and not loaded.
- **Xbox caches** take exactly the path they took before: the new code
  returns at once for them, and the original checks and messages follow
  unchanged.

## Loading and conversion

### The loader (`cache_file_formats.c`)

`port/linux/game/cache_file_formats.c` (`cache_file_formats.h`) holds the
format code: standalone C with fixed-width types and no game dependencies,
compiled into the native builds (as every `port/linux/game` unit is) and
into the report tool. Map files are untrusted input: every field is read
little-endian from a bounds-checked buffer, never through a structure laid
over the file, and every offset, count and size is checked before use, with
overflow-safe arithmetic. `port/tools/cache_file_report.c` reports what can
be done with each file:

```sh
clang --target=i686-pc-windows-msvc -fuse-ld=lld -Iport/linux/game \
    port/linux/game/cache_file_formats.c port/tools/cache_file_report.c -o cache_file_report.exe
cache_file_report [--maps DIRECTORY] [--stock-data-files] [--dump-tags FILE] FILE...
```

Resource maps are looked for next to the cache (or in `--maps`):
`bitmaps.map`, `sounds.map`, `loc.map`, or `data_files\<mod>-bitmaps.map` and
so on for an OpenSauce cache built with a mod set. `--stock-data-files` loads
a mod-set cache with the stock resource maps instead, for inspection only
(OpenSauce itself refuses such a map). `--dump-tags` writes the converted
tags to a file, as they would sit at `0x40440000`.

Loading, step by step (`custom_edition_cache_load`):

1. The header: signatures, version 609, terminated strings, file length
   within the file and the size limit (`0x18000000`, or `0x24000000` with
   OpenSauce memory upgrades), no compression, tag data within the file and
   the tag cache (23 MB, or 1.5 times that with memory upgrades). An
   OpenSauce header at offset `0x70` is checked as OpenSauce checks it:
   header version 1 or 2, `project_yellow` and `project_yellow_globals` tag
   versions 2, memory upgrade factor at most 1.5, no undefined flags, a mod
   name when the mod-set flag is set, and its tag definitions within the file.
2. The tag data is read to the start of the tag cache, which stands for
   `0x40440000`.
3. The tag index (`tags` signature), every tag instance (its handle must
   match its position, its name must lie in the tag data and be terminated,
   its address must lie in the tag data; only structure BSPs may have none;
   only bitmaps, sounds, fonts, unicode string lists and HUD message text may
   be held by resource maps), the model data range, and the scenario tag.
4. Every structure BSP: its tag, its range in the file, its place in the tag
   cache (above the tag data, within the tag cache), its header (`sbsp`
   signature, no Xbox vertex buffers, its structure pointer inside it), and
   its lightmaps and their materials: every block within the BSP and aligned,
   every material's uncompressed environment vertices (56 bytes) and
   lightmap vertices (20 bytes) within the BSP and of the size their counts
   give, lightmap vertices exactly when the lightmap has a bitmap, and every
   normal, binormal, tangent and incident radiosity within ±1.005 (the game's
   vertex compressor asserts on anything further out).
5. The header checksum is computed as OpenSauce computes it, before anything
   changes the tag data; a mismatch is reported, not refused.
6. The tags held by resource maps are read into the tag cache after the tag
   data, below the lowest structure BSP, and their pointers relocated. A
   sound's header in the map lacks its sample rate, encoding, compression
   and longest permutation length; they are taken from the header's copy in
   `sounds.map` ([Evidence](#observed-not-stated-by-the-sources-established-on-the-sample-maps)).
7. Every bitmap, sound, font, unicode string list, HUD message text, model
   (`mod2`) and transparent chicago shader (`schi`, `scex`) is walked through
   its blocks: every block and data pointer must lie within the loaded tags,
   every bitmap's pixels and sound's samples within their file, and every
   model part's strip and vertices within the model data, of the kinds Custom
   Edition writes (a precompiled strip, uncompressed 68-byte vertices), with
   at most 65535 vertices and at most 22 local nodes.

Then `custom_edition_cache_convert` changes what only needs its bytes
changed:

- **Shader types.** Custom Edition inserted *transparent chicago extended*
  as shader type 7, so its water, glass, meter and plasma shaders are types
  8 to 11 where this build has 7 to 10 (and would draw a Custom Edition glass
  shader as meter, and so on, unnoticed). Every shader gets this build's
  number; a shader whose type is not its group's is refused.
- **Transparent chicago extended shaders** (`scex`) become transparent
  chicago shaders (`schi`): the two layouts agree up to the maps, of which
  `scex` has two sets, for four and for two texture stages; the four-stage
  maps are kept (this build draws four stages), or the two-stage ones when
  there are no others, and the extra flags move to where `schi` has them.
- **Bitmaps and sound permutations** get the state of ones not yet drawn or
  played, and name their own tags: `bitmaps.map` and `sounds.map` hold the
  tag handles of whatever map they were built with.
- **Sounds this build cannot decode** (Custom Edition's Ogg Vorbis) are made
  unplayable, by emptying their pitch ranges, which the game skips.
- **OpenSauce's script nodes.** OpenSauce's memory upgrades make room for
  28501 script syntax nodes instead of 19001, and OpenSauce patches the game
  to accept that. This build takes the scenario's nodes only at its own
  number, so an upgraded array whose nodes in use fit in 19001 is made that
  size; one that uses more is refused.
- **Animation overlays naming animations that do not exist** are made to
  name none, which the game skips. `beavercreek_halo3.yelo` has two; Custom
  Edition reads past the graph's animations there, and this build's debug
  builds stop on it.
- **HUD elements drawn from double-resolution bitmaps.** Halo PC added a
  third scaling flag to HUD placements, *use high resolution scale*
  (OpenSauce `hud_definitions.hpp`), and draws a flagged element at half the
  size of its bitmap. Custom Edition's HUD bitmaps are made for that: in
  `bloodgulch.map` every flagged element draws a bitmap exactly twice the
  size of the one its Xbox counterpart draws (256×64 for 128×32, 512×512 for
  256×256), with the same scale, and every unflagged one a bitmap of the same
  size ([Evidence](#observed-not-stated-by-the-sources-established-on-the-sample-maps)).
  This build ignores the flag, so it drew them twice too large; the
  placements of the unit, weapon and grenade HUD interfaces and the HUD
  globals' messages that have it get half their scale, and lose the flag (32
  in `bloodgulch.map`, 14 in `beavercreek_halo3.yelo`).
- **The score hint.** String 100 of `ui\multiplayer_game_text` is Halo PC's
  `Hold "%s" for score`, which Halo PC fills in with its score key; this
  build copies it as it is (`game_engine.c`, the press-back-for-score
  message), so `"%s"` becomes `BACK`, the button this build reads for the
  score, in the same four characters.

### In the game (`custom_edition_cache.c`, `custom_edition_geometry.c`, `custom_edition_bitmaps.c`)

- **Models.** Every `mod2` part is given this build's part layout (the first
  0x68 bytes of the 0x84-byte Custom Edition part are the same fields; the
  parts are repacked in place), its vertices are compressed from the map's
  model data by the game's own `rasterizer_geometry_compress_vertices`, its
  node indices made the model's when its model's parts have local nodes, and
  its strip copied unchanged; the game's own `rasterizer_vertex_buffer_new`
  and `rasterizer_triangle_buffer_new` make its buffers, and the tags become
  `mode` tags. Before any of that, every index a part holds is checked
  against what it names (shaders, nodes, parts, vertices), and a model with
  44 nodes or more is refused, since this build's renderer skins at most 43.
- **Structure BSPs.** When the game loads a BSP, every material's vertices
  are compressed the same way (environment and lightmap vertices), and the
  material gets buffers and the compressed vertices the game reads for
  visibility, collision and lights (`scenario_structure_bsp_load` in
  `cache_files.c` calls `custom_edition_structure_bsp_load`).
- **Bitmaps.** Every bitmap must pass the game's own `bitmap_verify` and be
  of a kind the texture cache and the swizzling code handle (a format with a
  hardware texture, a compressed flag that agrees with the format,
  power-of-two sizes unless linear, square cube maps, all its pixels
  present). Its pixels are laid out for Halo PC, levels as the tag stores
  them, unswizzled; as they arrive, the game's own
  `rasterizer_xbox_bitmap_rebuild_hardware_format` lays them out as an Xbox
  cache would (swizzled, cube maps face by face, padded).
- **Chicago extra layers.** January's transparent chicago shader draws its
  extra layers in a loop that never advances
  (`rasterizer_xbox_transparent_geometry.c`, marked as a preserved bug). No
  Xbox map has such layers, so January never hung on it, but Custom Edition
  maps do (`beavercreek_halo3.yelo` has two shaders with them): the native
  builds advance the loop.
- **Channel orders.** Halo PC keeps two kinds of texture in other channels
  than this build reads them from (`enum custom_edition_channel_order`,
  [Evidence](#observed-not-stated-by-the-sources-established-on-the-sample-maps)):
  a model shader's *multipurpose map*, whose auxiliary (detail) mask,
  self-illumination, specular and color change masks Halo PC keeps in red,
  green, blue and alpha, where this build's model shaders read specular from
  red, self-illumination from green, color change from blue and the
  auxiliary mask from alpha; and a *HUD meter*, whose shape Halo PC keeps in
  color and fill order in alpha, where this build's meter shader
  (`rasterizer_xbox_dynavobgeom.c`) compares the color with the meter's value
  and discards what has no alpha. Reordering the pixels would mean
  decompressing them, which the texture cache has no room for, so the game
  finds the bitmaps drawn as multipurpose maps (by model shaders) and as
  meters (by unit and weapon HUD interfaces) when the map loads, and tells
  the renderer where each bitmap's pixels arrive and in which order their
  channels are; the renderer samples them with each channel taken from where
  Halo PC keeps it (`port/linux/src/xbox_textures.c`, a texture swizzle,
  which leaves them compressed). A bitmap also drawn another way (a
  multipurpose map that is also a base map, say) keeps its channels, since
  the renderer has one order for each texture, and is logged: 3 in
  `bloodgulch.map`, 4 in `beavercreek_halo3.yelo`, all multipurpose maps.
- **Reads.** The renderer write-protects the memory it has made textures of
  and learns of changes from the faults writes take; the kernel fails a read
  into such memory instead. Every read of the map is therefore made into a
  64 KB staging buffer and copied, as the platform's file layer does.
- **Texture memory.** A frame of `beavercreek_halo3.yelo` draws more than the
  22 MB of textures the Xbox texture cache holds, and textures that did not
  fit were drawn as the default one. The native builds' texture cache is
  twice the Xbox's (`halo_port_capacity.h`), for every map; with that map
  loaded, 52 MB of the 128 MB memory window were still free.
- **Scripts.** A compiled script names each function it calls, and each
  engine global it uses, by its index in the engine's tables, and Halo PC's
  tables have entries this build's do not, in among the ones both have: from
  some point on, an index names another entry here
  ([Evidence](#observed-not-stated-by-the-sources-established-on-the-sample-maps)).
  `hugeass.map`'s day and night script would call `playback` instead of
  `switch_bsp`. A compiled script also keeps every name, in its string data,
  so every call and every engine global is found again by name with the
  game's own `hs_find_function_by_name` and `hs_find_global_by_name`
  (`custom_edition_scripts.c`); a map whose scripts use one this build does
  not have is refused and the names logged. The value types are numbered
  alike in both builds.
- **Multiplayer vehicle placement.** This build places a multiplayer game's
  vehicles by type: only the first three of the globals' multiplayer
  vehicles are created, and of those the ones the variant's vehicle set
  names (`game_engine_remap_vehicle`; the built-in variants name the first).
  Custom Edition maps are made for retail Halo's rules: a vehicle placement's
  multiplayer spawn flags name the game types it is placed in by default,
  and a script may create any vehicle. `hugeass.map` depends on that (above).
  For a Custom Edition map in slayer, capture the flag, king of the hill or
  oddball, the native builds place the vehicles whose spawn flags name the
  game type by default and let any vehicle be created
  (`custom_edition_objects.c`, called from `object_types_place_all` and
  `game_engine_remap_vehicle` under `HALO_LINUX`); a variant without
  vehicles still has none, and race, which has no spawn flag, keeps this
  build's rule.
- **Multiplayer vehicles.** `game_engine_predict_resources` takes the three
  multiplayer vehicles Xbox globals always have; `beavercreek_halo3.yelo` has
  one, and oddball stopped on it. With fewer than three, the native builds
  predict none (`game_engine.c`, under `HALO_LINUX`).

## Tested

### Automated tests

`python -m pytest tools/test_cache_file_formats.py` (131 tests; needs clang).
The tests build complete synthetic caches and resource maps in memory (no
game data is stored in the repository) and run the report tool, compiled with
`-Wall -Wextra -Wpedantic -Werror` and undefined-behaviour trapping
(`-fsanitize=undefined -fsanitize-trap=undefined`), so an undefined operation
fails the test that triggered it. The module is also compiled as the game
compiles it (`-std=gnu89`) with warnings enabled, since the game build
silences them.

- Supported formats: a complete Custom Edition cache with every kind of
  resource-held tag; several pitch ranges; three structure BSPs; a model;
  a structure BSP material with and without a lightmap; OpenSauce caches with
  memory upgrades, appended tag definitions, OpenSauce tags, a mod set
  present, absent, and substituted; resource maps of each type; the `--maps`
  option.
- Conversion: every shader group's type; chicago extended shaders with and
  without four-stage maps; a shader with another group's type (refused);
  bitmaps and sound permutations naming their own tags; sound header fields
  taken from `sounds.map`; an Ogg Vorbis sound made unplayable; upgraded
  script nodes reduced, stock ones kept, too many refused; animation overlays
  kept and disabled; HUD placements with the high resolution scale halved
  (in a weapon HUD's statics and crosshair items) and ones without it kept;
  the score hint made to name BACK, and a placeholder left alone in another
  string list and in another string.
- Malformed input: every check of the loader and the conversion, with at
  least one case each (94 tests: header fields, OpenSauce fields, tag index, instances, names,
  addresses, scenario, structure BSP block, range, header and materials,
  model parts, resource map headers and entries, resource layouts, pixel and
  sample ranges, font style references, tag cache overflow, shader types,
  script nodes), plus 120 seeded
  random corruptions of a valid cache and its resource maps, each of which
  must end in a clean verdict. The random corruptions are sampled evidence
  of robustness, not proof.
- Original format: Xbox caches of build 01.01.14.2342 and 01.10.12.2276, and
  a compressed Xbox cache, are recognized and left to the original loader;
  other cache versions and unrelated files are named, not loaded.
- Real maps: when `HALO_CUSTOM_EDITION_MAPS` names a folder of maps, or they
  are extracted to the gitignored `assets/custom_edition`, four more tests
  check the results in the tables below.

The game-side conversion (models, structure BSPs, bitmap pixels) uses the
game's own functions and was tested by running the game (below); it has no
automated test. `python -m pytest tools/test_custom_edition_tag_footprints.py`
(4 tests) checks the tag comparison tool of
[the measurements below](#how-far-custom-edition-tags-are-from-the-xbox-tags).

### Maps

The sample supplied for this work (`custom_edition.zip`, 28 entries): the 20
stock Custom Edition maps, the three stock resource maps, three `.yelo` maps
and one custom `.map` with an OpenSauce header. Results of
`cache_file_report` on each (resource-held tags are counted as bitmaps /
sounds / loc; "ranges" are bitmap pixel ranges / sound sample ranges
checked):

| File | Size | OpenSauce | Tag cache | Tags | Resource tags | Ranges | Pointers relocated | Checksum | Load |
|---|---|---|---|---|---|---|---|---|---|
| `beavercreek.map` | 12.5 MB | no | 0x1700000 | 2400 | 436/334/104 | 659 / 948 | 3191 | matches | ok |
| `bloodgulch.map` | 13.3 MB | no | 0x1700000 | 2455 | 448/333/104 | 676 / 951 | 3213 | matches | ok |
| `boardingaction.map` | 13.3 MB | no | 0x1700000 | 2383 | 425/335/104 | 661 / 987 | 3171 | matches | ok |
| `carousel.map` | 12.1 MB | no | 0x1700000 | 2362 | 415/334/104 | 653 / 946 | 3149 | matches | ok |
| `chillout.map` | 12.1 MB | no | 0x1700000 | 2348 | 413/334/104 | 635 / 956 | 3145 | matches | ok |
| `damnation.map` | 13.4 MB | no | 0x1700000 | 2408 | 435/337/104 | 660 / 1020 | 3200 | matches | ok |
| `dangercanyon.map` | 14.5 MB | no | 0x1700000 | 2483 | 461/340/104 | 696 / 981 | 3253 | matches | ok |
| `deathisland.map` | 17.3 MB | no | 0x1700000 | 2547 | 481/340/104 | 720 / 1014 | 3299 | matches | ok |
| `gephyrophobia.map` | 14.9 MB | no | 0x1700000 | 2427 | 452/336/104 | 683 / 992 | 3227 | matches | ok |
| `hangemhigh.map` | 12.1 MB | no | 0x1700000 | 2332 | 410/332/104 | 623 / 957 | 3135 | matches | ok |
| `icefields.map` | 15.1 MB | no | 0x1700000 | 2415 | 435/334/104 | 684 / 988 | 3189 | matches | ok |
| `infinity.map` | 17.5 MB | no | 0x1700000 | 2466 | 452/335/104 | 678 / 989 | 3226 | matches | ok |
| `longest.map` | 12.0 MB | no | 0x1700000 | 2343 | 416/332/104 | 628 / 951 | 3147 | matches | ok |
| `prisoner.map` | 12.6 MB | no | 0x1700000 | 2333 | 411/332/104 | 624 / 965 | 3137 | matches | ok |
| `putput.map` | 13.5 MB | no | 0x1700000 | 2322 | 404/334/104 | 624 / 949 | 3127 | matches | ok |
| `ratrace.map` | 12.2 MB | no | 0x1700000 | 2355 | 419/336/104 | 641 / 1007 | 3161 | matches | ok |
| `sidewinder.map` | 14.2 MB | no | 0x1700000 | 2420 | 439/334/104 | 678 / 988 | 3197 | matches | ok |
| `timberland.map` | 14.0 MB | no | 0x1700000 | 2437 | 433/335/104 | 689 / 965 | 3188 | matches | ok |
| `ui.map` | 2.4 MB | no | 0x1700000 | 1412 | 0/0/0 | 450 / 125 | 0 | matches | ok |
| `wizard.map` | 12.1 MB | no | 0x1700000 | 2351 | 408/337/104 | 630 / 948 | 3141 | matches | ok |
| `beavercreek_halo3.yelo` | 237.6 MB | memory upgrades | 0x2280000 | 2644 | 279/194/102 | 1005 / 1286 | 2412 | matches | ok |
| `celer_exile_odst_v2.yelo` | 208.3 MB | memory upgrades | 0x2280000 | 3190 | 332/279/104 | 1219 / 1087 | 2868 | **differs** | ok, with 0x27EF4 unidentified bytes after the OpenSauce tag definitions |
| `fy_killzone.yelo` | 50.2 MB | memory upgrades, mod set `fy_killzone` | 0x2280000 | 2450 | 385/327/104 | 657 / 966 | 3075 | matches | not loaded: mod set absent; with `--stock-data-files`: ok |
| `extinctionrevanepic2.map` | 255.4 MB | memory upgrades, mod set `extinctionhr` | 0x2280000 | 4047 | 361/233/53 | 1360 / 1272 | 2176 | matches | not loaded: mod set absent; with `--stock-data-files`: ok |

`bitmaps.map` (1706 entries), `sounds.map` (752) and `loc.map` (176) are
valid resource maps. Two retail Xbox caches of build 01.10.12.2276 (`ui.map`,
`bloodgulch.map`) were recognized as Xbox caches.

Conversion of the two maps that were run:

| Map | BSP materials | Shaders renumbered | `scex` made `schi` | Bitmaps | Ogg Vorbis sounds silenced | Script nodes reduced | Overlays disabled | Model parts (vertices) |
|---|---|---|---|---|---|---|---|---|
| `bloodgulch.map` | 79 | 21 | 10 | 676 | 39 | no | 0 | 447 (100,945) |
| `beavercreek_halo3.yelo` | 67 | 14 | 4 | 1005 | 10 | yes | 2 | 673 (201,787) |

Of the other maps: `celer_exile_odst_v2.yelo` loads and converts in the
report tool, but the game refuses it for a model of 46 nodes; the two
mod-set maps need their mod sets, which were not available.

### In the game

Built with `ninja windows` (clang 22.1.8, Visual Studio 2026 x86 libraries,
SDL 3.4.16; a debug build) and run with `HALO_CUSTOM_EDITION=1`, a data root
holding one map and the stock resource maps, and the `init.txt` above, for
30 to 60 seconds each, with `HALO_SCREENSHOT_DIR` saving the screen:

| Map | Sound | Observed |
| --- | --- | --- |
| `bloodgulch.map` | off (`HALO_NO_AUDIO`) | loaded and converted as in the table above; 3,900 frames drawn in 35 s with no error: the level, its sky, scenery and base, the first-person arms holding a plasma pistol, and the HUD (health, shield, motion sensor), all textured |
| `bloodgulch.map` | on | the same, plus one `attempt to play a sound that was not a mono 22k compressed sound ...` in 40 s |
| `beavercreek_halo3.yelo` | off | loaded and converted as above; the level, the map's Halo 3 style HUD, and its first-person arms and plasma pistol, all textured, with no error in 40 s |
| `beavercreek_halo3.yelo` | on | the same for 60 s, with no sound refused |

After the owner reported visual bugs, the maps were run again, 20 to 25
seconds each, with the conversions above added one at a time and the frames
compared with the Xbox's data and with a screenshot the owner supplied of
`beavercreek_halo3.yelo` as Custom Edition draws it:

| Map, game | Observed |
| --- | --- |
| `beavercreek_halo3.yelo`, slayer | before the texture memory change, `YOU GOT STABBED` (the texture cache's failure) twice in 25 s and flat grey surfaces where textures did not fit; after it, neither |
| `beavercreek_halo3.yelo`, oddball | stopped in `game_engine_predict_resources` before the multiplayer vehicles guard; runs after it |
| `beavercreek_halo3.yelo`, capture the flag | before the staging buffer, a flood of `cannot read` messages and black patches; after it, none. With every change: the Halo 3 style HUD (grenades, shield meter, ammunition, motion sensor) placed and sized as in the owner's screenshot, and the first-person arms in the team's red, as there |
| `bloodgulch.map`, slayer | the HUD at the Xbox's size (the shield meter, the health meter under it, the ammunition counters and meters, with each round drawn) and `Hold BACK for score` |
| both | multipurpose maps drawn with their masks where this build's shaders read them (66 in `beavercreek_halo3.yelo`, 56 in `bloodgulch.map`), and the ammunition and unit meters (3 and 2) in this build's order |

`hugeass.map` (59 MB, a custom map from 2005, with the stock resource maps)
was run next, at the owner's request: in slayer for 40 and 60 seconds and in
capture the flag for 45. Before the script conversion its scripts ran with
Halo PC's indices; with it, 3 calls (`switch_bsp` twice,
`cinematic_set_title`) and 1 engine global (`rider_ejection`) were given
this build's index, as the offline comparison predicted. The player spawned
in the map's hangar, walked, got into a jet and fired; no error was logged
but the map's own `NETGAME MAP FAILURE: failed to find enough spawn points
for king 2/4` and the silenced sounds' message.

The hangar opened on a starfield, though, where the owner's screenshot of
the map in Custom Edition shows day: two seconds in, the map switched to its
night structure BSP (the log now names each structure BSP loaded by its
lightmap bitmap). Its `bsp` script turns the map to night when the unit
named `die` has no health, and `die` is a biped placed under an oddball-only
jet, which another script recreates every two seconds while it exists: this
build placed that jet in slayer, and the recreated jet killed the biped.
With the multiplayer vehicle placement below, the jet is not placed in
slayer, the map stays in its day structure BSP (40 seconds), and a jet flown
out of the hangar shows the sky, ring and green hills of the owner's
screenshot. A failure after the models are converted used to stop on the
game's `free` of NULL when releasing what was never allocated; that path now
checks first.

Before the sound conversion, the log filled with that message: the map's
copy of every resource-held sound's header says it is uncompressed. The game
checks a sound's format before its pitch ranges (`sound_new_impulse` in
`sound_manager.c`), so a silenced Ogg Vorbis sound still logs the message
when played; that the one left is such a sound is likely but was not
checked. The sound system accepting sounds is what was observed; whether
they are heard right was not checked, on a machine whose audio output could
not be listened to.

### After the merge with upstream

Upstream `main` at `223fa93f` brought a settings file, headers of its own
in place of the Xbox SDK's, a fullscreen mode as wide as the display, and
changes throughout the game sources; `HALO_CUSTOM_EDITION`, which this
branch read directly before, became the `game.custom_edition` setting. The
debug build was then built again and run, windowed, for 15 to 40 seconds
each:

| Run | Observed |
| --- | --- |
| `bloodgulch.map`, slayer, no `config.toml` and no variable | the game wrote `config.toml` with `custom_edition = false` under `[game]`, named the map and refused it, and the debug build stopped on it, as before |
| `bloodgulch.map`, slayer, `custom_edition = true` in that file and no variable | loaded and converted with the counts of the table above (32 HUD elements rescaled, the score hint, 56 multipurpose maps and 2 HUD meters reordered); 3,000 frames in 30 s, the level and HUD drawn as before, and no error but the silenced sounds' message |
| `beavercreek_halo3.yelo`, capture the flag, `HALO_CUSTOM_EDITION=1` | as before the merge: the Halo 3 style HUD and the red team's arms |
| `hugeass.map`, slayer, `HALO_CUSTOM_EDITION=1` | 3 calls and 1 engine global given this build's index; the day structure BSP kept for the 40 s; the hangar, HUD and score hint as before |

The Windows release build (`python tools/ci_build.py windows release`, as
upstream's workflow builds it, with link-time and profile-guided
optimisation) was built too, and ran `hugeass.map` in slayer for 30 seconds
with the same log and the day structure BSP kept.

Upstream `main` at `cd47170f` then brought internet play, a distributed
netcode (the default), an updater, extraction of the game data from a disc
image, deterministic floating-point maths, and native builds that accept
Xbox cache files of any build. The debug build was built again and run,
windowed, with the updater, internet play, joining from the clipboard and
Discord turned off: `bloodgulch.map` in slayer (`custom_edition = true` in
`config.toml`) and `beavercreek_halo3.yelo` in capture the flag for 30
seconds, `hugeass.map` in slayer for 40. The logs report the conversions of
the tables above, the level and HUD were drawn as before, and `hugeass.map`
kept its day structure BSP. Every run was one machine's local game: no
network game, with either netcode, was played on a Custom Edition map.

### Not tested or not observed

- **Playing.** Nobody played: the runs were given no input on purpose, and
  what the player did in them (in later runs it moved, fired and got into
  vehicles) was not directed and its source was not identified. No second
  player joined, and no game was played to its end.
- **Sound output** was not listened to (above).
- **Scripts.** Only `hugeass.map`'s were run, and only as far as a minute of
  play reaches: its day and night switch was not seen to happen. Every call
  and engine global of the other maps' scripts is found by name here too
  (the table below), but what they do was not watched.
- **Fullscreen**, upstream's default since the merge, drawing as wide as the
  display: every run was windowed, at the Xbox's 640x480.
- **Other maps.** Only the three above were run. The stock maps load and
  convert in the report tool with the same code; the game-side conversion
  was not tried on them.
- **The Linux and Android builds** were never run: the machine used has
  no 32-bit Linux sysroot or SDL3 and no Android NDK. Upstream's workflow
  (`.github/workflows/build.yml`) built them for the merged branch
  (`0fe82574`), debug and release, with the Windows builds, and ran
  `tools/test_linux_port.py` on Linux: all passed. Only one release run
  was made (above).
- **OpenSauce itself**: mod sets (none available; the file names are an
  assumption, below), protected caches, OpenSauce compression parameters,
  tag symbol or string id storage, and the runtime features `project_yellow`
  tags drive.

## Evidence

### Sources

- **OpenSauce** (GPL-3.0, Kornner Studios): the `OpenSauce-master.zip`
  archive of the upstream repository (SHA-256
  `f1097c9cf5975a208506c78c3af47d6b893f998a9f47dddf89c9bbcc1f4b7c73`; newest
  entry dated 2016-01-15; `yelo_version.hpp` names OpenSauce 4.0.0).
- **Reclaimer** (GPL-3.0, Gravemind2401): the `Reclaimer-master.zip` archive
  (SHA-256 `72fd5270abbdf662ff07a54e9cce80a6158e479c51ecfdcd979f7f9fbd6c2d03`;
  newest entry dated 2026-09-20).
- **The Xbox maps of retail build 01.10.12.2276** (`bloodgulch.map`,
  decompressed): not the 01.01.14.2342 build this repository reconstructs,
  but their model parts have its 0x68-byte layout and their shader types its
  numbering, which makes them a check on what the conversion produces.

OpenSauce and Reclaimer are GPL-3.0 and this repository is CC0: no code from
either was copied. They were read as documentation of the file formats, and
every layout used was then checked against the sample maps.

### Documented facts (source, checked against the sample maps)

| Fact | Source |
| --- | --- |
| Cache header layout, version 609, `head`/`foot` signatures | OpenSauce `blamlib/Halo1/cache/cache_files_structures.hpp` (`s_cache_header`) |
| Tag index (0x28 bytes, instances at 0x28, model data as file offsets) and tag instance (0x20 bytes, resource-map flag at 0x18) | same file (`s_cache_tag_header`, `s_cache_tag_instance`) |
| OpenSauce header at 0x70 and its fields, versions 1 and 2, validity rules | OpenSauce `YeloLib/Halo1/cache/cache_files_structures_yelo.hpp`, `cache_files_yelo.cpp` (`IsValid`) |
| Tag cache at `0x40440000`, 23 MB; memory upgrades ×1.5; file size limits | OpenSauce `cache_constants.hpp`, `saved_game_constants.hpp`, `blam_memory_upgrades.hpp` |
| Resource map header and entries | OpenSauce `data_file_structures.hpp` |
| Groups held by resource maps: bitmaps, sounds, fonts, unicode string lists, HUD message text | OpenSauce `blamlib/Halo1/cache/cache_files.cpp` (`cache_file_data_load`) |
| Structure BSP reference (0x20), header (0x18), structure, lightmap and material layouts | OpenSauce `scenario_definitions.hpp`, `structure_bsp_definitions.hpp`; this build's `structure_bsp_definitions.h` agrees |
| Bitmap, sequence, sprite, bitmap data layouts; pixels in `bitmaps.map` flag | OpenSauce `bitmaps/bitmap_group.hpp` |
| Sound definition, pitch range, permutation layouts; samples-in-`sounds.map` flag | OpenSauce `sound/sound_definitions.hpp` |
| Gbxmodel layout: the part (0x84 bytes) is this build's 0x68-byte part followed by a local node count and a 22-entry node table | OpenSauce `models/model_definitions.hpp` (`gbxmodel_geometry_part`) |
| Where a part's strip and vertices lie in the model data (triangle count, strip offset, vertex count and offset in the buffer fields; 68-byte vertices) | Reclaimer `Blam/Halo1/GbxmodelTag.cs` (`ReadPCMeshes`) |
| Shader types: *transparent chicago extended* inserted at 7, before water, glass, meter and plasma; its two map sets | OpenSauce `shaders/shader_definitions.hpp` |
| Script syntax nodes: 19001 in the stock engine, ×1.5 with memory upgrades, which OpenSauce patches the engine to accept | OpenSauce `hs_constants.hpp`, `blam_memory_upgrades.hpp`, `Halo1_CE/Game/Scripting.cpp` |
| HUD message text layout | OpenSauce `interface/hud_messaging_definitions.hpp` |
| Font (156 bytes) and unicode string list layouts | BlamLib `Blam/Halo1/Tags/Definitions/Misc.cs`, `Resources.cs` |
| Bitmap tags in `bitmaps.map` by index, pixels by absolute offset | Reclaimer `Blam/Halo1/CacheFile.cs`, `BitmapTag.cs`, `BitmapsAddressTranslator.cs` |
| The header checksum: CRC-32 of the structure BSPs (packed from 0x800), the model data and the tag data, not inverted | OpenSauce `cache_files_yelo.cpp` (`CalculateChecksumFromMemoryMap`), `memory_interface_base.cpp` (`CRC`); reproduces the stored checksum of 23 of the 24 sample caches |
| OpenSauce looks for `<name>.map`, then `<name>.yelo` | OpenSauce `cache_files_yelo.cpp` (`c_map_file_finder`) |

### Observed (not stated by the sources; established on the sample maps)

| Fact | Evidence |
| --- | --- |
| `time_t` in the OpenSauce build info is 64 bits | the build string starts 8 bytes after the timestamp in all 4 OpenSauce caches |
| The OpenSauce tag definitions start at the file length the header declares, and are zlib data | all 4 OpenSauce caches, each decompressed to its declared size |
| Structure BSPs load at the top of the tag cache (`address + size` = `0x41B40000`, or `0x426C0000` with memory upgrades) | all 24 caches |
| Bitmaps, fonts, unicode string lists and HUD text held by resource maps: the tag's address field is the entry index, the entry's name is the tag's path, the entry is the whole tag with addresses counting from its start | 9,550 bitmaps, 68 fonts, 2,248 string lists, 23 HUD texts across the sample |
| Sounds held by `sounds.map`: the map keeps the 0xA4-byte header, whose pitch range block has a count and no address; the entry named by the tag path holds the header again, then the pitch ranges and permutations, whose addresses count from the first pitch range | 7,397 sounds, 21,870 sample ranges |
| The map's copy of a resource-held sound's header has its compression and longest permutation length zero, and its encoding and sample rate zero too; the entry's copy has them as the Xbox has them | in `bloodgulch.map` and `beavercreek_halo3.yelo`, the two copies of all 527 resource-held sounds differ only in those four fields, in pointers, and in the map's own promotion sound reference; for the 315 sounds the Xbox `bloodgulch.map` shares, the entry's sample rate, encoding and the runtime fields after the longest permutation length equal the Xbox's; its compression does except for the 38 that Custom Edition has as Ogg Vorbis, and its longest permutation length for 273 (the others differ by a few milliseconds or more, among them the Ogg Vorbis ones); its pitch ranges' runtime fields all do |
| Sound permutations and resource-held bitmaps name tags of another map (the one the resource map was built with) | 632 of 676 bitmaps of `bloodgulch.map`; every resource-held permutation differs from the Xbox's own sound handle |
| Model parts: the strip and vertex data are in the model data, never in the part's blocks; the triangle buffer is a precompiled strip and the vertices uncompressed; the part flag 2 is set on exactly the parts that have a node table | all 24 maps (10,934 parts) |
| Converting a part's vertices with the game's compressor reproduces the Xbox's | for the 335 parts of the 60 models the Custom Edition and the Xbox `bloodgulch.map` share: identical strips; positions equal to within float rounding; texture coordinates and node weights exactly what the compressor makes of the Custom Edition values (clamped to ±1, as the Xbox also has them: `plant_broadleaf_short` has coordinates up to 3.0 in both); node indices the Custom Edition ones times 3, except that the Xbox writes an unweighted second node as -1 where the compressor writes the node Custom Edition names; normals within 0.002 |
| Every node index of a vertex names a node of its part's table (in models whose parts have local nodes) or of its model, every table entry a node of the model, and every vector is within ±1.0001 | all 24 maps |
| Structure BSP materials: environment vertices uncompressed (type 0), with lightmap vertices (as many as the environment ones) exactly in lightmaps that have a bitmap, and no compressed vertices | all 24 maps (the lightmap vertex type field is 0 or 2 regardless) |
| Custom Edition shader type values are 7 to 11 for `scex`, `swat`, `sgla`, `smet`, `spla`; the Xbox's are 8, 9, 10 for `sgla`, `smet`, `spla` | every shader of the 24 maps, and of the Xbox `bloodgulch.map` |
| Chicago shaders with extra layers: none in the stock maps or the Xbox `bloodgulch.map`; 2 in `beavercreek_halo3.yelo`, 2 in `celer_exile_odst_v2.yelo`, 4 in `extinctionrevanepic2.map`, 1 in `fy_killzone.yelo` | every `schi` and `scex` of the sample |
| Maps with OpenSauce memory upgrades have script node arrays of 28501 | all 4, and `extinctionrevanepic2.map` uses 637 of them |
| Compressed color plates are left out of caches: size kept, address 0 | every bitmap in `bitmaps.map` |
| Font style references in `loc.map` are all `NONE` | all 3 fonts |
| Sound compression value 3 is Ogg Vorbis, value 1 is Xbox ADPCM | in `sounds.map`, all 106 value-3 permutations start with `OggS`; all 1,365 value-1 permutations are whole 36-byte blocks |
| `bitmaps.map` bitmaps are not swizzled, and cube maps hold each level's six faces together | all 1,467; 38 of 40 cube maps match that order when checked against their next level (one is uniform, one does not match) |
| Multipurpose maps: Custom Edition's red, green, blue and alpha hold what the Xbox's alpha, green, red and blue do | the multipurpose maps the Custom Edition and the Xbox `bloodgulch.map` share, decoded and compared channel by channel (the cyborg's, the warthog's, the boulders') |
| HUD meters: Custom Edition's alpha holds the Xbox's color (the fill order), and its color the Xbox's alpha (the shape) | `hud_ammo_meters`: Custom Edition's 512×512 A8R8G8B8 averaged down to the Xbox's 256×256 A8Y8 differs from the Xbox's luminance by 6.6 on average in alpha and 23 in color, and from its alpha by 12.3 in color and 23.4 in alpha; `hud_unit_meters` has its sprites rearranged, and shows the same swap when viewed. January's meter shader reads the fill order from color and discards texels without alpha |
| HUD elements flagged *use high resolution scale* draw bitmaps twice the Xbox's size; others the same size | every unit and weapon HUD placement of `bloodgulch.map` whose tag the Xbox map also has (the motion sensor's foreground, unflagged, uses a 128×128 bitmap in both) |
| Vehicle placements: the byte at 0x58 is the multiplayer team and the word at 0x5A the multiplayer spawn flags, default bits 0 to 3 (slayer, capture the flag, king of the hill, oddball) and allowed bits 8 to 11 | `hugeass.map`'s 78 vehicle placements: the byte is 1 on exactly the blue team's; the word is 0x0303 on warthogs, tachikomas and pelicans, 0x0F0F on ghosts, 0x0404 on 16 unnamed jets, 0x0808 on the night jet and 0 on vehicles its scripts create; OpenSauce leaves these bytes unnamed |
| Script function and engine global tables: Halo PC's have entries this build's lacks, so a compiled index names another entry from some point on; value types are numbered alike | the compiled scripts of the sample, each call's function index against the name its first child keeps, and each engine global's index against its name: `timberland.map` 23 calls shifted (`player_effect_start` by 29), `ui.map` 11 (`camera_set` by 1), `hugeass.map` 3 calls and 1 global (`rider_ejection`, 158 against 147), `extinctionrevanepic2.map` 9 calls, and 4 of a function this build lacks; no other stock map has scripts; each call's value type is the type of its function here (1,485 of 1,485 in `hugeass.map`), or one the call site casts to |
| Bitmaps the Xbox keeps in monochrome formats and Custom Edition as 32-bit color at the same size keep their channels (A8Y8 as A8R8G8B8 with red the luminance and alpha the alpha) | 24 of the 26 such bitmaps of `bloodgulch.map` decode to exactly the Xbox's values; the other 2 have other content |

### Assumptions (not verified)

- **Mod set file names.** OpenSauce keeps mod sets under `maps\data_files\`
  (`data_file_yelo.cpp`). The loader expects `data_files\<mod>-bitmaps.map`
  and so on; but the examined source's `BuildName` never appends the mod
  name, which looks like a defect in that snapshot. No mod set was available
  to check either.
- **Placement of resource-held tags.** They are placed after the tag data,
  aligned to 4 bytes, as OpenSauce's unimplemented loader outlines
  (`s_cache_file_data_load_state`); how Custom Edition itself places them is
  not known, and nothing found depends on it.
- **Which chicago extended maps.** The four-stage maps are kept, as the
  renderer draws four stages; that Custom Edition's two-stage maps are a
  fallback for older hardware is an inference (BlamLib's exporter makes the
  same choice).
- **Centroid nodes of local-node parts** are taken to be the model's nodes:
  they are within the model's node count in every map, and they are only
  used to sort transparent parts.
- **Editing-kit pointers are cleared.** The definition pointers of relocated
  blocks and data are set to 0: they refer to nothing in the game process.
- **Name comparison** is exact (case-sensitive), which matched every sample
  sound.

## Limits and remaining work

- **Validation stops at what the loader reads.** The loader checks the
  container, the resource-held tags and everything it converts; the game
  reads every other tag as it reads Xbox caches, trusting it. A map made to
  crash the game can still do so, as an Xbox map could; debug builds also
  stop on data that Custom Edition's release build reads past unchecked, as
  with the animation overlays above, and other such data may turn up.
- **Ogg Vorbis sounds do not play** (39 of `bloodgulch.map`'s sounds, among
  them announcer and dialogue lines): a decoder, under a licence that fits a
  CC0 repository, would be needed.
- **Models of 44 nodes or more** cannot be drawn by this build's renderer:
  such maps are refused (`celer_exile_odst_v2.yelo`).
- **Linear bitmaps whose rows are not a multiple of 64 bytes** are drawn with
  the wrong row pitch: the texture cache gives the texture header the
  unpadded pitch rounded down. They are logged; none is in the maps run (the
  6 known are in `extinctionrevanepic2.map`).
- **Button prompts on replaced icon sheets.** HUD messages draw a button
  from sequence 0 to 3 of the HUD globals' icon bitmap, as the Xbox does;
  Halo PC names the key instead. `beavercreek_halo3.yelo` replaces the icon
  bitmap, and its sequence 2 is an energy sword, so `Hold [X] to pick up`
  shows the sword. Stock maps keep the Xbox's button icons.
- **A team icon.** The owner's screenshot of `beavercreek_halo3.yelo` in a
  team game shows a team-colored figure beside the ammunition counter; no
  unit, weapon or grenade HUD interface of the map draws one (the unit HUD's
  auxiliary overlays, where this build draws team icons, are empty), so it
  comes from something this build does not have, not identified.
- **Channel orders are per texture.** The multipurpose maps and meters also
  drawn another way keep Halo PC's channels (above); only the uses found in
  model shaders and HUD interfaces are looked at.
- **The texture cache** holds 44 MB; a map that draws more in a frame is
  drawn with default textures where they do not fit, and the game reports
  it (`YOU GOT STABBED`).
- **Silenced sounds still log.** Playing a silenced Ogg Vorbis sound logs
  `attempt to play a sound that was not a mono 22k compressed sound ...`,
  which the game also prints on the screen, in the release build too.
- **Scripts that use what this build does not have** are refused: of the
  sample, `extinctionrevanepic2.map` calls OpenSauce's
  `pp_set_effect_instance_active` (4 times), and needs a mod set anyway.
  OpenSauce's other runtime features (`project_yellow`) do not exist here.
- **The conversions are one-way and in memory:** nothing is written to the
  map files.

### How far Custom Edition tags are from the Xbox tags

`tools/custom_edition_tag_footprints.py` compares the tags a Custom Edition
cache and the Xbox cache of the same level both keep (same group, same name),
without any tag definitions. A tag's *footprint* is the distance from its
address to the next tag's (its structure and the block data after it); its
*structure size* is estimated as the distance to the lowest address in the
footprint that the footprint points at (its first child block). Equal
footprints, level after level, suggest a group's layout is unchanged; they do
not prove it.

The Xbox caches available are the retail ones, build 01.10.12.2276, not the
01.01.14.2342 build this repository reconstructs: they stand in for it, and
differences between the two Xbox builds are not measured. Run on the 14
levels both sets have (13 multiplayer levels and `ui`):

```
python tools/custom_edition_tag_footprints.py --blocks assets/custom_edition "<Xbox 2276>/maps"
```

- **48 groups have the same footprint for every shared tag in every level**:
  `Soul actv ant! bipd colo cont deca eqip flag fog font foot grhi hmt hud#
  hudg itmc jpt! lens lifi ligh lsnd mach metr mgs2 mply part pctl phys pphy
  scen schi senv sgla sky smet snde soso spla ssce str# swat trak udlg unhi
  vcky vehi wind`. The shader groups among them have the same layout, but
  not the same type values (above).
- **Groups whose footprints differ, but whose structures match**: UI widgets
  (`DeLa`, 464 of 657 footprints equal, 423 of 423 structures), animations
  (`antr`, 213/239), collision models (`coll`, 331/345), effects (`effe`,
  4140/4166), projectiles (`proj`, 197/210), weapon HUDs (`wphi`, 196/222),
  weapons (`weap`, 39/182, structures 182/182), globals (`matg`, 1/14,
  structures 14/14) and scenarios (`scnr`, 0/14, structures 13/14), plus, in
  `ui` only, bitmaps, sounds and string lists. These differ in their block
  data. The flamethrower's tags account for every `effe`, `proj` and `wphi`
  difference and all but one `coll` difference; 11 of the 14 weapons (every
  player weapon and both vehicle guns) differ in every level.
- **What differs in those blocks** (`--blocks`: the top-level blocks of each
  differing tag, found without definitions and compared): almost always
  the number of elements, or whether a block has any, which is content:
  weapons 143 count and 26 presence differences, UI widgets 158 and 63,
  scenarios 36 and 13, projectiles 13, weapon HUDs 78 presence, collision
  models 26 count. Every weapon difference checked in Blood
  Gulch lies in a predicted-resources list (8-byte elements, weapon
  offset `0x4E4`; OpenSauce `weapon_definitions.hpp`), or, for the
  flamethrower, also in its attachments; the globals differ in their
  cheat powerups (16 against 14). The rest (flamethrower effects 26,
  globals 13, animations 39, one collision model, and 3 in `ui`) keep their element counts but
  span a different amount of data up to the next block, which this method
  cannot split into element size and nested content: for the globals, the
  block in question (`player_info`, 0xF4-byte elements without blocks of
  their own) spans more than its elements, so other data sits in the span.
  No difference found has to be a layout change, and the two maps run did
  not show one.
- **Groups only Custom Edition has**: `mod2` (models), `scex` (a transparent
  shader type), `devc` and `tagc`, in all 14 levels; **only the Xbox has**:
  `mode` (models) and `sotr` (a transparent shader type), besides the groups
  Custom Edition keeps in resource maps.

## Verification

- **The byte-matched build is unchanged.** All 621 matching objects
  (`ninja all_source`, XDK `CL.exe`) were built from the January sources as
  they are on this branch and as they are at `f2fa457f`, and again after
  each merge, as they are on the branch and at `223fa93f`, then at
  `cd47170f`: each time every section and symbol table is identical, and
  the files differ only in their COFF time stamp, in the 59 objects the
  changed files made the second build recompile. Upstream's
  `configure.py` no longer writes the matching graph (the Xbox SDK it
  needs cannot be redistributed); for this check it
  was turned on in `tools/project_x86.py` (`SolutionConfig.matching`), in a
  checkout with the SDK's compiler, and turned off again. The game-source
  changes are all under `#ifdef HALO_LINUX`: `cache/cache_files.c`,
  `cache/cache_files_windows.c`, `rasterizer/rasterizer_geometry.h`
  (declarations of the buffer functions),
  `rasterizer/xbox/rasterizer_xbox_transparent_geometry.c` (the chicago
  extra layers), `cache/physical_memory_map.c` and
  `cache/xbox_texture_cache.c` (the texture cache's size) and
  `game/game_engine.c` (the multiplayer vehicles and their placement) and
  `objects/object_types.c` (their placement). No matching tool, reference
  binary or scoring rule was touched.
- The new game units (`custom_edition_*.c`) compile without warnings under
  `-Wall -Wextra` as well as the game's usual flags, which silence warnings.
- `tools/test_linux_port.py` fails 3 tests on this Windows host, on the
  merged branch and on upstream's `223fa93f` and `cd47170f` alike: two from
  the case-insensitive file system (`<StdDef.h>` and `POPPACK.H` spelt in
  another case), and a Linux-only UASM rule. Upstream's workflow runs it on
  Linux.
