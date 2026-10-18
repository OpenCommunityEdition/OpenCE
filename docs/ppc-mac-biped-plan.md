# Blood Gulch player-object conversion plan

Observed from the installed retail-v7 map copied to
`build/mac-audit/bloodgulch-v7.map`, with source inspection on 2026-10-15.
This is a dependency audit, not a new object-creation or hardware success claim.

The next useful increment is native conversion of the multiplayer biped's
model skeleton, animation graph and uncompressed animation streams, followed by
an actual engine animation-sampling test on that data. A biped scalar decoder
alone cannot make `object_new` work: `unit_new` requires an animation graph and
also creates the two starting weapons through recursive `object_new` calls.

## Which player definition is actually selected

`players.c` around lines 1420–1470 takes the multiplayer unit from
`game_globals.multiplayer_information[0].unit` when `game_engine_running()`.
The campaign `player_information[0].player_unit` is a different tag.

| Entry | Absolute tag index | Full handle | Name |
| --- | ---: | --- | --- |
| Game globals | 1583 | `e7a3062f` | `globals\globals` |
| Multiplayer biped | 147 | `e2070093` | `characters\cyborg_mp\cyborg_mp` |
| Campaign biped | 1861 | `e8b90745` | `characters\cyborg\cyborg` |
| Shared model | 148 | `e2080094` | `characters\cyborg\cyborg` (`mod2`) |
| Shared animation graph | 157 | `e211009d` | `characters\cyborg\cyborg` (`antr`) |
| Shared collision model | 181 | `e22900b5` | `characters\cyborg\cyborg` (`coll`) |

Globals contain one multiplayer-information record (root block offset `0x164`,
record stride `0xa0`, unit reference at record `0x10`), one player-information
record (`0x170`, stride `0xf4`), and one player-control record (`0x110`, stride
`0x80`). Preserve complete salted handles, not just absolute indices.

## Root biped layout and live references

The root is `0x4f4` bytes: object at `0x000` of size `0x17c`, unit at `0x17c`
of size `0x174`, biped at `0x2f0` of size `0x204`. Offsets below are from the
whole tag. They were calculated from the actual source declarations using
explicit 32-bit primitive/pointer-shaped types; add real engine-header ABI
assertions on the MacBook/PPC before using them as a native overlay.

| Field | Offset | Blood Gulch MP value |
| --- | --- | --- |
| Model reference | `0x28` | tag 148, `mod2`, not engine `mode` |
| Animation reference | `0x38` | tag 157 |
| Collision-model reference | `0x70` | tag 181 |
| Physics reference | `0x80` | NONE |
| Modifier shader | `0x90` | tag 210 `spla`, shield hit |
| Creation effect | `0xa0` | NONE |
| Attachments | `0x140` | 1; stride `0x48`; light tag 212 |
| Widgets | `0x14c` | 0 |
| Functions | `0x158` | 2; stride `0x168` |
| Change colors | `0x164` | 2; stride `0x2c`, nested permutation blocks |
| Predicted resources | `0x170` | 5 |
| Integrated-light effect | `0x188` | NONE |
| Spawned actor variant | `0x24c` | NONE |
| Unit melee damage | `0x288` | tag 216 `jpt!` |
| HUD references | `0x2a8` | 2; stride `0x30`; tags 217 and 230 `unhi` |
| Dialogue variants | `0x2b4` | 1; stride `0x18`; reference at +8 to tag 231 `udlg` |
| Powered seats | `0x2cc` | 0 |
| Initial weapons | `0x2d8` | 2; stride `0x24`; tags 234 assault rifle and 473 pistol |
| Unit seats | `0x2e4` | 0 (animation seats are a separate block) |
| Biped melee damage | `0x314` | NONE |
| Material effects | `0x38c` | tag 556 `foot` |
| Standing/crouching camera heights | `0x400` / `0x404` | approximately 0.62 / 0.35 |
| Standing/crouching collision heights | `0x424` / `0x428` | approximately 0.7 / 0.5 |
| Collision radius | `0x42c` | approximately 0.2 |
| Contact points | `0x4e8` | 2; stride `0x40`; marker names are bytes |

Convert all typed scalars, runtime cached values, shorts, references and nested
blocks; leave marker strings/padding alone. The existing tag-schema files
provide reference and nesting evidence but intentionally omit scalar fields,
so they are not a complete endian schema.

