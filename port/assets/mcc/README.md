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
| `<map>.fp` | The fingerprint of the disc's map: an MD5 of each part of its simulation data (each tag, the order, the scenario, each BSP's collision). An imported map whose fingerprint is the same is written as the disc's map and plays with players of the disc; any other is written with MCC's map version. |

What is not here:

- No pixels, sounds, models or collision geometry of the disc's. The few
  tags the disc has and MCC lacks or changed as geometry (the flamethrower,
  `flag_base`, `teleporter_base`, `c_field_generator`, the cyborg's passenger
  animations) are left as MCC has them, so those maps are written with MCC's
  version.
- No data of MCC's. A tag MCC has only in another map (the warthog, from
  a10; the ghost's gun, from b30) is named by a `donor` line, and the import
  takes it from the user's own MCC map (into the data root's `mcc_import`
  folder).

The files are made from the disc's maps and MCC's with the prototype tools
(not yet in the repository): the comparison of each map's tags, field by
field, writes the operations; the fingerprint is computed from the disc's map
by the same code as `mcc_import_fingerprint.c`.
