# Common constants and representation reference

Verified against canonical on 2026-09-06. Start here before guessing literals.

This is a lookup guide, not a new constants owner. Most rows record high-use
names that already have canonical owners; the supplied-source crosswalk is
explicitly separate. Future reconstructions should include the
owning header and use the existing symbol or macro; they should not duplicate
these declarations in a translation unit merely to obtain a desired code
shape.

`Confirmed` below means the value and spelling are present in the named
canonical owner. Float bits show the rounded binary32 representation; tag
immediates use the pinned MSVC convention. January corroboration is called out
explicitly below. A recognizable bit pattern is a lookup clue, not proof of
which source expression produced it. `Context-specific` means a value belongs
to the stated enum, storage type, or algorithm, not an unrelated global owner.

## Core values and representations (confirmed)

| Purpose | Canonical symbol / type | Value or representation | Owning source | Important boundary |
| --- | --- | --- | --- | --- |
| Boolean true/false | `TRUE`, `FALSE` | `1`, `0` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Use these for the project's one-byte `boolean` APIs. A `boolean` i386 return may define only `AL`; a `void` return promises no value. Do not require unrelated upper `EAX` bits. |
| Invalid signed index/handle | `NONE` | `-1` (`0xFFFFFFFF` as a 32-bit `long`) | [source/cseries/cseries.h](../source/cseries/cseries.h) | This is not `NULL`. Confirm the destination width: a byte or word sentinel has the narrowed all-ones representation, and some APIs use zero as a distinct valid or invalid value. |
| Simulation rate | `TICKS_PER_SECOND`, `ACTUAL_TICKS_PER_SECOND` | `30`, `30` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Express seconds-to-ticks as `seconds * TICKS_PER_SECOND`; express per-tick scale as `1.0f / TICKS_PER_SECOND`. Do not invent a `SECONDS_PER_TICK` owner. The latter folds to float bits `0x3D088889`. |
| Wall-clock unit | `MILLISECONDS_PER_SECOND` | `1000` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Do not mix millisecond tag fields with 30 Hz game ticks. Existing code explicitly converts when needed. |
| Display cadence | `VBLANKS_PER_SECOND` | `60` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Vblanks are not simulation ticks even though both are integer rates. |
| Integer widths | `LONG_BITS`, `SHORT_BITS`, `CHAR_BITS` | `32`, `16`, `8` | [source/cseries/cseries.h](../source/cseries/cseries.h) | These describe the January/Win32 ABI. Preserve signedness as well as width. |
| Integer extrema | `LONG_MAX`, `LONG_MIN`, `UNSIGNED_LONG_MAX`, `SHORT_MAX`, `SHORT_MIN`, `UNSIGNED_SHORT_MAX` | `2147483647`, `-2147483648`, `0xFFFFFFFF`, `32767`, `-32768`, `0xFFFF` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Prefer the named bound. `UNSIGNED_LONG_MAX` is also the canonical all-object mask, but that does not make it a universal sentinel. |
| Scalar real | `real` | `typedef float real` (4-byte IEEE-754 binary32 in target ABI) | [source/cseries/cseries.h](../source/cseries/cseries.h) | Use `f` suffixes for ordinary `real` constants. Use `double` only where the genuine API or default vararg promotion requires it. |
| Engine boolean | `boolean` | `typedef byte boolean`; `byte` is `unsigned char` | [source/cseries/cseries.h](../source/cseries/cseries.h) | It is one byte, not C/C++ `bool` and not a four-byte `long`. This affects fields, parameters, stack snapshots, and return checks. |
| Tag word | `tag` | `typedef unsigned long tag` (32 bits in target ABI) | [source/cseries/cseries.h](../source/cseries/cseries.h) | Preserve the four-character group token and the unsigned 32-bit type. |
| Coordinate axes | `_x`, `_y`, `_z` | `0`, `1`, `2` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Use these only as component/projection indices. They are not vector values or bit masks. |
| Geometric tolerance | `_real_epsilon` | `0.0001f`, binary32 bits `0x38D1B717` | [source/math/real_math.h](../source/math/real_math.h) | This is not a universal replacement for exact-zero tests. January functions use both epsilon and exact `0.0f`; preserve the observed predicate and NaN behavior. |
| Pi | `_pi` | `(real)M_PI`, binary32 bits `0x40490FDB` | [source/math/real_math.h](../source/math/real_math.h) | Use the project cast rather than a hand-rounded decimal when the source works in `real`. |
| Angle conversion | `DEGREES_TO_RADIANS`, `RADIANS_TO_DEGREES` | `((real)angle) * _pi / 180.f` or `((real)angle) * 180.f / _pi` | [source/math/real_math.h](../source/math/real_math.h) | Angles stored in tags are not uniformly degrees or radians. First authenticate the field's unit; then use the owner macro. Expression order can affect exact float bytes. |
| Finite real envelope | `REAL_MIN`, `REAL_MAX` | `-3.4028235e38f`, `3.4028235e38f` | [source/math/real_math.h](../source/math/real_math.h) | `REAL_MIN` here is the most-negative finite bound, not the C-library meaning “smallest positive normal.” |

