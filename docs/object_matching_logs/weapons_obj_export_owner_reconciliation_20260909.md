# Weapons export-owner reconciliation (2026-09-09)

## Scope and disposition

This packet admits the public `weapon_export_function_values` and its sole
private helper, `weapon_trigger_get_charged_fraction`, as one atomic owner
boundary.  It starts from canonical
`1d98839b5` (parent publication tip `6f8644c0c`) and supersedes only the hold in
`weapons_obj_export_owner_schema_rejected_20260906.md`.

The earlier rejection explicitly allowed reopening for genuinely changed
source/type/compiler-owner context.  The current tree satisfies that condition:
after moving the runtime flag to its shared owner, the previously sensitive
GameEngine sentinel `_populate_statistic_buffer` remains strict exact at 560
padded bytes.  The pair is therefore no longer being admitted by waiving an
inherited loss.

## Authenticated owners

- `weapon_export_function_values` keeps its existing public owner in
  `items/weapons.h`; its declaration is reformatted to the campaign parameter
  convention.
- January's private `_code_000ead20` is renamed in place to
  `weapon_trigger_get_charged_fraction`, marked static, and declared/defined
  only in `weapons.c`.  HCEA's symbol build independently authenticates that
  private name, `real` return, and `(long, short)` ABI.
- Modes 0 through 16 are defined in the dedicated
  `items/weapon_export_function_mode.h`, following the HCEA source/header
  boundary for the selector stored in `function_modes[4]`.  January's jump
  table proves the exact values and behavior; the `_weapon_function_*`
  spellings are corroborated by the HCEA database.  HCEA-only cases 17 and 18
  are not added.
- Runtime bit zero is named `_weapon_overheated_bit` in the dedicated
  `items/weapon_datum_flags.h`, also following the HCEA source/header boundary.
  The obsolete duplicate local enumeration in `first_person_weapons.c` is
  removed.  The unrelated existing bit-five owner in `weapons.h` is unchanged;
  no intervening January values are invented.
- `weapons.c` directly includes `game/game.h` for `game_time_get`; no local or
  implicit prototype is used.

Typed weapon and definition access use the existing subsystem macros, tag
block traversal uses `TAG_BLOCK_GET_ELEMENT`, and parent traversal uses the
typed `object_get` macro.  All switch values use semantic enumerators and all
new functions have explicit terminal returns.

The two pointer initializations in the charged switch arm may look unused, but
they execute January's tag-block and trigger-index validation paths.  Removing
them deletes 48 padded bytes and two relocations.  Retaining those observable
DEBUG validations is source-faithful behavior, not artificial padding.

## Exact result

The isolated whole-TU gate reports:

| Function | Padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `weapon_export_function_values` | 912 | 31 | strict exact |
| `weapon_trigger_get_charged_fraction` | 144 | 8 | strict exact |

Weapons advances from 47 to 49 strict-exact functions, with zero inherited
Weapons losses and no emitted `_point_from_line3d`.  The normalized hashes
remain the independently recorded January values from the September 1 ledger:
`1f408000df04ee0db2ee5b5677e13c5a036303de69ccf4b58c388d4a28651c84`
and
`26e7b612bdcdb8bf3a44017fc2c4192291944140b01273d19d34e327816f0eae`.

Targeted gates also preserve all accepted sections in the two high-risk shared
header consumers and the protected Units sentinel:

- GameEngine: 169 exact, including `_populate_statistic_buffer`;
- FirstPersonWeapons: 26 exact.
- Units: 189/189 exact, including the 1,920-byte
  `_unit_preprocess_node_orientations`.

A rejected intermediate trial put both enums into the broad existing headers.
The universal semantic sweep correctly caught that this reopened Units despite
the new Weapons sections being exact.  That placement received no credit and
was removed.  The dedicated owner headers restore Units without changing either
new Weapons section; this is the same narrow-owner strategy used elsewhere in
the campaign for VC7's definition-position sensitivity.

The fake-match scanner reports zero review leads across all four changed
source/header paths.  The repository-wide sweep, semantic byte accounting,
and remaining shared-header blast validation are deferred to the containing
>=10,000-meaningful-byte publication batch.  No residual or unwritten Weapons
function is credited by this packet.
