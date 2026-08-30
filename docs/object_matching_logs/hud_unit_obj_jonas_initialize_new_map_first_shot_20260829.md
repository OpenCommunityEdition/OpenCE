# `hud_unit.obj`: new-map reset first-shot exact recovery

Date: 2026-08-29

Author: Jonas lane

Base commit: `9463bd1306cff2d79910075b008d348b05b3b3f2`

## Scope and selection

This lane freezes exactly one previously unwritten function in
`source/interface/hud_unit.obj`:

```text
_hud_initialize_unit_interface_for_new_map
```

Vehicles, Units, Matrix Math, AI Debug, every documented fixed point, and all
other live-agent object reservations were excluded. No sibling function,
header, configuration file, semantic exception, parked record, or compiler
option is changed. The base `source/interface/hud_unit.c` blob was
`5fcf77ad2dae15f5495dbd826e8162b1598c1ac6`.

The target is a high-confidence link-closed recovery. January calls only the
established `display_assert`, `system_exit`, and `csmemset` symbols, refers to
the existing HUD global and assertion strings, and has no private helper or
anonymous-data dependency.

## Authenticated evidence

The HCEA donor at
`src/blam/interface/hud_initialize_unit_interface_for_new_map.c` in repository
commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` is Git blob
`b40d8e185be2faa38a13c057ea760770bdf60823` and physical SHA-256
`ed1a5049bc15676b24e249949b31b7f73d7e2abbd4438ad1e1d0f048c082a606`.
It authenticates the natural topology: clear the aggregate, initialize every
local-player HUD state, clear its sound flags, and fill its last-sound handles
with `NONE`.

January Xbox COFF fixes the PC-specific details that the later donor cannot:

- four local players rather than two;
- a `0x58`-byte state stride and a `0x164`-byte aggregate;
- state fields at `+0x22`, `+0x24`, and `+0x28`;
- the inline loop/helper order and the two assertion source lines;
- a 240-byte padded COMDAT with 19 ordered relocations.

Applicable campaign, Claude/AGENTS, tree, tooling, and existing HUD Unit
ledgers were read before the attempt. Repository history and ledger search
found no earlier production candidate for this function.

## Frozen natural reconstruction

Production replaces only the aggregate's formerly opaque prefix with named
typed state fields and compile-time layout assertions. The C89 function uses a
`short` player index, typed state pointer, named fields, `sizeof` expressions,
the established assertion macro, `NONE`, and an explicit terminal `return;`.
Its statement order follows the authenticated donor and January disassembly.

A non-emitting `/Zs` preflight initially exposed that this translation unit
does not import the repository's `real` typedef. Changing the three new state
fields to the identical native C89 `float` type was the only parse correction;
it did not emit an object or change the frozen function topology. The second
parse-only check passed. The baseline object remained raw SHA-256
`e2b6f809a605f9cf61bfb291d79664bf9a05e41a60a6234a85fede7a94b19fdb`.

Exactly one normal production compile was then run. The emitted candidate was
preserved as
`build/audit/hud_unit_initialize_new_map_first_shot_20260829.obj`, raw
SHA-256
`3127abe8c2a1370e5ec2cf78d93dd6f0c0d535d5ecbabf18f71ce6cb4691e931`.

The retained source is readable defined C89 with one parameter per line and
explicit `void`/return form. It uses no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, optimizer barrier, raw address or offset access,
pointer/integer reconstruction, representation pun, undefined behavior, byte
forcing, compiler change, tool exception, or admission exception.

## Strict result

| Measure | January target | First candidate |
| --- | ---: | ---: |
| Meaningful instruction extent | 239 | 239 |
| Padded COMDAT extent | 240 | 240 |
| Ordered relocations | 19 | 19 |
| Normalized SHA-256 | `fedc99986889e06979b952cc815eec7ff108946a204784dbc4b1092181686f3e` | `fedc99986889e06979b952cc815eec7ff108946a204784dbc4b1092181686f3e` |

The last meaningful instruction is the `ret` at `+0xEE`; the final byte is
the normal alignment NOP. Hardened padded-COFF comparison reports
`all_equal: true`. All 19 relocation addresses, types, symbolic identities,
destinations, and addends match in order. The January split object has raw
SHA-256
`8e675992cb876c291c4ee4c18ae69a0be143f61f5161bf87cdde5f6633df5cfd`.

All 12 inherited exact siblings remain exact:

- `_code_000c6590`;
- `_code_000c65a0`;
- `_unit_hud_shield_meter_mapper_init`;
- `_hud_initialize_unit_interface`;
- `_hud_dispose_unit_interface_from_old_map`;
- `_hud_dispose_unit_interface`;
- `_scripted_hud_show_health`;
- `_scripted_hud_blink_health`;
- `_scripted_hud_show_shield`;
- `_scripted_hud_blink_shield`;
- `_scripted_hud_show_motion_sensor`;
- `_scripted_hud_blink_motion_sensor`.

The focused object gate advances to 13 exact, zero residual, and nine
unwritten functions of 22.

## Repository gates

The complete `halobetacache_build`, `libcmt_build`, report, semantic-report,
and progress graph passes. Final evidence:

- semantic audit: 470 units, 4,918 functions evaluated, 4,801 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,811 accepted exact, and zero
  unit errors;
- strict progress: 384/833 linked objects, 4,772/11,060 functions,
  606,426/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- tooling suite: 212/212 tests pass;
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes and 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
- `build/semantic_report.json` SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`;
- `git diff --check` and the added-line forbidden-construct scan are clean.

This function earns strict partial-function credit. The translation unit
correctly remains partial; no whole-object or data credit is claimed.
