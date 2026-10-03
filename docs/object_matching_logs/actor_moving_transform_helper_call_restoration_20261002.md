# actor_moving.obj: `actor_move_transform_avoidance_vector` helper-call restoration, 2026-10-02 (zero credit)

Lane `claude/remaining-frontier-20260926`. **Lane-verified, pending independent canonical reconciliation.** Not pushed.
Owner rulings of 2026-10-02, verbatim in `research/remaining_frontier_20260926/OWNER_PACKET.md`: Q-UP14 (the
vector-to-point casts admitted per site under seven conditions), Q-UP16 (land it as a unit-scoped zero-credit
fidelity correction, measured without the held `actor_move_test_avoidance_vector` candidate), Q-UP18 (land it with
the guard removal, as gated in `R118i_X`; "a narrow exception for the transform-helper restoration only"). Research
record: `research/remaining_frontier_20260926/lead/upstream_punpck/cards/UP-14_avoidance_function_sites.md`.

No byte of any function changes. `_actor_move_transform_avoidance_vector` was exact and its section is strictly
identical before and after. No credit is claimed; the new helper copy earns none.

## What changed, every item

`source/ai/actor_moving.c`:

1. `actor_move_transform_avoidance_vector`: the three hand-written expansions of the line helper and their local
   `component` (no PDB lists it) become three calls, as the unoptimised build has them:
   ```
   *direction_vector = *global_zero_vector3d;
   point_from_line3d((real_point3d *)direction_vector, &avoidance_data->forward, (avoidance_vector->i), (real_point3d *)direction_vector);
   point_from_line3d((real_point3d *)direction_vector, &avoidance_data->left, (avoidance_vector->j), (real_point3d *)direction_vector);
   point_from_line3d((real_point3d *)direction_vector, &avoidance_data->up, (avoidance_vector->k), (real_point3d *)direction_vector);
   ```
2. SIX casts `(real_point3d *)direction_vector`: the first and the fourth argument of each call.
3. THREE whole-value parenthesised scales, `(avoidance_vector->i)`, `(avoidance_vector->j)`, `(avoidance_vector->k)`.
   They are BYTE-INERT here: bare scales give the same section. They are per-site inferred spellings, taken from
   the unoptimised build's load-first order at each of the three sites; they are not authenticated source spelling.
4. The unit's two guard lines, `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` before the includes and its `#undef`
   after them, are removed. The restoration needs it: with the guard the three calls are real calls and two exact
   functions are lost (measured).
5. Consequence of 4: `actor_moving.obj` emits one new copy of `_point_from_line3d` (48 bytes, no relocations,
   COMDAT selection ANY), the 34th on the board, strictly equal to January's selected copy in
   `ai/action_charge.obj`. It receives no credit. `ai/actor_moving` is one of the 17 January objects that
   reference the helper out of line (owner ruling 2026-09-21 no. 1). Removing the guard by itself changes no
   function section of the unit.

Nothing else: no scope change, no rename, no other function's text, no header, no configuration file. The held
`actor_move_test_avoidance_vector` candidate is NOT in this commit; that function keeps its text and its two calls
of this helper.

## The six casts (ruling Q-UP14)

| Condition | Finding |
| --- | --- |
| The object is a vector in a first-party PDB | the parameter `direction_vector` is `union real_vector3d *` (2011 release PDB) |
| The unoptimised build passes that object as the point argument at each call | function at 0x467840, calls at 0x467877, 0x46789c, 0x4678c1: `mov reg, [ebp+0x10]; push reg` for the result and again for the first argument, `[ebp+8] + 0x18 / 0x24 / 0x30` for the vector |
| The casts are byte-inert | measured: with and without them the function has the same section |
| The function is strictly exact | 144 B, 1 relocation, exact before and after |
| Compatible layout and alignment | compile-time assertions with January's compiler and this unit's flags: both types 12 bytes, alignment of `real`, members at 0, 4 and 8; a false control assertion fails to compile |
| Constness preserved | `direction_vector` is a pointer to a non-const vector; no qualifier is removed |
| Listed here | six casts, item 2 above |

**The view is first-party-attested; the spelling of the cast is inferred, not recovered source.** Without the
casts the unit gains six C4133 warnings.

## The three parentheses

| Site | Unoptimised build | January |
| --- | --- | --- |
| `(avoidance_vector->i)` | `mov edx, [ebp+0xc]; movss xmm0, [edx]; push ecx; movss [esp], xmm0` (0x46785f) | cannot tell: bare gives the same bytes |
| `(avoidance_vector->j)` | `movss xmm0, [eax+4]` before the slot is reserved (0x467886) | cannot tell |
| `(avoidance_vector->k)` | `movss xmm0, [ecx+8]` before the slot is reserved (0x4678ab) | cannot tell |

Compiler-version gap, disclosed as before: that build was made with MSVC 19.00.24234, which is not on the measuring
machine; the load-first reading is bracketed by 13.00.9254, 19.12 and 19.51, not tested on 19.00. The parenthesis
class in general stays held.

Other first-party support for the three one-line calls: the 2011 release PDB's line table gives this function the
lines 2698 (`{`), 2699, 2702, 2705, which fits a zero copy, three one-line calls, a blank line and `return;`.

## Lane gate

Measured alone first (the restoration without the held candidate), then gated fresh in the lane: `R118i_X` against
`R117i_J` (incremental build, objdiff 3.3.1). Zero exact losses, zero gains.

- Stable diff: gained 0, regressions 0. Functions 7,482 / 7,573, code 1,602,328 / 1,770,166, data 3,311,591 /
  3,922,163, objects 396 / 468, strict section owners 7,655 / 8,252, parks 59: all unchanged.
- Admission candidates 9, 0 contradicted, 0 rejected, 0 revoked. Tools tests 1,700 passed, 5 skipped. Fake-match
  scan: the same 26 inherited leads. Whitespace checks pass.
- `actor_moving.obj`: all 64 function sections present before are strictly identical after; 33 exact before and
  after; one section is new, the helper copy. External symbols: one definition more (`_point_from_line3d`), one
  undefined reference fewer (the same symbol), nothing else: no new data, COMMON, constant or string.
- Real calls of the helper per function unchanged and equal to January's: `_actor_move_calculate_movement` 1,
  `_actor_move_try_evasion_vector` 1.
- Selected-provider pair links in both input orders: PASS for the new copy and for all 58 surplus external
  definitions. Whole-board diagnostic link before and after: LNK1120 419 = 419, the same 398 unresolved names,
  LNK2005 only the inherited `_real_local_random`; no image was produced.
- /W3 4,082 = 4,082, no unit differs. /W4 for `actor_moving.c` 95 = 95.
