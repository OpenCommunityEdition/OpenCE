# `hud_weapon.obj` weapon-interface allocator recovery (2026-08-29)

## Result

Starting from pushed canonical commit
`a955ee0e7c5af927e4a5e96ef6835be70d43ec0e`, one previously unwritten
production body was compiled once. `_hud_initialize_weapon_interface` is
strict exact on that first compile: 64 padded bytes, seven ordered
relocations, and normalized SHA-256
`3ab5739809ece905f4e4a2078a33e5221a6a8690a7fd7d85f3c3d98c3a4eb01b`.
`hud_weapon.obj` advances from 3/16 to 4/16 exact functions, with zero
residual bodies retained.

## Evidence and provenance

The prior object ledger
`hud_weapon_obj_jonas_strip_path_name_20260828.md` was read before editing.
It authenticates the January split object, XDK 3911 compiler, campaign
comparator, current three-function baseline, and the independently rejected
crosshair flag setter. No earlier allocator candidate or compile was found.

January's complete function bytes independently prove the allocation size
`0x1E4`, null type-name argument, allocation name `"hud weapon interface"`,
global owner `_bss_00453ac4`, assertion identifier `weapon_hud_globals`, source
path `c:\\halo\\SOURCE\\interface\\hud_weapon.c`, and line `0x6B`. The exact
call order is `game_state_malloc`, assignment, null test, `display_assert`,
and `system_exit`.

The donor census covered the following pinned trees:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, whose typed
  `weapon_hud_globals_definition.h` has SHA-256
  `1bbeca32ce1540bf3fbbed271f542a72d81b77410d64fe51dcb8a25eb8af44de`;
- Stian full-history commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, whose
  `hud_weapon.c` has SHA-256
  `7ece37b0ca7ef23717b7e3fde78778c63421efe197ae5741a1e0698ba31ac8e4`;
- Pastudan full-history commit `918af885935ec470a31256ecce9a977b12b01f80`, whose
  `hud_weapon.c` has SHA-256
  `7c9ae70cb4ed7d22eab598ac81213f94c63a0db0cbcddb32625da4b1e1fcc27f`;
- Punpckhdq full-history commit `fc47f9a15a0aea7f0081394c4d66db5c68ab5323`, whose
  symbol-map skeleton has SHA-256
  `c27b7b79816372db28ff5152de934918f396fd138c1dbac63a7c6e885fc23249`.

The applicable Pastudan and Stian `CLAUDE.md`/`AGENTS.md` generations were
fingerprinted and reviewed. Duplicate copies were identified by hash rather
than treated as independent evidence. Their relevant constraints agree with
the campaign: binary-first evidence, C89, explicit unknowns, stable ABI and
layout, small reviewable changes, narrow verification, and no inline
assembly. The later Xbox/HCEA trees corroborate subsystem intent and typed
state grouping; January COFF alone governs byte acceptance.

## Retained source shape

Production names the established global pointer, models the still-unknown
runtime payload as a reserved prefix plus the known trailing script-flags
field, and proves the `0x1E0` field offset and `0x1E4` complete size at compile
time. It calls the existing typed `game_state_malloc` declaration with
`sizeof(*weapon_hud_globals)` and uses the house assertion macro.

The retained body is readable defined C89 with an explicit return. It uses no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, raw offset or address access, representation cast, pointer/integer
reconstruction, union pun, undefined behavior, object patch, compiler/tool
exception, or byte forcing. Units, Vehicles, Matrix Math, AI Debug,
configuration, and unrelated source are untouched.

## Strict object gate

```text
== exact 4  residual 0  unwritten 12  (of 16 listed)
```

The full Ninja build, report, semantic scan, and progress generation pass.
The campaign reports 382/833 linked objects, 4,727/11,060 exact functions,
599,187/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes.
The semantic scan evaluates 4,883 functions across 470 units, with 4,756
semantic exact, 129 hidden exact, 4,766 accepted exact, and zero unit errors.
Object admission is 0 candidates / 0 contradictions / 0 revocations; the
parked ledger is 12 active / 0 stale / 0 invalid; and all 205 tooling tests
pass. `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes and 87 relocations, normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

## New-map reset continuation

After the allocator was integrated and pushed at commit `397c7cb2`, its
adjacent new-map reset was treated as an independent one-shot function.
January bytes prove a null assertion at source line `0x73`, followed by
`csmemset(weapon_hud_globals, -1, 0x1E4)`. The same natural topology appears
in the authenticated Stian/Pastudan donor family and is consistent with the
typed HCEA state owner.

The production C uses the already proven typed pointer, house assertion,
`NONE`, and `sizeof(*weapon_hud_globals)`. Its first and only compile is strict
exact:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hud_initialize_weapon_interface_for_new_map` | 64/64 | 7/7 | `719b9e897833a78791c35b1255ce9abb95efb7ad97084955578c51fd8d24c111` |

The original allocator remains independently exact at 64 bytes and seven
relocations. The retained-only object gate advances to five exact, zero
residual, and eleven unwritten functions. No alternate spelling, declaration,
loop, lifetime, compiler, or tool shape was tried.

The continuation's full Ninja graph also passes. The semantic scan evaluates
4,884 functions across 470 units, with 4,757 semantic exact, 129 hidden exact,
4,767 accepted exact, and zero unit errors. Progress advances to 4,728/11,060
exact functions and 599,247/2,198,102 meaningful code bytes while remaining
at 382/833 linked objects and 1,856,018/4,176,062 data bytes. Admission remains
0/0/0, the parked ledger remains 12 active / 0 stale / 0 invalid, all 205
tooling tests pass, and the protected Units function remains strict exact at
1,920 bytes / 87 relocations with hash `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
