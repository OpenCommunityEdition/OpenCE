# Adversarial review: scratch/lane_b/w/players/candidate.c -> source/game/players.c

Reviewer: independent, Lane B, 2026-09-21. Worktree HEAD 7583df5d. All numbers below were
re-measured by me; none are copied from the worker. Artifacts are in this directory.

## 1. Re-gate (from scratch)

- `unitcheck.py source/game/players candidate.c`:
  BASELINE `== exact 67  residual 3  unwritten 0  (of 70 listed)`,
  CANDIDATE `== exact 68  residual 2  unwritten 0  (of 70 listed)`,
  `SUMMARY gains=1 regressions=0 other=0`. Gain: `_player_examine_nearby_device residual 176 -> EXACT 176`.
- `gate.py --all` row diff baseline vs candidate (gate_base.txt / gate_cand.txt): exactly one row
  changes (`_player_examine_nearby_device`), plus the tally. No other row moves.
- Per-function object diff (cmpobj.py, base.obj vs cand.obj, 82 function sections each): the ONLY
  function whose size or normalized sha changes is `_player_examine_nearby_device`. No symbol
  appears or disappears. `_player_set_action_result` and `_player_teleport_internal` are
  byte-for-byte the production bodies.
- `tinfo.py` January: size=176 relocs=9 sha=f95e7e1cc6a51fcf. `relocdiff.py` on my cand.obj:
  `shape: size 176/176, relocations 9/9, sha equal`, 9/9 rows `=` at identical addresses, types
  and targets, multiset equal. `alndiff.py`: `target 80 instructions, ours 80 instructions /
  no normalized instruction differences`. EXACT is confirmed.

## 2. Guard

`--forbid-emitted-symbol _point_from_line3d --all`: `== emitted-symbol guard passed` on BOTH the real
source and the candidate. Not newly emitted. guard_ok.

## 3. Ownership

`newsyms.py`: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`.

## 4. fake_match_scan

0 review leads on the candidate, 0 on the real source. No new findings.
(The scanner does not detect decoration-only parens, so this is not evidence for the macro.)

## 5. Complete diff

`1 file changed, 9 insertions(+), 1 deletion(-)`. Two hunks:

1. macros section: a 6-line disclosure comment + `#define DEVICE_BOUNDING_SPHERE_RADIUS(device)
   ((device)->object.bounding_sphere_radius)` + blank line.
2. `player_examine_nearby_device`: radius argument `device->object.bounding_sphere_radius` ->
   `DEVICE_BOUNDING_SPHERE_RADIUS(device)`.

Line endings: source 3723 LF / 3723 CR; candidate 3731 / 3731. CRLF preserved, no churn.
No header/config/tool edit. No volatile/register/pragma/flag/offset/punning/UB/dummy local.
No baseline-exact function text is touched (hunk 2 is inside the target function only; hunk 1 is
file scope). `match_assert` carries explicit line literals (28 sites, zero `__LINE__`-based asserts),
so the +8 line shift is byte-inert - confirmed by the per-function object diff.

Semantics: single macro parameter used once, no multiple evaluation, same type and value as the
plain field read. Function is instruction-identical to January on every path (player_get ->
unit_get mask 3 -> device_get mask 0x380 -> camera -> sphere && frontfacing && can_change ->
set_action_result(…, 0xa, device_index, NONE)). No semantic defect introduced or fixed.

## 6. STRIP TEST (my own variants, review/var/, results in strip_results.txt)

| variant | spelling | gate row |
|---|---|---|
| s0_plain | call site reverted to `device->object.bounding_sphere_radius`, macro left defined | residual [size 160!=176, sha] |
| s0b_plain_nodefine | define removed + plain call site | residual [size 160!=176, sha] |
| s1_macro_noouter | `#define …(device) (device)->object.bounding_sphere_radius` | residual [size 160!=176, sha] |
| s2_bareparen | no macro, `(device->object.bounding_sphere_radius)` | EXACT 176 |
| s3_macro_double | `(((device)->object.bounding_sphere_radius))` | EXACT 176 |
| s4_macro_noinner | `(device->object.bounding_sphere_radius)` as macro body | EXACT 176 |
| a1_static_inline_fn | TU-local `static __inline real device_get_bounding_sphere_radius(const *)` | residual [sha]; frame 0x10 != 0xc, `mov eax/mov ecx,eax/push ecx` + dead store `[ebp-4]` - structural fail |
| a2_object_ptr | `struct object_datum *device` view, plain field read | residual [size 160!=176, sha] |
| a3_helper_locals | existing `object_get_bounding_sphere(index,&center,&radius)` + locals | residual [relocs 10!=9, sha] |
| worker dev/local.c | named `real radius` before camera call (re-gated by me) | residual [sha] |
| worker dev/local2.c | named `real radius` after camera call (re-gated by me) | residual [sha] |
| worker dev/cast.c | `(real)` cast (re-gated by me) | residual [size 160!=176, sha] |