January examples that corroborate the float rows are the
`__real@38d1b717`, `__real@41f00000`, and `__real@3d088889` relocations in
`build/split/source/objects/damage.obj`. They represent `_real_epsilon`,
`(real)TICKS_PER_SECOND`, and `1.0f / TICKS_PER_SECOND`; they do not justify
replacing the source-level owners with hexadecimal literals.

## Canonical macros whose semantics matter

| Macro | Definition owner | Correct use | Pitfall |
| --- | --- | --- | --- |
| `MIN(a,b)`, `MAX(a,b)` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Ordinary project min/max spelling when January establishes the same comparison and argument order. | Each expression can be evaluated more than once. Argument order controls signed-zero and NaN outcomes: do not normalize or swap operands merely because finite results look equivalent. |
| `FLOOR`, `CEILING`, `PIN` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Named lower/upper/clamp operations. | `PIN` is a macro composition, not an IEEE `fminf`/`fmaxf` substitute. It may evaluate the value repeatedly and lets a NaN value pass through under its current ordering. |
| `FLAG(bit)` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Convert an enum *bit index* to a mask. | Most `_..._bit` enum members are indices, not masks. Do not apply `FLAG` again to an existing `_..._mask_*` value. Preserve the owning integer width, especially near bit 31. |
| `TEST_FLAG`, `SET_FLAG` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Test or mutate a named bit in the authenticated field. | Do not replace an enum bit with a raw hex mask. Avoid side-effecting lvalues/expressions because these are macros. |
| `VALID_FLAGS(flags,bits)` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Validate that no bit at or above the declared count is set. | `bits` is a count, not a mask; signedness and shift width matter. |
| `BIT_VECTOR_SIZE_IN_LONGS`, `BIT_VECTOR_TEST_FLAG`, `BIT_VECTOR_SET_FLAG` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Long-word-packed bit vectors, 32 flags per `long`. | The macros index a `long` array. Do not silently cast an original `byte *` API to this representation. A byte-buffer use needs independent owner/caller evidence, as documented for the collision BSP boundary in [docs/object_matching_logs/collision_bsp_obj_sphere_cluster_20260904.md](object_matching_logs/collision_bsp_obj_sphere_cluster_20260904.md). |
| `NUMBEROF(array)` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Compile-time count of an actual array in the same scope. | On a pointer it returns a pointer-size ratio, not the pointee count. |
| `VALID_INDEX(index,count)` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Signed lower-bound and upper-bound check. | Preserve the signed type. Replacing it with an unsigned comparison can alter code shape and invalid-negative behavior. |
| `DATUM_INDEX_NEW`, `DATUM_INDEX_TO_ABSOLUTE_INDEX`, `DATUM_INDEX_TO_IDENTIFIER` | [source/cseries/cseries.h](../source/cseries/cseries.h) | Construct or split the 16-bit absolute index / 16-bit identifier handle representation. | A datum handle is not a plain array index. Mask before indexing; do not discard the identifier except at a proven boundary. |
| `TAG_BLOCK_GET_ELEMENT` | [source/tag_files/tag_groups.h](../source/tag_files/tag_groups.h) | Typed access through `tag_block_get_element_with_size(..., sizeof(type))`. | The type and element size are part of the runtime contract. Avoid a raw `address + index*stride` replacement. |

