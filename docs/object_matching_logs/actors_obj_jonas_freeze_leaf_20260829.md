# `actors.obj` January freeze leaf

## Result

`_actors_freeze` matches in the first and only natural production candidate.
It contributes 62 meaningful / 64 padded code bytes and four ordered
relocations. `source/ai/actors.obj` advances from 25/76 to 26/76 strict
functions and remains `NonMatching`.

The target and candidate normalized SHA-256 is
`f689e2ee11a8c1958103b166c7970cd015cf4eecbe11990c32f92ce61f3fda52`.
Their relocation packets are identical:

- `+0x0D`: i386 `REL32` to `_actor_iterator_new`;
- `+0x16`: i386 `REL32` to `_actor_iterator_next`;
- `+0x26`: i386 `REL32` to `_code_00029e70`;
- `+0x2F`: i386 `REL32` to `_actor_iterator_next`.

The immutable first-shot object is
`build/audit/actors_freeze_first_shot_20260829.obj`, 9,306 bytes, SHA-256
`3e443b36ca544e2d33c759e9bfe117ab697b33a3acc44a64577d72a1f19f8e06`.
The January split object is 56,916 bytes, SHA-256
`632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.

## Retained source and layout

The pre-wave `source/ai/actors.c` blob is
`0fa2c2b6b482ae7abb0c4e609c8cf826c0a67afd`; the retained blob is
`af5a9f90b5ca8480edd18bf13f013b6028728d4f`. The complete function body is:

```c
void actors_freeze(
	void)
{
	struct actor_iterator iterator;

	actor_iterator_new(&iterator, TRUE);
	while (actor_iterator_next(&iterator))
	{
		code_00029e70(iterator.index);
	}

	return;
}
```

The translation-unit-local typed iterator is the already authenticated 0x1C
layout used by `ai.c`, `ai_communication.c`, `ai_debug.c`, and `encounters.c`:
an embedded 0x10 `data_iterator`, two boolean fields, two pad bytes, `index` at
`+0x14`, and `next_index` at `+0x18`. Compile-time assertions freeze the total
size and the only accessed field offset. The public `actor_iterators.h`
prototypes provide the ordinary cdecl ABI; no shared-header definition or ABI
change is introduced.

## January and cross-build evidence

The complete January COMDAT disassembly fixes a 0x1C stack iterator,
`actor_iterator_new(&iterator, TRUE)`, a top-tested iterator loop, the actor
handle load from `[ebp-8]` (`iterator.index`), and one call to
`code_00029e70` per live actor. Its terminal `ret` is at `+0x3D`, followed by
two NOPs. The retained typed loop emits that complete topology, instruction
stream, padding, and relocation schedule naturally.

An independent January-PC reconstruction at commit
`918af885935ec470a31256ecce9a977b12b01f80`, source blob
`ecf9bb77905c2950e9941038ff82a5eee3712282`, independently records the same
0x1C iterator, active-only flag, iterator-next loop, `index` load at `+0x14`,
and per-actor helper call. Its source payload SHA-256 is
`0105cdaf6f2c3c11e382b018920307fecf9c9057dfe887db0cd0e4b1c7b8600d`.
That file is readable reverse-engineering evidence, not authentic source; its
anonymous helper labels and raw-array spelling were not transplanted.

The clean HCEA reference at commit
`c168af2e747d3095d9a29418ae401f3a39544863`, `actors_freeze.c` blob
`e619f5e4bace1d80e9dfea4f0d078066731615e2`, authenticates the public name,
no-argument signature, 0x1C actor iterator, active-only iteration, and
freeze-every-live-actor semantics. Its later Xbox body expands the per-actor
operation inline and carries a documented phantom helper argument. That
cross-build difference was treated as a version boundary. January's split
object and the independently agreeing January-PC reconstruction are
authoritative for the short helper-call topology.

## History, documentation, and one-shot boundary

All four actor-specific ledgers were read completely before mutation:

- `actors_obj_codex_checkpoint.md`;
- `actors_obj_jonas_maintenance_pair_20260821.md`;
- `actors_obj_jonas_reset_leaf_first_shot_20260826.md`;
- `actors_obj_jonas_public_leaf_batch_20260829.md`.

The encounter-iterator ledger was also read because it previously rejected
this function before the January-PC helper topology had been established. It
contains no candidate body or compile for `_actors_freeze`; the reachable
source history, ledger census, and registered-worktree census likewise found
no earlier production attempt. This wave therefore consumes the function's
first candidate.

The campaign `AGENTS.md`, `CLAUDE.md`, matching methodology, acceleration
playbook, verification and snapshot policies, prototype/ABI references,
knowledge-base policy, object tooling documentation, current actor types, and
the pinned VC7/XDK build contract were applied. They require complete padded
COFF bytes plus ordered relocations and prohibit byte forcing, source-shape
sweeps, compiler control, raw offsets, address transplants, punning, undefined
behavior, and knowledge-base exceptions.

The typed structure, include, assertions, and one natural loop were applied
together. The ordinary production `actors.obj` edge compiled exactly once.
The strict comparator passed immediately; there was no declaration, control-
flow, source-shape, flag, or compiler retry.

## Acceptance and scope

`python -m tools.campaign.gate source/ai/actors --all` reports 26 exact, zero
residual, and 50 unwritten functions. Direct hardened comparison passes the
new leaf and all 25 inherited accepted actor functions.

The tracked implementation scope is only `source/ai/actors.c`; this ledger is
the sole documentation addition. The retained code is readable typed
C89-style source with explicit return. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, compiler attribute, barrier, raw address or
offset access, pointer/integer reconstruction, pointer or union pun, undefined
behavior, synthetic anchor, or object-byte forcing.

Units, Vehicles, Matrix Math, AI Debug, campaign configuration, semantic
exceptions, parked records, completion labels, and build rules are untouched.
Full cumulative build, semantic, progress, admission, park, protected-Units,
and test evidence is recorded after the implementation-and-ledger commit.
