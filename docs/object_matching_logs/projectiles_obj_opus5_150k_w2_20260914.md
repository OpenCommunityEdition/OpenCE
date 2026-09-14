# `projectiles.obj`: opus5 150K house-clean lane, wave w2 (2026-09-14)

## 1. Scope and provenance

- Source TU: `source/items/projectiles.c`. Target: `build/split/source/items/projectiles.obj` (January split).
- Lane `opus/150k-house-clean-20260914`. **Nothing landed: `git diff --stat -- source/items/projectiles.c` is empty.**
  No header, config, build or tool file was touched; no ninja, configure, pytest or git write was run.
- Measurement: `tools/campaign/gate.py` (guard `--forbid-emitted-symbol _point_from_line3d` on every real-file and
  candidate gate unless stated), `alndiff`, `relocdiff`, `tinfo`, `scratch/parkcheck.py`, `tools/fake_match_scan.py`.
  Two scratch aligners were added: `scratch/workers/projectiles/nd.py` (relocation-annotation-normalised) and
  `nds.py` (also normalises `[ebp±N]` so only structural differences remain).
- Ledgers read: all eight `projectiles_obj_*` logs (the `opus5_pdb_ida_20260914` do-not-repeat list is binding), the
  aim_ballistic section of `lane_reconciliation_batch5_fuzzy_ui_world_20260909`, `actor_moving_obj_opus5_100k_20260914`
  (normalize3d rename law) and the `opus5_100k_consolidated_HANDOFF_20260914` rejections. `branch_sweep`: 14 blobs,
  none better than the current 24 exact.
- Evidence: January COFF bytes and relocations; HCEA `src/blam/items/projectile_aim_ballistic.c` (semantics only);
  January `action_obey.obj` `_cross_product3d` (exact out-of-line copy of the header inline).
- Per-function worker report: `scratch/workers/projectiles.md`; candidates in `scratch/workers/projectiles/`.

## 2. Baseline and final (real file, guard passed both times)

`== exact 24  residual 6  unwritten 0  (of 30 listed)` at baseline and at the end (row-by-row identical).
Parks: `_projectile_new`, `_projectile_accelerate`, `_projectile_aim_ballistic` - `parkcheck` drift 0. Fake scan: 0 leads.

| Residual | Target padded / relocs | Tags | Outcome |
| --- | --- | --- | --- |
| `_projectile_aim_ballistic` (park) | 880 / 35 | relocs 36, sha | NOT-LANDED; scratch 880/35, 19 differing insns |
| `_projectile_collision_test_line` | 512 / 10 | size 496 | OWNER-BLOCKED (`_point_from_line3d` guard) |
| `_projectile_collision` | 2928 / 100 | sha | NOT-LANDED (x87 forwarding decision) |
| `_projectile_update` | 4064 / 179 | sha | NOT-LANDED (register permutation) |
| `_projectile_new` (park) | 432 / 16 | sha | SKIPPED-EXHAUSTED (5 shapes, identical bytes) |
| `_projectile_accelerate` (park) | 496 / 32 | sha | SKIPPED-EXHAUSTED (5 shapes, inert) |

## 3. `_projectile_aim_ballistic` (park, scratch only)

January facts:
- `i*i + j*j` is computed before the gravity/MAX statement and held on the x87 until
  `fld k; fmul k; fadd st(1); fstp c; fstp st(0)`: a named horizontal-distance local precedes `acceleration`.
- 2a is divided twice with plain `fdiv [ebp-0x18]`; the only 1.0 literal is `1/t`. The first assert text is literally
  `4.0f * a * c > 0.0f` and its `fst` value is reused as the discriminant's 4ac. So 4ac and 2a are CSE'd inline
  expressions. The named `two_a` local made VC7 build a reciprocal (extra `__real@3f800000` relocation).
- The frame is 0x34 with `result` spilled to `[ebp-1]`. After the normalize fallback, aim_vector i/j are phi'd in ebx/edi.

Shapes (full TU each; `nd` = differing instructions, relocation annotations normalised):

| ID | Shape | Size / relocs | nd | Result |
| --- | --- | --- | ---: | --- |
| base | real file | 880 / 36 | 100 | frame 0x30, result in bl |
| b1 | c before acceleration | 880 / 36 | 100/102 | worse |
| b2 | `-b / two_a` | 880 / 36 | 100 | inert |
| b3 | horizontal_distance_squared first | 880 / 36 | 102/99 | x87 prefix exact |
| c1 | b3 + inline `4.0f * a * c`, `(2.0f * a)` | **880 / 35** | 97/101 | relocations equal |
| d1, e1, e3 | tail block scope, result-as-flag, scale_vector3d | 880 / 35 | = c1 | inert |
| e2 | reuse `b` for desired b | 880 / 35 | 90/94 | different slots |
| nr1 | **remove per-TU `#define normalize3d normalize3d_inline` + local prototype** (base body) | 880 / 36 | 46/42 | all 24 exact rows kept |
| **nr2** | nr1 + c1 body | **880 / 35** | **19** | frame 0x34, `[ebp-1]` result, ebx/edi aim phi |
| p1-p3, s_a/s_b/s_e/s_f, v_c, pv2, pv6, ne1, ne3 | declaration order, scopes, `&&` merge, compare subject, reuse t | 880 / 35 | 19 | inert |
| v_d, pv1, pv4, nr2e2 | reversed compare, parameter as v_desired, inline discriminant, reuse b | - | 23-103 | worse |

