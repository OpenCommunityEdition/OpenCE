# Halo Custom Edition and OpenSauce caches in the native builds

Experimental work on branch `experimental/custom-edition-yelo-loading`,
based on `bnunu/halo-ce-universal` `main` at `f2fa457f`. It teaches the
native builds (Windows, Linux, Android) to recognize and load Halo Custom
Edition caches (`.map`, cache version 609), OpenSauce caches (`.yelo`, and
`.map` files with an OpenSauce header), and the Custom Edition resource maps
`bitmaps.map`, `sounds.map` and `loc.map`. The byte-matching build is not
affected: every game-source change is under `#ifdef HALO_LINUX`, and the two
game units that changed compile to the same MSVC objects as before (see
[Verification](#verification)).

**These maps do not run.** Recognizing a map, loading its data and running it
are separate milestones, and only the first two are reached. A map parsing,
or even loading, says nothing about OpenSauce compatibility.

## What "support" means

| Milestone | What it requires | Status |
| --- | --- | --- |
| **1. Recognize** | Tell the format and variant from the header alone, check every header field against the file, and name what the map needs: its resource maps, an OpenSauce mod set, memory upgrades | **Done**, in the loader library, the report tool and the game |
| **2. Load** | Put the map's tag data where its pointers expect it (`0x40440000`), read the tags kept in resource maps and relocate their pointers, and check every tag instance, name and address, every structure BSP (file range, header, place in the tag cache), every bitmap's pixels and every sound's samples (range in their file), and the header checksum | **Done in the loader library and the report tool** (`cache_file_report`). The game itself refuses these maps before loading them: loading inside the game process needs a reserved address window it does not have yet |
| **3. Run** | The game starts the map, draws it, plays its sounds and runs its scripts | **Not done, not attempted.** The map's tags are laid out for Halo PC, not for this build; see [Blockers](#what-running-a-map-would-take) |

## What the native builds do now

- **A Custom Edition cache in `maps\`** is named and refused, instead of being
  rejected as "an old version" of this build's caches:

  ```
  'd:\maps\ui.map' is a Halo Custom Edition cache (build 01.00.00.0609): this build recognizes it but cannot run it (docs/custom_edition_caches.md)
  ```

  The message is logged to `debug.txt` in every build, and debug builds stop
  on it as the original check stops on a bad cache
  (`port/linux/game/custom_edition_cache.c`, called from
  `cache_file_header_verify` in `source/cache/cache_files.c`).
- **An OpenSauce `.yelo` file** is found when there is no `.map` of that name,
  as OpenSauce looks for one (`cache_file_get_map_path` in
  `source/cache/cache_files_windows.c`), and refused the same way:

  ```
  'd:\maps\ui.yelo' is a Halo Custom Edition cache with an OpenSauce header (build 01.00.00.0609): this build recognizes it but cannot run it (docs/custom_edition_caches.md)
  ```

- **Xbox caches** take exactly the path they took before: the new code
  returns at once for them, and the original checks and messages follow
  unchanged (`the cache file 'ui' belongs to a different build
  (01.10.12.2276)` for an Xbox cache of another build, observed).

## The loader and the report tool

- `port/linux/game/cache_file_formats.c` (`cache_file_formats.h`): the format
  code, standalone C with fixed-width types and no game dependencies, compiled
  into the native builds (as every `port/linux/game` unit is) and into the
  tool. Map files are untrusted input: every field is read little-endian from
  a bounds-checked buffer, never through a structure laid over the file, and
  every offset, count and size is checked before use, with overflow-safe
  arithmetic.
- `port/tools/cache_file_report.c`: reports what can be done with each file.

  ```sh
  clang --target=i686-pc-windows-msvc -fuse-ld=lld -Iport/linux/game \
      port/linux/game/cache_file_formats.c port/tools/cache_file_report.c -o cache_file_report.exe
  cache_file_report [--maps DIRECTORY] [--stock-data-files] FILE...
  ```

  Resource maps are looked for next to the cache (or in `--maps`):
  `bitmaps.map`, `sounds.map`, `loc.map`, or `data_files\<mod>-bitmaps.map`
  and so on for an OpenSauce cache built with a mod set. A cache that needs a
  mod set that is not there is not loaded. `--stock-data-files` loads it with
  the stock resource maps instead, for inspection only (OpenSauce itself
  refuses such a map), and the report says it did.

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
   be held by resource maps), the model vertex and index data range, and the
   scenario tag.
4. Every structure BSP: its tag, its range in the file, its place in the tag
   cache (above the tag data, within the tag cache), and its header (`sbsp`
   signature, no Xbox vertex buffers, its structure pointer inside it).
5. The header checksum is computed as OpenSauce computes it, before anything
   changes the tag data; a mismatch is reported, not refused.
6. The tags held by resource maps are read into the tag cache after the tag
   data, below the lowest structure BSP, and their pointers relocated.
7. Every bitmap, sound, font, unicode string list and HUD message text tag,
   from the map or from a resource map, is walked through its blocks: every
   block and data pointer must now lie within the loaded tags, and every
   bitmap's pixels and sound's samples within their file.

What loading does not do: it does not load the structure BSPs (the game loads
one at a time when it switches to it), bitmap pixels or sound samples (the
texture and sound caches stream them), or the OpenSauce tag definitions; and
it does not interpret any other tag group.

## Tested

### Automated tests

`python -m pytest tools/test_cache_file_formats.py` (91 tests; needs clang).
The tests build complete synthetic caches and resource maps in memory (no
game data is stored in the repository) and run the report tool, compiled with
`-Wall -Wextra -Wpedantic -Werror` and undefined-behaviour trapping
(`-fsanitize=undefined -fsanitize-trap=undefined`), so an undefined operation
fails the test that triggered it. The module is also compiled as the game
compiles it (`-std=gnu89`) with warnings enabled, since the game build
silences them.

- Supported formats: a complete Custom Edition cache with every kind of
  resource-held tag; several pitch ranges; three structure BSPs; OpenSauce
  caches with memory upgrades, appended tag definitions, OpenSauce tags, a mod
  set present, absent, and substituted; resource maps of each type; the
  `--maps` option.
- Malformed input: every check in the list above, with at least one case
  each (69 tests: header fields, OpenSauce fields, tag index, instances,
  names, addresses, scenario, structure BSP block, range and header, resource
  map headers and entries, resource layouts, pixel and sample ranges, font
  style references, tag cache overflow), plus 120 seeded random corruptions
  of a valid cache and its resource maps, each of which must end in a clean
  verdict. The random corruptions are sampled evidence of robustness, not
  proof.
- Original format: Xbox caches of build 01.01.14.2342 and 01.10.12.2276, and
  a compressed Xbox cache, are recognized and left to the original loader;
  other cache versions and unrelated files are named, not loaded.
- The tests were checked against deliberately broken checks: disabling the
  tag handle check or the structure BSP signature check fails exactly the
  test for it.
- Real maps: when `HALO_CUSTOM_EDITION_MAPS` names a folder of maps, or they
  are extracted to the gitignored `assets/custom_edition`, three more tests
  check the results in the table below.

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

### In the game

Built with `ninja windows` (clang 22.1.8, Visual Studio 2026 x86 libraries,
SDL 3.4.16) and run from a data root holding only one map each time (debug
build; the game loads `ui` first):

| `maps\` held | Observed in `debug.txt` |
| --- | --- |
| the Custom Edition `ui.map` | the Custom Edition message above, then the stop on it |
| `fy_killzone.yelo` renamed `ui.yelo`, no `ui.map` | the `.yelo` was found; the OpenSauce message above |
| the Xbox 01.10.12.2276 `ui.map` | the original `the cache file 'ui' belongs to a different build (01.10.12.2276)`, from the original line |

No map ran, and no game data of build 01.01.14.2342 was available, so
nothing past the first map load was exercised.

### Not tested

- The Linux and Android builds were not built (no 32-bit Linux sysroot or
  SDL3, no Android NDK on the machine used). Their build scripts gained the
  same include directory as the Windows one; the units they compile are the
  same.
- Release builds (`configure.py --release`) were not built or run.
- No OpenSauce mod set was available; the mod set file names are an
  assumption (below).
- No sample map has more than one structure BSP, in-map sounds other than
  `ui.map`'s, protected caches, OpenSauce compression parameters, tag symbol
  or string id storage, or an OpenSauce minimum version; the synthetic tests
  cover several BSPs.
- No Halo PC retail cache (version 7) was available; such caches are reported
  as "a cache of an unknown version".

## Evidence

### Sources

- **OpenSauce** (GPL-3.0, Kornner Studios): the `OpenSauce-master.zip`
  archive of the upstream repository (SHA-256
  `f1097c9cf5975a208506c78c3af47d6b893f998a9f47dddf89c9bbcc1f4b7c73`; newest
  entry dated 2016-01-15; `yelo_version.hpp` names OpenSauce 4.0.0).
- **Reclaimer** (GPL-3.0, Gravemind2401): the `Reclaimer-master.zip` archive
  (SHA-256 `72fd5270abbdf662ff07a54e9cce80a6158e479c51ecfdcd979f7f9fbd6c2d03`;
  newest entry dated 2026-09-20).

Both are GPL-3.0 and this repository is CC0: no code from either was copied.
They were read as documentation of the file formats, and every layout used was
then checked against the sample maps.

### Documented facts (source, checked against the sample maps)

| Fact | Source |
| --- | --- |
| Cache header layout, version 609, `head`/`foot` signatures | OpenSauce `blamlib/Halo1/cache/cache_files_structures.hpp` (`s_cache_header`) |
| Tag index (0x28 bytes, instances at 0x28, model data as file offsets) and tag instance (0x20 bytes, resource-map flag at 0x18) | same file (`s_cache_tag_header`, `s_cache_tag_instance`) |
| OpenSauce header at 0x70 and its fields, versions 1 and 2, validity rules | OpenSauce `YeloLib/Halo1/cache/cache_files_structures_yelo.hpp`, `cache_files_yelo.cpp` (`IsValid`) |
| Tag cache at `0x40440000`, 23 MB; memory upgrades ×1.5; file size limits | OpenSauce `cache_constants.hpp`, `saved_game_constants.hpp`, `blam_memory_upgrades.hpp` |
| Resource map header and entries | OpenSauce `data_file_structures.hpp` |
| Groups held by resource maps: bitmaps, sounds, fonts, unicode string lists, HUD message text | OpenSauce `blamlib/Halo1/cache/cache_files.cpp` (`cache_file_data_load`) |
| Structure BSP reference (0x20) and header (0x18) | OpenSauce `scenario_definitions.hpp`, `structure_bsp_definitions.hpp` |
| Bitmap, sequence, sprite, bitmap data layouts; pixels in `bitmaps.map` flag | OpenSauce `bitmaps/bitmap_group.hpp` |
| Sound definition, pitch range, permutation layouts; samples-in-`sounds.map` flag | OpenSauce `sound/sound_definitions.hpp` |
| HUD message text layout | OpenSauce `interface/hud_messaging_definitions.hpp` |
| Font (156 bytes) and unicode string list layouts | BlamLib `Blam/Halo1/Tags/Definitions/Misc.cs`, `Resources.cs` |
| Bitmap tags in `bitmaps.map` by index, pixels by absolute offset | Reclaimer `Blam/Halo1/CacheFile.cs`, `BitmapTag.cs`, `BitmapsAddressTranslator.cs` |
| The header checksum: CRC-32 of the structure BSPs (packed from 0x800), the model data and the tag data, not inverted | OpenSauce `cache_files_yelo.cpp` (`CalculateChecksumFromMemoryMap`), `memory_interface_base.cpp` (`CRC`); reproduces the stored checksum of 23 of the 24 sample caches |
| OpenSauce looks for `<name>.map`, then `<name>.yelo` | OpenSauce `cache_files_yelo.cpp` (`c_map_file_finder`) |

### Observed (not stated by the sources; established on the sample maps)

| Fact | Evidence |
| --- | --- |
| `time_t` in the OpenSauce build info is 64 bits | the build string starts 8 bytes after the timestamp in all 4 OpenSauce caches |
| The OpenSauce tag definitions start at the file length the header declares, and are zlib data | all 4 OpenSauce caches; one decompressed to its declared size |
| Structure BSPs load at the top of the tag cache (`address + size` = `0x41B40000`, or `0x426C0000` with memory upgrades) | all 24 caches |
| Bitmaps, fonts, unicode string lists and HUD text held by resource maps: the tag's address field is the entry index, the entry's name is the tag's path, the entry is the whole tag with addresses counting from its start | 9,550 bitmaps, 68 fonts, 2,248 string lists, 23 HUD texts across the sample |
| Sounds held by `sounds.map`: the map keeps the 0xA4-byte header, whose pitch range block has a count and no address; the entry named by the tag path holds the header again, then the pitch ranges and permutations, whose addresses count from the first pitch range | 7,397 sounds, 21,870 sample ranges |
| Compressed color plates are left out of caches: size kept, address 0 | every bitmap in `bitmaps.map` |
| Font style references in `loc.map` are all `NONE` | all 3 fonts |
| Sound compression value 3 is Ogg Vorbis, value 1 is Xbox ADPCM | in `sounds.map`, all 106 value-3 permutations start with `OggS`; all 1,365 value-1 permutations are whole 36-byte blocks |
| `bitmaps.map` bitmaps are not swizzled | all 1,467 |

### Assumptions (not verified)

- **Mod set file names.** OpenSauce keeps mod sets under `maps\data_files\`
  (`data_file_yelo.cpp`). The tool expects `data_files\<mod>-bitmaps.map`
  and so on; but the examined source's `BuildName` never appends the mod
  name, which looks like a defect in that snapshot. No mod set was available
  to check either.
- **Placement of resource-held tags.** They are placed after the tag data,
  aligned to 4 bytes, as OpenSauce's unimplemented loader outlines
  (`s_cache_file_data_load_state`); how Custom Edition itself places them is
  not known, and nothing depends on it yet.
- **Fields left as they are.** A resource entry's `owner_tag_index` in bitmap
  data names a tag of whatever map the resource map was built with; its
  meaning to the Custom Edition runtime is not established, so it is not
  rewritten. A `sounds.map` entry's copy of the sound header differs from the
  map's copy (compression, promotion reference, pitch range block); the map's
  copy is used.
- **Editing-kit pointers are cleared.** The definition pointers of relocated
  blocks and data are set to 0: they refer to nothing in the game process.
- **Name comparison** is exact (case-sensitive), which matched every sample
  sound.

## What running a map would take

Milestone 3 needs at least the following, none of which exists. Each item
cites the evidence that makes it a blocker.

1. **An address window for the tags.** Custom Edition tag data holds absolute
   pointers based at `0x40440000` (from 2.2 MB of it in `ui.map` to 10.6 MB
   in `celer_exile_odst_v2.yelo`, in a tag cache of up to 36 MB). The native builds
   reserve only the Xbox window at `0x80000000`
   (`port/linux/src/xbox_memory.c`); loading in the game needs a second
   reserved window, `0x40440000`–`0x426C0000`, on every platform.
2. **The loader in the game.** `scenario_tags_load` reads Xbox tag data to
   `0x803A6000` and registers Xbox vertex and index buffer arrays from the
   tag index (`tags_header_register_vertex_and_index_buffers`), whose
   `+0x14`/`+0x1C` fields hold pointers in this build but file offsets in
   Custom Edition caches. The game copies each map into a fixed-size cache
   partition slot (multiplayer slots are 0x2F00000 bytes; the sample `.yelo`
   files are up to 255 MB), which Custom Edition caches do not need.
3. **Tag layouts.** The tags are Halo PC's, not this build's: for instance
   objects refer to `mod2` (gearbox model) tags, 66 to 70 of them in each
   sample multiplayer map and no `mode` tag at all, while this build's code
   asks for `mode`. Every tag group the game reads would have to be compared
   field by field with this build's definitions and converted where they
   differ; no such comparison has been made.
4. **Resources.** Model vertices and indices are PC buffers in the file's
   model data; bitmap data are PC textures (every sample bitmap unswizzled,
   in DXT1/3/5, 16- and 32-bit and P8 bump formats) where this build's texture cache
   expects Xbox textures; 106 of the 1,471 `sounds.map` permutations are Ogg
   Vorbis, which this build cannot decode (the other 1,365 appear to be Xbox
   ADPCM, which it can).
5. **Scripts.** Compiled scripts call functions by their index in the
   engine's function table; Custom Edition's table and this build's are not
   established to agree, and OpenSauce raises the table to 1,024 functions
   (`blam_memory_upgrades.hpp`).
6. **OpenSauce itself.** Every OpenSauce cache in the sample holds
   `project_yellow` (`yelo`) and `project_yellow_globals` (`gelo`) tags,
   which drive OpenSauce runtime features (scripting extensions,
   post-processing, memory and game state upgrades) that this build does not
   have; OpenSauce also defines mod sets, protected caches, and string id and
   tag symbol storage.

## Verification

- The two game units with new `HALO_LINUX` code were compiled with the
  byte-matching toolchain (`ninja all_source`, XDK `CL.exe`) with and
  without the change: the objects differ only in byte 4, part of the COFF
  time stamp. The other 619 matching objects were not recompiled between the
  two builds. No matching tool, reference binary or scoring rule was touched.
- `tools/test_linux_port.py` fails 3 tests on this Windows host before and
  after the change alike (symlink privilege, a case-insensitive file system,
  a Linux-only UASM rule).