## Tag strings and common group IDs (confirmed)

`TAG_STRING_LENGTH` is **31** ([source/tag_files/tag_files.h](../source/tag_files/tag_files.h)). A
canonical fixed tag string buffer that includes its terminator is therefore
usually `char name[TAG_STRING_LENGTH + 1]`, or 32 bytes. The constant is a
payload limit, not permission to omit the final NUL.

MSVC packs the following four-character constants into the shown immediate
value. Always write the named owner; the hex form is provided only to read
disassembly and relocations. On little-endian x86 the bytes in memory appear
least-significant byte first.

| Domain | Symbol | Source token / i386 immediate | Owner |
| --- | --- | --- | --- |
| Game globals | `GAME_GLOBALS_TAG` | `'matg'` / `0x6D617467` | [source/game/game_globals.h](../source/game/game_globals.h) |
| Scenario | `SCENARIO_TAG` | `'scnr'` / `0x73636E72` | [source/scenario/scenario_definitions.h](../source/scenario/scenario_definitions.h) |
| Structure BSP | `STRUCTURE_BSP_TAG` | `'sbsp'` / `0x73627370` | [source/structures/structure_bsp_definitions.h](../source/structures/structure_bsp_definitions.h) |
| Base object definition | `OBJECT_DEFINITION_TAG` | `'obje'` / `0x6F626A65` | [source/objects/object_definitions.h](../source/objects/object_definitions.h) |
| Damage effect | `DAMAGE_EFFECT_DEFINITION_TAG` | `'jpt!'` / `0x6A707421` | [source/objects/damage_effect_definitions.h](../source/objects/damage_effect_definitions.h) |
| Bitmap | `BITMAP_GROUP_TAG` | `'bitm'` / `0x6269746D` | [source/bitmaps/bitmap_group.h](../source/bitmaps/bitmap_group.h) |
| Sound | `SOUND_DEFINITION_TAG` | `'snd!'` / `0x736E6421` | [source/sound/sound_definitions.h](../source/sound/sound_definitions.h) |

Four-character C constants are implementation-defined outside this compiler
context. Do not byte-swap them manually or replace a typed getter such as
`object_definition_get` with a raw `tag_get` call only to expose the numeric
immediate.

## Common engine enums and limits (confirmed, domain-specific)

