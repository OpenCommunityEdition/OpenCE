# `first_person_weapons.obj`, Opus5 150K lane wave 3 (2026-09-14)

## Outcome

Nothing landed. `source/interface/first_person_weapons.c` is unchanged, and `git diff --stat` is empty.

| function | target | real file (baseline = final) | status |
| --- | --- | --- | --- |
| `_first_person_weapon_update` | 1536 / 59 / `912d95683f14ff39` | 1520 / 60 / `a112cf32c0e3baf8` | OWNER-BLOCKED, parked |

Final real-file checks:

- Gate: 33 exact, 1 residual, 0 unwritten; every row is identical to the baseline.
- Guard: passes.
- Parkcheck: 1 park, 0 drift.
- Fake scan: 0 leads.
- Owner census: unchanged.

Duplicate prevention read:

- `first_person_weapons_obj_opus5_150k_w2_20260914.md`
- `first_person_weapons_obj_update_cluster_reconciliation_20260912.md`
- `first_person_weapons_obj_update_private_cluster_boundary_20260904.md`
- `first_person_weapons_obj_opus_reconciliation_20260904.md`
- the jonas ledgers
- `scratch/workers/first_person_weapons.md` (u1, u2, t1-t4)
- `config/parked.json`

## Blocker re-evaluation

**(a) Inlining `magnitude3d`: no longer a standalone blocker.**

- The TU-local defines `REAL_MATH_EXTERNAL_MAGNITUDE3D`, `REAL_MATH_EXTERNAL_MAGNITUDE_SQUARED3D` and `REAL_MATH_EXTERNAL_SQUARE_ROOT` are reconstruction defects (laws_w2 A22).
- Removing them emits select-any COMDATs for these helpers. w2 treated those as a LNK2005 collision.
- The current census outweighs that premise. `build/base` already emits select-any `_square_root` in 83 objects, `_magnitude_squared3d` in 91 and `_magnitude3d` in 66.
- January has exactly one NODUPLICATES owner of each, in action_charge and action_alert.
- laws_w2 A4 classes this surplus as accepted and systemic.

**(b) Unguarded NULL dereference: still blocking.**

- In both the moving block and the overcharged-jitter block, January merges `count ? TAG_BLOCK_GET_ELEMENT(..., 0, ...) : NULL` into one pointer value (`xor eax,eax; jmp`, then `cmp dword [eax+0x10],N`).
- It then dereferences that pointer without a null test.
- A form that merges the count instead of the pointer carries an integer, so no source with defined behaviour reproduces these bytes.
- The exact sibling functions in the same TU (`first_person_weapon_set_state`, `first_person_weapon_build_node_matrices`) guard with `if (count)` and `if (element)`.
- Brief section 5 forbids reproducing a NULL-dereference path, so this function cannot become admissibly exact.

No new shapes were spent. Any further work on register ties would sit on the inadmissible w2 `u2` base.

## Proposals

Informational only; no file change is requested. The orchestrator may simplify the park's reopen criterion to the owner ruling on the unguarded dereference alone. The real_math COMDAT clause is now covered by the A4 census.

**Reopen criterion:** an owner ruling that admits the unguarded dereference the target bytes prove. After that, start from w2 `t3.c` (the `real_vector2d turning` aggregate) with the three `REAL_MATH_EXTERNAL` defines removed.