**The strip test FAILS.** The plain spelling does not match; only a spelling with an OUTER paren
does. The effect is carried by the outer paren alone, is idempotent rather than graded
(s2 == s3 == s4 == candidate, all EXACT), and no undecorated alternative I or the worker could
construct reaches exact (9 measured, all fail; 4 of them structurally). I could not find a plain
spelling to recommend instead.

By the standing rule this is a decoration-only match = fake, UNLESS the narrow owner admission
applies. I checked that admission at its source
(`C:/halo-worktrees/claude-lane-a-ai-core-20260920/docs/object_matching_logs/claude_lane_a_adjudication_paren_accessor_20260920.md`).
The owner's quoted words are: *"Admit the named TU-private accessor macro with disclosure. It is
semantically meaningful and hygienically parenthesized; keep the equivalent collisions.c
precedent."* That ruling was given on `_ai_test_line_of_fire`. The sentence that says players.c:3037
is "unblocked" is written by the Lane A agent in a "Consequences" section, and the Lane B packet's
"OWNER-UNBLOCKED LEVER" is orchestrator text. Neither is the owner speaking about THIS site.

Showing against the four admission criteria:

1. Conventional accessor parenthesisation - MET (`((device)->object.bounding_sphere_radius)`).
2. Measured idempotent - MET (s2/s3/s4/candidate).
3. Every undecorated alternative measured and failing - MET as far as two independent agents could
   construct them (plain, no-outer macro, const ptr, nested if, cast, two named locals, static
   inline accessor, object_datum view, existing helper).
4. First-party-attested name that carries meaning - WEAK. In Lane A the macro encoded a real
   asymmetry (`width` read as a radius, attested by two different real_math.h parameter names).
   Here `DEVICE_BOUNDING_SPHERE_RADIUS(device)` is a pure upper-case restatement of the field it
   reads; it adds no meaning, and the candidate's own comment says "INFERRED FROM JANUARY'S BYTES,
   not attested". The field name and the `radius` parameter name are attested; the accessor is not.
   A `DEVICE_`-prefixed accessor for a generic `object.` field also has no counterpart in devices.h
   or objects.h.

Two further points the owner should see before ruling:

- This is now the THIRD site on the same callee's same argument that needs the paren
  (collisions.c:1572 bare, Lane A ai.c macro, this one), with bipeds.c:4029 a fourth pending. Three
  of the four read `->object.bounding_sphere_radius`. That pattern is better explained by ONE
  shared original accessor (an objects.h-level macro) than by three unrelated TU-private
  decorations. A per-TU `DEVICE_…` macro in players.c may be the wrong home even if the lever is
  right. That is a header question and outside this worker's remit, but it argues for an owner
  decision rather than a silent third instance.
- The candidate comment cites `ai.c LINE_OF_FIRE_PILL_SPHERE_RADIUS`, which does not exist on the
  Lane B branch (73c05198 is not an ancestor of HEAD 7583df5d). If landed before the Lane A merge,
  the comment is a dangling cross-reference. Cosmetic, fix at landing.

Verdict for `_player_examine_nearby_device`: **HOLD_FOR_OWNER**. Bytes are genuinely exact and the
showing is complete and independently reproduced, but the match exists only with the decoration,
and the owner's admission has not been given for this site or for a semantically empty accessor
name. If the owner extends the admission, the candidate diff can land exactly as written (or as
the bare-paren s2 form, byte-identical, if the owner prefers the collisions.c spelling).

## 7. Semantics vs January on all paths

Instruction-identical (80/80) and relocation-identical (9/9), so every path agrees. Nothing to add.

## 8. Residual functions

- `_player_set_action_result`: untouched, normalized sha identical to production. NO_CHANGE.
- `_player_teleport_internal`: untouched, normalized sha identical to production. NO_CHANGE.
  The worker's tele/t1.c / f1.c variants are NOT in candidate.c and were not reviewed for landing.
  The two production defects the worker reports (duplicated else arm -> 62 vs 61 relocs; tail
  `biped_get` where January pushes mask 3) are consistent with the packet's relocation multiset
  (`+_biped_fix_position`) but I did not independently verify the second one.
- None of the three functions has a `config/parked.json` entry; no park refresh is needed.

## Overall

**DO_NOT_LAND (pending owner).** The candidate's only content is the held hunk pair; with it held
there is nothing else to land, so LAND_PARTIAL is empty. Zero regressions, guard and ownership
pass, no fake-scan leads. On an explicit owner "yes" for this site the diff lands verbatim:
+176 padded bytes, 67 -> 68 exact.
