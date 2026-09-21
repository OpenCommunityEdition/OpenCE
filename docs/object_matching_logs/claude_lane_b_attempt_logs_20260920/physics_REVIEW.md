# Independent adversarial review: source/physics/physics (Lane B), 2026-09-21

Candidate: `scratch/lane_b/w/physics/candidate.c` (488-line diff, +249/-239) over `source/physics/physics.c`.
All numbers below were re-measured by the reviewer; nothing from the worker's report was trusted.
Note: the worker's REPORT.md does not exist on disk (the worker says the harness refused the write).

## 1. Re-gate (from scratch)

`scratch/orch/unitcheck.py` : `gains=0 regressions=0 other=3`.

| function | baseline | candidate |
|---|---|---|
| `_compute_ground_plane` | residual 336 [sha] | residual 336 [sha] (source untouched) |
| `_physics_compute_vehicle_collision` | residual [size 1152!=1168, sha] | residual [sha], 355/355 insns, one 4-byte store 2 insns early |
| `_physics_compute_new` (PARKED) | residual [size 3120!=2944, relocs 51!=49, sha], 975 insns, 170 aln regions | residual [size 2960!=2944, sha], 915 vs 910 insns, 94 aln regions, relocs 49/49 |
| `_physics_update_old` | residual [size 5456!=5168, relocs 118!=115, sha] | residual [size 5216!=5168, sha], 1644 vs 1621 insns, relocs 115/115 |

13/13 baseline-exact functions stay EXACT. NO function is claimed exact and none is: **zero exact credit is at stake**.
Relocation multisets verified with relocdiff (49/49, 115/115, 14/14).

## 2-4. Guard / ownership / fake scan

- `--forbid-emitted-symbol _point_from_line3d`: PASS for the candidate and for the real source.
- `newsyms.py`: newly DEFINES 0, no longer defines 0 - OWNERSHIP PASS.
- `fake_match_scan.py`: 0 leads on candidate, 0 on real source - no new findings.
- `CL /Zs /W3`: no C4013. 4 NEW C4244 (double -> real) at candidate.c 856, 901, 1846, 1895 - all inside the
  macro's `real line_distance = (distance)` where the argument is the double-typed `fabs(...)` chain.
- Line endings: CRLF only in both files (2193 -> 2203 lines), preserved. No header/config/tool edits. `git status` on
  source/ config/ tools/ is clean.

## 5. Diff audit

Hunks outside the target residual functions: exactly one - the file-scope `/* NonMatching ... */` comment above
`physics_update_old`. The old text (0x14A0 / 114 relocs) was stale; the new text (0x1460, 115/115 relocs) is accurate
FOR THE CANDIDATE ONLY (5216 = 0x1460 verified). No baseline-exact function body is touched.

### The blocking finding: 21 new manual expansions of a barred helper

`PHYSICS_POINT_FROM_LINE3D` is, by its own header comment, a scalar hand expansion of `point_from_line3d` written so
the TU does not emit the `_point_from_line3d` COMDAT. It pre-exists in the file with ONE use (exact
`_physics_update_new`, commit 4dd35758, landed before the ruling below). The candidate adds:

- 1 use in `_physics_compute_vehicle_collision` (replacing an existing 3-line hand expansion - same class),
- 9 uses in `_physics_compute_new`,
- 11 uses in `_physics_update_old`,

and in compute_new / update_old each use REPLACES authentic `scale_vector3d` + `add_vectors3d` helper calls.

Rules this collides with:
- WORKER_BRIEF section 5, REJECTED ON SIGHT: "manual expansion of a helper whose real call is available".
- Lane A owner ruling 2026-09-21 (HANDOFF section 10 and rejected_hypotheses line 1132): "Manual expansion of a helper
  whose real call is barred is refused" - applied there to `_actors_spawn_from_unit`, which was kept RESIDUAL even
  though the expansion made it strict EXACT. Here the expansion does not even produce an exact function.

Pre-existence of the macro is a grandfathered instance, not an admission for 21 further sites. The worker concedes the
tension and sets needs_owner_ruling=true on all three functions. January really does inline `point_from_line3d` at
these sites (verified, e.g. cn 0x80d..0x838 `fld st(0); fmul [edi+0x10]; fadd [edi+0x10c]; fstp`), so the authentic
source is the plain call, which I re-measured as FORBIDDEN-EMITTED-SYMBOL (review/vc_real_call.c, section-size 48).
The macro is therefore a surrogate for a barred call = exactly the refused construct. It is also an inferior surrogate:
by the worker's own numbers its 4 block locals per use shift frame slots (update_old frame 0x170 vs 0x16c; compute_new
+16 bytes), whereas the (also inadmissible) static-helper form is far closer. Landing the macro body now means the
bodies get rewritten a second time, and the compute_new park refreshed twice, whichever way the owner rules.

### Non-macro content (credible, evidence-backed; verified against January where checked)

