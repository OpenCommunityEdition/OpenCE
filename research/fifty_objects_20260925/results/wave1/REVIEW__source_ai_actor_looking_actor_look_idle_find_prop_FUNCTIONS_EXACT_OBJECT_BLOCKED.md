# review source/ai/actor_looking :: _actor_look_idle_find_prop (FUNCTIONS_EXACT_OBJECT_BLOCKED)
approve=True

## checks
All checks were re-run independently. Notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_actor_looking\REVIEW.md. The orchestrator's slug contains '::', which Windows paths cannot contain, so the directory is named review_actor_looking.

(1) Patch reproduction: I ran `git apply` on production.patch in a scratch repo with core.autocrlf=true. It applies cleanly and gives a file byte-identical to cand_final.c, with CRLF kept. The patch itself is LF-only, and GNU patch --binary does not apply it.

(2) gate.py --source cand.c --all: 15 exact, 1 residual (_actor_look_update, 4704 vs 4720), 0 unwritten. Production is 14 exact, 2 residual. No FORBIDDEN rows, and _point_from_line3d is absent.

(3) I compared every section of my rebuilt base.obj against cand.obj. Only _actor_look_idle_find_prop changes (b145a6e8 -> d9c69b27), and _cosine and _sine are new. _actor_look_update (0ecf37ee) and decode_direction are unchanged; only their $L label numbers move. The rebuilt base matches build/base.

(4) object_audit: FAIL(2) -> FAIL(1). The only remaining DIFF is _actor_look_update. 80 January symbols, 0 differ. New surplus is +_cosine and +_sine, 16 B each.

(5) objdiff 3.3.1 on a mini project (od_proj/): code 3894 -> 4496, so find_prop goes from 99.145% to 100% (+602). Data is 1700/1700 in both.

(6) Surplus identity: _cosine and _sine are section_infos_equal (IDENTICAL) to January's only definer, split actor_combat.obj, and all 17 pre-existing surplus COMDATs are still IDENTICAL. provider_link --baseline=production: PASS for both. Our actor_combat.obj defines both, so the pair-link is a real test. nodup_census finds 0 NODUP copies.

(7) pdb_storage: 0 disagreements. No parks, admission rejections or semantic_data entries exist for this unit.

(8) /W3 output is identical for base and candidate. The only C4013 is the existing game_time_get one.

(9) /Od re-derivation with odbuild: the TU calls wrapper 0x455220 at exactly the 15 patched cosine sites:
- find_prop 0x45dbd0 x4
- find_random_vector 0x45d670 x2
- update 0x45f5d0 x5, including the stationary site 0x4611d3, whose result is stored to its own local slot
- test_validity 0x462380 x4, identified by its calls to 0x4620b0 and 0x4621c0

Sine wrapper 0x455320 is called x2 in find_random_vector. The chain is 0x455220 -> 0x4551f0 (direct cos(float)) -> 0x4553b0. The direct level has its own callers elsewhere, so the /Od build distinguishes `cosine(x)` from `cos(x)`. Positive control: motion_sensor.c:1118 spells a direct `sin(`, and its /Od twin 0x642330 calls 0x4552f0 directly. HCEX shows the stationary local is held in a register and has no recorded name.

(10) STRIP TEST and plainer spellings: `(real)cos` (production) is residual, and a lab with plain `cos(` (no cast) in find_prop (labA_plaincos.c) is also residual. The candidate adds no decoration: it removes a cast and uses an existing genuine real_math.h helper that 32 .c files already use, and every site is /Od-attested.

(11) c1.c gated: find_prop is EXACT, but only `_cosine` is emitted.

(12) Ledger review: no prior cosine()-helper probe was recorded for this unit. The only earlier non-adoption was rasterizer_xbox_water on 2026-09-14, where the helper made no byte difference and a lane-local freeze was in force. That was before the owner's 2026-09-21 ruling that admits the all-inlined header-inline COMDAT class.

## issues
Nothing blocks approval. Conditions and corrections for the integrator:

(a) The integrator must still run the full ninja build, the stable-verdict sweep (diff the rows per function, not the aggregate count) and the full COMDAT ownership sweep, and see zero regressions, before the new _cosine and _sine emissions are admitted under rule 6. Both new symbols must be named in the ledger.

(b) Apply with `git apply` (worktree core.autocrlf=true). The patch is LF-only, the working file is CRLF, and GNU patch fails.

(c) Answer to the worker's rule-6 question. _cosine does not depend on the non-exact _actor_look_update:
- c1.c emits the identical _cosine without touching update.
- The strictly exact callers are find_prop, which needs the helper to match, plus test_validity and find_random_vector.
- update's bytes do not change.

_sine has one caller, find_random_vector, which is exact under both spellings, so only per-site /Od attestation supports it. That fits the admitted class (the caller is strictly exact). If the owner wants no surplus that the bytes do not force, the fallback is c1.c (emits only _cosine), at the cost of a TU that spells trig two ways. I recommend production.patch.

(d) Mistakes in the worker's report (none change the verdict):
- It says "15 sites in all"; there are 17 call sites (15 cosine + 2 sine).
- It says c1 has "the same surplus"; it does not, because c1 emits only _cosine.
- It says sine chains to 0x4553b0; it chains 0x455320 -> 0x4552f0 -> 0x455430. Cosine goes through the intermediate 0x4551f0.

(e) The object stays blocked by _actor_look_update (4704 vs 4720, register/slot residual; the worker's stop rule fired). Credit to book: +602 meaningful bytes (608 padded) for _actor_look_idle_find_prop only.
