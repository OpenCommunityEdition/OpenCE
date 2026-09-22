# Adversarial review: source/ai/actor_type_flood (Lane B, 2026-09-21)

Candidate: `scratch/lane_b/w/actor_type_flood/candidate.c` (one function rewritten:
`_actor_type_flood_desire_shamble`, PARKED at baseline, 1312 vs January 1296).

## Verdict

`_actor_type_flood_desire_shamble`: byte claim CONFIRMED EXACT, source credible, but
**OWNERSHIP FAIL -> HOLD_FOR_OWNER. Overall DO_NOT_LAND until the owner rules.**

## Re-gate from scratch

| check | result |
|---|---|
| unitcheck | baseline exact 1 -> candidate exact 2; gains=1 regressions=0 other=0 |
| gate --fn | `EXACT 1296 _actor_type_flood_desire_shamble` (January size 1296, 44 relocs, sha 067d3d1a80f86aaa) |
| alndiff vs January | 398/398 instructions; only relocation-SPELLING rows (literal `symbol:` vs `defined-noncode:`), zero real regions |
| `_flood_decide_action` | EXACT 368, untouched (prefix of file byte-identical to production up to the function) |
| guard `_point_from_line3d` | passed |
| newsyms | **`+ _real_random_range .text VIOLATION: January does not define it` -> OWNERSHIP FAIL** |
| fake_match_scan | candidate 0 leads, real source 0 leads -> no new findings |
| line endings | production 374 CRLF / 0 bare LF; candidate 416 CRLF / 0 bare LF; tabs only |
| sections | base vs candidate differ ONLY by the function's .text (1312->1296) and the new 32-byte `_real_random_range` COMDAT; no literal/data movement |

## The blocking finding

Production `actor_type_flood.obj` defines `_actor_type_flood, _vector_from_points3d,
_dot_product3d, _flood_decide_action, _actor_type_flood_desire_shamble`. January's split
object defines only the three real functions. The candidate newly emits the shared-header
`__inline` COMDAT `_real_random_range`, which the production object does not define and
January's split object lacks (January's split owner: `ai/action_alert.obj` only). By the
lane rule this is a violation, so the exact form cannot be accepted by a reviewer.

Showing for the owner (all measured by me):
1. January proves the inline-helper call: per-arm `mov eax,ecx / mov [ebp-0x10],ecx / push eax /
   mov ecx,edx / mov [ebp-0xc],edx / push ecx` (0x4a4-0x4af, mirrored 0x4b8-0x4c3) = dead
   parameter temporaries of an inlined 2-arg helper, then one shared
   `get_global_random_seed_address` + `_real_seed_random_range` tail at 0x4c4.
2. First-party /Od build (fn 0x480be0): two per-arm 2-arg calls 0x4813c9 / 0x4813f5 -> fn 0x42f360.
3. Hand-expanding the helper (review/partial_noemit.c) is NOT exact: `[size 1280!=1296, sha]`;
   the only real regions left are exactly those parameter temporaries. So the helper call is
   byte-required; it is not decoration.
4. Emission cannot be avoided in the .c: a non-static `__inline` that is referenced always
   emits its COMDAT under VC7 (docs/object_matching_logs/point_from_line3d_link_selection_evidence_20260919.md);
   the header inline linkage law forbids `static`; no header edit proposed.
5. Precedent: 11 production base objects already emit `_real_random_range`; in 10 of them
   January's split object lacks it (action_obey, action_search, actors, encounters, players,
   items, projectiles, damage, breakable_surfaces, units). Owner tolerated `_random_range`,
   refused `_point_from_line3d`; `_real_random_range` has no ruling.

## Hunk-by-hunk credibility (complete diff read)

- goto body -> `result`/`crouch_switching` flags, else-if chain, ONE return. /Od: `[ebp-0x11]=0`,
  `[ebp-0x12]=0` at top, every arm `mov [ebp-0x11],K; jmp join`, single `ret`. Plausible 2001 Bungie code.
- six `result = FALSE;` arms: /Od has all six (0x480c9d, d41, d7d, de0, e0e, e1f). STRIP S3 (remove
  them) -> `[size 1312!=1296]`: byte-bearing AND first-party attested. Not steering.
- `value_changed` / `change_value` flags: /Od `[ebp-0x1d]=0` at 0x480e6a, `[ebp-0x1e]=0` at 0x480e87,
  declared exactly where the candidate declares them; they carry real control flow.
- chain-assigned counters: /Od `xor edx,edx; mov [ebp-0x44],dx; mov ax,[ebp-0x44]; mov [ebp-0x40],ax; ...`
  (the /Od signature of `a = b = c = 0`), order backward/middle/forward and crouching/standing agree
  with the increments. STRIP S2 (declare-and-initialise) -> residual `[sha]`: byte-bearing, double attested.
- `prop->distance < 15.0f` (was `<=`): January 0x267 `fcomp 15.0; test ah,5; jp` = `<`. Genuine bug fix.
- `if (--timer == 0)`: /Od temp `[ebp-0x96]` pattern. STRIP S4 (separate decrement) also EXACT -> inert,
  either spelling acceptable; /Od-attested one kept.
- `boolean result = FALSE` initialiser: STRIP S1 (`boolean result;`) also EXACT -> byte-inert; /Od-attested,
  house style, not steering. Every path assigns `result`, so either spelling is defined. Owner may prefer either.
- `adjustment` three-statement form: /Od 0x4812b1-0x481322 (worker already dropped /Od's inert `= 0.0f`).
- `(short)MAX(change_time * 30.0f, 31.0f)`: /Od evaluates `change_time*30` twice (0x481403, 0x48141c) = MAX macro.
- retained production line `real_seed_random(get_global_random_seed_address())` is a PRE-EXISTING manual
  expansion of `real_random()` (the /Od build calls no-arg fn 0x42e1c0). STRIP S5 (`real_random()`) is also
  EXACT but emits a second new COMDAT `_real_random` (6 production objects already emit it). Not a worker
  edit; the owner can rule on both symbols at once.
- No volatile/register/pragma/casts/raw offsets/dummies/UB; no header or config edits; no unrelated churn.

## Fallback built by the reviewer (NOT the default)

`review/partial_noemit.c` = candidate with the two `real_random_range(lo, hi)` calls spelled in
production's existing seed form. unitcheck gains=0 regressions=0 other=1
(`[size 1312!=1296]` -> `[size 1280!=1296]`), newsyms OWNERSHIP PASS, guard passed, fake scan 0,
alndiff 33 real regions -> 2. Zero credit; the parked function's bytes change (park entry must be
refreshed). It keeps the `<` fix and the attested structure, but knowingly hand-spells a helper
January proves was called, so it should land only if the owner REFUSES the emission.

## Files

review/full.diff, od_fn.txt (identical to worker's od dump), aln_cand.txt, aln_partial.txt,
strip_s1..s5 *.c, partial_noemit.c, census.py, secs.py, mkvariants.py.
