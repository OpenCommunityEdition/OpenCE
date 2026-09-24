# Owner ruling request: actor_combat P1 (cross_product2d)

## The question (yes or no)

**Question.** Do you admit patch 01 while `_actor_aim_grenade` is still not strict exact (576 bytes against
January's 544)? Patch 01 does two things:

- It deletes actor_combat.c's hand-written NODUP `cross_product2d` copy and its `#define`/`#undef` rename, so
  January's own `_cross_product2d` section is emitted by the genuine real_math.h `__inline`.
- It spells the one /Od-attested call site in `_actor_aim_grenade` as
  `cross_product2d((real_vector2d const *)&actor->input.facing_vector, &aim_vector2d) > 0.0f`.

The edit is byte-inert on all 88 actor_combat sections. The only change is that the `_cross_product2d` COMDAT
selection moves from NODUPLICATES to ANY.

**If yes,** patches 02 and 03 are also applied, and path_obstacle_avoidance, path_smoothing, real_math,
collision_features and path_structure_bsp become Matching (Halo 379 -> 384). Patch 03b is optional; it also
retires the decals `_cross_product2d` rejection.

**If no,** nothing changes. The reopen criterion stays: `_actor_aim_grenade` goes strict exact with the /Od
helper calls.

## Which house-rule clauses this touches

| Rule | Status under P1 |
|---|---|
| Rule 6: never hand-copy a header helper | P1 **removes** a standing violation. The NODUP copy is the same class as the 17 hand copies removed by the Layer 1/2 repairs (931ed8dc and earlier). |
| Rule 6: a header `__inline` may emit a COMDAT only if the copy is identical to January's selected provider | holds. The copy is `section_infos_equal` to January's actor_combat section. actor_combat **is** January's selected provider, so the section is January-owned and not candidate-only surplus. |
| ... the full sweep has zero regressions | 1 TU changes. It has 34/34 identical gate rows, 88/88 keyed sections identical apart from the selection, and an unchanged park fingerprint. The integrator's full sweep is still pending. |
| ... provider links pass in BOTH orders | holds. All 10 other board definers PASS against P1; all 10 FAIL against production. |
| ... the caller is strictly exact (wave-3 law (i)) | **fails**. `_actor_aim_grenade` is 576 bytes against 544. This is the ruling. |
| View cast: per-site first-party /Od attestation | holds. See evidence item 1 below. |
| View cast: compatible prefix layout | holds. `real_vector3d {i,j,k}` has `real_vector2d {i,j}` as its prefix, and the helper reads only i and j. |
| View cast: byte-inert strip test | holds. Removing the cast leaves 0 keyed differences against P1; the only effect is one extra C4133 warning. |
| View cast: strictly exact caller | **fails**. It is the same caller as above. |
| View cast: ledger disclosure | provided below, as it would land. |

- **Same as damage patch A and the hs `real_random_range` cleanup** (both rejected on law (i)): P1 also emits a
  header-inline COMDAT from a non-exact caller.
- **Different from them:** those patches added a NEW surplus COMDAT to an object whose January split lacks it.
  P1 adds no surplus. The `_cross_product2d` section already exists, already matches exactly, and is owned by
  actor_combat in January's split. Only its spelling and COMDAT selection change.

## First-party evidence FOR

1. **/Od build (halo_cache_symbols.exe), fn 0x44ebb0 = `_actor_aim_grenade`.** Its RTC names are `aim_vector`,
   `new_desired_target`, `horizontal_aim_vector` and `new_aim_vector`.
   - At 0x44ed38-0x44ed45 it pushes `&horizontal_aim_vector` ([ebp-0x44], an 8-byte RTC aggregate), then pushes
     `actor+0x140` (`input.facing_vector`, a real_vector3d), then calls fn 0x455250 (`cross_product2d`,
     through the 0x408512 thunk).
   - So the source is a real call `cross_product2d(&facing_vector, &horizontal_aim_vector)`, with the 3D facing
     vector passed as the 2D operand.
   - It is the only `cross_product2d` caller in the actor_combat range (0x44c000-0x453000).
   - The same function also calls `dot_product2d(&facing, &h)` at 0x44ed01. That shows the helper style of the
     whole block.
2. **January bytes** (`build/split/source/ai/actor_combat.obj`, `_actor_aim_grenade`).
   - At 0xe4, `lea ecx,[edi+0x174]` binds `&facing_vector` once. Both the 2D dot product (0xea/0xf0) and the 2D
     cross product (0x10a/0x114) read through that one register. This is the pointer-parameter signature of
     inlined helpers taking `&facing`, not two independent field expansions.
   - `h.j*f.i - h.i*f.j` (0x105-0x120) is the only 2D cross-product pattern in the whole January object. It is
     exactly `cross_product2d(&facing, &h)` inlined.
3. **January ownership.** January's linker selected actor_combat's `_cross_product2d` (the split defines it),
   and every other January object inlined it.
   - The genuine header inline emits a byte-identical copy of it here.
   - The hand-written copy exists only to reproduce the header body under a NODUP selection that no other header
     inline has.
4. **Prior exact diagnostic.** In the aim-grenade lane (`claude_actor_aim_grenade_20260924.md`), the only body
   that reaches `_actor_aim_grenade` EXACT is G1 (544 bytes, 0 slot or register differences).
   - G1 is built on the /Od statement list with the helper calls. Per that ledger's owner packet, G1's
     `_cross_product2d` "comes from the header" and is `section_infos_equal` to January's copy, and its pointer
     casts are per-site /Od-attested and byte-inert.
   - Its residual blocker is a declaration-scope and invented-goto question about `horizontal_aim_vector`. That
     question is unrelated to cross_product2d.
5. **Landed precedent.** The same cast expression `(real_vector2d const *)&actor->input.facing_vector` is
   already in Matching objects:
   - action_obey.c:2593, as a `dot_product2d` argument, and action_obey.c:1259, as a view copy;
   - action_vehicle.c:395.

   It also appears in actions, actors and action_charge (NonMatching). Those Matching callers are exact; this
   one is not.
6. **Status quo cost.** 10 objects on the board carry an LNK2005 on `_cross_product2d` against production
   actor_combat. Five of them are otherwise complete by every audit in MEASURED.md.

## First-party evidence AGAINST / risks

1. **The caller is not strictly exact.** Byte exactness cannot discriminate "call" from "hand expansion" at this
   site: both give identical bytes in the current 576-byte body. So the spelling rests on /Od attestation plus
   the January `lea ecx` binding, not on an exact caller.
2. **The /Od build is a later revision.** It attests the later source. January's scope for
   `horizontal_aim_vector` demonstrably differed from it (the frame is 0x24 against 0x28). That scope difference
   does not touch this statement, but the /Od source is not January's source.
3. **Only the cross product is restored.** P1 leaves the adjacent `dot_product2d(&facing, &h)` site
   hand-expanded, although the same /Od function calls it. Restoring it too is outside this packet, because it
   would newly emit a `_dot_product2d` COMDAT (action_charge-owned) from the non-exact caller.
4. **Precedent risk.** Admitting P1 sets a precedent: a header COMDAT spelling change is allowed in a non-exact
   caller when the change is byte-inert. The distinguishing condition is that the emitted section is
   January-owned by the same object, so no new surplus is added. Any future use should be limited to that
   condition.

## Disclosure text as it would land

There is no BUG comment: nothing here reproduces an authentic defect. No source comment is added.

Proposed ledger entry (`docs/object_matching_logs/claude_fifty_objects_20260925.md`, under the batch that lands it):

> **actor_combat P1 (owner ruling YES, <date>).**
>
> - actor_combat.c drops its hand-written NODUP `cross_product2d` copy and the `cross_product2d_inline`
>   rename. The real_math.h `__inline` now emits January's actor_combat-owned `_cross_product2d` section,
>   which is `section_infos_equal` to it. The COMDAT selection changes from NODUPLICATES to ANY, and all 88
>   sections are otherwise byte-identical.
> - `_actor_aim_grenade` spells the /Od-attested call (0x44ed45: push &horizontal_aim_vector, push
>   &input.facing_vector, call cross_product2d) as
>   `cross_product2d((real_vector2d const *)&actor->input.facing_vector, &aim_vector2d)`.
> - **View-cast disclosure:**
>   - the site is per-site /Od-attested;
>   - real_vector3d's i,j prefix is the real_vector2d layout;
>   - the strip test is byte-inert (0 section differences; the cast only suppresses C4133);
>   - January binds &facing_vector once (`lea ecx,[edi+0x174]`) for both inlined 2D products.
> - **Exception disclosure:** the caller `_actor_aim_grenade` is NOT strict exact (576 vs 544, the held
>   declaration-scope residual). The owner admitted the COMDAT and the cast anyway, because the section is
>   January-owned by this object and adds no surplus.
> - **Unlocked:** path_obstacle_avoidance, path_smoothing, real_math, collision_features and
>   path_structure_bsp. Each passes gate --all, object_audit, PDB storage, surplus identity, and pair links in
>   both orders against this actor_combat.
> - **Rejections:** the stale collision_features `_projection_from_vector3d` rejection is retired. Its surplus
>   is identical to January's decals copy and pair-links. [If 03b: the decals `_cross_product2d` rejection is
>   retired, because its NODUP premise is gone; decals stays NonMatching on 2 residuals.]
> - **Still conflicted (not P1):** geometry `_plane2d_from_points` and bipeds `_object_get_type`.

Held-items table update: remove the "actor_combat P1" row, and in its place record the five admissions.
