# `projectiles.obj`: opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Source TU `source/items/projectiles.c`; target `build/split/source/items/projectiles.obj`.
- **Real file unchanged**: `git diff --stat -- source/items/projectiles.c` is empty. No header, config, build or tool
  file touched; no ninja/configure/pytest/git write.
- Measurement: `tools/campaign/gate.py` (guard `--forbid-emitted-symbol _point_from_line3d` on every gate unless a
  diagnostic says otherwise), `tools.campaign.alndiff`, `tools.campaign.tinfo`, `scratch/parkcheck.py`,
  `tools/fake_match_scan.py`. Scratch helpers: `scratch/workers/w3_projectiles/slots.py` (per-function `[ebp±N]`
  reference census, target vs candidate), `nd.py`/`nds.py` (relocation- and slot-normalised alignment), `try.py`.
- Read: all `projectiles_obj_*` ledgers (incl. `opus5_150k_w2` and `opus5_pdb_ida` do-not-repeat lists),
  `scratch/workers/projectiles.md` (w2 worker), `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D),
  `actor_moving_obj_opus5_100k_20260914` (normalize3d rename law), the 100K handoff rejections, HCEA
  `src/blam/items/projectile_aim_ballistic.c` / `projectile_update.c` / `projectile_accelerate.c` (semantics only).
- `branch_sweep`: 14 blobs, none better than the current 24 exact. C4013 census: no projectiles.c entry.
- Worker report: `scratch/workers/w3_projectiles.md`; candidates in `scratch/workers/w3_projectiles/`.

## 2. Baseline and final (real file)

`== exact 24  residual 6  unwritten 0` at baseline and at the end (row-by-row identical, guard passed).
Parks `_projectile_new`, `_projectile_accelerate`, `_projectile_aim_ballistic`: parkcheck drift 0. Fake scan: 0 leads.

| Row | Target padded / relocs | Outcome |
| --- | --- | --- |
| `_projectile_aim_ballistic` (park) | 880 / 35 | **PARK-REOPEN-PROPOSED: scratch candidate strict EXACT** |
| `_projectile_collision_test_line` | 512 / 10 | OWNER-BLOCKED (`_point_from_line3d` lane rule) |
| `_projectile_collision` | 2928 / 100 | NOT-LANDED (cell-packing / x87 retention tie) |
| `_projectile_update` | 4064 / 179 | NOT-LANDED (frame-cell and register permutation) |
| `_projectile_new` (park) | 432 / 16 | SKIPPED (laws_w2 D; scheduling) |
| `_projectile_accelerate` (park) | 496 / 32 | SKIPPED (laws_w2 D; x87 store forwarding) |

## 3. `_projectile_aim_ballistic` — strict EXACT in scratch

Candidate: `scratch/workers/w3_projectiles/aim_ballistic_reopen_candidate.c`
(= current real file + the three-line normalize3d rename removal + the reopened aim body).

### 3.1 Starting point

w2 left `nr2.c` at 880/35 with 19 differing instructions, after two structural fixes:
- `4.0f * a * c` and `(2.0f * a)` inline, which removes the reciprocal relocation;
- removing the per-TU `#define normalize3d normalize3d_inline` and the local prototype (actor_moving 100K law).

### 3.2 w3 evidence and shapes

| ID | Shape (on nr2) | Differing insns | Result |
| --- | --- | ---: | --- |
| m1 | `v_desired = MIN(v_desired, square_root(v_desired_sq))` (cseries `MIN` = `a>b?b:a`) | 19 | byte-identical |
| m2 | `MIN(base_velocity, ...)` | 896 B | worse |
| v6 | **store order**: `result = FALSE; aim_vector = delta;` (same order as the `!solution` block) | 18 | kept; result store interleave fixed |
| v7 | v6 + `square_root(v_desired_sq) < base_velocity` | 22 | worse |
| v9 | v6 + `v_desired = base_velocity;` hoisted before `if (forced_velocity)` | 4 | slots fixed, copy misplaced |
| v10-v12 | `&&` merge / ternary default | 18 | VC7 canonicalises |
| v13, v18 | `real v_desired = base_velocity;` without the else copy | 40 | worse |
| **v19 / v22** | v6 + `real v_max = base_velocity;`, used by `v_desired = v_max;` and `if (v_max > square_root(v_desired_sq))` | **0** | **strict EXACT** |
| v21 | statement `v_max = base_velocity;` before the forced test | 0 | exact; v22 kept (house rule 17) |

Diagnostics (not candidates):
- diag1/diag2/diag5: base_velocity referenced only in the else arm or the nested compare, or not at all;
- diag6: dead block local;
- diag8: `real v_desired = base_velocity;` plus the else re-store, which is exact but a redundant store (R12), so it is not proposed;
- diag9: one-use alias.

### 3.3 Slot census (after v6)

