# `effects.obj`: stop-first-person-weapon strict first shot (2026-08-30)

## Result

`_effects_stop_on_first_person_weapon` was tested once as a natural typed C89
reconstruction. The sole code-producing candidate is a strict nonmatch. All
production source and header edits were inverse-reverted, so
`source/effects/effects.obj` remains at its prior eight exact functions and
receives no code, data, semantic, or completion credit from this lane.

This is a fixed compiler-topology boundary. Do not repeat the public-donor
`data_next_index` outer loop, the three-local typed record spelling, the
signed node-designator test, or the direct linked-list cursor form without
new source-shape evidence.

## Frozen authority and provenance

- Starting commit: `ce2ff96f49ff1c8805859c131ead4415305b0439`.
- January split object:
  `build/split/source/effects/effects.obj`, SHA-256
  `e36f91756ba500987dbecd7a16972a88e769d03b2efd11120ff4e5328256d8f6`.
- Clean production source/header blobs after the inverse revert:
  `2a9a097710b6665d637bffd28d934b8d34426058` and
  `4bd0233f29ca2dfcf69fde88be4c1c7c091ad9af`; payload SHA-256
  `91e7603f2960ae072b12345cf0d48719c494fe4372979d657f0343eb1275249a`
  and
  `b198941ba4552c3f3055cea8d6492252af60ae6fd2df67c919f9ff9202fadea4`.
- Clean HCEA reference: checkout
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
  `src/blam/effects/effects_stop_on_first_person_weapon.c`, blob
  `bc5e6f1e92774929eeb4517fb5de5f3ba9e19c5d`, payload SHA-256
  `d24c2d39552a771b8ce8278bafb4f3599d1ea7b0faf4fdce380ca5c031259cff`.
- Stian January-x86 reference: commit
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, file
  `src/halo/effects/effects.c`, blob
  `637aa856a65bc9f8720a7d9188014d49589dda4c`, payload SHA-256
  `a82288a9bdbc3fb805f20190fd76778d7cb9a89f1f47174161b9b2ae66bec200`.
- Baseline regression manifest:
  `build/audit/effects_stop_fp_weapon_baseline_20260830.json`, SHA-256
  `a902d57dd7ce5dc2c626538b97fefffc75ec410ced24cfa550d7f2c48cb84d53`.

A complete Git-history census and all registered worktrees contained zero
implementation of this function in the current exact-match repository. The
HCEA and Stian references independently agree on the public cdecl callees,
effect and effect-location fields, location-list traversal, node-designator
predicate, unlink/delete order, and final owner reset. January COFF is the
sole byte, layout, ABI, padding, and relocation authority.

## Frozen candidate

The candidate used named `effect_datum`, `effect_location_datum`, and
`effect_definition` fields with compile-time size and offset checks. It used
only the public typed APIs `data_next_index`, `datum_get`, `tag_get`, and
`datum_delete`. The full-width datum indices were retained, the player owner
and node designator were signed shorts, the location counter used the donor's
explicit short narrowing, and all declarations preceded statements.

The first parse-only `/Zs` prerequisite stopped before code generation because
`effects.h` had been included before its required real-math typedef context.
Moving that owner header after `real_math.h` was the sole declaration/include
reconciliation. It did not change the frozen body and emitted no object. The
next invocation was the only normal XDK 3911 candidate compile.

The preserved artifact is:

- `build/audit/effects_stop_fp_weapon_first_candidate_20260830.obj`;
- raw SHA-256
  `64cbd4effaa362095c4c42ea12b79da13eb3cc437edbcdd77c6a085adb88b177`.

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Meaningful bytes | 241 | 256 |
| Padded bytes | 256 | 256 |
| Relocations | 11 | 11 |
| Normalized SHA-256 | `75f540f60c7fbc988baa3cfbea4d8342d4d6e79233a5b63ce16d60ab91da9a60` | `ae9e7407f8ec0fbe66ab66e109b8c1bfa453994fdbace1e4757936c5d8899345` |

All eleven relocation types, destinations, and addends agree in semantic
order. Their addresses do not. The first `_effect_data` relocation agrees at
`+0x07`, but the first `_data_next_index` call is at `+0x0F` in January and
`+0x10` in the candidate; later relocations remain shifted by the different
register and loop schedule.

## Fixed boundary

January keeps the initial effect index in EAX, writes it to the `EBP-4` stack
local, checks `NONE`, and only then saves EBX/ESI/EDI. The natural donor body
caused VC7 to save ESI before the initial `data_next_index` call and retain the
outer index in ESI. That changes the entry schedule, loop back-edge, register
roles, relocation offsets, and meaningful extent even though the public call
sequence and padded extent agree.

There is a second observable lowering difference in the same frozen shot:
January tests the node designator's high byte with `test ah, ah`, while the
natural signed-short predicate emitted `test ax, ax`. No alternate bit-mask
spelling, declaration scope, loop spelling, parameter width, or local-lifetime
variant was attempted. Reopen only with an authentic January source record or
an independently exact same-shape donor that explains both register lifetime
and node-test lowering.

The candidate contained no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or byte-offset access,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing expression.

## Restoration and validation

The source and owner-header payloads are byte-identical to the starting Git
blobs. A normal restored-object rebuild reproduces all eight inherited exact
Effects functions with no loss and no newly exact or changed-nonexact owner.
The baseline regression check reports `ok: true`, zero failures, and zero
warnings. This ledger is the lane's only retained tracked change; Units and
all user-protected sources remain untouched. No push is performed.
