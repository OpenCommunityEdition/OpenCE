# bipeds.obj: `biped_jump` helper-call restoration, 2026-10-02 (zero credit)

Lane `claude/remaining-frontier-20260926`. **Lane-verified, pending independent canonical reconciliation.** Not pushed.
Owner rulings of 2026-10-02, verbatim in `research/remaining_frontier_20260926/OWNER_PACKET.md`: Q-UP13 (held until
the casts had evidence and a ruling), Q-UP14 (the vector-to-point casts admitted per site under seven conditions),
Q-UP16 (land it as a unit-scoped zero-credit fidelity correction). Research record:
`research/remaining_frontier_20260926/lead/upstream_punpck/cards/UP-13_biped_get_sight_position_direct_arguments.md`,
section 17.

No byte changes: `_biped_jump` was exact and its section is strictly identical before and after. No credit is
claimed.

## What changed

`source/units/bipeds.c`, `biped_jump`: the hand-written expansion of the line helper and its local
`velocity_delta` become the helper call.

```
before                                                    after
real velocity_delta = jump_magnitude-upward_velocity;     point_from_line3d(
                                                              (real_point3d *)&jump_velocity,
jump_velocity.i += biped->object.up.i*velocity_delta;         &biped->object.up,
jump_velocity.j += biped->object.up.j*velocity_delta;         jump_magnitude-upward_velocity,
jump_velocity.k += biped->object.up.k*velocity_delta;         (real_point3d *)&jump_velocity);
```

Nothing else: no parenthesis, no scope change, no rename, no other function, no header, no configuration file.

## The two casts (ruling Q-UP14)

`(real_point3d *)&jump_velocity`, twice: the first and the fourth argument of the call. The helper's parameters
there are `real_point3d const *p` and `real_point3d *result`; the object is a `real_vector3d`.

| Condition | Finding |
| --- | --- |
| The object is a vector in first-party PDBs | `union real_vector3d new_velocity`, a local of `biped_jump`, in the 2003 demo PDB and both 2011 PDBs (the lane's name for it is `jump_velocity`) |
| The unoptimised build passes that object as the point argument at this call | call of `point_from_line3d` at 0x8c08d9: `lea ecx, [ebp-0x24]; push ecx` (result), the scale `[ebp-0x14] - [ebp-0x30]` computed as the argument, `[ebp-8] + 0x3c` (`&biped->object.up`), `lea eax, [ebp-0x24]; push eax` (first argument). One object, both point parameters. No local holds the scale |
| The cast is byte-inert | measured: with the casts and without them `_biped_jump` has the same section |
| The function is strictly exact | 432 B, 10 relocations, exact before and after |
| Compatible layout and alignment | compile-time assertions with January's compiler and this unit's flags: both types are 12 bytes, alignment of `real`, members at 0, 4 and 8, the array view at 0; a control assertion that is false fails to compile |
| Constness preserved | `jump_velocity` is a non-const local; no qualifier is removed (the first parameter adds `const`) |
| Listed here | two casts, both above |

**The view is first-party-attested; the spelling of the cast is inferred, not recovered source.** The later build is
a C++ compile and must have had an explicit conversion at this call; how it was written is not known. Without the
casts the unit gains two C4133 warnings in C.

Not changed although first-party names differ (declaration corrections are ruled on in batches): the vector is
`new_velocity` in the PDBs.

## Lane gate

`R117i_J` against `R116i_B` (incremental build, objdiff 3.3.1). Zero exact losses, zero gains.

- Stable diff: gained 0, regressions 0. Functions 7,482 / 7,573, code 1,602,328 / 1,770,166, data 3,311,591 /
  3,922,163, objects 396 / 468, strict section owners 7,655 / 8,252, parks 59: all unchanged.
- Admission candidates 9, 0 contradicted, 0 rejected, 0 revoked. Tools tests 1,700 passed, 5 skipped. Fake-match
  scan: the same 26 inherited leads. Whitespace checks pass.
- `bipeds.obj`: all 85 function sections strictly identical before and after (the strict comparator); 46 exact
  before and after. The object's fingerprint differs only in the numbering of compiler labels. No external symbol
  is added or removed; the object already held its copy of `_point_from_line3d` (strictly equal to January's).
- Real calls of the helper per function unchanged and equal to January's (10 in four functions): the restored call
  is expanded inline, as January's code is.
- Selected-provider pair links in both input orders: PASS for all 84 surplus external definitions. Whole-board
  diagnostic link before and after: LNK1120 419 = 419, the same 398 unresolved names, LNK2005 only the inherited
  `_real_local_random`; no image was produced.
- /W3 4,082 = 4,082, no unit differs. /W4 for `bipeds.c` 101 = 101.
