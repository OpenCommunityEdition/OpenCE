# Independent adversarial review: source/units/vehicles (Lane B)

Reviewer did not write the candidate. Every number below was re-measured; nothing is copied from the worker's report.
Candidate: `scratch/lane_b/w/vehicles/candidate.c` vs `source/units/vehicles.c`.

## 1. Re-gate (from scratch)

- `unitcheck.py`: BASELINE `== exact 35  residual 4  unwritten 0  (of 39 listed)`; CANDIDATE `== exact 36  residual 3  unwritten 0  (of 39 listed)`;
  `GAINS: + _create_ghost_effect residual 656 -> EXACT 656`; `SUMMARY gains=1 regressions=0 other=0`.
- `gate.py --all` on the candidate: 36 EXACT rows; the 35 baseline-exact rows are all still EXACT (row-by-row, not the aggregate).
- `_create_ghost_effect`: tinfo size 656, relocdiff `22 target, 22 candidate, 0 differing row(s)`; alndiff 200/200 instructions, the only
  listed blocks are relocation-spelling (`symbol:` vs `defined-noncode:.rdata:` for __real@/string literals) - same noise every exact function shows.
- Guard `--forbid-emitted-symbol _point_from_line3d`: candidate PASS; real source PASS (no new emission).
- `newsyms.py`: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`.
- `fake_match_scan.py`: candidate 0 leads, real source 0 leads -> no NEW findings.
- `CL /Zs /W3` (review/w3.py): candidate warning set == baseline minus `C4101 'scaled' unreferenced local`. (Baseline has ONE C4013,
  matrix3x3_rotation_to_quaternion - the worker's "two C4013" is slightly off; pre-existing either way.)
- Line endings: 2711 -> 2721 CRLF, 0 bare LF/CR, no trailing whitespace, tabs only. Diff = exactly two hunks (fighter_new body, ghost
  marker count); no hunk touches a baseline-exact function or file-scope text. No header/config/tool edits. No parked/semantic entries
  exist for any vehicles function (config/parked.json, semantic_matches.json checked) -> no park refresh needed.

## 2. _create_ghost_effect - ACCEPT_EXACT (with a disclosure the orchestrator must record)

Change: `long marker_count = call()` -> `short hover_marker_count = call(); short jet_marker_count = 0;` and loop bound
`marker_index<hover_marker_count+jet_marker_count`.

On its face this is the rejected-on-sight shape (a zero-valued local whose only semantic effect is nil), so I tried to kill it.

Strip tests (review/s1,s3,s4.c, all measured):
| spelling | gate |
|---|---|
| `short marker_count`, `marker_index<marker_count` | residual `[size 640!=656, sha]` (16-bit compare) |
| `long marker_count` (baseline) | residual `[sha]` (count store before index zero) |
| `short count; long total = count;` loop on total (the in-file pelican idiom) | residual `[sha]` |
| `short hover`, `marker_index<hover+0` | residual `[size 640!=656, sha]` |
| `short hover; short jet = 0;` `marker_index<hover+jet` (candidate) | EXACT 656 |
Only the zero-local spelling matches. Under a bare strip test that is a fake. It survives ONLY because of first-party attestation,
which I verified independently (review/rv_dbg.py, my own pefile+capstone read of halo_cache_symbols.exe; data only, not executed):
- "hover thrusters" has exactly two code refs: 0x8f55c1 (ghost) and 0x8f5d00 (pelican); "jet thrusters" one: 0x8f5d39 (pelican).
- ghost 0x8f55d1..0x8f5617: `mov word [ebp-0x6d8],ax` (call result) ; `xor edx,edx; mov word [ebp-0x6dc],dx` (SECOND short = 0) ;
  `mov word [ebp-0x6e0],0` (index) ; loop test `movsx idx; movsx [-0x6d8]; movsx [-0x6dc]; add; cmp; jge`.
- pelican 0x8f5d10..0x8f5d86: identical three-slot layout, the second short receives the "jet thrusters" call result, loop test is the same sum.
- NEW corroboration (reviewer probe review/p1.c): rewriting the baseline-exact `_create_pelican_effect` into that debug-build form
  (`short marker_count = call(hover); short jet_marker_count = call(jet, &markers[marker_count], 16-marker_count);`
  `marker_index<marker_count+jet_marker_count`) gates `EXACT 512 _create_pelican_effect` against January. So January's bytes accept the
  later build's source shape for this family, i.e. the ghost loop is the pelican loop with the jet count zeroed - coherent program
  history, not an invented steering variable. January's own byte order (index zero-store BEFORE the 32-bit count store) independently says
  the 32-bit count is a hoisted loop-invariant of the loop condition, not a declared-initialised long.
- Semantics: object_get_marker_by_name returns `short`; old and new iterate the identical range on all paths. No UB, no punning.

Disclosures:
- The NAMES `hover_marker_count`/`jet_marker_count` are inferred (scalars have no RTC record; HCEA PDB lists only stack aggregates:
  `thrusters`, `collision`, `direction`, `vector`, `midpoint`, `fake_marker_names/forwards/points`). Descriptive, house-rule compliant,
  but not attested. HCEA calls the array `thrusters`, so `*_thruster_count` may be closer; not a gate item.
- If the owner's position is that a zero-valued local is inadmissible regardless of first-party attestation, this is the single
  construct to pull; the showing above is complete for that ruling. I judge it admissible (source Bungie demonstrably wrote).
- Optional follow-up, NOT part of this landing: pelican is immutable here, but its first-party form (p1.c hunk) is also EXACT.

## 3. _update_alien_fighter_physics_new - ACCEPT_FUZZY_ZERO_CREDIT

Gate row unchanged: `residual 1088 [relocs 33!=32, sha]`. alndiff blocks 58 (baseline) -> 28 (candidate); after removing
reloc-spelling/branch-immediate noise the candidate differs from January ONLY at (a) the `call _point_from_line3d` site (+0x113..+0x14b,
January inlines; cause is the TU-wide `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`, a pre-existing guard-mandated define the candidate does not
touch) and (b) one commuted `fld/fmul` pair in the cross_product3d k-term (+0x20f). Frame and every other slot now equal January.

Credibility check against first-party evidence I re-derived myself:
- RTC stack-variable descriptor of debug fn 0x8f6a20 (parsed at 0x8f7294, 12 entries): magic_force, magic_torque, desired_velocity,
  acceleration, current_rotation, desired_rotation, rotation, rotation_quaternion, angle, axis, desired_angular_velocity,
  angular_acceleration - every aggregate name in the candidate is attested. HCEA release PDB agrees on 10 of them and shows
  `acceleration` and `rotation_quaternion` sharing frame offset 0xA0 (scope-disjoint), supporting the two bare blocks; the baseline
  body already used bare blocks here, so it is not a new style.
- Debug build confirms: compute_acceleration(..., &acceleration); scale(&acceleration, mass, &magic_force); scale(&magic_force,
  seat_power, &magic_force) IN PLACE (0x8f6b62..0x8f6ba2); moments sum/3 passed directly (no `average` local); torque scaled in place;
  `thrust_delta` slot [ebp-0x168] assigned twice per arm; ONE `thrust += delta` after the join (0x8f717c..0x8f7192); five const
  stores 0.05, 0.2, 0.005, 0.01, 0.05 at 0x8f6ebc..0x8f6f04 (values decoded by me).
- Semantics equal the old body on every path (same calls, same operands; old body's reuse of `desired.left` as quaternion-axis
  scratch and of `torque`/`thrust`/`axis` for unrelated quantities is gone). New body is strictly more credible than the old one.

WORKER CLAIM REFUTED (non-blocking): "the five `real const` locals are needed (without them the magnitude3d sum order differs)".
Reviewer strip test review/f1.c (literals 0.2f/0.01f/0.05f/0.05f/0.005f in place, five consts deleted): the function's aligned diff vs
January is BYTE-IDENTICAL to the candidate's (`diff aln_fighter_cand.txt aln_fighter_f1.txt` empty). Same strip on
candidate_owner_ruling.c (review/f2.c): still `EXACT 1088`. So the consts are byte-inert in both configurations. They are first-party
attested (later build) but carry five invented names and no January byte evidence; also the debug build initialises them AFTER the
torque scale calls (a nested block start / first-use declaration), whereas the candidate declares them at the top of the `if` block.
Either spelling is admissible; by the strip-test rule the plain-literal spelling is the conservative one. Orchestrator's choice -
not a reason to reject.

Pre-existing, untouched by the diff (noted only): manual 2D cross expression for the yaw where the debug build calls cross_product2d
(needs a punning cast + new COMDAT - correctly NOT landed); `(-angle)*k*(1.0f/_pi)` and `*(1.0f/3)` where the later build divides.

## 4. _update_human_plane_physics, _update_alien_scout_physics - NO_CHANGE

The diff contains no hunk in either function; gate rows identical to baseline. Nothing to land, nothing to review for admissibility.
The worker's "measured EXACT" plane/fighter bodies live only in candidate_owner_ruling.c, which I confirm is NOT landable as-is
(it is the guard/ownership-failing configuration by the worker's own account; I re-gated only its fighter_new = EXACT). I did verify the
one factual premise offered for the owner ruling: January's `_vehicle_update` does carry `@0268 REL32 _point_from_line3d`, and
January's fighter_new has zero references to it. No credit claimed, none granted.

## 5. Overall: LAND

Counted gain 1 function / 656 padded bytes, 0 regressions, guard PASS, ownership PASS, no new scan leads, no out-of-scope hunks.
Land candidate.c whole. Optional orchestrator edits before landing (both byte-neutral, measured): replace the five fighter_new
`real const` locals with literals (review/f1.py is the exact replacement spec). Record the ghost first-party attestation
(addresses above + the pelican corroboration) in the ledger next to the landing.
