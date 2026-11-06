# Halo: The Master Chief Collection's Halo 1 maps

The Halo 1 maps of Halo: The Master Chief Collection (MCC) are cache files of
version 13 (build string `01.03.43.0000`). Put in the data root's `maps`
folder in place of the disc's, they run on the native builds: each loads as a
Custom Edition map does (`docs/custom_edition_caches.md`), once its tags are
moved to where a Custom Edition map's are. The MCC installation's folder is
`Halo The Master Chief Collection/halo1`; its maps are in `halo1/maps`.

What runs, and what does not yet:

- Every map's tags and structure BSPs load, are checked by the tag validator
  and are converted: the 23 maps the disc has (campaign and multiplayer) and
  `ui.map`. Halo PC's six multiplayer maps load too, but the menus do not
  list them.
- Their bitmaps are read from MCC's `bitmaps.map`, beside the maps.
- Their sounds are silent: MCC keeps them in its FMOD sound banks
  (`halo1/sound/pc`), which are not read yet.
- MCC's `ui.map` holds Halo PC's menus, not the Xbox's: the game's own
  menus that are not in it (the network pregame screen) do not open, and
  Halo PC's list widgets stop the game. Until the menus run on it, a
  temporary launcher takes the main menu's place
  (`port/linux/game/mcc_launcher.c`): a list of the campaign's levels and
  the multiplayer maps, each played as the console's `map_name` plays a
  level or as a local slayer game for controller 1.
- The HUD is drawn at the Xbox's size, from MCC's remastered bitmaps.
- Not on the web build, which runs no Custom Edition map either (its tag
  cache is not reserved there).

## Loading

`custom_edition_cache_load` (`port/linux/game/cache_file_formats.c`) takes a
version 13 cache as it takes a Custom Edition one, but for these:

1. The tag data is at `0x50000000` (`MCC_TAG_DATA_ADDRESS`), where Custom
   Edition's is at `0x40440000`. The tag index (`0x28` bytes, ending in
   `tags`) and the tag instances are Custom Edition's. No tag is held in a
   resource map.
2. Each structure BSP ends 64 MB above the tag data's address
   (`MCC_ADDRESS_WINDOW_BYTES`): `0x54000000` minus its size. Its header's
   words at `0x04` and `0x08` (the Xbox's vertex buffers' count and address)
   are the size and file offset of its vertex region, which lies just before
   it in the file: each material's environment vertices (56 bytes each),
   then its lightmap vertices (20 bytes each), in turn. The materials' own
   vertex data fields are empty. The loader checks that the region ends where
   the BSP starts, that the materials' vertices fill it exactly, and that
   every normal is a unit vector's.