January reuses base_velocity's incoming home `[ebp+8]` for `b_desired` and `t`. Ours never reuses `[ebp+8]`, so
every later spilled group shifts by one home:
- discriminant/sign/vertical: `[ebp+0xc]` becomes `[ebp+0x20]`;
- horizontal: `[ebp+0x20]` becomes t_max's `[ebp-0x1c]`.

### 3.4 New measured law: PARAMETER-HOME OFFER

VC7 13.00.9254 reuses a parameter's incoming stack home for later spilled temporaries only when the parameter is
referenced on the dominating entry path. The reference may emit no code, for example a copy that is propagated away.
- Homes whose parameters are referenced only inside arms are never reused in either build: `target_ballistic_fraction_min` `[ebp+0x1c]` and `lob` `[ebp+0x24]`.
- The unused `target_velocity_min` `[ebp+0x18]` is never reused either.
- The same holds for base_velocity once its entry reference is removed (diag1/2/5).
- Any entry-path reference restores January's packing (v9, v19, v21, diag6, diag8).

**Detector:** compare the positive `[ebp+N]` rows of a slot census. A January temp packed into a parameter home that
ours never reuses means January's source references that parameter on the entry path.

### 3.5 Authenticity

- January's bytes prove a code-free entry-path reference to `base_velocity` together with a separate else-arm copy.
  The copy is at +0x13e, before the fraction-pointer test.
- `real v_max = base_velocity;` is the non-redundant spelling of that fact. It names the projectile's maximum available
  launch speed, which is used as the default desired speed and as the cap tested against the minimum-ballistic-fraction
  speed. It completes the function's `v_min` / `v_desired` / `t_max` vocabulary.
- It has no dead or duplicated store, and no one-use alias: it has two uses.
- The alternative dead initialiser (diag8) is also exact but is the R12 redundant-store class.

### 3.6 Admission checks (candidate)

- whole-TU gate `== exact 25 residual 5 unwritten 0`; row diff vs baseline is exactly the aim row (residual -> EXACT).
- guard passed.
- parkcheck: new/accelerate OK. aim moves to target sha `fbc7e95f`, which is the reopen.
- tinfo record diff vs `scratch` base object: aim plus one new `_normalize3d` header-inline COMDAT (96 B). All other 29
  functions, including the three non-parked residuals, are byte-identical.
- Owner census vs `build/base`: candidate-only `_normalize3d` (.text) and `__real@3f1a36e2e0000000` (its double
  epsilon literal).
  - January's projectiles.obj references `_normalize3d` undefined.
  - `build/base/source/ai/actor_moving.obj` (accepted 100K precedent) carries exactly this pair.
  - The object already carries 21 header-inline COMDATs of the same systemic class.
- fake scan 0 leads.

## 4. Other rows

- **collision_test_line** (owner-blocked).
  - January's four side points are four `point_from_line3d` inline instances: radius is reloaded per instance and
    `-radius` is CSE'd once. `right_end.z` is never stored.
  - No other real_math.h helper gives per-instance parameter homes, and explicit math CSEs offset*radius.
  - ct1 (A22 detector, `COLLISIONS_EXTERNAL_COLLISION_TEST_LINE` removal) is inert: it only adds the header COMDAT.
- **collision.**
  - January homes `speed_squared` in the cell shared with the spilled `-velocity_noise` argument temp and keeps it on
    the x87 (fst) into the minimum-velocity compare. The attach-edge `fstp st(0)` block then moves the epsilon block.
  - Ours packs it into the speed/magnitude cell.
  - No law applies, and the parameter-home census is identical. No shapes (do-not-repeat).
- **update.** The frame is a permutation of identical-reference cells:
  - new_velocity vs time_remaining;
  - two 8/9-ref pairs;
  - the collision_count pair.
  - The eax/ecx and esi/edi exchanges come on top. The parameter home census is identical. No shapes (U3-U5).
- **new / accelerate.** Scheduling and x87 forwarding only. No A-law or the new law applies. Both bodies are
  byte-identical in the candidate.

## 5. Proposals

1. **Park reopen `_projectile_aim_ballistic`** (880 padded).
   - Candidate path: `scratch/workers/w3_projectiles/aim_ballistic_reopen_candidate.c`.
   - Apply as one commit: the aim body plus deletion of lines 117 `#define normalize3d normalize3d_inline`, 121
     `#undef normalize3d`, and 322-324 `real normalize3d(real_vector3d *v);` (a consumer-local prototype, R8).
   - Unpark `_projectile_aim_ballistic`; the new/accelerate park measurements are unchanged.
   - Census disclosure: `_normalize3d` + `__real@3f1a36e2e0000000` header-inline COMDAT pair (actor_moving precedent).
2. No header or config text is required.

## 6. Reopen criteria

- collision_test_line: owner ruling admitting a `_point_from_line3d` COMDAT in projectiles.obj plus a law for the
  x87-enregistered direction vector (w2 tl1).
- collision: a measured cell-packing order law for a spilled argument temp vs a named real.
- update: a measured VC7 frame-ordering law reproduced in another TU.
- new/accelerate: argument-push hoisting / first-component x87 store forwarding laws (unchanged).
