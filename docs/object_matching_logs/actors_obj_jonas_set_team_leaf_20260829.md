# `actors.obj` set-team public leaf

## Result

`_actor_set_team` matches in the first and only natural production candidate.
It contributes 177 meaningful / 192 padded code bytes and seven ordered
relocations. `source/ai/actors.obj` advances from 26/76 to 27/76 strict
functions and remains `NonMatching`.

Target and candidate share normalized SHA-256
`04500b15a9a94b3c72b0233f0692206a9ae27ec6605e1ef620ea560370442276`.
Their relocation packets are identical:

- `+0x08` `DIR32 _actor_data`;
- `+0x11` `REL32 _datum_get`;
- `+0x29` `DIR32 _swarm_data`;
- `+0x30` `REL32 _datum_get`;
- `+0x51`, `+0x78`, and `+0x9E` `REL32 _object_get_and_verify_type`.

The immutable first-shot object is
`build/audit/actor_set_team_first_shot_20260829.obj`, 9,678 bytes, SHA-256
`71c209d592e555fbc03975a34ff1617a85113c3bc1e396dd9b45467743f87ace`.
The January split object is 56,916 bytes, SHA-256
`632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
The pre-wave `source/ai/actors.c` blob is
`af5a9f90b5ca8480edd18bf13f013b6028728d4f`; the retained blob is
`7c3f1862282f3c2ee6592edecf3de6d47a4bec95`.

## Typed behavior and January topology

The retained function resolves the actor with `actor_get`. A cached swarm
iterates the typed `swarm->unit_indices` array; an uncached swarm walks the
typed `unit->unit.swarm_next_unit_index` chain; a non-swarm actor updates its
single unit when present. Every unit is resolved with the established
`unit_get` macro and receives the caller's short team value through
`unit->object.owner_team_index`.

Complete January disassembly fixes the same three-way topology and source
ordering: cached-swarm fallthrough, a branch to the uncached chain when the
cache handle is `NONE`, and the non-swarm tail. It independently fixes the
short loop counter, unit type mask 3, owner-team field at `+0x68`, linked-list
field at `+0x1AC`, swarm count at `+0x02`, and member array at `+0x18`. The
typed repository layouts reproduce each offset without raw access.

## Independent source evidence

The clean HCEA source at commit
`c168af2e747d3095d9a29418ae401f3a39544863`, `actor_set_team.c` blob
`eacc92f221dab003b6c3d2c00cab3ab3f45c96d1`, authenticates the public name,
ordinary two-argument signature, and all three unit-propagation cases.

The independent January-PC reconstruction at commit
`918af885935ec470a31256ecce9a977b12b01f80`, actor source blob
`ecf9bb77905c2950e9941038ff82a5eee3712282`, separately records the exact
January object/unit helper topology, fields, loop counter width, and branch
cases. Its raw pointer spelling and numeric offsets were not transplanted;
they served only as version-specific semantic corroboration for the typed
current declarations.

## History, one-shot discipline, and scope

All actor-specific ledgers, the encounter iterator ledger, `AGENTS.md`,
`CLAUDE.md`, matching methodology, acceleration playbook, verification and
snapshot policies, prototype/ABI references, knowledge-base policy, object
tooling documentation, current actor/unit/object layouts, and pinned VC7/XDK
contract were read before mutation. Reachable-history, all-ledger, and
registered-worktree searches found no earlier emitted candidate for this
function.

One readable typed function was applied and the ordinary production
`actors.obj` edge compiled exactly once. The first strict comparison passed
complete padded bytes, normalized hash, and all ordered relocation
addresses/types/destinations. There was no declaration, control-flow,
source-shape, flag, or compiler retry.

`python -m tools.campaign.gate source/ai/actors --all` reports 27 exact, zero
residual, and 49 unwritten functions. Direct hardened comparison passes the
new leaf and all 26 inherited accepted actor functions.

The implementation scope is only `source/ai/actors.c`; this ledger is the
sole documentation addition. The retained source is readable typed C89-style
code. It uses no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address/offset access, pointer/integer
reconstruction, pointer or union pun, undefined behavior, synthetic anchor,
or byte forcing. Units, Vehicles, Matrix Math, AI Debug, configuration,
semantic exceptions, parked records, completion labels, and build rules are
untouched. Full cumulative gates are recorded after commit.

## Committed cumulative validation

The implementation-and-ledger commit is
`6fa7217959e1ea194e1d03b03a5a74c1b68de934`. The complete Halo/libcmt
graphs, report, semantic report, and progress generation pass. Canonical
outputs are `build/report.json`, 1,599,315 bytes, SHA-256
`68c298505b1a29fc9cec03ecd896fa9cccb45201329df1cb813153bab0be44fa`,
and `build/semantic_report.json`, 3,269,715 bytes, SHA-256
`719622b90eb05da8ae33f234a61f3581286e7b5796cb636403790212f17aee47`.

The semantic audit scans 470 units and evaluates 4,922 functions: 4,805
semantic exact, 131 hidden exact / 78,940 hidden code bytes, 4,815 accepted
exact, one ordinary-only, one structural, zero rejected, 12 local skips,
2,807 missing, and zero unit errors.

Campaign progress is 384/833 linked objects, 4,776/11,060 functions, 607,354
of 2,198,102 code bytes, and 1,856,050 of 4,176,062 data bytes. Admission is
zero candidates, contradictions, and revocations. Parks are 12 active, zero
stale, and zero invalid. All 212 tests pass. The protected Units sentinel
remains strict exact at 1,920 padded bytes, 87 ordered relocations, and
normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