| Domain | Symbol(s) | Value | Owner and boundary |
| --- | --- | --- | --- |
| Local rendering/input players | `MAXIMUM_LOCAL_PLAYERS` | `4` | [source/game/players.h](../source/game/players.h). This is not the network/player-datum capacity. |
| Difficulty | `_game_difficulty_level_easy`, `_game_difficulty_level_normal`, `_game_difficulty_level_hard`, `_game_difficulty_level_impossible` | `0`, `1`, `2`, `3` | [source/game/game.h](../source/game/game.h). Use the enum; a raw ordinal is ambiguous at a call site. |
| Campaign team | `_game_team_default`, `_game_team_player`, `_game_team_human`, `_game_team_covenant`, `_game_team_flood`, `_game_team_sentinel` | `0` through `5` | [source/game/game.h](../source/game/game.h). Multiplayer `_team_red=0`, `_team_blue=1` intentionally overlaps numerically; the field/API determines the namespace. |
| Object types | `_object_type_biped` through `_object_type_sound_scenery` | `0` through `11`; `NUMBER_OF_OBJECT_TYPES=12` | [source/objects/object_types.h](../source/objects/object_types.h). Values are ordinals used by object headers and switch statements. |
| Object masks | `_object_mask_unit`, `_object_mask_item`, `_object_mask_device`, `_object_mask_all` | `0x3`, `0x1C`, `0x380`, `0xFFFFFFFF` | [source/objects/object_types.h](../source/objects/object_types.h). These are already masks; test them against an object type with the established `TEST_FLAG(mask, type)` convention. |
| Object capacity | `MAXIMUM_OBJECTS_PER_MAP` | `2048` | [source/objects/objects.h](../source/objects/objects.h). Capacity is not the current datum count. |
| Rendered-object capacity | `MAXIMUM_RENDERED_OBJECTS` | `256` | [source/objects/objects.h](../source/objects/objects.h). Do not use it to size the master object datum array. |
| Markers/regions/attachments | `MAXIMUM_MARKERS_PER_OBJECT`, `MAXIMUM_REGIONS_PER_OBJECT`, `MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT` | `64`, `8`, `8` | [source/objects/objects.h](../source/objects/objects.h) and [source/objects/object_definitions.h](../source/objects/object_definitions.h). These belong to different arrays despite sharing an object definition. |
| Damage material modifier capacity | `MAXIMUM_NUMBER_OF_MATERIAL_TYPES` | `40` | [source/game/game_globals.h](../source/game/game_globals.h); used by `damage_definition.material_modifiers` at [source/objects/damage_effect_definitions.h](../source/objects/damage_effect_definitions.h). It is storage capacity, not the current named-material count. |

## Damage flags relevant to the current caller/helper packet

These are confirmed but context-specific. The enum values are **bit indices**;
use `TEST_FLAG`/`SET_FLAG` rather than the numeric masks shown for audit
convenience.

| Storage | Symbol | Bit / mask | Owner |
| --- | --- | --- | --- |
| `being_damaged_flags` | `_object_being_damaged_body_depleted_bit` | `0` / `0x01` | [source/objects/damage.h](../source/objects/damage.h) |
| `being_damaged_flags` | `_object_being_damaged_shield_depleted_bit` | `3` / `0x08` | [source/objects/damage.h](../source/objects/damage.h) |
| `being_damaged_flags` | `_object_being_damaged_by_friendly_bit` | `4` / `0x10` | [source/objects/damage.h](../source/objects/damage.h) |
| `being_damaged_flags` | `_object_being_damaged_multiplied_by_difficulty_bit` | `5` / `0x20` | [source/objects/damage.h](../source/objects/damage.h) |
| `damage_data.flags` | `_damage_no_statistics_bit` | `7` / `0x80` | [source/objects/damage.h](../source/objects/damage.h) |
| `object.damage_flags` | `_object_passed_shield_damage_threshold_bit` | `1` / `0x0002` | [source/objects/objects.h](../source/objects/objects.h) |
| `object.damage_flags` | `_object_shield_depleted_bit` | `3` / `0x0008` | [source/objects/objects.h](../source/objects/objects.h) |
| `object.damage_flags` | `_object_shield_over_charging_bit` | `4` / `0x0010` | [source/objects/objects.h](../source/objects/objects.h) |
| `object.damage_flags` | `_object_cannot_take_damage_bit` | `11` / `0x0800` | [source/objects/objects.h](../source/objects/objects.h) |

Do not merge the two “shield depleted” names: one is an event/result bit in a
32-bit accumulator and the other is persistent object state in a 16-bit
field. Their current numeric bit happens to be the same.

