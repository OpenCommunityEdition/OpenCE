# Independent review: source/game/player_control (Lane B) - 2026-09-21

Candidate: `scratch/lane_b/w/player_control/candidate.c` (sha1 10131e025989, == try_1.c).
Verdict: **LAND** (`_player_control_modify_desired_angles` ACCEPT_EXACT). One disclosure for the orchestrator below.

## Re-gate (from scratch, reviewer-built objects)
- unitcheck: baseline 48/1/0 -> candidate 49/0/0; gains=1 regressions=0 other=0.
- `gate.py --fn _player_control_modify_desired_angles`: EXACT 1584.
- alndiff vs January (review/cand.obj): 503/503 insns; all 32 differing rows are relocation-SPELLING only
  (symbol: vs defined-noncode:), 0 real rows. relocdiff: 83/83, 0 differing rows by target.
- base.obj vs cand.obj, all 133 COFF sections: only `.text` #100 (this function) raw bytes differ (reloc table hash equal)
  plus the `.debug$S` path string. No data section moved.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: 0 newly defined, 0 lost. OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate, 0 on real source -> no new findings.
- Line endings: 1952 lines / 1952 CR in source, candidate, try_2 - preserved. Same byte length (54505).

## Diff (complete: 1 hunk, 3 lines, both inside the non-exact function)
then arm: `magnitude3d(v) * error * 0.08f` -> `error * 0.08f * magnitude3d(v)`
else arm: `magnitude3d(v) * constants->look_autolevel_scale * error` -> `error * constants->look_autolevel_scale * magnitude3d(v)`
No local, cast, paren, temp, statement, header, config or baseline-exact function touched. Nothing to strip (strip test vacuous).

## Admissibility - the real question
History: 2026-09-16 checkpoint held this function as "factor order chosen from a 12-order search (operand permutation
steering) - owner-ruling queue". Park reopen criterion: "new local or source provenance for the else-arm schedule".
New provenance, verified by me (not taken from the worker):
1. /Od first-party fn 0x5de0d0 (assert string + magnitude3d call + `[constants+0x54]`; my dump is byte-identical to the worker's).
   then arm 5de784: `movss xmm0,[error]; mulss [0.08f]` spilled to a temp BEFORE `call magnitude3d(&unit+0x24)`, then mulss.
   else arm 5de7e9: `mov ecx,[constants]; movss xmm0,[error]; mulss [ecx+0x54]` spilled BEFORE the call, then mulss.
   => source order is `error * K * magnitude3d(v)` in BOTH arms (/Od does not reorder; left product must precede the call).
2. Assert line anchors: January 1172/1266/1277, /Od 1251/1345/1356 -> spans 94 and 11 lines IDENTICAL in both builds.
   The function text between the anchors has the same line count in 2002 and the later build: the /Od shape is
   very likely January's own source text (uniform +79 shift from earlier file growth).
3. January bytes, measured by me over all 6 else-arm orders (then arm fixed): EXACT for E*S*M and S*E*M only;
   E*M*S, S*M*E, M*E*S, M*S*E all residual [sha]. January's bytes therefore demand exactly the TREE SHAPE
   `(a*b) * magnitude3d()` (call as the right operand of the outer product) - the same shape /Od shows. E*S vs S*E is
   byte-inert; /Od selects E*S.
This is double attestation (first-party /Od + January bytes), not a search pick; the new spelling is also the more
natural one (both arms parallel), whereas the old spelling mirrored optimised x87 order. Plain commutative product,
identical value on every path; bytes identical to January so semantics equal January by construction.

then-arm edit: byte-inert (try_2.c = else-arm only is also EXACT, unitcheck 1/0/0). It is not steering (moves no bytes)
and is directly /Od-attested; landing only the else arm would leave an asymmetric pair that LOOKS like a one-arm
permutation. I recommend candidate.c; try_2.c is a verified minimal fallback if the orchestrator prefers it.

Not adopted (correctly): /Od shows two `const real`-style locals ([ebp-0x110]=2/pi, [ebp-0x118]=0.08f). January frame
0xb0 already matches without them; adding them would be uncredited decoration.

## Orchestrator actions
- `_player_control_modify_desired_angles` is PARKED (config/parked.json entry, class instruction-scheduling) and its bytes
  change (now exact): remove/refresh that park entry or ninja PROGRESS will fail.
- Disclose in the landing note that this item sat in the owner-ruling queue (0916) and is landed on NEW first-party
  provenance (/Od order + identical assert-anchor spans + 2-of-6 January tree-shape measurement).
- Brief section 3 lists "operand order of `*`" as measured dead: it is LIVE when one factor is an inlined call
  (expression tree shape), as measured here.

Files: review/cand.obj, base.obj, alndiff_cand.txt, alndiff_cand_full.txt, alndiff_base.txt, od_5de0d0_review.txt,
perm_else_{ESM,EMS,SEM,SME,MES,MSE}.c, gate_guard.txt, fake_cand.txt, fake_base.txt.
