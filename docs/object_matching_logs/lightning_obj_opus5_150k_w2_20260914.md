# `lightning.obj` park re-investigation (Opus5 150K house-clean, wave w2, 2026-09-14)

## Result

No source change. `source/objects/widgets/lightning.c` is byte-identical to the lane base
(`git diff --stat` empty). The unit stays **7 exact / 2 parked residual / 0 unwritten**; no function
became exact, no park is proposed for reopening, and no fuzzy body was landed.

| function | January padded / meaningful / relocs | candidate | disposition |
|---|---|---|---|
| `_lightning_offset_marker_position` | 240 / 237 / 22, sha `2fe97798519acba8` | 240 / 22, sha `18303a5b66698933` | parked; unchanged |
| `_lightning_submit` | 2576 / 2571 / 80, sha `fb46c25559e11027` | 2576 / 80, sha `4b830e4a539a01c4` | parked; unchanged |

Final real-file gate: `== exact 7 residual 2 unwritten 0`, `_point_from_line3d` guard passed,
`scratch/parkcheck.py` 2 parks / 0 drift, owner census vs `build/base` shows no new or dropped owners,
`tools/fake_match_scan.py` 0 leads.

## Prior work read (do-not-repeat respected)

- `lightning_obj_jonas_offset_helper_fixed_point_20260826.md`
- `lightning_obj_jonas_complete_donor_one_shot_20260828.md`
- `lightning_obj_fable_small_families_20260901.md`
- `lightning_obj_credible_fuzzy_completion_20260902.md`
- `lightning_obj_fable_salvage_reconciliation_20260902.md`
- `config/parked.json` entries for both functions; the Opus5 100K handoff (it has no lightning entries).
- `git log --all -- source/objects/widgets/lightning.c`: the last change was `5d3444f79`, which moved the
  lock enum to its owner. `branch_sweep.py` found no better blob.

## New evidence

1. **HCEX.pdb locals.** `DIA2Dump -sym`, SHIP PDB:
   - The helper's only stack local is `offset` (`union real_vector3d`). Its parameters are `position`,
     `matrix` and `random_position_bounds`.
   - Submit's locals are `marker`, `bitmap`, `points[0x1001]`, `centroid`, `bounds` and `up`.
   - The HCEA DB type for `global_z_axis3d` is `const real_point3d *`, so the repository header
     typing is authentic. Only lightning uses the axis globals.
2. **Oracle scan of January `add esp` placement.** Script: `scratch/workers/lightning/oracle_scan.py`,
   run over every split object. It counts the pattern "x87 op, then `add esp`, then store" (the helper's
   January form) against "`add esp`, then x87 op, then store" (our form).
   - Pattern A (January form): 19 sites, 7 of them in exact functions.
   - Pattern B (our form): 7 sites, 5 of them in exact functions.
   - Exact A oracles include:
     - a compound `-=` on a local aggregate (`_unit_preprocess_node_orientations`);
     - a compound `*=` through a pointer (`_effect_random_angular_velocity`);
     - an explicit subtraction (`_weather_particle_system_box_offset_from_point3d`);
     - an inline `scale_vector3d` (`_detail_sound_random_offset`).
   - Exact B oracles include:
     - compound `+=`, `-=` and `*=` through pointers (`_object_offset_interpolation`,
       `_object_compute_autoaim_target`, `_pre_evaluator_panic`, `_code_0019bf70`);
     - an inline `vector_from_points2d` (`_decal_projection_create`).
   - No source-level discriminator separates the two patterns: not the operator kind, compound versus
     explicit assignment, inline helper versus direct code, or destination kind.
3. **Reload/store oracle scan.** It looked for `fstp st(0); mov r,[ebp-x]; fstp [mem]`. No site exists
   outside lightning, so no oracle exists for submit's last transposition.
4. **Precedent for point/vector casts in admitted exact code:**
   - `device_machines.c:341` (`add_vectors3d` on a point);
   - `items.c:582-974`;
   - `hud_draw.c:1127-1143`;
   - `matrix_math.c:902/914`.

## Rejected shapes (scratch only; none landed)

### Helper

- **Tail spellings in a probe TU** (`probe1.c`), all byte-identical and all still placing `add esp`
  before the `fadd`:
  - `+=`;
  - `offset + position`;
  - `position + offset`;
  - `add_vectors3d` with point/vector casts, in both argument orders;
  - an aliasing result pointer;
  - a `(real)` cast.
- **`set_real_point3d` tail:** the same hash as the base.
- **Draws inside `set_real_vector3d` arguments:** the calls are evaluated eagerly (22 differing lines),
  which confirms that named draws are correct.
- **Head variants** (identical or worse):
  - block-scoped draws;
  - reversed component order;
  - bounds-first products;
  - the `real_local_random()` wrapper;
  - `random*2.f`.
- **Declaration context:** 15 variants, all leaving the hash unchanged:
  - typedef size asserts removed;
  - the forward prototype removed;
  - 1 to 32 added extern declarations.

  The residual is not a declaration-count lottery.

### Submit

These shapes were measured on top of v1, which uses `up = *(real_vector3d const *)global_z_axis3d;`.
v1 alone closes the axis-copy register packet and leaves only the reload-versus-`texture.x`-store
transposition.

Vertex-2 store orders (P = point, C = color, TX = texture.x, TY = texture.y):

| shape | vertex-2 order | result |
|---|---|---|
| s1 | P, C, TX, TY | reload lands correctly, but the color store hoists before the point.z `fstp` |
| s2 | TX first | the `texture.x` store hoists to 0x850 |
| s3 | P, C, TY, TX | same hash as s1 |
| s4 | P, TX, TY, C | 4 differing lines |
| s5 | P, TY, TX, C | reload lands correctly, but `texture.y` hoists |

Texture-coordinate locals:

- s6, `u` including `u_offset`: 317 differing lines.
- s7, no `u` local: 176 differing lines.

Measured scheduling behaviour: the first integer store placed between vertex-2 `point.z` and
`texture.x` is hoisted ahead of the deferred `point.z` store. The `vertex_index` reload then lands
before the `texture.x` store. January has that reload position with no hoisted store, and no ordinary
store order reproduces it.

## Orchestrator proposal (policy, no credit by itself)

Re-adjudicate the 2026-09-02 rejection of `up = *(real_vector3d const *)global_z_axis3d;` as
representation punning, in light of:

- the admitted exact point/vector casts listed above;
- the authentic `real_point3d` typing of the global.

This matters only if the remaining reload/store transposition is solved. No header or config change is
proposed.

## Reopen criteria

- **Helper:** a VC7 rule, demonstrated on an exact oracle, that places a deferred `add esp` after a
  same-destination x87 `fadd`, or authoritative January source for the tail.
- **Submit:** a rule, proven on an oracle, that puts a spilled-register reload ahead of a CSE'd x87
  store without hoisting an integer store, together with the cast adjudication above.
- Do not repeat any shape listed here or in the earlier lightning ledgers.
