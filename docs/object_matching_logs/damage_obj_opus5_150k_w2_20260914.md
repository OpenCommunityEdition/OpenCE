# `damage.obj` wave w2 park re-investigation (opus5 150K house-clean lane, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/objects/damage.c`. The target is `build/split/source/objects/damage.obj` (January 2002 Xbox debug build, VC7 13.00.9254).
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `12f7375d4`. Worker claim `w2:damage`.
- Ledgers read first:
  - `damage_obj_opus5_100k_waveD_20260914.md`, including its do-not-repeat list;
  - `damage_obj_area_of_effect_cluster_20260904.md`;
  - `damage_obj_object_damage_update_20260904.md`;
  - `damage_obj_small_vitality_leaves_20260904.md`;
  - `damage_obj_object_cause_damage_fuzzy_20260904.md`;
  - `collision_bsp_damage_owner_reconciliation_20260907.md`;
  - the two `config/parked.json` entries.
- Duplicate checks:
  - `git log --all -- source/objects/damage.c` shows 35 commits.
  - `branch_sweep.py` found 22 blobs. None beats the current 32/2/0.
- Evidence used:
  - January COFF bytes and relocations (alndiff and tinfo);
  - HCEA `object_damage_body.c` and `area_of_effect_cause_damage_to_object.c`, for semantics only;
  - exact-tree instruction censuses over `build/split` × `build/base` (scripts in `scratch/workers/damage/`);
  - standalone VC7 probes compiled with damage.obj's own flags (`probe.py`, `p1..p16.c`).
- Inherited from the 100K lane, already exact and not claimed: `_object_cause_damage`.

## 2. Baseline and final gate

`gate.py source/objects/damage --all --forbid-emitted-symbol _point_from_line3d` gave **exact 32 / residual 2 / unwritten 0**, and the guard passed. Both residuals are locked parks:

| Function | Target size / relocs / sha16 | Real-file (parked) size / relocs / sha16 |
| --- | --- | --- |
| `_area_of_effect_cause_damage_to_object` | 1328 / 51 / d3f929d4ce73b3cd | 1312 / 52 / 46952a116c873e6f |
| `_object_damage_body` | 1360 / 43 / 15116626eee783a5 | 1392 / 44 / 9e29dfe9abf1237a |

**The real file was not modified.** `git diff --stat` is empty.
- The final real-file gate is identical to the baseline row by row.
- `parkcheck` reports 2 parks with 0 drift.
- No strict-exact credit is claimed.

## 3. `_area_of_effect_cause_damage_to_object`

Start point: the waveD A1 tail-recursion scoping. The escaping vectors and collision results are scoped to the core-radius branch and the direct branch. This gives relocation parity, 1312/51.

### January facts (0x40a-0x462)

1. The `normalize3d` result (distance) stays on the x87 stack.
2. The radius delta is computed as `fld cutoff; fsub falloff; fcom 0`.
3. The then-arm is `fxch st(1); fsub falloff; fdiv st(1); fsubr 1.0; fxch st(1); fstp st(0)`, followed by the PIN tests.
4. **Layout:** the `delta <= 0` arm (`fstp st(0); jmp 45a`) sits between the `< 0` arm and the `> 1` test.

### Shapes (whole TU; the real file is untouched)

| Shape | Result |
| --- | --- |
| s1: if/else computes the linear scale (`1.f` with no falloff band), then a separate `scale = PIN(scale, 0.f, 1.f);` | 1312/51. **Recovers January's block layout**: the else arm is placed as in January and the extra `jmp` is gone. The only remaining difference is the dead-delta kill: `fstp st(1)` (2 bytes) against January's `fxch st(1); fstp st(0)` (4 bytes). That puts the section across a 16-byte padding boundary. |
| s2: `scale = 1.f` pre-initialised, if-PIN | 1328/52. Distance and delta spill to the frame (worse). |
| s3: distance/radius_delta/scale at function scope | identical to A1 (inert) |
| s4: `?:` then a separate PIN | identical to s1 |
| s5: if/else then explicit `if (scale<0) ... else if (scale>1) ...` | identical to s1 |

### Mechanism study

- **Probes.** More than 40 standalone forms all emit `fstp st(1)`. They covered:
  - operand spellings, including HCEA's `-(falloff - distance)`;
  - a named quotient;
  - a struct-member delta;
  - reusing the distance variable as the result;
  - declaration orders;
  - later uses of delta or distance;
  - computing delta before the call (VC7 does not defer the load).
