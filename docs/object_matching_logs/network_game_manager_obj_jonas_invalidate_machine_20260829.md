# `network_game_manager.obj` invalidate-machine recovery (2026-08-29)

## Result

Starting from canonical commit `26d7a358ccb78b692cedc38384c82f4c6a1b1776`,
one previously unattempted typed donor shape was compiled once.
`_network_game_invalidate_machine` is strict exact: 176 padded bytes, eight
ordered relocations, and normalized SHA-256
`193f549816e10a9884f9e52280539271f101de6627d1b232d4ecebde61a6ad90`.
The object advances from 12/19 to 13/19 exact functions, with six functions
still unwritten and no residual retained.

## Evidence and source shape

The complete prior donor/provenance and tool audit is recorded in
`network_game_manager_obj_jonas_public_donor_pair_20260828.md`. That ledger
pins and reviews the applicable Stian and Pastudan `AGENTS.md` and
`CLAUDE.md` files, the HCEA semantic tree, the January target object, the XDK
compiler, and the hardened campaign tooling. Its earlier three-function batch
did not attempt this function.

January disassembly and both target-version public reconstructions agree on
the same behavior: validate a typed game and an unsigned machine index below
four; invalidate the selected machine record; clear its first UTF-16 name
element; then invalidate each of the sixteen typed player records owned by
that machine. Production uses the existing exact
`network_game_invalidate_player` body as a natural same-translation-unit
inline source. Naming the machine's 32-element UTF-16 field replaces an
equally sized unknown byte array and preserves the 0x44-byte machine layout.

The retained body is ordinary readable C89. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, optimizer barrier, raw offset or
address access, representation cast, pointer/integer reconstruction, union
pun, undefined behavior, object patch, compiler exception, or byte forcing.
Units, Vehicles, Matrix Math, AI Debug, configuration, and unrelated sources
are untouched.

## Strict gate

The first production object compile emitted exactly one ordinary CL edge.
The hardened comparator proves identical complete padded bytes and identical
relocation addresses, types, symbolic destinations, and addends. The
retained-only object gate reports 13 exact, zero residual, and six unwritten:

```text
UNWRITTEN   320  _code_0011a750
UNWRITTEN   144  _network_game_add_machine
UNWRITTEN   416  _network_game_add_player
UNWRITTEN   432  _network_game_create_game_objects
UNWRITTEN   224  _network_game_remove_machine
UNWRITTEN   192  _network_game_remove_player
== exact 13  residual 0  unwritten 6  (of 19 listed)
```

The full Ninja build, report, semantic scan, and progress generation pass.
The campaign reports 382/833 linked objects, 4,725/11,060 exact functions,
599,066/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
The semantic scan evaluates 4,881 functions across 470 units, with 4,754
semantic exact, 129 hidden exact, 4,764 accepted exact, and zero unit errors.
Object admission is 0 candidates / 0 contradictions / 0 revocations; the
parked ledger is 12 active / 0 stale / 0 invalid; and all 205 tooling tests
pass. `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes and 87 relocations, normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
