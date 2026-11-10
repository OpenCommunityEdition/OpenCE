# The MCC import's data

The importer (`halo --import-mcc`, `port/linux/game/mcc_import.c`) turns the
Halo 1 maps of a Steam installation of The Master Chief Collection into the
Xbox disc's maps. These files tell it where MCC's maps differ from the disc's.
The game embeds them (`tools/mcc_patches_embed.py`, desktop builds only). A
file of the same name in the data root's `mcc_patches` folder is read in
place of the game's copy, so that a changed file can be tried without a build.

| File | Contents |
| --- | --- |
| `<map>.p2` | The disc's values as operations, one per line (`mcc_import_patch.c` describes each kind): numbers, flags, names, the order of the tags, and the HUD's placements and sizes. A Halo PC rebuild (beavercreek, bloodgulch, boardingaction, carousel, damnation, hangemhigh, sidewinder) has the tag order and the HUD alone. |
| `ui.p2` | The menus' map's: the tags of MCC's (Halo PC's) the game reaches, in MCC's order, its preload list emptied, and the disc's sizes of its bitmaps. It has no fingerprint: it is never the disc's. |
| `<map>.fp` | The fingerprint of the disc's map: a SHA-256 of each part of its simulation data (each tag, the order, the scenario, each BSP's collision; the first 16 bytes of each, all 32 of the whole map's). An imported map whose fingerprint is the same is written as the disc's map and plays with players of the disc; any other is written with MCC's map version. |

What is not here:

- No pixels, sounds, models or collision geometry of the disc's. The few
  tags the disc has and MCC lacks or changed as geometry (the flamethrower,
  `flag_base`, `teleporter_base`, `c_field_generator`, the cyborg's passenger
  animations) are left as MCC has them, so those maps are written with MCC's
  version.
- No data of MCC's. A tag MCC has only in another map (the warthog, from
  a10; the ghost's gun, from b30) is named by a `donor` line, and the import
  takes it from the user's own MCC map (into the save folder's
  `z/mcc_import`).

How they are made:

- A fingerprint is the game's (`mcc_import_fingerprint.c`), of the disc's
  map. Anyone with the disc can make it again and compare it with the file:

  ```sh
  HALO_DATA_ROOT=<the disc's game data> halo --fingerprint a10 > a10.fp
  ```

  `halo --fingerprint <map>` prints the fingerprint of the data root's
  `maps/<map>.map` (the disc's, or one the import wrote). The fields it
  takes as text or leaves out are `port/linux/game/mcc_fingerprint_fields.h`,
  which `python tools/mcc_fingerprint_fields.py` writes from the game's
  structures (`--check` tells whether it is what it would write).
- The operations come from a comparison of each map's tags with the disc's,
  field by field, made with tools that are not in the repository: what they
  hold is data of the disc's (see the pull request's question on it).
