# Independent adversarial review: source/game/aim_assist (Lane B)

Reviewer did not write the candidate and re-measured everything from scratch. Nothing outside
`scratch/lane_b/w/aim_assist/review/` was written; no ninja/configure/pytest/git-write command was run.
Worktree `git status` shows NO tracked modification (worker respected the source/header/config boundary).

## Verdict

| function | verdict | exact confirmed |
|---|---|---|
| `_local_player_aim_assist` (368) | **ACCEPT_EXACT** | yes |
| `_player_aim_projectile` (832) | **HOLD_FOR_OWNER** (bytes exact, body credible; cannot land while it depends on a TU-local stand-in prototype) | yes |

**Overall: LAND_PARTIAL.** Land the `local_player_aim_assist` hunks now (+368 B). Hold the stand-in prototype
hunk and the `player_aim_projectile` body until the owner rules on where `pin_normal_to_cone3d` is declared.
A ready, verified partial file is `review/rv_partial_lpaa_only.c` (real source + only the
`local_player_aim_assist` function replaced, CRLF 614/614).

## 1. Re-gate (mine, not the worker's)

- `unitcheck.py` on candidate.c: baseline 14/1/1 -> candidate 16/0/0, gains=2 regressions=0 other=0 (`unitcheck.txt`).
- `gate.py --all` candidate: all 16 rows EXACT; baseline: 14 EXACT + residual 368 [sha] + UNWRITTEN 832 (`gate_cand.txt`, `gate_base.txt`).
  All 14 baseline-exact rows are still EXACT.
- `tinfo.py`/`relocdiff.py` on my own `review/cand.obj`:
  - `_player_aim_projectile`: size 832/832, relocs 42/42, sha equal, 0 differing rows, target multiset identical
    (the only alndiff rows are `symbol:` vs `defined-noncode:` spellings of the SAME literal COMDAT names - content-hashed, identical).
  - `_local_player_aim_assist`: size 368/368, relocs 12/12, sha equal, 0 differing rows; alndiff 156==156, no normalized differences.
- Partial file `rv_partial_lpaa_only.c`: unitcheck 15/0/1, gains=1 regressions=0; newsyms OWNERSHIP PASS (0 new symbols); guard passed; fake scan 0 leads.
  (`_local_player_aim_assist` stays EXACT without `player_aim_projectile` in the TU - measured, not assumed.)

## 2. Guard

`--forbid-emitted-symbol _point_from_line3d`: passed for candidate, passed for the real source, passed for the partial file.

## 3. Ownership

`newsyms.py` on the full candidate exits **OWNERSHIP FAIL**: newly defines `_set_real_point3d`, `_valid_real`,
`_valid_real_normal3d`, `_valid_realcmp` (plus ok `_player_aim_projectile`, `__real@43000000`, and 3 ungated literals).
My own census (`census.py`, `owncmp.py`) confirms the worker's numbers and adds one fact the worker did not state:

- build/base already emits `_set_real_point3d` in 17 objects, `_valid_real` 44, `_valid_real_normal3d` 33, `_valid_realcmp` 34;
  build/split has each exactly once (link-order-first attribution of a folded COMDAT).
- The PRODUCTION baseline `build/base/source/game/aim_assist.obj` ALREADY defines 12 header-inline COMDATs that January's
  split object lacks (`_arccosine _cosine _cross_product3d _dot_product3d _magnitude3d _magnitude_squared3d _normalize3d
  _scale_vector3d _sine _square_root _subtract_vectors3d _vector_from_points3d`). The 4 new ones are the same class.
- January's own `_player_aim_projectile` calls `_valid_real_normal3d` out of line (reloc @+0x38). Its only definition is the
  `real_math.h` inline, so January's original object necessarily carried that COMDAT; no matching body can avoid emitting it.
  `_set_real_point3d` is the consequence of the first-party-attested setter call (see 5); the non-setter spelling does not match (P5).

Judgement: a false positive of the systemic header-inline class, not a `_point_from_line3d`-class violation. It only
arises from `player_aim_projectile`, which is HELD anyway; the recommended partial landing is a clean OWNERSHIP PASS.
The orchestrator/owner should rule on the class together with the prototype question.

## 4. fake_match_scan

