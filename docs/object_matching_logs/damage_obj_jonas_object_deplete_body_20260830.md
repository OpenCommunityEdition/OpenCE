# `damage.obj` `object_deplete_body` first-shot recovery (2026-08-30)

## Result

The first and only code-producing candidate for the previously unwritten
`source/objects/damage.obj::_object_deplete_body` is strict exact.  The
readable typed C89 implementation is retained.  It adds 209 meaningful bytes,
224 padded bytes, eight ordered relocations, and one function.  The object
advances from 17/34 to 18/34 exact functions and remains `NonMatching`.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `209 / 224` | `8` | `47bb1ba1fbf3377b4e408dc954a20dba26a5538fad6d4cbcf022d017f72c752d` |
| First production candidate | `209 / 224` | `8` | `47bb1ba1fbf3377b4e408dc954a20dba26a5538fad6d4cbcf022d017f72c752d` |

The January split object is
`build/split/source/objects/damage.obj`, raw SHA-256
`3fe5495d2a6ce3075c3fa6b04782cb7bbf97c3a3890f2e6f6f4f29ef8e5b08ec`.
The frozen first candidate is preserved as
`build/audit/damage_object_deplete_body_first_candidate_20260830.obj`, raw
SHA-256
`adf83ecbd49699bffb3a06866a952ded93b9dc251a664ef24f52552b02f56ad9`.
Whole-object hashes include compiler metadata; the function hash above covers
the complete padded code packet with relocations normalized by semantic
identity.

## Documentation, history, and provenance

All applicable campaign methodology, exact-match acceleration, object-ledger,
Claude, AGENTS, and production-tooling documentation was read before source
work, along with both existing `damage.obj` ledgers.  A full reachable-history
and registered-worktree census found no implementation of
`object_deplete_body` (`history_hits=0`, `worktree_hits=0`), and no prior ledger
records a candidate for it.  This was therefore a fresh production shot, not a
retry or uncommitted-tree transplant.

The authenticated HCEA checkout at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies the semantic donor
`src/blam/objects/object_deplete_body.c`, Git blob
`34ae9271f91d9181330fc11939b1cf464df6993c`, payload SHA-256
`e81714aa2516045855079d79341fccb06ce894aaa66cd64347ff80dfc7388f5f`.
The January target disassembly independently agrees with its complete branch,
iteration, call, and field-access topology.  January PC remained the sole byte
and relocation authority.

The pre-edit production source blob was
`9c23416f17c476bdbccb7efdacc1d439150127ed`.  The frozen candidate source is
Git blob `1c399f06ab634aaf5b8b92bd3366c31848e7faa9`, payload SHA-256
`5cbf3f9f70f8f56aa6b6e9dd061cfd1a0114ab700f1c19e2496f4f9579989071`.

## Typed implementation and strict packet proof

The retained body uses the canonical object, unit, collision-model, effect,
and cheat declarations.  Nine compile-time `offsetof` sentinels prove the
named layouts consumed by value: object definition index, type, damage flags,
next sibling, first child, unit player index, unit parent seat, collision-model
tag reference, and body-depleted-effect tag reference.  These are compile-time
layout assertions, not runtime raw-offset access.

The implementation marks a live object dead, resolves its collision model and
starts the named body-depleted effect, kills qualifying seated biped children
of a vehicle while following the typed sibling chain, and then depletes the
shield.  It uses public cdecl APIs and an explicit terminal return.  It uses no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, compiler
barrier, raw address or runtime byte offset, pointer/integer reconstruction,
representation pun, undefined behavior, byte forcing, compiler change, tool
exception, or admission exception.

The complete ordered relocation schedule is identical:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x0F` | `REL32` | `_object_get_and_verify_type` |
| `+0x3C` | `REL32` | `_tag_get` |
| `+0x51` | `REL32` | `_tag_get` |
| `+0x68` | `REL32` | `_effect_new_from_object` |
| `+0x83` | `REL32` | `_object_get_and_verify_type` |
| `+0x9C` | `DIR32` | `_cheat` |
| `+0xAF` | `REL32` | `_unit_kill` |
| `+0xC5` | `REL32` | `_object_deplete_shield` |

An XDK `/Zs` parse-only pass completed first and emitted no object or candidate
bytes.  The frozen normal production edge was then compiled exactly once and
its output was immediately preserved under the fingerprint above.  No source,
declaration, control-flow, or compiler variant was compiled.

## Frozen translation-unit and preservation gates

The clean pre-edit regression manifest is
`build/audit/damage_object_deplete_body_baseline_20260830.json`, SHA-256
`e2de17f0902970160a4c3d03301ea4b622c6a2e66473bea06e1fd6cc50830cb9`,
captured from commit `1c97aba81d9ae90b42265cf1649dadf4ae1fac49`.
The first-candidate check reports exactly `_object_deplete_body` under
`newly_exact`, `changed_nonexact: []`, and twelve directly stable accepted
owners.  Adding the new code owner and include-derived debug inventory causes
the expected generic `SYMBOL_SET_CHANGED` result and accepted-evidence shifts
for `_code_00126090`, `_object_can_take_damage`,
`_object_cannot_take_damage`, `_object_set_melee_attack_inhibited`, and
`_object_set_ranged_attack_inhibited`.  No adjudication or exception suppresses
those structural findings.

Independent hardened owner comparison proves all 18 current exact named
functions in `damage.obj` byte- and relocation-identical, including all 17
inherited functions and the new owner.  The generated report likewise contains
34 functions and exactly those 18 ordinary 100% owners.  No source-owned data
was added or changed, and this lane claims no data credit.

## Full validation

The complete Halo and libcmt build graphs pass.  Semantic audit scans 470
units and 4,920 functions, reports 4,803 semantic-exact and 4,813
accepted-exact functions, and has zero unit errors.  Campaign progress is
384/833 linked objects, 4,774/11,060 functions, and
607,085/2,198,102 code bytes; Halo is 282/468 linked objects,
4,607/7,574 functions, and 594,171/1,770,166 code bytes.  The strict board is
277/619 objects, 4,785/8,246 functions, and 664,007/1,922,669 padded bytes.

Admission remains 0 candidates / 0 contradictions / 0 revocations.  Park
validation remains 12 active / 0 stale / 0 invalid.  All 212 tooling tests
pass.  The regenerated report and semantic-report SHA-256 values are
respectively
`74ed875a68063580b2455068fb2eff5a581b61bcb5c1bdc953d6ab6a84b75ebc`
and
`01de5085c3683f17ab0eee3c5bb1ab36c5b49aadecf40e8b77fa81edc7f044ea`.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Vehicles, Units, Matrix Math, AI Debug, and all other excluded or concurrently
owned source paths remain untouched.  No GitHub push is performed by this
isolated lane.