- depth_fraction if/else with `<` and integer store of 1.0f: January 0x4d3..0x4eb - verified.
- water pressure chain `water_density/density*mass*depth_fraction*gravity`: January 0x512..0x520 - verified.
- compute_new antigrav: January RECOMPUTES `antigrav_height + radius` after `_collision_test_vector` (0x8a4/0x8aa), so
  dropping the named `probe_length` and writing the sum twice is byte-evidenced - verified. (update_old keeps
  `probe_length`; inconsistent between the siblings, unverified.)
- ground_effect polarity `height > 0 ? 1 - h/H : 1`: January 0x8cd..0x8e8 - verified.
- six in-place force adds: `mass_points` is memset to zero before the loop in BOTH functions, so
  `add_vectors3d(&force, &normal_force, &force)` reads zeroed memory, not uninitialised - OK.
- NULL test on the powered definition, PIN on a named alignment (removes the macro's double evaluation of the dot
  product), per-branch friction scale, `set_real_vector3d` after the matrix call: natural, no UB, no dummy locals.
- update_old axis snapshot AFTER `object_translate`: January call at 0x104a, loads of [edi],[edi+4],[edi+8] at
  0x104f.. - verified. The baseline initialiser executes BEFORE the call; a load cannot cross a call, so this is a
  genuine sequencing defect in the real source, independent of the macro.
- Weak spots (mild byte-chasing, semantically inert): per-site `add_vectors3d` argument-order flips that differ
  between the two sibling functions (e.g. `velocity_relative_to_ground + projected` in compute_new vs
  `projected + velocity_relative_to_ground` in update_old) and differing lift-factor orders between siblings. Prior
  ledger (150K w1) measured multiply-chain permutation byte-inert and the add order as M8-ordinal territory, so these
  carry no source evidence. Harmless but unproven churn.

## 6. Strip tests (scratch copies under review/)

| variant | result |
|---|---|
| `vc_flag_end.c`: candidate with `collision = TRUE;` moved to the END of the if-block (natural position) | alndiff output byte-identical to the candidate's. The candidate's "directly after the second cross product" position is arbitrary; if this hunk ever lands, land the end-of-block spelling. |
| `vc_hand_vt.c`: macro stripped to `direction.i*(radius - penetration) + point0.x` (same form the exact `_physics_test_vector` uses) | back to `[size 1152!=1168, sha]`. Only the pointer-local do-while macro reaches 1168. |
| `vc_real_call.c`: authentic `point_from_line3d(&point0, &direction, ..., &collision_point)` | FORBIDDEN-EMITTED-SYMBOL `_point_from_line3d` (exit 1). |
| `partial_axis_only.c`: REAL source + only the axis-snapshot move (2 lines) | unitcheck NO CHANGE (13 exact, all four residual tag rows identical), guard PASS, ownership PASS, fake scan 0; update_old 1697 vs 1696 insns, copy now after `_object_translate` as in January. |

So in vehicle_collision the plain spelling does not reach the size and the decorated (macro) one does not reach exact
either; nothing qualifies for the narrow owner admission (no match exists to admit).

## 7-8. Per-function verdicts

- `_compute_ground_plane`: NO_CHANGE. Untouched; park unchanged.
- `_physics_compute_vehicle_collision`: HOLD_FOR_OWNER. Swaps one manual expansion for another (no new class of
  violation, semantics identical) and de-steers the flag, but gains zero credit and is governed by the same
  `point_from_line3d`-in-physics.obj question. Not worth a landing on its own; if admitted later, use the end-of-block
  flag spelling.
- `_physics_compute_new` (PARKED): HOLD_FOR_OWNER. Non-macro corrections are credible, but the body depends on 9 new
  uses of the refused construct. If it ever lands, `config/parked.json` `_physics_compute_new` measurements
  (size 3120 / 51 relocs / sha) go stale and must be refreshed by line surgery (never re-serialise), or ninja PROGRESS
  fails.
- `_physics_update_old`: HOLD_FOR_OWNER for the macro-bearing body (11 new uses). Carve-out: the axis-snapshot
  sequencing fix is accepted at zero credit and is the only thing I recommend landing now.

## Overall: LAND_PARTIAL (narrow) - candidate.c as submitted must NOT land

Land only `scratch/lane_b/w/physics/review/partial_axis_only.c` (= real source + 2-line change in
`physics_update_old`: `real_vector3d rotation_axis;` and `rotation_axis = object->object.angular_velocity;` placed
directly after `object_translate(...)`). Zero credit, no park affected, no symbols/config change. If the orchestrator
prefers no zero-credit landings, DO_NOT_LAND is equally acceptable - nothing is lost.

Owner question to raise (one ruling unblocks all three functions): for physics.obj, may (a) the authentic
`point_from_line3d` call land under an ownership exception like `_random_range`, or (b) the pre-existing
`PHYSICS_POINT_FROM_LINE3D` macro be extended to further sites, or (c) neither? Until then keep candidate.c in scratch
as research; its decoded non-macro findings should be re-spelled in whatever form the owner admits.

Housekeeping: the real file's NonMatching comment above `physics_update_old` is stale (says 0x14A0 / 114 relocs;
measured 5456 = 0x1550 / 118). Not fixed by the partial.
