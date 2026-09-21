# Independent adversarial review - source/units/bipeds (Lane B)

Reviewer re-gated everything from scratch at worktree HEAD 7583df5d; no worker number was trusted.
Nothing outside `scratch/lane_b/w/bipeds/review/` was written. `git status` shows no tracked file modified.

## 1. Re-gate

- `unitcheck.py source/units/bipeds candidate.c` -> BASELINE `== exact 42 residual 9 unwritten 0 (of 51 listed)`,
  CANDIDATE identical, `NO CHANGE  SUMMARY gains=0 regressions=0 other=0`.
- `gate.py --all` on real source vs candidate: all 51 rows textually identical (review/gate_base.txt vs gate_cand.txt;
  the only diff line is the guard banner).
- Raw COFF comparison base.obj vs cand.obj with review/cmpfn.py (82 code sections): every function is raw-byte identical
  except `_biped_update_moving`. `_biped_try_to_make_footsteps` and `_biped_update` differ ONLY in the compiler-generated
  names of internal jump-table labels (`$L33414`->`$L33413` etc.) - raw bytes identical, harmless.
- ZERO new exact functions. Zero regressions. No credit is claimed and none is due.

## 2. Guard / ownership / scanner / hygiene

- `--forbid-emitted-symbol _point_from_line3d`: passed for real source AND candidate (the function only REFERENCES it, 1/1 like January).
- `newsyms.py`: OWNERSHIP PASS. `+ __real@3f847ae140000000` (January references it), `- __real@3f847ae147ae147b` (January lacks it).
- `fake_match_scan.py`: 0 leads on candidate, 0 on real source -> no new findings.
- Line endings: real 4364 CRLF / 0 lone LF, candidate 4366 CRLF / 0 lone LF, no BOM. Preserved.
- `_biped_update_moving` is NOT in config/parked.json -> no park refresh needed. The eight parked biped rows are raw-byte untouched.

## 3. Complete diff (5 hunks, 6 insertions / 4 deletions)

| # | hunk | where | verdict |
|---|---|---|---|
| H1 | `#define realcmp(a, b) (fabs((a) - (b)) < _real_epsilon)` | FILE SCOPE, line 358 | byte-inert, not required - see strip test; recommend DROP |
| H2 | `if (fabs(dyaw) >= 0.0001f)` -> `if (!realcmp(dyaw, 0.f))` | update_moving | semantic change is REAL and January-proven; spelling: recommend plain |
| H3 | `actor_is_leaping((short)biped->unit.actor_index)` -> no cast | update_moving | ACCEPT (fixes a wrong narrowing of a long datum index; prototype in ai/actors.h is `long`) |
| H4 | swap two independent constant stores (velocity_max first, then angle) | update_moving | ACCEPT (matches January emission order AND struct declaration order; VC7 emitted source order in both builds) |
| H5 | `fabs(crouch_delta) > 0.01` -> `0.01f` | update_moving | ACCEPT (relocation multiset proof) |

January evidence verified by me (review/full_base_moving.txt, full_cand_moving.txt):
- H2: January +0x48e `fabs; fcomp qword [__real@3f1a36e2e0000000]; fnstsw ax; test ah,5; jnp` = skip when `fabs(dyaw) < eps`
  ONLY when ordered-less; NaN falls INTO the block. Baseline emitted `test ah,1; jne` (NaN skips). Candidate emits January's
  exact three instructions. So the new source matches January on ALL paths including the unordered one; the old did not.
- H3: January +0x9c2 `mov eax,[ebx+1A4h]; cmp eax,-1; je; push eax`. Baseline had `movsx ecx,ax; push ecx`. Candidate = January.
- H4: January +0x9da `mov [ebp-88h],3dcccccdh` then `mov [ebp-84h],3f000000h`. Baseline emitted the reverse; candidate = January (mod frame base).
- H5: relocation-target multiset vs January: baseline jan-only `{__real@3f847ae140000000}` / ours-only `{__real@3f847ae147ae147b}`;
  candidate `{}` / `{}` (97/97, symbolically equal). Site +0xa89 `fcomp qword` now targets the same literal.

None of the hunks is volatile/register/pragma/barrier/flag/raw-offset/punning/UB/dummy-local/inert-steering. No header or config edit.
No baseline-exact function touched.

