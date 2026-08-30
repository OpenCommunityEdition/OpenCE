# `weapons.obj` total-rounds exact grouping boundary

## Result

`_weapon_set_total_rounds` emitted strict exact on its first and only
code-producing candidate compile.  The 288 padded bytes, all 12 ordered
relocations, and normalized SHA-256 are identical to January.  The source is
nevertheless not retained: adding the external COMDAT changes the frozen
whole-translation-unit ownership fingerprints of the existing private tail.
The candidate body and owner-header declaration were inverse-reverted, so this
wave claims no code or data credit.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful bytes | `275` | `275` |
| Padded bytes | `288` | `288` |
| Relocations | `12` | `12` |
| Normalized SHA-256 | `58f291b2ebd123b056e11e282031d7777991af13848fb2b0dc3d4d3f6a90d227` | `58f291b2ebd123b056e11e282031d7777991af13848fb2b0dc3d4d3f6a90d227` |

The frozen candidate whole-object SHA-256 was
`4beae716d1b650843ba2a9e22f1f621db0f90749068ba9d77d7f6f1105893d5e`.
The January split-object SHA-256 is
`5dd414f3f77e67aa7b6609f9131debfa8a9b3b23afab6a57fe4e790410cff2fe`.
Raw object hashes include nondeterministic debug metadata; the complete
function packet and regression manifest are the acceptance authorities.

## Provenance and prior-work census

All current `weapons.obj` ledgers, the campaign methodology and acceleration
playbook, the applicable external `AGENTS.md` and `CLAUDE.md`, and both public
donors were read before source work.  Current ancestry contains only the
initial empty symbol listing for this function.  A scan of all 1,524
registered worktrees found no source definition, and no existing object ledger
records a production candidate for this symbol.

The independent public reconstructions are:

- HCEA commit `8a48b087b958028e0a284eb7c645069a78f28e3c`, dedicated file
  `src/weapon_set_total_rounds.c`, blob
  `6c115fd6fa56c382075412ddd86ff5b726759ad3`;
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, file
  `src/halo/items/weapons.c`, blob
  `07064992d82bffe20d1b0f5438c1082e406b1306`, whole-file SHA-256
  `148b5a1fbfbf94849fd8b871669dac603ae61c9afce0c2fda9b648d5f0069138`.

Both donors agree that the routine walks each weapon magazine, clamps the
requested total to the definition's maximum capacity, then clamps loaded
rounds to the new total.  The complete January disassembly independently
confirms the typed weapon and tag lookups, the non-null `rounds_array` assert
at source line `0xC0A`, the signed-short loop, the inlined magazine bounds
assert at line `0x672`, the typed tag-block access, and both signed minimum
operations.

The current headers already prove every consumed layout.  Existing exact
functions use the same `weapon_datum`, `weapon_definition`,
`weapon_magazine_definition`, and `weapon_magazine` fields, and the existing
strict-exact private `weapon_magazine_get` helper supplies the January inline
assert topology.  No new structure field, tag layout, raw offset, callee
prototype, or storage owner was inferred.

## Frozen one-shot source

The candidate added one owner-header declaration and one external definition
in the natural January-order gap before `weapon_delete`.  Its body used:

- `weapon_get` and `weapon_definition_get`;
- the canonical `match_assert` for `rounds_array`;
- a `short magazine_index` loop;
- `weapon_magazine_get` followed by `TAG_BLOCK_GET_ELEMENT`; and
- two ordinary `MIN` assignments to `rounds_total` and `rounds_loaded`.

The frozen source/header blobs were respectively
`b2da1c0759b033e5c9736cea26a42f4d73917479` and
`d9ff55e94c5e0a7ceefa3d7ce69601fcbfd521eb`.  A Ninja dry run exposed exactly
one production edge, `CL build/base/source/items/weapons.obj`.  That edge was
invoked once.  There was no preprocessing failure, declaration correction,
placement retry, expression variant, type variant, or second candidate
compile.

