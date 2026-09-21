# Adversarial review: source/ai/action_obey (candidate.c)

Verdict: `_action_obey_control` HOLD_FOR_OWNER; overall DO_NOT_LAND pending owner ruling.

## Re-gate (from scratch)
- unitcheck: baseline 26/1/0 -> candidate 27/0/0; gains=1 (`_action_obey_control` 992), regressions=0, other=0.
- gate.py --fn `_action_obey_control`: EXACT 992. alndiff: 266/266 insns, only 3 relocation-spelling rows remain.
- Guard `--forbid-emitted-symbol _point_from_line3d`: pass (exit 0). `_random_range` still emitted (January owns it here).
- newsyms: newly DEFINES 0, drops 0. OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate and on real source. No new findings.
- Line endings: CR==LF count in both files (2920 -> 2914), pure CRLF preserved.
- Diff: ONE hunk (-8/+2) in the jump block of `_action_obey_control` only. No baseline-exact function, header, config touched. Not parked.

## The hunk
Removes invented `real_vector2d facing` + if/else copy; single block local `alignment_vector`:
`alignment_vector = *(real_vector2d const *)&actor->input.facing_vector;` then
`if (normalize2d(&alignment_vector) == 0.f) alignment_vector = *global_forward2d;`.
Semantics identical to old body and to January on both paths (Jan 0x30e-0x34b: load +0x174/+0x178 -> [ebp-8],[ebp-4];
normalize2d; ==0 -> load *global_forward2d, else reload local; store 0x448 then 0x444).

## Evidence, independently checked
- /Od fn 0x43c2d0 regenerated with odbuild.py: byte-identical to worker dump; assert "ai\action_obey.c" line 0x72b.
  RTC: exactly two aggregates, both `alignment_vector` (no `facing`). 0x43c7c7: ONE base load, two loads, two stores
  = one aggregate-copy statement; scalar statements reload the base each time (0x43c8ae, 0x43c8bd). `jp` skips an
  overwrite of the SAME slot from 0xa31df4 (global_forward2d); no else arm.
- January: frame 0x8 (one 8-byte local), consistent. Single local removal is doubly attested.

## STRIP TEST (all built under review/, gate --fn)
| variant | spelling | result |
|---|---|---|
| strip_a | `.i = ..i; .j = ..j;` | residual [sha] |
| strip_b | `.j` then `.i` | residual [sha] |
| strip_c | `set_real_vector2d(&alignment_vector, fv.i, fv.j)` | residual [sha] |
| strip_e | bound `real_vector3d const *facing_vector`, member-wise | residual [sha] |
| strip_d | `.n[0] = ..n[0]; .n[1] = ..n[1];` | EXACT (unitcheck 27/0, 0 regressions, ownership pass) |
| try_2 | cast without const | EXACT (const idempotent) |
| try_4 | two locals + cast | residual [sha] |

The natural plain spelling (`.i/.j`) FAILS. strip_d is cast-free and exact, but I do NOT judge it "the plain spelling":
`.n[k] = .n[k]` with constant indices has ZERO precedent in source/ (grep), is semantically identical to `.i/.j`
(so the `.n[]` choice is itself byte-steering), and is contradicted by /Od (two scalar statements would reload the base).

## Why HOLD and not ACCEPT
Brief s5 lists `*(real_vector2d const *)&v3` as rejected on sight. Only decorated spellings match. Showing for owner:
/Od attests one aggregate copy into a single `alignment_vector`; January frame agrees; every `.i/.j` alternative measured
and failing; const idempotent; production precedent of the identical idiom in EXACT functions:
source/ai/actor_looking.c:623,645,646 (commit 51c94f13e, `x2d = *(real_vector2d const *)v3; normalize2d(&x2d)`),
and this file already passes `(real_vector2d const *)&actor->input.facing_vector` (line 2619).

Owner options: (A) admit cast -> land `candidate.c`; (B) cast-free -> land `review/strip_d.c` (exact, verified, but
unprecedented spelling and not /Od-shaped); (C) neither -> keep real source (residual 992).