## 4. STRIP TEST (the worker's strip test compared against the wrong alternative)

The worker stripped `realcmp` back to `>=` and concluded the macro is required. The correct undecorated alternative is the plain
negated compare. Measured (objects in review/, compared section-by-section with cmpfn.py against cand.obj):

| variant | dyaw test | file-scope macro | result vs candidate object |
|---|---|---|---|
| stripA.c | `if (!(fabs(dyaw) < _real_epsilon))` | removed | ALL 82 sections byte+reloc IDENTICAL |
| stripB.c | `if (!(fabs(dyaw) < 0.0001f))` | removed | ALL 82 identical |
| stripD.c | `if (!(fabs(dyaw - 0.f) < _real_epsilon))` | removed | ALL 82 identical |
| stripC.c | `if (fabs(dyaw) >= 0.0001f)` (old) | removed | `_biped_update_moving` differs (test ah,1/jne) |

=> The macro buys NO bytes. The real finding is the `!(a < b)` form. Per the strip-test rule the plain spelling should land.
Additional reasons to prefer stripA: (a) it removes the only hunk outside the target function (file-scope `#define`, which would be the
fifth TU-private duplicate of a macro that belongs in a shared header); (b) the same TU already spells this idiom plainly inside the
baseline-EXACT `_biped_snap_facing` (bipeds.c:3156 `if (!(fabs(cosine_of_angle - 1.f)<_real_epsilon))`).
The macro form is NOT a fake (first-party attested: January assert strings `!realcmp(determinant, 0.0f)` in matrix_math and
`realcmp(plane3d_distance_to_point(...` in biped_limp_noodle) - it is simply unnecessary. If the owner prefers the macro idiom the
candidate as written is policy-clean; bytes are identical either way.

stripA.c fully re-verified: unitcheck NO CHANGE (0 regressions), guard passed, OWNERSHIP PASS, fake_match_scan 0 leads, 4364 CRLF / 0 LF,
diff vs real source = the four in-function hunks only (4 insertions / 4 deletions).

## 5. Per-function verdicts

- `_biped_update_moving`: ACCEPT_FUZZY_ZERO_CREDIT. Still residual `[size 4000!=4080, sha]`, frame 0x68c vs January 0x67c (structural,
  open). The new body is strictly more correct than the old (long index no longer truncated; NaN path, literal type and store order
  now January's). Not byte-chasing: every edit is a semantic/typing correction with direct January evidence. Zero credit.
- The other eight residuals: NO_CHANGE (raw bytes identical to baseline; nothing proposed).

## 6. Items for the orchestrator / owner (not part of this landing; worker claims below were NOT re-measured by me)

- try_cd1.c (`_biped_check_discard` EXACT with January's mismatched vararg order) - original-bug item, correctly kept out. Owner ruling.
- try_t1.c (`_biped_update_turning` with `(real_vector2d *)&v3` casts) - punning, correctly kept out. Owner ruling.
- Radius accessor in update_moving: worker reports double-paren is NOT idempotent -> does not meet the narrow admission; correctly not used.
- PRE-EXISTING defect in the REAL source, confirmed by me: bipeds.c:2329-2332 `distance = base.y*n.j + (n.i*base.x + base.z*n.k) - d;`
  inside `_biped_find_nearby_support_surface` is a hand-expanded, hand-reassociated dot product, while that function's park record says
  "The source-authentic project helper is retained rather than expanding and reordering its dot-product terms". The landed source
  contradicts its own park evidence (manual expansion of `plane3d_distance_to_point` + operand steering). Needs house-rule review;
  restoring the helper would change a PARKED function's bytes and require a park refresh.
- `_biped_update_physics`: frame is set through `__chkstk` (0xAFAC January vs 0xAFB4 ours per worker) - triage's "none/none" hides a
  structural 8-byte surplus. Worth fixing the triage tool to read the `mov eax,N; call __chkstk` form.

## 7. Overall

LAND_PARTIAL (zero credit). Land `scratch/lane_b/w/bipeds/review/stripA.c` semantics: take H3, H4, H5 verbatim; replace H2 with
`if (!(fabs(dyaw) < _real_epsilon))`; DROP H1. Object is byte-identical to the worker's candidate. Landing candidate.c verbatim is
also safe (no regression, no policy violation) if the owner prefers the `realcmp` idiom.