The candidate reproduces all 12 relocation offsets, types, destinations, and
zero addends.  These include the two object/tag lookups, the source path and
`rounds_array` assertion payloads, both assertion/exit pairs, the loop's tag
lookup and magazine assertion payloads, and the final
`tag_block_get_element_with_size` call.

## Whole-TU rejection

Direct hardened code comparison finds no lost exact function.  The baseline
strict set is 37 functions; the candidate set is those same 37 plus
`_weapon_set_total_rounds`.  The new function is therefore a genuine exact
packet, not an approximate or isolated false positive.

The frozen full regression manifest still fails closed.  VC7 groups the new
external COMDAT before the existing private tail, changing the recorded
full-section fingerprints of:

- `_code_000ec1a0` and `_code_000ec960`;
- `_weapon_put_away`, `_weapon_ready`, and `_weapon_reloading`;
- `_weapon_reset`, `_weapon_set_state`, and `_weapon_stop_reload`.

Their runtime packets remain directly strict exact, but their numeric COFF
section ownership changes.  The two deliberately retained nonexact owners
`_code_000ec300` and `_weapon_update` change for the same reason.  The manifest
also reports the newly supplied exact `rounds_array` read-only owner, one
associated anonymous `.debug$F` record, and the expected symbol-set addition.
The campaign has no lawful general transition recipe for this external-COMDAT
insertion, and a one-off comparator or adjudication exception is prohibited.

This reproduces the grouping boundary documented by the earlier exact
`_random` and `_animation_update` shots, while adding new durable evidence:
the total-rounds body itself is now proved exact.  Do not retry body spellings
or physical placements.  Reopen retention only after a repository-wide,
fail-closed ownership model lawfully represents external-COMDAT insertion, or
after enough of the original translation unit is reconstructed to restore the
January section topology without waiving any inherited owner.

## Restoration and validation

The definition and header declaration were inverse-applied with the patch
tool.  The final source/header blobs are exactly their canonical inputs:
`f6abe256f6b4becb78c6db398fd577e98a8cc6e3` and
`0708017d702f478782f31e183671ef3b11a51c47`.  The restored rebuilt
`weapons.obj` SHA-256 for this run is
`05083e3258b39cee4e950d87e9ef2be9f354fa23a8e74b134b50e8240cb00906`.
The original regression manifest is fully green after restoration: all 37
accepted functions remain `still_exact`, with no failure, warning,
changed-nonexact function, or newly exact function.

The complete Halo, libcmt, report, semantic-report, and progress graph pass.
The semantic audit scans 470 units, evaluates 4,919 functions, finds 4,802
semantic-exact and 4,812 accepted-exact functions, and reports zero unit
errors.  Campaign progress remains 384/833 linked objects, 4,773/11,060
functions, and 606,876/2,198,102 code bytes; Halo remains 282/468 linked
objects, 4,606/7,574 functions, and 593,962/1,770,166 code bytes.  The strict
board remains 277/619 objects, 4,784/8,246 functions, and
663,783/1,922,669 padded bytes.

Admission is 0 candidates / 0 contradictions / 0 revocations.  The parked
inventory is 12 active / 0 stale / 0 invalid.  All 212 tooling tests pass;
the sole warning is the sandbox preventing pytest's optional cache write.
The regenerated report and semantic-report SHA-256 values are respectively
`b13ef78fdc5dce935a82cdf93d76a474c3273dc54148975139f7c3114b2d43d0`
and
`51d605379dee1e1d44685c048d51150e13d504ae617a5ee1d1cb450d93fb431b`.

The protected Units sentinel `_unit_preprocess_node_orientations` remains
strict exact at 1,920 padded bytes, 87 ordered relocations, and normalized
SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

The rejected implementation used readable typed C89 with one parameter per
line and an explicit `return;`.  It introduced no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, raw address or
structure offset, pointer/integer reconstruction, representation pun,
undefined behavior, byte forcing, object patch, compiler-option change,
semantic exception, comparator exception, or tool exception.  Only this
additive ledger is retained; nothing is pushed.
