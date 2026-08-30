# `actors.obj` erase public leaf

## Result

`_actor_erase` matches in the first and only natural production candidate. It
contributes 149 meaningful / 160 padded code bytes and nine ordered
relocations. `source/ai/actors.obj` advances from 27/76 to 28/76 strict
functions and remains `NonMatching`.

Target and candidate share normalized SHA-256
`5b46c09fa85abac41eacacec2a88f5b987d5c40bdcca2d94d5395bbd32ef6323`.
Their relocation packet is identical: `_actor_data` at `+0x04`, `_datum_get`
at `+0x10`, `_actor_swarm_detach_from_unit` at `+0x33`,
`_object_delete_immediately` at `+0x43`, `_object_delete` at `+0x4A`,
`_actor_delete` at `+0x5D`, `_actor_died` at `+0x6E`, and the non-swarm
delete pair at `+0x7E` and `+0x8A`.

The immutable first-shot object is
`build/audit/actor_erase_first_shot_20260829.obj`, 10,223 bytes, SHA-256
`32ac984f02e717eebe9fb8d036a2512a9e9903b986616673e4539b305c772776`.
The January split object is 56,916 bytes, SHA-256
`632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
The pre-wave source blob is
`7c3f1862282f3c2ee6592edecf3de6d47a4bec95`; the retained source blob is
`2ac26a90b758532c5e57c0f6b4d8b09a7de0a7e7`.

## Evidence and retained behavior

The clean HCEA donor at commit
`c168af2e747d3095d9a29418ae401f3a39544863`, `actor_erase.c` blob
`736fb41ec4327979f603d61b05d80dfcfd1950f0`, authenticates the public name,
two-argument signature, swarm-unit drain, immediate/deferred deletion choice,
actor deletion, and non-swarm death path.

The independent January-PC reconstruction at commit
`918af885935ec470a31256ecce9a977b12b01f80`, actor source blob
`ecf9bb77905c2950e9941038ff82a5eee3712282`, separately fixes the January
branch topology, repeated reload of the swarm unit head, and all helper calls.
Complete January disassembly and relocations agree. The reconstruction's raw
offset spelling and one misnamed immediate-delete helper were not transplanted;
the January relocation and current public `objects.h` declaration identify
`object_delete_immediately` authoritatively.

The retained typed function resolves the actor with `actor_get`. For a swarm,
it repeatedly detaches and deletes the current typed
`meta.swarm_unit_index`, reloads the head, then calls `actor_delete(..., TRUE)`.
For an ordinary actor it saves `meta.unit_index`, calls `actor_died`, and
deletes that unit through the caller-selected immediate or deferred helper.

## One-shot, history, and scope boundary

All actor ledgers, the AI erase caller ledger, `AGENTS.md`, `CLAUDE.md`,
matching methodology, acceleration playbook, verification/snapshot policies,
prototype/ABI references, knowledge-base policy, object tooling documentation,
current typed declarations, and pinned VC7/XDK contract were read first.
History, ledger, and registered-worktree searches found caller context but no
earlier emitted `actor_erase` body.

One readable typed function plus ordinary public-helper prototypes was applied
and the production `actors.obj` edge compiled exactly once. The first strict
comparison passed complete padded bytes, normalized hash, and all ordered
relocation addresses/types/destinations. There was no retry or tuning.

The Actors focused gate reports 28 exact, zero residual, and 48 unwritten
functions; all 27 inherited accepted functions remain exact. Implementation
scope is only `source/ai/actors.c`; this ledger is the sole documentation
addition. The retained source uses no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address/offset access, pointer/integer
reconstruction, pointer or union pun, undefined behavior, synthetic anchor,
or byte forcing. Units, Vehicles, Matrix Math, AI Debug, configuration,
semantic exceptions, parks, completion labels, and build rules are untouched.
Full cumulative validation is recorded after commit.
