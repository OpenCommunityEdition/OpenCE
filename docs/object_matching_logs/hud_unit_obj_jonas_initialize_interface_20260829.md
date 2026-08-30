# `hud_unit.obj` unit-interface allocator recovery (2026-08-29)

## Result

Starting from canonical commit `cbc900ec836dc75fc97880cd7d819e45c6660326`,
the complete HCEA unit-HUD allocator was reconciled with the January PC
assertion and compiled once as ordinary typed C. The resulting
`_hud_initialize_unit_interface` is strict exact: 64 padded bytes, seven
ordered relocations, and normalized SHA-256
`ac99d4cb0ab1d0a0665d946ab7f7cd96e643704ec499324ae90f0c47b359326d`.
`hud_unit.obj` advances from 11/22 to 12/22 exact functions and retains zero
residual bodies.

## Provenance and source

The complete prior object/tooling audit is in
`hud_unit_obj_jonas_script_flags_set_flag_20260827.md`. It pins the January
object and XDK compiler, applies the campaign's strict COFF/relocation rules,
and authenticates the HCEA source tree. The relevant donor is
`research-cache/halocea-cseries-20260820/src/blam/interface/hud_initialize_unit_interface.c`.
That later build proves the allocation name, null type-name argument, global
owner, and unit-HUD allocation intent. January COFF independently proves the
0x164-byte allocation, post-assignment null assertion, source path and line
0x110, exact strings, and all seven relocation identities.

The already retained typed `unit_hud_globals` layout is exactly 0x164 bytes.
Production assigns `game_state_malloc("hud unit interface", NULL,
sizeof(*unit_hud_globals))` through the established global owner and then
asserts the typed pointer. A lowercase source alias preserves the authentic
assertion text while resolving to the existing `_bss_00453ac0` owner.

The body is readable C89 with an explicit return. It uses no assembly,
`volatile`, `register`, pragma, intrinsic, optimizer barrier, raw offset or
address access, representation cast, pointer/integer reconstruction, union
pun, undefined behavior, object patch, compiler exception, or byte forcing.
Units, Vehicles, Matrix Math, AI Debug, configuration, and unrelated source
are untouched.

## Strict object gate

```text
UNWRITTEN    64  _code_000c65c0
UNWRITTEN   112  _code_000c6600
UNWRITTEN   544  _code_000c6b80
UNWRITTEN   128  _hud_fix_unit_data
UNWRITTEN   240  _hud_initialize_unit_interface_for_new_map
UNWRITTEN   544  _hud_play_unit_sounds
UNWRITTEN   688  _hud_render_damage_indicators
UNWRITTEN  3504  _hud_render_unit_interface
UNWRITTEN    64  _hud_tick_shield
UNWRITTEN    48  _hud_update_unit
== exact 12  residual 0  unwritten 10  (of 22 listed)
```

The full Ninja build, report, semantic scan, and progress generation pass.
The campaign reports 382/833 linked objects, 4,726/11,060 exact functions,
599,128/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
The semantic scan evaluates 4,882 functions across 470 units, with 4,755
semantic exact, 129 hidden exact, 4,765 accepted exact, and zero unit errors.
Object admission is 0 candidates / 0 contradictions / 0 revocations; the
parked ledger is 12 active / 0 stale / 0 invalid; and all 205 tooling tests
pass. `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes and 87 relocations, normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
