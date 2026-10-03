# actor_moving.obj: `actor_move_test_avoidance_vector` exact, 2026-10-02 (+1 function, +743 meaningful / +752 padded bytes)

Lane `claude/remaining-frontier-20260926`. **Lane-verified, pending independent canonical reconciliation.** Not pushed.
Owner rulings of 2026-10-02, verbatim in `research/remaining_frontier_20260926/OWNER_PACKET.md`: Q-UP14 (the
vector-to-point casts admitted per site under seven conditions), Q-UP15 (the candidate held for its two scopes until
independent lifetime evidence), Q-UP21 (land after fresh full gates; it "narrowly lifts Q-UP15": January's slot
sharing plus the separately attested variables supports the disjoint-scope reconstruction, and the exact brace
placement remains inferred, not recovered source). Research record:
`research/remaining_frontier_20260926/lead/upstream_punpck/cards/UP-14_avoidance_function_sites.md` (sections 2 to 9).

`_actor_move_test_avoidance_vector` (752 bytes, 16 relocations) goes from residual to strictly exact. One other
function's bytes move and it stays residual: `_actor_move_vector_avoidance`, 4,144 -> 4,128 bytes (January 4,144).

## What changed, every item

Only the body of `actor_move_test_avoidance_vector` in `source/ai/actor_moving.c` (the unit's guard lines and the
transform helper were already changed by `41d3b992`):