- **Census.** `fxch st(1); fstp st(0)` occurs in only two exact January functions: `_unit_adjust_projectile_ray` and `_object_compute_autoaim_target`.
- **Reproduction.** Probe `p8.c` reproduces it. The killed value is a scalarised member of an aggregate whose surviving sibling value is consumed by an inline three-use helper (`point_from_line3d`). Replacing that downstream use with a call, a compare or plain stores flips the output back to `fstp st(1)` (`p9.c`).
- **Conclusion.** This is an x87 register-assignment tie: the then-arm result takes the delta slot rather than the distance slot. No natural AOE spelling reaches it.

**Classification:** the structure, layout and relocations now match. The residual is one x87 kill-form tie.

**Reopen criterion:** a measured source mechanism that places a post-division result in the dividend's x87 slot.

## 4. `_object_damage_body`

Start point: the waveD v1 child walk. It uses `(struct unit_datum *)object_get(child_object_index)`, with one lookup per child and `unit.player_index` read from the same pointer (January 0x357-0x38c). This is the idiom of the admitted exact `object_deplete_body` in the same TU. The result is 1360/43, and the first 0x456 bytes are identical.

### Remaining differences

1. **Dying-region loop.** January reloads `edi` (object_index) after the register-ABI `object_destroy_region` call, so `push edi` serves `object_deplete_body`. We reload before the call and again after the deplete: +2 instructions.
2. **Damaged-effect index register:** `eax` (January) vs `ecx` (ours).
3. **Localized-effect `damage_resistance` reload register:** `ecx` vs `edx`.
4. **`*body_damage = damage_amount`:** January uses an integer `mov`; we use `fld`/`fstp`.

### Diagnostics

These were diagnostic experiments, not landing shapes.

- **Integer-copy census.** `intcopyscan.py` finds 29 exact functions that integer-copy a real frame local. One is the sibling `_object_damage_shield` tail (`*shield_damage_reference = shield_damage; *total_damage_reference = total_damage;`).
- **Probes** `p12`-`p15` always gave `fld`/`fstp`. They covered conditional zero definitions, calls, compares, and 0 to 8 extra real locals.
- **Body bisection** (`b1`-`b4`) keeps the x87 copy after dropping any one of: the dying loop, the deathless block, the effect blocks, or the headshot block.
- **Shield mutations** (`sh1`-`sh4`) keep the integer copies after removing the stun block, the overcharge copy, the else-arm zero, or the initial copy.
- No local source mechanism was found.

**Classification:** a global register / x87-region allocation tie in the tail. The structural cause, the extra `unit_get` lookup, is solved.

**Reopen criterion:** a measured VC7 law for integer-versus-x87 copies of a real frame local, or for reload placement of a register-ABI argument after a clobbering call inside a loop.

## 5. Orchestrator proposal: park re-baseline (not a reopen)

Candidate: `scratch/workers/damage/park_rebase_candidate.c`. It is the current real file with only these changes:

- the waveD A1 AOE scoping;
- the waveD v1 body child walk;
- the s1 clamp statement split;
- one indentation fix (`did_damage = TRUE;`).

Measurements of the candidate:

- whole TU exact 32 / residual 2; the EXACT row list is identical to the baseline;
- the point guard passes;
- the owner census is identical to `build/base` (79 symbols);
- fake scan: 0 leads.

| Park | New base size / relocs / sha16 | Aligned diff lines (base → candidate) |
| --- | --- | --- |
| `_area_of_effect_cause_damage_to_object` | 1312 / 51 / 93606db11e3fdb8f | 97 → 20 |
| `_object_damage_body` | 1360 / 43 / 72b75755994a203b | 269 → 38 |

Neither function is strict exact, so no park reopen is proposed. Refreshing the park bodies and measurements is an orchestrator/config decision.

## 6. Do-not-repeat (additions)

**AOE:**
- `scale = 1.f` pre-initialisation;
- function-scope clamp locals;
- `?:` plus a separate PIN;
- an explicit if/else-if clamp after the if/else;
- all 40+ probe spellings listed in `scratch/workers/damage.md`, including the HCEA `-(lo - d)` form, a named quotient, a struct delta, reusing the distance variable, declaration orders, and computing delta before the call.

**Body:**
- block-removal bisections;
- the probe families for the integer-copy tail;
- do not re-grind the edi reload placement without a new law.
