# Encounters `_encounters_update_dirty_status` strict one-shot recovery (2026-08-30)

## Outcome

`source/ai/encounters.obj::_encounters_update_dirty_status` is reconstructed
in readable typed C89 and is strict-equal to the January object on every
meaningful and padded byte plus all ordered relocation identities.

| Measurement | January target | Final candidate |
| --- | ---: | ---: |
| meaningful code bytes | 126 | 126 |
| padded COMDAT bytes | 128 | 128 |
| ordered relocations | 6 | 6 |
| normalized SHA-256 | `0d69450e03bbb97515a5b92e0a38d2a9c6b6669ae379ad3686cc61231c49ecd9` | `0d69450e03bbb97515a5b92e0a38d2a9c6b6669ae379ad3686cc61231c49ecd9` |

The final generated Encounters object has whole-object SHA-256
`8e1561219eff4f355d0721d005b8d94cf0eb11d59ad46e7d2fa32afc9815ef6d`.

## Provenance and fresh-history proof

The primary semantic and layout donor is Halo CEA commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`, file
`src/encounters_update_dirty_status.c`, Git blob
`530c1ea788adac327a9797e5420393397b5eb583`, payload SHA-256
`d97e7f96822c8059ef26e0322c446372dc9a774df79b586cfd52e0c732793cc0`.
The independent Pastudan reconstruction at commit
`918af885935ec470a31256ecce9a977b12b01f80`, file
`src/halo/ai/encounters.c`, Git blob
`083b04483d94b4469cd0f2b8348aaccb565f9049`, payload SHA-256
`ec8f4b4e34ddeb3263c0caaf116520198424401801cb9a3a6e90a876bc60fe06`,
agrees on the initialization guard, iterator traversal, dirty-status gate, and
status-update call. Claude and Stian reconstruction trees independently show
the same January iterator topology. January COFF and disassembly remain the
sole byte authority.

Before mutation, the complete Encounters ledger set, campaign/tooling rules,
registered worktrees, Git history, Claude documentation, public-source trees,
typed headers, callers, target disassembly, and target relocations were read
and searched. No prior source body, measured candidate, or exclusion exists
for this symbol. All six January relocation destinations are public cdecl or
data owners; the candidate has no private register-ABI dependency.

The pre-edit source blob was
`4da6d94c5e643214dde5a25d7d2e8d6a82f5ac2b`; the final source blob is
`868f2b146d0224d5faadf4b709854f82629bc632`.

## Frozen natural typed C

The retained body uses the existing typed `encounter_iterator_new` and
`encounter_iterator_next` helpers, tests the named
`struct encounter_datum::status_dirty` field, and passes the iterator's named
datum index to the ordinary typed `encounter_update_status` declaration. A
compile-time layout assertion fixes `status_dirty` at its independently
authenticated structure position without generating code. The compiler
naturally inlines the two already-exact iterator helpers into the January
packet.

The candidate is ordinary readable C89 with the void parameter on its own
line and an explicit terminal return. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or runtime byte
offset, pointer/integer reconstruction, representation pun, undefined
behavior, byte forcing, object patch, compiler change, tool exception, or
admission exception.

A production-argument `/Zs` parse-only check passed before code generation.
The existing object SHA-256 remained
`15c3b0dc36640ad5158fcb8c90f9416a4e4c729bedcb6192a3598a856740aec3`
before and after that check, proving it emitted no object. Exactly one
production object compile was then performed. No alternate body, spelling,
declaration, topology, tuning sweep, or compiler variant was compiled.

The complete ordered relocation schedule is identical:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x07` | `DIR32` | `_ai_globals` |
| `+0x14` | `DIR32` | `_encounter_data` |
| `+0x1E` | `REL32` | `_data_iterator_new` |
| `+0x31` | `DIR32` | `_ai_globals` |
| `+0x45` | `REL32` | `_data_iterator_next` |
| `+0x71` | `REL32` | `_encounter_update_status` |

## Translation-unit and full gates

The clean pre-edit regression manifest is
`build/audit/encounters_dirty_status_pre_20260830.json`, SHA-256
`db0df3b4b148ccea9625996bcd109b9a6a5cc267d4fe124bf14babf316a209a8`,
captured from canonical commit
`3a53f9ac5612d99470c3972776a22511e6cd7121`. Its final check reports this
function as the sole `newly_exact` owner, all eleven inherited exact functions
in `still_exact`, and `changed_nonexact: []`. The fail-closed
`SYMBOL_SET_CHANGED` result is the expected additive function owner only; no
pre-existing symbol or section owner was removed or changed. Independent
complete comparison of all twelve current exact Encounters functions reports
`all_equal: true`. The focused campaign gate reports 12 exact, zero residual,
and 49 unwritten functions.

Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
`progress` targets pass. Semantic audit scans 470 units and 4,954 functions,
reports 4,837 semantic-exact and 4,847 accepted-exact functions, and has zero
unit errors. Campaign progress is 384/833 linked objects, 4,808/11,060
functions, and 610,370/2,198,102 code bytes; Halo is 282/468 linked objects,
4,641/7,574 functions, and 597,456/1,770,166 code bytes. Final
`build/report.json` and `build/semantic_report.json` SHA-256 values are
respectively
`fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`
and
`3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.

Admission remains zero candidates, zero contradictions, and zero revocations.
Park validation remains 12 active, zero stale, and zero invalid. All 212
tooling tests pass. `git diff --check` and the changed-source policy inspection
pass.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Vehicles, Units, Matrix Math, AI Debug, and every concurrent owner remain
untouched. Temporary campaign-gate scratch output was removed, and no GitHub
push is performed.