nr1 applies the accepted actor_moving 100K law to this TU. With the plain `real_math.h` inline, all 11 January
`_normalize3d` sites in update and collision stay out of line (7/7 and 4/4). All 24 exact rows stay exact, and one
SELECT_ANY owner, `_normalize3d`, is added. January references that COMDAT undefined; it is the same class accepted
for actor_moving. aim_ballistic's allocator facts then match January. The remaining 19 instructions are stack slots
for three spilled-temp groups:
- January reuses base_velocity's `[ebp+8]` for b_desired/t, `[ebp+0xc]` for discriminant/sign/vertical, and
  `[ebp+0x20]` for horizontal.
- nr2 never reuses `[ebp+8]` and shifts each group one slot.

The nr2 candidate is `scratch/workers/projectiles/nr2.c`: aim sha16 `275feb9ae23677fc`, 0 fake-scan leads, all non-park
rows identical. It is not exact, so there is no park-reopen proposal.

## 4. `_projectile_collision_test_line` (guard-blocked)

- January reloads `collision_radius` for each side point and shares one `fchs; fld st(0)` for -radius. These are the
  `point_from_line3d` inline expansions.
- New in this wave: with `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` removed (scratch `pl0.c`, guard off), update's
  `_point_from_line3d` call at +0xbf7 still stays out of line (179/179 relocations unchanged). January's TU therefore saw
  the inline and necessarily emitted the SELECT_ANY COMDAT itself. The switch only serves the lane guard.
- The first-test vector shares `[ebp-0x1c]` with right_start, and offset shares `[ebp-0x10]` with the second test
  vector. The first-test vector is therefore in a scope disjoint from the side-point block.
- Scratch `tl1.c` (guard off; not admissible in the real file) has a block-scoped first-test vector and 4x
  `point_from_line3d`. Its frame is exactly 0x40, and the side points and both tests are byte-identical. Size is 528
  (43 differing instructions).
  - Ours spills `direction` into offset's slot and computes k, j, i before storing. January keeps direction on the
    x87 and stores offset i/j/k progressively.
  - The return-FALSE block placement also differs.
- Byte-identical to tl1: tl2 (split statements), tl3 and tl4 (direction scope), tl5 (`&offset` as both b and result).
  Worse: tl6/tl7 (frame 0x4c) and tl8 (single-exit, 53).
- Diagnostic only: a TU-local direct-store cross product reaches 512. It is inadmissible, and it is not a header fix,
  because January's exact out-of-line `_cross_product3d` (action_obey.obj) proves the header's temp form.

## 5. `_projectile_collision` and `_projectile_update`

- collision (`nds`: 130 structural instructions):
  - January keeps `speed_squared` on the x87 (`fstp st(3); ...; fst [ebp-0x38]`) and forwards it into the
    minimum-velocity `fcompp`. That needs an attach-edge `fstp st(0)` compensation block, which moves the
    speed<epsilon block after the reflect block. Ours homes the value and reloads, so the block stays inline.
  - `ce` (two `magnitude_squared3d` calls, no local) makes the effect-marker region identical (130 -> 70). It loses
    the hoisted single computation (921 instructions), so it was rejected.
  - cf (function-scope magnitude), cg (operand order) and chh (nested attach test) are inert. nr1 is inert.
- update (`nds`: 68): pure register permutations:
  - eax/ecx for detonation_timer_running vs flags;
  - esi/edi for definition vs the translational_velocity address;
  - one extra reload;
  - set_action immediate vs register;
  - one store-order pair.
  - nr1 and pl0 are inert. No further spellings, per the U3-U5 do-not-repeat list.

## 6. Orchestrator proposals (no worker action taken)

1. **normalize3d rename removal for projectiles.c** (source-only; affects a parked measurement).
   - Change: delete the three lines `#define normalize3d normalize3d_inline`, `#undef normalize3d`, and the local
     `real normalize3d(real_vector3d *v);` prototype.
   - Evidence: actor_moving 100K law. January update/collision call `_normalize3d` 11 times, all still out of line.
     The 24 exact rows are unchanged.
   - Cost: new SELECT_ANY `_normalize3d` owner (January-undefined reference; same class accepted for actor_moving).
     It re-baselines the `_projectile_aim_ballistic` park from 880/36 to 880/35 (sha16 `275feb9ae23677fc` together
     with the c1 body), a strictly closer body.
   - No credit. Apply only together with the park re-measure; otherwise leave it as evidence.
   - Consumers: none; no header changes.
2. **`_point_from_line3d` owner ruling evidence.**
   - projectiles.obj's own January update code references `_point_from_line3d` by REL32 while its inline is visible, so
     a faithful compile of this TU emits the COMDAT.
   - Relaxing the guard for this unit would let collision_test_line use the natural 4x `point_from_line3d` form (tl1).
     That form is still not exact (cross-product x87 residual).

## 7. Reopen criteria

- aim_ballistic: a measured VC7 law for spilled-temp parameter-slot reuse (why January frees `[ebp+8]`), applied to nr2.
- collision_test_line: guard relaxation plus a law for x87 enregistration of a vector consumed by an inlined
  temp-form `cross_product3d` whose result is address-exposed.
- collision: a law for x87 first-use forwarding of a named real into the next conditional block.
- update: a forced-register anchor for the esi/edi definition vs field-address choice.
- new/accelerate: unchanged from the previous run (argument-push hoisting / first-component x87 store forwarding).