1. The two calls of `actor_move_transform_avoidance_vector` (the unoptimised build's function does not call it) are
   replaced by six written-out calls of `point_from_line3d` on two zero-initialised vectors, as the unoptimised build
   has them, each vector first set by a struct copy `= *global_zero_vector3d`:
   ```
   origin_offset = *global_zero_vector3d;
   point_from_line3d((real_point3d *)&origin_offset, &avoidance_data->forward, (avoidance_ray->offset.i), (real_point3d *)&origin_offset);
   ... (left, offset.j), (up, offset.k); then divergence_direction with divergence.i / .j / .k
   ```
2. The local `scale` (no PDB lists it) and its three hand-written lines become one call,
   `scale_vector3d(&divergence_direction, avoidance_ray->length*avoidance_data->avoid_distance, ray_direction)`;
   the operand order of that product is the unoptimised build's (byte-inert).
3. A seventh call, `point_from_line3d(&avoidance_data->origin, &origin_offset, (avoidance_data->avoid_width),
   ray_origin)`, replaces the lane's hand expansion there.
4. SEVEN whole-value parenthesised scales, one per call (items 1 and 3). See "The seven parentheses".
5. TWELVE casts: `(real_point3d *)&origin_offset` and `(real_point3d *)&divergence_direction`, the first and the
   fourth argument of the six axis calls. See "The twelve casts".
6. Locals renamed to the PDBs' names: `offset` -> `origin_offset`, `divergence` -> `divergence_direction`, `object_t`
   -> `intersect_t`; declarations in the PDBs' record order (byte-inert).
7. Two locals added, both `real_vector3d`, names and types from the PDBs: `vector_to_origin` (the structure test's
   vector) and `dummy_normal` (the pill test's normal). The lane text had reused `offset` for both roles.
8. SCOPES, INFERRED: `vector_to_origin` is declared in a new bare block around the two structure tests, and
   `dummy_normal` in the body of the object loop. `collision_result` is declared inside the same bare block; that
   placement is byte-inert and rests on the PDB record order alone. All three placements are inferred, not
   recovered source. See "The scopes".

Nothing else: no other function's text, no header, no guard, no configuration file, no comment. No park existed for
this function.

## The scopes (ruling Q-UP21)

January passes the address of ONE frame slot, `[ebp-0x14]`, to two different real calls: the structure test at
+0x173 and the pill test at +0x27a. The same slot is `origin_offset`'s. Twelve scopings were compiled with January's
compiler (`tools/tav2_probe.py`, outputs `up14/tav2_probe.txt` and `up14/tav2_probe_2.txt`):

| Tested form | Result |
| --- | --- |
| this text: block around the structure tests, normal in the loop body | EXACT (frame 0x438, slot 0x14 at both calls) |
| the normal in a block around the loop instead | EXACT |
| the block opening earlier, before the seventh call (i.e. before `origin_offset`'s last use) | EXACT |
| one variable for both roles, in one block over the tests and the loop | EXACT |
| both vectors at function scope; either one at function scope | residual (frame 0x450 or 0x444) |
| two variables in one scope (the loop inside the block) | residual (two slots) |
| one variable reused at function scope; `origin_offset` reused for both roles; for either role | residual |

Among the tested forms the bytes require the escaped vector storage to be out of function scope; that is a
statement about storage lifetimes, not about where an opening brace stands. The bytes do not choose between one
reused variable in one block and two variables in non-overlapping scopes: January alone still permits the
single-variable alternative. The separate-variable reconstruction relies on the later first-party records: the
2003 PC demo PDB and both 2011 PDBs list `vector_to_origin` and `dummy_normal` as separate `union real_vector3d`
locals (in the demo PDB all three vectors share one frame offset), and the unoptimised build gives them three
distinct slots. Given two variables, the tested forms require their scopes not to overlap and neither to be the
function's. Where the first block opens, and whether the normal is declared in the loop body or around the loop,
is not determined; no first-party record of a block exists. This text takes the smallest block and the loop body.

## The seven parentheses (admitted for this function only, ruling Q-UP21)

| Site | Unoptimised build (function 0x466b10) | January |
| --- | --- | --- |
| `(avoidance_ray->offset.i)` | load-first, `[ecx+4]`, call 0x466c09 | needs it (bare: residual) |
| `(avoidance_ray->offset.j)` | load-first, `[edx+8]`, call 0x466c2e | needs it |
| `(avoidance_ray->offset.k)` | load-first, `[eax+0xc]`, call 0x466c53 | needs it |
| `(avoidance_ray->divergence.i)` | load-first, `[edx+0x10]`, call 0x466c8e | needs it |
| `(avoidance_ray->divergence.j)` | load-first, `[eax+0x14]`, call 0x466cb3 | needs it |
| `(avoidance_ray->divergence.k)` | load-first, `[ecx+0x18]`, call 0x466cd8 | needs it |
| `(avoidance_data->avoid_width)` | load-first, `[edx+0x6040]`, call 0x466d00 | cannot tell (bare gives the same bytes) |

Each of the six axis scales made bare alone leaves the function residual (six tests). At the second axis site,
seven spellings that are not whole-value parentheses give one and the same residual section, `(real)(x)` and
`(0, x)` are exact too, and `((x))` is not. So the spelling is inferred from per-site first-party codegen, not
authenticated source spelling. Compiler-version gap,
disclosed as before: the unoptimised build was made with MSVC 19.00.24234, which is not on the measuring machine;
the load-first reading is bracketed by 13.00.9254, 19.12 and 19.51, not tested on 19.00. The parenthesis class in
general stays held; this is no broader exception.

## The twelve casts (ruling Q-UP14 conditions, retained)

| Condition | Finding |
| --- | --- |
| The object is a vector in a first-party PDB | `origin_offset` and `divergence_direction` are `union real_vector3d` in the demo PDB and both 2011 PDBs |
| The unoptimised build passes that object as the point argument at each call | each local's address is the first argument and the result of three calls (0x466c09 .. 0x466cd8) |
| The casts are byte-inert | measured: the function has the same section without them (twelve C4133 warnings then) |
| The function is strictly exact | yes, with this text |
| Compatible layout and alignment | the compile-time check with January's compiler and the unit's flags (`tools/layoutcheck.py`): both types 12 bytes, alignment of `real`, members at 0, 4 and 8; a false control fails |
| Constness preserved | both locals are non-const; no qualifier is removed |
| Listed here | twelve casts, item 5 above |

The view is first-party-attested; the spelling of the cast is inferred, not recovered source.

## Lane gate

Measured first in a second build shadow (gate `SG_F2`), then gated fresh in the lane: `R119i_F` against `R118i_X`
(incremental build, objdiff 3.3.1). Every requirement of ruling Q-UP21 holds.

- Stable diff: gained 1 (`_actor_move_test_avoidance_vector`, 752 padded bytes), regressions 0.
- Functions 7,482 -> 7,483 / 7,573; code 1,602,328 -> 1,603,071 / 1,770,166 (+743 meaningful bytes); data 3,311,591
  / 3,922,163 unchanged; objects 396 / 468 unchanged (no object credit changes); strict section owners 7,655 ->
  7,656 / 8,252; parks 59, none stale or invalid.
- Admission candidates 9, 0 contradicted, 0 rejected, 0 revoked. Tools tests 1,700 passed, 5 skipped. Fake-match
  scan: the same 26 inherited leads. No new warning; whitespace checks pass.
- Whole board: 1 of 621 objects differs, `actor_moving.obj`. In it: `_actor_move_test_avoidance_vector` residual
  (544 bytes, 17 relocations) -> EXACT (752 bytes, 16 relocations); `_actor_move_vector_avoidance` residual ->
  residual, 4,144 -> 4,128 bytes (135 relocations either way; January 4,144); every other function section is
  strictly identical (three differ only in internal `$L` label numbers); 33 -> 34 exact; no section, definition or
  reference is added or removed.
- Helper: the object already defines `_point_from_line3d` (since `41d3b992`); its copy is strictly equal to
  January's selected copy; real calls of the helper per function 2 = 2 = January's 2
  (`_actor_move_calculate_movement`, `_actor_move_try_evasion_vector`); the expanded sites stay expanded.
- Selected-provider pair links in both input orders: PASS for all 58 surplus external definitions. Whole-board
  diagnostic link before and after: LNK1120 419 = 419, the same 398 unresolved names, LNK2005 only the inherited
  `_real_local_random`; no image was produced.
- /W3 4,082 = 4,082, no unit differs. /W4 for `actor_moving.c` 95 = 95. Layout assertions for the casts: PASS (nine),
  the false control fails to compile.

Transcript: `research/remaining_frontier_20260926/lead/upstream_punpck/up16/land_R119i_F.txt` (banked with the
landing records).
