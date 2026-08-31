# Claude unwritten-function wave: provenance audit and narrow admission (2026-08-31)

## Scope

This ledger records the independent audit of a small subset of Claude campaign
commit `e02971dfd` and related recovered Weapons/Decals source against canonical
commit `bb913dc00`. Direct function equality was treated as necessary but not
sufficient: owned data, type and field names, translation-unit ownership, frozen
first-shot evidence, and all inherited accepted functions were reviewed before
admission.

## Accepted exact function

| Unit | Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `source/items/weapons` | `_weapon_set_current_amount` | `343 / 352` | `13` | `ef897bdb704568f2ed368d80d662d47450dd62fee3f32d6bc4d82b375368e5bc` |

The retained function is ordinary typed C corroborated by the later HCEA source.
It uses the canonical `real`, `weapon_datum`, `weapon_definition`, trigger, and
magazine structures, the shared `fast_ftol` helper, and no compiler controls or
source-shape tricks. It has no recovered cross-TU caller, so no speculative
declaration is added to `weapons.h`.

The existing public pickup declaration is corrected only to the authenticated
parameter order and names: `weapon_index`, `item_object_index`,
`local_player_index`, and `rounds_picked_up`. A full rebuild and accepted-ledger
comparison prove that this names-only header change removes no accepted function,
including the protected Units sentinel.

## Honest fuzzy retention

`_decals_delete_permanent_from_cluster` is retained without exact credit. Its
ordinary-C linked-list traversal is corroborated by January assertion strings and
line numbers and by the later HCEA reconstruction. Target and candidate are both
400 padded bytes with the same 29 relocation identities; objdiff is `99.81035%`.
The only normalized-byte residual is the order of two adjacent stack reloads at
`+0x13D/+0x140`. The generic classifier cannot prove the reads independent, so
the park remains fail-closed as `unclassified`. Exact hashes, negative controls,
and reopen criteria are recorded in `config/parked.json`.

The function calls the still-unwritten public
`decal_get_first_decal_index(short, short)` target. Only its owner-TU prototype is
declared; no synthetic standalone body or helper COMDAT is emitted.

## Rejected candidates

The following byte-exact results were removed rather than credited:

- `_actor_move_get_avoidance_direction`: code was exact, but two entries in the
  owned angle table were one ULP wrong (`5*pi/4` and `7*pi/4`).
- `_bitmap_clone` and `_rgb_colors_interpolate`: exact only after the source-shape
  retuning already rejected by their frozen first-shot ledgers; no new January
  provenance lawfully reopened that boundary.
- `_decal_get_first_decal_index`: the body matched in isolation, but the previously
  rejected whole-Decals symbol/debug ownership transition was not re-proved.
- `_projection_from_vector3d`, `_triple_product3d`, and
  `_bsp3d_get_plane_from_designator`: unowned standalone duplicates introduced by
  macro aliases, with no local caller or exact donor proving Decals ownership.
- `_weapon_handle_potential_inventory_item`: the instructions matched through a
  layout-only surrogate whose field types and names contradicted the authenticated
  `weapon_ammunition_object` definition.
- `_animation_update`: its `void` signature is supported, but that does not resolve
  the frozen whole-Weapons COMDAT-grouping rejection.
- `_biped_get_physics_pill`: the body matched only while semantically using fields
  named `collision_unused0` and `collision_height_standing` as different physical
  quantities; the authentic biped header must be repaired and blast-radius-tested
  in a separate lane.
- `_midpoint3d`: an artificial standalone duplicate that would violate the January
  inline/COMDAT boundary.

No rejected body or owned data remains in the admitted diff.

## Validation

- Clean `configure.py` plus full `ninja all_source progress build/report.json`.
- Focused gate: `_weapon_set_current_amount` is exact; Decals permanent deletion is
  one measured residual with unchanged hashes after rejected neighbors were pruned.
- Isolated canonical baseline: `4,975` accepted functions and `633,936` meaningful
  code bytes. Audited result: `4,976` and `634,279`; exactly one gain, `+343`
  meaningful (`+352` padded) bytes, and zero losses.
- Semantic audit: `4,976` accepted, zero unit errors.
- Park manifest: `32` active, zero stale, zero invalid.
- Changed-source fake-match scan: zero findings.
- Tooling tests: `238 passed`.
- `git diff --check`: clean.