January-specific category, side-effect, material-response, and private-helper
values should remain in the owning Damage enum unless a genuine shared owner
is found. A comparison against `1`, `2`, or `3` in disassembly is not enough
to infer which enum namespace it belongs to. Likewise, the Damage aftermath
upward bias and similar algorithm coefficients are not general math
constants; retain a semantic local expression and document its function-level
provenance instead of exporting a global name.

## Assertions and object-access macros

See [the shared assertion API and Units example](assertions.md). Assertions test
an invariant that must be true; a diagnostic error condition usually needs
negating. `_object_connected_to_map_bit` is bit 11 in `object.flags`, whereas
`_object_header_connected_to_map_bit` is bit 5 in a separate header flags byte.
Do not substitute one for the other.

Use typed accessors such as `unit_get`, `weapon_get`, `object_definition_get`,
`bitmap_group_get`, and `sound_definition_get` from their existing subsystem
headers. Keep raw `tag_get` and object-access casts encapsulated there rather
than repeating casts at each call site.

## Supplied-source crosswalk (evidence, not yet canonical declarations)

The user supplied three CE source files on 2026-09-06. Their hashes, local
locations, target fingerprints, and integration caveats are recorded in the
[source reconstruction map](user_source_reconstruction_map_20260906.md).
The following names occur there; do not assume they are already available
from canonical headers or export TU-private coefficients globally.

| Supplied name / expression | Value | Canonical correspondence and scope |
| --- | --- | --- |
| `RANDOM_A` | `1664525L` / `0x0019660D` | Literal in [source/math/random_math.c](../source/math/random_math.c); LCG multiplier, not a general math constant. |
| `RANDOM_C` | `1013904223L` / `0x3C6EF35F` | Same owner; LCG increment. Seed arithmetic is unsigned 32-bit modulo arithmetic. |
| `RANDOM(seed)` | Update seed, then take its upper 16 bits | Source macro uses a 16-bit right shift; do not substitute low bits or mutate the seed twice. |
| `REAL_RANDOM(seed)` | `(1.f / (real)0xffff) * RANDOM(seed)` | Divisor is `65535`, not `65536`; the source real result can reach both 0 and 1. Integer range scaling instead uses a right shift by 16 and is upper-exclusive for its valid range. Preserve expression order and promotions. |
| `RANDOM_DIRECTION_TABLE_GEOSPHERE_SEGMENT_COUNT` | `16` | RandomMath's geosphere construction argument, not a player capacity or direction-table length. |
| `COUNTDOWN_TIMER_CANNOT_ACCLERATE_AMOUNT` | `MILLISECONDS_PER_SECOND - 1` = `999` | Source spelling includes `ACCLERATE`; canonical server calls it `NETWORK_GAME_MINIMUM_COUNTDOWN_TIME`. |
| `CLIENT_COUNTDOWN_TIMER_MODIFICATION_AMOUNT` | `5 * MILLISECONDS_PER_SECOND` = `5000` | Canonical server `NETWORK_GAME_COUNTDOWN_ADJUSTMENT`. |
| `LAN_INITIAL_SERVER_GAME_COUNTDOWN` | `31 * MILLISECONDS_PER_SECOND - 1` = `30999` | Canonical server `NETWORK_GAME_COUNTDOWN_TIME`. |
| `LOCAL_INITIAL_SERVER_GAME_COUNTDOWN` | `11 * MILLISECONDS_PER_SECOND - 1` = `10999` | Canonical server `NETWORK_GAME_SPLITSCREEN_COUNTDOWN_TIME`. |
| `MAXIMUM_SERVER_GAME_COUNTDOWN` | `31 * MILLISECONDS_PER_SECOND - 1` = `30999` | Separate semantic role from the LAN initial time, despite equal values. |
| `NETWORK_JOIN_GAME_TOKEN_SIZE` | `16`, corroborated by January's token copy | Name is referenced by supplied source but its defining header is absent. Restore the genuine shared owner before using it across callers; do not claim its original declaration was supplied. |
| `NETWORK_GAME_CLIENT_CONNECTION_PROCESS_TIMEOUT_MILLISECONDS` | Debug `120000`; release `15000` | Supplied client enum selects `120` or `15` times `MILLISECONDS_PER_SECOND` under `DEBUG`; do not use one build's timeout in the other. |
| `NETWORK_GAME_CLIENT_GAME_SEARCH_INTERVAL_MILLISECONDS` | `2000` | Supplied client search cadence, `2 * MILLISECONDS_PER_SECOND`. |
| `NETWORK_GAME_CLIENT_GAME_ADVERTISED_GAME_TIMEOUT` | `6000` | Supplied client: twice the search interval plus `2 * MILLISECONDS_PER_SECOND`; not the connection timeout. |
| `NETWORK_GAME_CLIENT_GAME_PING_INTERVAL_MILLISECONDS` | `1000` | Supplied client ping cadence, `1 * MILLISECONDS_PER_SECOND`. |
| `NETWORK_GAME_CLIENT_INITIAL_PACKET_SEQUENCE_NUMBER` | `1` | Supplied client protocol sequence origin; not a universal first array index. |