3. The header's checksum is Custom Edition's (OpenSauce's CRC), but over
   each BSP's region and the BSP in turn, as they lie in the file.
4. The tags are moved to `0x40440000` (`mcc_tags_rebase`): the tag index and
   instances by the loader, every tag's blocks, data and references by the
   tag validator's schemas (`tag_validate_rebase`, `cache_file_rebase_proc`).
   Only the schemas can say which words are pointers: many words of the
   maps' animations and sounds (about 20,000 across the maps) lie in the
   tags' address range and are not pointers. A tag of a group that has no
   schema (`devc`, `tagc`: Halo PC's, which the game never reads) is left as
   it is.
5. Each BSP reference is made to cover the BSP and room for its region after
   it, at the top of the tag cache. As the BSP loads
   (`custom_edition_structure_bsp_read`, from `scenario_structure_bsp_load`),
   its region is read after it, its pointers are moved
   (`tag_validate_rebase_structure_bsp`: its own, and its references' names,
   which are in the tags), and each material is given its vertices in the
   region (`mcc_structure_bsp_vertices_attach`): it is then a Custom Edition
   BSP.
6. The scripts' syntax nodes are a data array with room for 32,767 nodes,
   where the game's has room for 19,001 and runs no script of another size
   (`mcc_scripts_fit`). The array is made the game's size. a10's nodes are
   in slots past its last (18,837 nodes in 20,076 slots): each is moved to a
   free slot below it, and every reference to it (a node's next node and
   first argument, a script's and a global's root) follows it.
7. A bitmap's pixels are in MCC's `bitmaps.map` when its flags have bit 8,
   and in the map itself when they have bit 9 (its lightmaps), which the
   game would refuse and is cleared (`mcc_bitmaps_prepare`). `bitmaps.map` is
   a Custom Edition resource map of pixels alone, one entry per bitmap
   (named `<tag>_<index>`), whose offsets the bitmaps give.
8. A sound's samples name a `sounds.map` MCC does not ship (Halo PC's). Its
   sounds are made unplayable (`mcc_sounds_mute`) until they are taken from
   MCC's banks.
9. The HUD is laid out at twice the Xbox's size, with bitmaps four times the
   Xbox's (`mcc_hud_convert`): every placement's offset is doubled (but the
   few with Halo PC's high resolution scale flag, all vehicles' health,
   whose offsets are the Xbox's), the digits' metrics and the damage
   indicators' offsets are doubled, and every bitmap a placement draws has
   the half HUD scale flag. Each placement's offset and scale is halved, the
   weapons' numbers' and the grenade count's too; the Custom Edition
   conversion then halves those of flagged bitmaps again, as it does a
   Custom Edition map's; the digits' bitmap is given the flag, and
   `hud_draw_numbers` halves an MCC map's digits once more.
10. Its bitmaps' channels are the Xbox's, not Halo PC's: its model shaders'
    multipurpose maps are not reordered as a Custom Edition map's are
    (`custom_edition_bitmaps_orders_set`). Its HUD meters keep the fill
    order in color as the Xbox's do, but their alpha is the shape alone, a
    flat 225 to 255, where the Xbox's carries the fill order too; the meter
    reads both, so the color is sampled for alpha as well
    (`_custom_edition_channels_hud_meter_mcc`).

`port/tools/cache_file_report.c` checks a version 13 cache as far as the move
of its tags, which needs the game's schemas.

## In the game

`mcc_level_name` (`port/linux/game/custom_edition_cache.c`) says whether the
file of one of the game's own levels in its maps folder is a version 13
cache. Such a level is never precached (it is read in place), is loaded as a
Custom Edition map is (`cache_files.c`), and plays on any build's network
games but with machines that have MCC's maps too: its version in a network
game is `MCC_MAP_VERSION`, where the disc's maps' is 0
(`cache_files_map_version`; network version 42, `port/linux/NETCODE.md`).
Map sharing never sends one: it shares Custom Edition maps alone.

## Evidence

Established on the maps of MCC's Steam release `2025.08.16.178512.1`,
compared with the maps of an NTSC Xbox disc (revision 2, build
`01.10.12.2276`):

- The layouts in "Loading" above, on all 30 maps: every tag index, BSP,
  vertex region and checksum as described (82 BSPs, 19,240 materials).
- After the move, every map's tags and BSPs pass the tag validator with no
  correction but one in `ui.map` (a GameSpy font's glyph of negative width).
- For the tags the disc's maps have too, the root structures are the same
  size; what differs is what their blocks hold (Halo PC's `mod2` models, its
  menu widgets, longer preloading lists).
- Bitmaps: rebuilt with `rasterizer_xbox_bitmap_rebuild_hardware_format`, the
  pixels of 402 of bloodgulch's 422 bitmaps and 542 of a10's 549 that have
  the disc's size and format are the disc's, byte for byte. The HUD's are
  remastered (two or four times larger, some in other formats, four tags
  with another count of bitmaps).
- Collision: the campaign's BSPs are the disc's (63 BSPs, a few bytes of a
  few planes apart). Of the multiplayer maps, chillout, longest, prisoner,
  putput, ratrace and wizard have the disc's collision and lightmaps;
  beavercreek, bloodgulch, boardingaction, carousel, damnation, hangemhigh
  and sidewinder are Halo PC's rebuilds, with other collision and lightmaps.
- Sounds: the classic effects are `sound\old_sfx\…` in
  `halo1/sound/pc/sounds_adpcm.fsb` (FSB5, IMA ADPCM), 3050 of 3052 samples
  the disc's Xbox ADPCM byte for byte; the dialogue is CELT at 44.1 kHz in
  `sounds_dialogs.fsb`; the classic music is `sound\old_music\music\…` in
  `sounds_music.fsb`, one sample for each of a tag's permutations, of the
  disc's lengths. Each bank's `lst/<bank>.lst.bin` gives each tag path's
  samples.
- In the game (Windows build): MCC's `ui.map`, bloodgulch (a local game,
  played) and a10 (its scripts ran its opening, switching its BSPs) load
  and run.
