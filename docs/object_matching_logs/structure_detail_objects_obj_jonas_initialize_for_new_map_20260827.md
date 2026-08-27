# `structure_detail_objects.obj` new-map reset recovery (2026-08-27)

## Scope and prior-ledger status

This wave adds only `_structure_detail_objects_initialize_for_new_map` and a
typed source alias for the translation-unit-owned runtime pointer in
`source/structures/structure_detail_objects.c`. The prior bounded ledger
explicitly says this function was excluded and never authored or tested, so
this is fresh evidence rather than a replay of either rejected candidate.

The pre-edit source blob was `71fa727d5860c397157cb3a9995322c64a27c3a1`;
the retained source blob is `fc3f52f5716e8cf9735d7b01ea24dc17882f59ca`.

## Frozen target and source evidence

The January target is 73 meaningful bytes / 80 padded bytes, with normalized
SHA-256
`40ecb31b699e9cda0148cc10e0279d8f441ff71c30553d3574ad53f3c1f51bb5`.
Its eight relocations are:

- `DIR32 +0x01` to `_debug_detail_objects + 4`;
- `DIR32 +0x0e` to the exact source-file string;
- `DIR32 +0x13` to the exact `detail_object_global_runtime_data` condition
  string;
- `REL32 +0x18` to `_display_assert`;
- `REL32 +0x1f` to `_system_exit`;
- `DIR32 +0x27` to `_debug_detail_objects + 4`;
- `REL32 +0x34` to `_csmemset`;
- `DIR32 +0x3a` to `_debug_detail_objects + 4`.

The target topology is a non-null assertion at source line `0x6d`, a zero of
the complete `0xa430` runtime record, and a clear of the first player's
initialized byte at offset `0x520e`. The binary-backed HCEA documentation in
`C:\halo-worktrees\claude-ai-conversation-2\src\halo\structures\structures.c`
independently confirms this operation and layout. The January target remains
the byte and relocation authority.

The source-level alias
`detail_object_global_runtime_data -> debug_detail_objects.runtime_data`
preserves typed access while allowing the established `match_assert` macro to
stringize the original condition spelling. Normal macro expansion naturally
produces the target's three separate TU-global pointer loads; no scheduling
device is used.

The target object SHA-256 is
`1BEA58EFF13264FB6E322E07E27E7A204D3390A025A1B67525ABB3E0C15344DA`.
The frozen baseline is
`build/audit/structure_detail_objects_initialize_for_new_map_baseline_20260827.obj`,
SHA-256
`159C3FA51CD9EF03EE714A349EF1A882BCF2C45E52CCC21CFF0026405D34E7F2`.

## First-shot result

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/structure_detail_objects_initialize_for_new_map_first_shot_20260827.obj`,
SHA-256
`8469814C843C5629618287006BDDFEC7F142291EACDE495841BF41FF4F8AA8DC`.

The hardened comparator proves all 80 padded function bytes and all eight
relocations exact in address, type, identity, addend, and ownership. The four
inherited exact functions remain exact; `lost_exact` is empty and the sole
`newly_exact` entry is `_structure_detail_objects_initialize_for_new_map`.
The object advances from 4/15 to 5/15 strict functions.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,646 code bytes, 4,250 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,732 code bytes, 4,083 functions, 1,831,586 data bytes;
- semantic audit: 4,361 evaluated, 4,216 semantic exact, 4,276 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the no-argument and explicit terminal-return house
rules. It uses typed field access and C89 source. No protected source,
assembly, intrinsic, raw address, pointer pun, barrier, exception, compiler
substitution, or object patch was used.