## The immediate native-data dependencies

1. **Model tag 148:** `model` root `0xe8`; markers at `0xac` (8), nodes at
   `0xb8` (19), regions at `0xc4` (1), geometries at `0xd0` (5), shaders at
   `0xdc` (4). Shader references are 149/151 (`schi`) and 152/155 (`soso`).
   Model nodes have default translation/rotation, hierarchy shorts and a
   runtime inverse matrix; preserve them exactly. `mod2` geometry parts are
   `0x84`, while this engine's parts are `0x68`: byte-swapping in place is
   insufficient. Audit/adapt `custom_edition_geometry.c`'s existing
   `custom_edition_model_convert` after bounded endian decoding. Its current
   native pointer casts require converted data before use. Convert model
   vertices and triangle strips separately from the tag-data area.
2. **Animation tag 157:** root `0x80`; 24 unit seats, 1 weapon-animation
   entry, 176 unit-damage indices, 23 sound references, 19 nodes, 254 animations.
   Empty blocks: object overlays, vehicle/device/first-person animations.
   Each animation is `0xb4`: mixed 16/32-bit metadata, node masks and three
   `tag_data` records at `0x48` (frame info), `0x8c` (defaults), `0xa0` (frames).
   Every observed animation has flags zero and compressed-data offset zero.
   The payload totals are 40,168 bytes frame info, 86,928 defaults and 857,064
   frames. This makes exact uncompressed stream conversion a manageable first
   target: walk node translation/rotation/scale masks and frame-info type,
   decode compressed-quaternion *components* (8-byte quaternion representation)
   as 16-bit values, and translations/scales as float words. An uncompressed
   animation stream can still contain packed quaternion components. Do not
   treat the entire payload as float words. Validate stream lengths against
   frame/node counts before invoking `model_animations.c`. Reject compressed
   streams initially rather than accidentally passing LE header offsets to
   native readers. Nested unit-seat weapon classes/types and animation-index
   lists must also become native before `unit_set_or_test_seat_and_weapon_label`.
3. **Collision tag 181:** root `0x298`; resistance portion `0x258`, materials
   at `0x234` (4), regions at `0x240` (1), modifiers at `0x24c` (0), pathfinding
   spheres at `0x280` (1), nodes at `0x28c` (19). Collision nodes have stride
   `0x40`, BSP block at +`0x34`. Sixteen nodes contain one BSP and three contain
   zero. Reuse the eight-block collision-field decoder concept, but this is
   tag-addressed collision data, not the separate BSP file-address mapping.
   Do not point the level-BSP decoder at these tag bytes without a range-domain
   abstraction. Resistance/effect references and region permutations are also
   consumed by object vitality/region initialization.
4. **Starting weapons and effects:** `unit_add_initial_weapons` creates weapon
   234 and 473 even while making the unit. Their object/model/animation/item/
   weapon blocks and referenced ammunition/projectiles/effects need their own
   closure audit. The one biped attachment creates a light. A correct playable
   creation path cannot silently drop these references or return fake success.

## Engine services required by actual creation

`objects.c:object_new` allocates the object header and per-node storage, calls
`object_type_new`, chooses color and region permutations, initializes vitality,
computes node matrices, reconnects to the map, exports/computes functions and
creates attachments. `object_types.c:object_type_new` invokes the registered
part-definition constructors, including unit and biped parts; calling only
`biped_new` is not equivalent.

`units.c:unit_new` refuses a missing animation graph, establishes seat/weapon
animation selection, assigns dialogue, and recursively creates initial weapons.
`bipeds.c:biped_new` initializes support state, origin/pathfinding point and
facing; later updates require actual unit-animation and collision behavior.

Consequently, use genuine object data pools, block allocation, tag lookup with
native ownership/relocation, animation sampling, model-node transforms, map
cluster/location reconnection and the relevant type registries. Link errors in
these paths are a list of missing services to implement, not justification for
no-op replacements. A tiny harness may isolate a real animation function using
real decoded map data without claiming that it creates a player object.

Recommended order: native skeleton + uncompressed animations and a source-based
sampling probe; biped/collision-resistance/root block conversion and native
reference closure validation; weapon dependency audit; full constructor path
with actual allocation/type/map services. Report each intermediate check as
such until an actual unit datum survives creation and updates in a map session.
