# `damage.obj` `object_destroy` first-shot recovery (2026-08-30)

## Result

The first and only code-producing candidate for the previously unwritten
`source/objects/damage.obj::_object_destroy` is strict exact.  The retained
typed C89 body adds 105 meaningful bytes, 112 padded bytes, seven ordered
relocations, and one function.  The object advances from 18/34 to 19/34 exact
functions and remains `NonMatching`.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `105 / 112` | `7` | `3280aaa59ee756c8c77d8c5d45e6476c730c43875594cdbd11f636be36a55287` |
| First production candidate | `105 / 112` | `7` | `3280aaa59ee756c8c77d8c5d45e6476c730c43875594cdbd11f636be36a55287` |

The January split object is `build/split/source/objects/damage.obj`, raw
SHA-256
`3fe5495d2a6ce3075c3fa6b04782cb7bbf97c3a3890f2e6f6f4f29ef8e5b08ec`.
The frozen candidate is preserved as
`build/audit/damage_object_destroy_first_candidate_20260830.obj`, raw
SHA-256
`1443297009970d72e5d23b7e760140d0f2c7043fc83b4623154b08100df24287`.

## Documentation, history, and provenance

All applicable campaign methodology, acceleration, object-ledger, Claude,
AGENTS, and tooling instructions were read before source work, together with
all three existing Damage ledgers.  Reachable Git history and registered
worktrees contain no production definition of `object_destroy`, and no prior
Damage ledger records an emitted candidate for it.

The authenticated HCEA checkout at commit
`c168af2e747d3095d9a29418ae401f3a39544863` supplies
`src/object_destroy.c`, Git blob
`ed3992dc3e53aac63eb31a458adc48ceb7dd3eaa`, payload SHA-256
`2dba86495e5cdab5b3951053e0945bbe9b546a260d13abc728eac683e80bd5e5`.
Its ordinary object/tag/depletion/effect/child-notification/delete sequence is
the semantic donor.  The later HCEA-only grunt-name/HCEX-effect prefix is not
present in January PC.  January disassembly independently fixes the complete
PC topology, field accesses, and call order, and remains the sole byte and
relocation authority.

The pre-edit source blob was
`1c399f06ab634aaf5b8b92bd3366c31848e7faa9`.  The frozen candidate source is
Git blob `4d338f6769e6a70fba7f8072b5b99f8f47e7e464`, payload SHA-256
`01f2932d0b87f07f591ee76de02313744bd55464ea8ed0c87ecb06ace3f27274`.

## Typed closure and strict packet proof

The implementation uses the canonical typed object, object-definition,
collision-model, damage-resistance, effect, and deletion APIs.  A compile-time
offset sentinel proves the named `body_destroyed_effect` tag-reference index at
`+0xC8` in the collision model.  The body resolves the object and definition,
calls the already exact `object_deplete_body`, emits the collision model's
body-destroyed effect when present, calls the already exact recursive
child-notification helper `code_00126090`, and deletes the object.

There is no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
barrier, raw address or runtime byte offset, pointer/integer reconstruction,
representation pun, undefined behavior, byte forcing, compiler change, tool
exception, or admission exception.  Parameters are split one per line and the
void body has an explicit terminal return.

The complete ordered relocation schedule is identical:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x0C` | `REL32` | `_object_get_and_verify_type` |
| `+0x19` | `REL32` | `_tag_get` for the object definition |
| `+0x21` | `REL32` | `_object_deplete_body` |
| `+0x37` | `REL32` | `_tag_get` for the collision model |
| `+0x4F` | `REL32` | `_effect_new_from_object` |
| `+0x58` | `REL32` | `_code_00126090` |
| `+0x5E` | `REL32` | `_object_delete` |

Exactly one normal XDK 3911 production edge compiled the frozen body.  No
alternate declaration, spelling, body, control flow, or compiler variant was
compiled.

## Translation-unit and full gates

The clean pre-edit regression manifest is
`build/audit/damage_object_destroy_baseline_20260830.json`, SHA-256
`200e845c127cc12d276a32d55e533f5d268f6397d28ac769b027783407493a4e`,
captured from commit `ed3af05b9b81fcca86897f575f4b60fe2b75bf81`.
Its candidate check reports `_object_destroy` as the sole `newly_exact` owner,
`changed_nonexact: []`, and fourteen directly stable accepted owners.  Adding
the new COMDAT produces the same expected symbol-set and section-number
evidence shifts seen in earlier Damage additions for four small exact owners;
no exception suppresses those findings.  Independent hardened comparison
proves all 19 current exact Damage functions byte- and relocation-identical.

Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
`progress` targets pass.  Semantic audit scans 470 units and 4,921 functions,
reports 4,804 semantic-exact and 4,814 accepted-exact functions, and has zero
unit errors.  Campaign progress is 384/833 linked objects, 4,775/11,060
functions, and 607,190/2,198,102 code bytes; Halo is 282/468 linked objects,
4,608/7,574 functions, and 594,276/1,770,166 code bytes.

Admission remains zero candidates, zero contradictions, and zero revocations.
Park validation remains 12 active, zero stale, and zero invalid.  All 212
tooling tests pass.  The regenerated report and semantic-report SHA-256 values
are respectively
`1ed249911413f2a9976b56ae66be8b7882ee4f12f2ca668a92a2bf9a4958eaf8`
and
`26f1e82aade2cad1455bdf3bb48e9e402130c9de2308cbb610ce61ac47dd723c`.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Vehicles, Units, Matrix Math, AI Debug, and all other excluded or concurrently
owned source paths remain untouched.  No GitHub push is performed.