Countdown correspondences are in
[source/networking/network_server_manager.c](../source/networking/network_server_manager.c).
The supplied server's client-machine flag indices are connected `0`,
validated `1`, level-loaded `2`, and precached `3`. Canonical currently uses
joined/loaded names for indices 1/2; numeric agreement alone does not authorize
a global semantic rename. These flags are not object-datum flags.

The supplied RandomMath also references `_full_circle`, `_half_circle`, and
`_quarter_circle`, but their defining header was not supplied and they are not
present in the current canonical math headers. Do not invent a declaration
or infer exact expression ordering solely from those names.

The supplied client names its two flag indices
`_network_game_client_connected_to_server_bit = 0` and
`_network_game_client_sent_join_request_to_server_bit = 1`. Canonical uses
shorter inferred names and literal timing values; restore names only with
ordinary strict-object and consumer checks, not by unreviewed bulk replacement.

## Reconstruction checklist

1. Search for an existing owner before adding a constant, enum, macro, or
   typedef. Include that owner instead of redeclaring it locally when header
   placement is safe.
2. Establish the storage type and namespace before interpreting `-1`, `0`, or
   a bit number. `NONE`, `NULL`, a tag index, and a byte terminator are not
   interchangeable merely because some bytes match.
3. Compare exact float bytes and x87 predicates before replacing a literal
   with `_real_epsilon`, `MIN`, `MAX`, or `PIN`. Finite agreement does not
   prove NaN, infinity, tangent, or signed-zero agreement.
4. Treat enum `_..._bit` members as bit positions and `_..._mask...` members
   as masks. Keep the owning field width; do not widen a byte/word flag field
   to `long *` for macro convenience.
5. Treat every maximum as a capacity for its named owner, not a current count
   or a substitute capacity for a nearby array.
6. Preserve public type widths at ABI boundaries. In particular, `real` is
   float and `boolean` is byte even when i386 register or stack mechanics make
   a mismatched declaration appear to work.

Useful corroborating ledgers:

- [docs/object_matching_logs/damage_obj_object_damage_update_20260904.md](object_matching_logs/damage_obj_object_damage_update_20260904.md)
  documents project `real`, enum, flag, tick, and clamp use in a verified
  Damage owner.
- [docs/object_matching_logs/bipeds_obj_jonas_disconnect_bsp_20260827.md](object_matching_logs/bipeds_obj_jonas_disconnect_bsp_20260827.md)
  records the January all-bits-set `NONE` representation in real object
  fields.
- [docs/object_matching_logs/collision_bsp_obj_sphere_cluster_20260904.md](object_matching_logs/collision_bsp_obj_sphere_cluster_20260904.md)
  records a specifically authenticated byte-buffer/bit-vector boundary; it is
  an exception requiring evidence, not a blanket license for representation
  casts.
