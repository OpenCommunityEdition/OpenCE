# Encounters `_encounterless_activate` strict one-shot recovery (2026-08-30)

## Outcome

`source/ai/encounters.obj::_encounterless_activate` is reconstructed in
readable typed C and passes the hardened January acceptance gate on complete
padded bytes and all ordered relocation identities.

| Measurement | January target | Final candidate |
| --- | ---: | ---: |
| meaningful code bytes | 85 | 85 |
| padded COMDAT bytes | 96 | 96 |
| ordered relocations | 7 | 7 |
| normalized SHA-256 | `9d30a12bb044165f6cb858e7049aaf4313c39742bce1241cad291244b199f9e2` | `9d30a12bb044165f6cb858e7049aaf4313c39742bce1241cad291244b199f9e2` |

The first frozen candidate object is preserved at
`build/audit/encounters_encounterless_activate_first_candidate_20260830.obj`
with whole-object SHA-256
`abb44440275ec62e8bcd44734e6cbf863d9b1e5bea703f1fa95fb1d18b24f872`.
The final declaration-reconciled generated object has whole-object SHA-256
`9d5bf1b03f413cce2de5612ae0dfe14ffa2ad3e6b9b9d2495c8f40e79bd1736f`.

## Provenance and fresh-history proof

The semantic donor is Halo CEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/ai/encounterless_activate.c`, Git blob
`d942fd1303235f858f97467de0d017096321ab1f`, payload SHA-256
`ff937aacc6b8e473febb782847c048264afdae0e09ecfe6f32251f9e8adc824a`.
It supplies the actor lookup, encounterless timer assignment, and activation
call. January PC disassembly independently fixes the named encounterless
assertion, line number `0x720`, timer value 90, complete control flow, and
relocation order. The January object remains the sole byte authority.

Before mutation, ancestry searches, all registered object ledgers, Claude
documentation, and exact-definition scans across the registered worktrees
found no prior source body or measured attempt for this symbol. The applicable
Encounters ledgers, acceleration playbook, HCEA donor, typed actor layouts, and
January object were read before the production edit. Preflight also rejected
several apparently small alternatives without mutation or compilation when
they proved non-link-closed, already frozen, target-data-owning, or dependent
on undefined behavior; none was used to tune this body.

The pre-edit Encounters source blob was
`0b42bac42e659d7856deba54ce4dce9ec49fbf38`; the final source blob is
`4da6d94c5e643214dde5a25d7d2e8d6a82f5ac2b`. The focused declaration-only
header is blob `ce5ce7c12824a2e05203ab4477b310d6512dee23`.

## Natural typed C and ordered relocation proof

The retained body resolves the actor through the canonical typed accessor,
asserts the named `meta.encounterless` flag, writes the named
`meta.encounterless_active_timer`, and calls the typed `actor_set_active` API.
The return type is independently supported by January's AL result and the
authenticated public source definition. There is no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or runtime byte
offset, pointer/integer reconstruction, representation pun, undefined
behavior, byte forcing, compiler change, tool exception, or admission
exception. The parameter is on its own line and the void function has an
explicit terminal return.

The complete ordered relocation schedule is identical:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x04` | `DIR32` | `_actor_data` |
| `+0x10` | `REL32` | `_datum_get` |
| `+0x28` | `DIR32` | source-path `.rdata` |
| `+0x2D` | `DIR32` | `actor->meta.encounterless` assertion `.rdata` |
| `+0x32` | `REL32` | `_display_assert` |
| `+0x39` | `REL32` | `_system_exit` |
| `+0x4A` | `REL32` | `_actor_set_active` |

Exactly one natural XDK 3911 production emission compiled the frozen function
body. No alternate body, spelling, topology, or compiler variant was compiled.

## Declaration-only blast-radius reconciliation

The first exact emission placed the identical `actor_set_active` prototype in
the broad `actors.h` owner. A required full dependent rebuild then exposed a
real structural regression in protected Units: its function bytes, size, and
87 relocation addresses were stable, but eight constant/format-string
relocations changed from external to TU-defined ownership. The hardened Units
comparator correctly rejected that object-global ownership change.

With explicit campaign-owner authorization, the function body was kept
byte-for-byte frozen and only the declaration was isolated into the focused
typed `actor_activation.h` header consumed by `encounters.c`. One
integration-only reconciliation emission followed; no code lever or body retry
was permitted. It preserved `_encounterless_activate` at the exact packet above,
preserved all ten inherited Encounters packets, and restored the complete Units
sentinel including its original relocation identities. The broad `actors.h`
blob is identical to its pre-edit blob
`3f1d86b7f2e0a42d454aa5d0d05d4ff86d9b3be2`.

## Translation-unit and full gates

The clean pre-edit regression manifest is
`build/audit/encounters_encounterless_activate_pre_20260830.json`, SHA-256
`4ee0359218edef21428bb7a043a648c63c226c4db73b15bcfc54f758a58d94fa`,
captured from commit `90d4da2f0b9be3452c1949337bf0d60e3b5d39fe`.
Its final candidate check reports `_encounterless_activate` as the sole
`newly_exact` owner, all ten inherited exact functions in `still_exact`, and
`changed_nonexact: []`. The only expected structural findings are the newly
emitted assertion-string `.rdata` owner and the corresponding symbol-set
addition. No exception suppresses those findings. Independent complete-object
comparison proves all eleven current exact Encounters functions byte- and
relocation-identical.

Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
`progress` targets pass without warnings. Semantic audit scans 470 units and
4,922 functions, reports 4,805 semantic-exact and 4,815 accepted-exact
functions, and has zero unit errors. Campaign progress is 384/833 linked
objects, 4,776/11,060 functions, and 607,275/2,198,102 code bytes; Halo is
282/468 linked objects, 4,609/7,574 functions, and 594,361/1,770,166 code
bytes. Final `build/report.json` and `build/semantic_report.json` SHA-256 values
are respectively
`a19bfe62cf3935eab37f8436d128babb7b1ed27bcf40e2ee09201ec814910ef7`
and
`972e25fc400ca3d0b608414e381505f3cfde11edff8ed60b3ea33c0de5afff89`.

Admission remains zero candidates, zero contradictions, and zero revocations.
Park validation remains 12 active, zero stale, and zero invalid. All 212
tooling tests pass. `git diff --check` and the changed-source policy inspection
pass.

The protected `_unit_preprocess_node_orientations` sentinel is strict exact at
1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Vehicles, Units, Matrix Math, AI Debug, and all other excluded or concurrently
owned source paths remain untouched. No GitHub push is performed.
