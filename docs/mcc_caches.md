# Halo: The Master Chief Collection's Halo 1 maps

The Halo 1 maps of Halo: The Master Chief Collection (MCC) are cache files of
version 13 (build string `01.03.43.0000`). The game does not play them: it
imports them, once, as the Xbox disc's maps (cache version 5), and plays those
as it plays the disc's. The MCC installation's folder is
`Halo The Master Chief Collection/halo1`; its maps are in `halo1/maps`, its
sounds in FMOD banks in `halo1/sound/pc`.

## The import

On a first start without game data the desktop builds offer an Xbox disc
image or MCC on Steam (`port/linux/src/mcc_install.c`, found through Steam's
libraries, or a folder the player picks). The import itself is
`halo --import-mcc <output folder> [--from <halo1 folder> | steam] [<map>...]`
(`port/linux/game/mcc_import.c`). The first start runs it in other copies of
the game, one map each and up to twelve at once (a core and about 0.5 GB of
memory each), the largest maps first: a campaign map takes about half a
minute, most of it its sounds, and a multiplayer map a few seconds (all 24
maps, on a 24-core computer: 266 s one at a time, 75 to 85 s four at once,
35 to 40 s twelve at once). MCC's folder is read as the drive `m:\` and
nothing is written to it. For each map:

1. It is loaded as below, as a Custom Edition map is, and its tags checked.
2. The importer's own fixes undo what Halo PC and MCC changed and need no
   data of the disc's (`mcc_import_patch.c`).
3. The map's operations give its tags the disc's values where they differ,
   and the disc's tag order (`port/assets/mcc/<map>.p2`, embedded in the game).
   A tag MCC has only in another map (the warthog, the ghost's gun) is taken
   from that map of the player's.
4. The tags are written in the disc's order, with the structure BSPs, the
   bitmaps (the HUD's brought to the disc's sizes), the sounds from MCC's
   banks (Xbox ADPCM copied, CELT decoded and encoded as Xbox ADPCM, music
   in the disc's 65,520-byte pieces) and the models' vertex buffers.
5. The written map's simulation data is fingerprinted and compared with the
   disc's (`port/assets/mcc/<map>.fp`). A map that matches is written as the
   disc's and plays with players of the disc; any other is marked with
   `MCC_MAP_VERSION` in its header and plays only with other such maps
   (`cache_files_map_version`; network version 44,
   `port/linux/NETCODE.md`). `halo --fingerprint <map>` prints a map's
   fingerprint, the disc's or an imported one's, as the `.fp` files have it
   (`port/assets/mcc/README.md`). Halo PC's seven rebuilds of the multiplayer maps
   (beavercreek, bloodgulch, boardingaction, carousel, damnation, hangemhigh,
   sidewinder) have other collision, and are always MCC's.

A map larger than its cache file holds (the Xbox's: 0x11600000 bytes for a
campaign level, 0x2F00000 for a multiplayer map, 0x2300000 for the menus' map)
cannot be loaded: it is imported again with its largest level and effects
textures halved (a compressed one by its next mipmap) until it fits, the HUD's
and the menus' left as they are. ratrace and Halo PC's rebuilds need it.

`ui.map`, the menus' map: MCC's is Halo PC's, with Halo PC's own menus. The port
draws the PC menus from its own files (`port/assets/menus`), so the import
writes only what the game reaches of it: the scenario, the globals and the
tags the code and the menus name, but not Halo PC's main menu and the screens
it leads to; its bitmaps at the disc's sizes. A picture the menus take from the
Xbox's map is the PC version's frame of the same place, and one MCC lacks (the
controller picture of the profile's settings) is not drawn (`menu_tags.c`).
ADD PLAYER's screen, the Xbox's four-player profile select, whose widgets MCC
lacks, is the port's own in their places (the menus' `map_lacks`): the same
texts, functions and layout on the PC menus' flat panel. The Xbox's own menus
(`display.menus = "xbox"`) need the disc's `ui.map`.

What does not import yet:

- Halo PC's six multiplayer maps.
- A few tags of the disc's are assets MCC lacks or changed as geometry (the
  flamethrower, `flag_base`, `teleporter_base`, `c_field_generator`, the
  cyborg's passenger animations): without them a map is written as MCC's.
- Every multipurpose map is drawn in the Xbox's channel order. MCC's flag
  for the Xbox's order (bit 6 of a model shader's flags) does not say which
  are Halo PC's: the scorpion's and Halo PC's fuel rod gun's
  (`weapons\plasma_cannon`) shaders lack it, and their bitmaps draw right in
  the Xbox's order (the scorpion's is the disc's, byte for byte). The
  flamethrower's screen (`flamethrower_screen`, Halo PC's alone) may hold
  Halo PC's order.

## Loading

The import loads a map with `custom_edition_cache_load`
(`port/linux/game/cache_file_formats.c`), which takes a version 13 cache as it
takes a Custom Edition one, but for these:

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
   vertex data fields are empty; the words at `0xB8` and `0xCC` of each
   material give its two parts' offsets in the region, which are those of
   this order (all 3,939 materials of a10, d40 and bloodgulch). The loader
   checks that the region ends where the BSP starts, that the materials'
   vertices fill it exactly, and that every normal is a unit vector's.
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
   it, at the top of the tag cache. As the BSP is read
   (`custom_edition_structure_bsp_read`, from the import), its region is read after it, its pointers are moved
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
   and in the map itself when they do not (the BSPs' lightmaps).
   `bitmaps.map` is a Custom Edition resource map of pixels alone, one entry
   per bitmap (named `<tag>_<index>`), whose offsets the bitmaps give. Bit 9
   is MCC's own: every bitmap in the map has it, and so do a few in
   `bitmaps.map` (a10: 30, among them environment textures), so it does not
   say where the pixels are. The game would refuse it, and it is cleared
   (`mcc_import_bitmaps_prepare`).
8. A sound's samples name a `sounds.map` MCC does not ship (Halo PC's). Its
   sounds are made unplayable (`mcc_sounds_mute`) until they are taken from
   MCC's banks.
9. The HUD is laid out at twice the Xbox's size, with bitmaps four times the
   Xbox's (`mcc_import_hud_convert`): every placement's offset is doubled
   (but the few with Halo PC's high resolution scale flag, all vehicles' health,
   whose offsets are the Xbox's), the digits' metrics and the damage
   indicators' offsets are doubled, and every bitmap a placement draws has
   the half HUD scale flag. Each placement's offset and scale is halved, the
   weapons' numbers' and the grenade count's too; the Custom Edition
   conversion then halves those of flagged bitmaps again, as it does a
   Custom Edition map's; the digits' bitmap is given the flag, and
   `hud_draw_numbers` halves an MCC map's digits once more.
10. Its bitmaps' channels are the Xbox's, not Halo PC's: its model shaders'
    multipurpose maps are not reordered as a Custom Edition map's are, and
    its HUD meters are taken in the Xbox's order (fill order in color, shape
    in alpha: `custom_edition_bitmaps_orders_set`), where Halo PC's would be
    read from alpha (a flat 225 to 255 in MCC's: the bars never emptied).

`port/tools/cache_file_report.c` checks a version 13 cache as far as the move
of its tags, which needs the game's schemas.

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
- Imported (Windows build): the ten campaign maps and chillout, longest,
  prisoner, putput, ratrace and wizard match the disc's fingerprint with the
  disc's assets; every imported map loads and runs, and a hosted game on an
  imported multiplayer map places the disc's items.