candidate.c: 0 leads. real source: 0 leads. partial file: 0 leads. No new findings.

## 5. Diff read (4 hunks, +127/-15) and first-party evidence - INDEPENDENTLY re-derived

I re-disassembled the later Win32 /Od symbol build myself (read-only capstone; `w32_lpaa.txt`, `w32_pap.txt`) and
re-parsed both RTC descriptors. Every structural claim the worker made checks out:

`local_player_aim_assist` (0x589420, RTC 0x589734 = parameters/target/velocity/target_velocity/magnetism_velocity):
- `mov [ebp-8],-1` is the FIRST statement, before `director_get_perspective`; `[ebp-8]=target.object_index` is the last
  statement of the innermost block; single exit `mov eax,[ebp-8]` -> result local declared first + ONE return.
- `lea edx,[ebp-0x30]; push edx` precedes `call player_control_get_zoom_level` -> the zoom level is a nested argument.
- real call `subtract_vectors3d(&target_velocity,&velocity,&[ebp-0xb0])`.
- `[ebp-0xb8]=&target.vector`, `[ebp-0xbc]=&magnetism_velocity` are stored pointer locals and EVERY later access goes
  through them; then `[ebp-0xc0]` = i*i+j*j and `[ebp-0xc4]` = square_root. /Od does not invent pointer locals and does not
  inline (set_real_point3d is a real call in this build), so these are source variables.
- yaw = (p->i*v->j - p->j*v->i)/h2; pitch = (h*v->k - p->k/h*(p->i*v->i + p->j*v->j))/(p->k*p->k + h2) - exactly the candidate.
- /Od frame order = candidate declaration order.

`player_aim_projectile` (0x589db0, RTC 0x58a224 names all 11 aggregates the candidate uses, same sizes):
- assert line immediates are 0x4d/0x4f/0x8c = 77/79/140 - IDENTICAL to January's. The first 140 lines of aim_assist.c did not
  change length between the two builds: strong same-source corroboration for this function.
- statement order matches the candidate one for one, including: result local = NONE first; nested `unit_get_zoom_level`
  (`&parameters` pushed before the call); explicit `vector_from_points3d(..., &camera_to_unit)` + `magnitude3d` in the caller
  (a `distance3d` call would be a single call with no caller-frame vector at /Od) ; `camera_displacement= camera_direction`;
  `normalize3d` result discarded; `scale_vector3d`; then 0x58a05f..0x58a09c builds (z+k, y+j, x+i, &camera_position) and calls
  0x4ac810 whose body is three stores +0/+4/+8 returning the pointer = `set_real_point3d` (header API at real_math.h:1046, 63 call sites in source/).
- `collision_test_vector`'s return is ignored and `collision.point` is read unconditionally in BOTH builds - original behaviour, not a candidate defect.

Timestamps corroborate "evidence before compile": win32_589420.txt 02:59:37 -> try_4.c 03:00:49 (first compile, EXACT);
try_0 02:40 (control residual) -> try_1 02:47 (first setter compile, EXACT). This was not a spelling search.

Per hunk:
1. **File-scope stand-in prototype (lines 69-76)** - OBJECTION. House rule: prototypes live in the genuine owner header; prior
   ledgers record a consumer-local prototype as inadmissible. P6 proves a declaration is REQUIRED (no prototype -> C4013 path ->
   residual [sha]), so it cannot simply be dropped. Must not land; needs the header packet (owner decision).
2. **`player_aim_projectile` body** - coherent 2001-style Bungie C; same `match_assert` + collision-user push/pop idiom as the
   baseline-exact `aim_assist_clear_line_of_sight` (real source lines 251-282); typed `player_get`/`unit_get`; no casts, no raw
   offsets, no volatile/pragma, one return, explicit `return`. Placement follows January address order.
3/4. **`local_player_aim_assist`** - result local + single return, nested zoom call, RTC-attested renames, two pointer locals in an
   inner block, 2020 pitch association. No baseline-exact function is touched. CRLF preserved (720/720, 0 bare LF).

## 6. Strip tests (mine, each derived from the FINAL candidate by exact text replacement; `mkvariants.py`, `strip_results.txt`)

