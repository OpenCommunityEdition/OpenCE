# Adversarial review: ai/actor_looking::_actor_look_idle_find_prop (FUNCTIONS_EXACT_OBJECT_BLOCKED)
Reviewer dir: scratch/w/review_actor_looking/ (the orchestrator slug contains '::', illegal on NTFS).

## R0 patch reproduction
- production.patch is LF-only; source is CRLF in worktree (index LF, core.autocrlf=true).
- Simulated `git apply` in a scratch repo with core.autocrlf=true: applies cleanly and yields a file
  byte-identical to scratch/w/actor_looking/cand_final.c (CRLF preserved). GNU `patch --binary` does NOT
  apply it (LF patch vs CRLF file) - integrator must use git apply. cand.c here = cand_final.c.

## R1 independent gate (cand.c = cand_final.c, rebuilt here)
- gate --all: 15 exact / 1 residual (_actor_look_update 4704!=4720) / 0 unwritten (gate_cand.txt).
- production (gate_base.txt): 14 exact / 2 residual. Delta = _actor_look_idle_find_prop residual -> EXACT. No function lost.
- object_audit (audit_cand.txt vs audit_base.txt): only differences = find_prop DIFF->ok, +_cosine (16), +_sine (16)
  surplus; FAIL(2)->FAIL(1) (remaining DIFF = _actor_look_update); 80 January symbols, 0 differ.
- Section-by-section base.obj vs cand.obj (my rebuilds): ONLY _actor_look_idle_find_prop changes (b145a6e8 -> d9c69b27),
  plus new _cosine/_sine; _actor_look_update (4704, 0ecf37ee) and every other section unchanged (internal $L label
  renumbering only). Rebuilt base == build/base for every code/data section.
- objdiff 3.3.1 mini-project (od_proj/): cand 4496/10834 code, 1700/1700 data; base 3894/10834, 1700/1700.
  find_prop 99.145% -> 100% (+602 credited). decode_direction 96.93% pre-existing jump-table under-credit, unchanged.

## R2 surplus
- New surplus vs production: _cosine (16 B), _sine (16 B). January single definer of each = build/split actor_combat.obj;
  both section_infos_equal IDENTICAL (surplus_cand.txt). All 17 pre-existing surplus COMDATs remain IDENTICAL.
- provider_link --baseline=production: _cosine PASS, _sine PASS (provider_link_cand.txt). Our actor_combat.obj
  emits both (31 base definers each), so the pair-link is a real test. nodup_census: 0 NODUP hand copies.
- Minimal alternative c1.c (gated here -> c1.obj): find_prop EXACT, 15/1/0, but it emits ONLY `_cosine` (no `_sine`).
  The worker's "same surplus" claim for c1 is WRONG (harmless: it concerns the fallback, not the proposed patch).

## R3 /Od attestation (independently re-derived with odbuild.py)
- actor_looking /Od TU: 0x455220 (float->float wrapper) is called at 15 sites: find_prop 0x45dbd0 x4
  (0x45dc7f/dcad/dcdd/dd0b), find_random_vector 0x45d670 x2, update 0x45f5d0 x5 (0x45f938/966/996/9c4 + stationary
  0x4611d3, result fstp'd to its own local slot [ebp-0xd4] = a named local), 0x462380 x4 (calls get_looking_definition
  0x4620b0 and valid_look_vector 0x4621c0 like find_prop = actor_looking_test_validity). 0x455320 (sine wrapper) x2
  in find_random_vector (cos pushed first = right-to-left args). Matches the 17 patched sites exactly.
- Chain: 0x455220 -> 0x4551f0 -> 0x4553b0 = (float)cos((double)x); 0x455320 -> 0x4552f0 -> 0x455430 (worker said
  sine also chains to 0x4553b0 - imprecise, immaterial). 0x4551f0/0x4552f0 are the DIRECT cos/sin(float) level and
  have their own direct callers elsewhere (6/7), so the /Od build DISCRIMINATES `cosine(x)` from `cos(x)`.
  Positive control: motion_sensor.c:1118 spells direct `sin((real)game_time_get()*...)`; its /Od twin 0x642330
  calls 0x4552f0 directly (game_time_get 0x5c9930 just before), while sine()/cosine() sites call the wrappers.
- HCEX actor_look_update: the stationary cosine local is enregistered (no name) -> `stationary_cosine` is a descriptive
  name, required because C's declarator scope makes `real cosine = cosine(x);` refer to the float. Acceptable (rule 7).

## R4 strip test / fake-matching review
- The change REMOVES a cast and uses a genuine, pre-existing real_math.h `__inline` helper (already used by 32 .c files);
  nothing is added that has no semantic role: no parens, casts, dummies, volatile, pragmas, loops.
- Plain spellings measured: `(real)cos(x)` (production) residual; LAB `cos(x)` without cast in find_prop
  (labA_plaincos.c) residual too. Only the helper reproduces January - but the helper is per-site /Od-attested at all
  4 find_prop sites and the /Od build is shown to distinguish helper vs direct call, so this is source recovery, not
  steering (same standard as the admitted /Od-readout lever). Mechanism (float-typed helper return temp changes
  the x87 value live into the join block) is consistent with the worker's recorded L1/L2/V1/V2/J6/J8/m26 labs.
- Other 13 sites (test_validity, find_random_vector, update) are byte-inert (all sections unchanged) and /Od-attested
  per site; changing them makes the TU spell trig one way like the first-party source. Semantics identical (all args
  are `real` fields/locals; helpers return real).
- House rules: no new functions/params/returns; `real` kept; no prototypes added; no header/config/symbols change;
  CRLF preserved. /W3 output IDENTICAL base vs cand (only C4146, real_math.h C4244s, pre-existing C4013 game_time_get).
- _point_from_line3d guard: no FORBIDDEN rows, symbol absent.

## R5 rule 6 / COMDAT question raised by the worker
- `_cosine`: callers find_prop (EXACT, and byte-REQUIRES the helper), test_validity (EXACT), find_random_vector (EXACT),
  update (non-exact, bytes unchanged by its 5 sites). The COMDAT is not contingent on update (c1 without the update
  sites emits the identical `_cosine`). Owner ruling 2026-09-21 (lane B handoff s3 item 2) admits the all-inlined
  header-inline COMDAT class given genuine header inline + byte-identical COMDAT + strict-exact caller + sweep: met
  except the integrator sweep.
- `_sine`: sole caller find_random_vector, EXACT under both spellings -> admitted only on /Od per-site attestation.
  If the owner/integrator wants zero byte-unforced surplus, fall back to c1.c (only `_cosine`), accepting a TU that
  spells trig two ways. Reviewer prefers production.patch (authentic, consistent with /Od).
- Data: all January .rdata sections ok; objdiff data 1700/1700 unchanged. pdb_storage 0 disagreements; no parks,
  no admission rejections, no semantic_data entries for this unit.

## Verdict: APPROVE production.patch (FUNCTIONS_EXACT_OBJECT_BLOCKED; object still blocked by _actor_look_update)
Integrator conditions: apply with `git apply` (LF patch vs CRLF worktree; GNU patch fails), full ninja, stable-verdict
sweep with per-function row diff (zero regressions), full COMDAT ownership sweep; state BOTH new emissions
(_cosine, _sine) in the ledger. Credit: +602 meaningful bytes (608 padded) for _actor_look_idle_find_prop.
Worker-report inaccuracies (non-blocking): "15 sites" is 17 call sites (15 cosine + 2 sine); c1 does NOT have the same
surplus (only _cosine); sine chain target misstated.