| variant | change | `_local_player_aim_assist` | `_player_aim_projectile` |
|---|---|---|---|
| L1_noptr | direct `target.vector.*` / `magnetism_velocity.*` | residual [sha] | EXACT |
| L7_ponly | only the position pointer | residual [sha] | EXACT |
| L2_tworet | two returns, no result local | residual [sha] | EXACT |
| L3_zoomlocal | named `short zoom_level` | residual [sha] | EXACT |
| L4_declorder | result local declared after perspective | residual [sha] | EXACT |
| L5_noblock | no inner bare block; 4 locals hoisted, assigned later | **EXACT** | EXACT |
| L6_pitchalt | `(sum)/h*p->k` association | **EXACT** | EXACT |
| P1_distance3d | `distance3d()` helper | EXACT | **EXACT** |
| P2_zoomlocal | named zoom local | EXACT | **EXACT** |
| P3_od_declorder | trace-block declarations in the /Od frame order | EXACT | **EXACT** |
| P4a_flat | no sibling blocks at all | EXACT | residual [sha] |
| P4b_noblendblock | `desired_direction` hoisted out of its block | EXACT | residual [sha] |
| P5_pluseq | `camera_position.x+= ...` instead of the setter | EXACT | residual [sha] |
| P6_noproto | stand-in prototype deleted | EXACT | residual [sha] |

Reading:
- The two pointer locals are load-bearing AND only that spelling matches. Under a mechanical strip test that is the fake
  signature; it is NOT a fake here because the variables are directly visible as stored pointer locals with exactly these
  initialisers in a first-party /Od build of the same function (re-verified by me), the evidence preceded the compile, and
  aliasing two long operands with short pointers for a formula is ordinary period C. They are not an inert store added to
  move bytes; they are recovered source. Names are descriptive (originals unrecoverable: RTC lists aggregates only).
- Single return / result-first / nested zoom: each load-bearing, each /Od-attested, each consistent with the single-exit law.
- Sibling blocks in `player_aim_projectile` are load-bearing and evidenced: RTC names `camera_vector` and `desired_direction`
  as SEPARATE variables while January homes them in the same slot (-0x54) -> disjoint scopes.
- Byte-inert free choices (L5, L6, P1, P2, P3): the worker resolved L6/P1/P2 to the /Od-attested form - correct.
  NON-BLOCKING notes for the orchestrator: (a) L5 (no bare block) is an equally exact, slightly plainer spelling of
  `local_player_aim_assist`; the block form keeps declare-and-initialise and matches the idiom the sibling function needs, so
  either is acceptable. (b) P3 orders the trace-block declarations as the /Od frame does (camera_vector, collision,
  camera_to_unit, camera_displacement, unit, distance) - the worker cites that frame as evidence but did not follow it;
  `rv_P3_od_declorder.c` is 16/16 and is the more faithful text when the function eventually lands.

## 7. Semantics vs January on all paths

Both functions are byte-identical to January, so every path is January's. Source-level: no UB introduced; the unconditional
`collision.point` read and the h==0 division are original behaviour present in both first-party builds (the latter unchanged
from the baseline source). The pitch/yaw algebra is equivalent to the baseline's formula.

## 8. Parks

Neither function has a `config/parked.json` entry (grep: none); no park refresh and no symbols.json change is needed.

## Partial-landing instructions (exact)

TAKE: diff hunks 3 and 4 only = the whole `local_player_aim_assist` function as in candidate.c lines 660-722
(equivalently: copy `review/rv_partial_lpaa_only.c` over `source/game/aim_assist.c`; it is the real source with only that
function replaced; verified 15 exact / 0 residual / 1 unwritten, regressions 0, ownership PASS, guard PASS).
DROP for now: hunk 1 (stand-in prototype, candidate lines 69-76) and hunk 2 (`player_aim_projectile`, lines 564-661).
WHEN the owner approves a home for `boolean pin_normal_to_cone3d(real_vector3d const *, real_vector3d const *, real, real, real_vector3d *)`
(real_math.h insertion is KNOWN to flip `_decals_delete_permanent_from_cluster` by declaration count; the focused P9 header
`math/real_math_cones.h` has zero blast radius but is owner-held): land hunk 2 unchanged (optionally with P3's declaration
order), include the approved header, never land hunk 1, and re-verify the WHOLE board per-function, not by aggregate count.
