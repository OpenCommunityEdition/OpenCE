# RF-J wave 4 (2026-09-26): `_player_effect_update_camera_impulse` (parked; 752 padded / 738 meaningful): EXACT candidate (saved by the lead)

Lane HEAD 262dcfa6; patch-only; no source/config/tools/build/parked.json edits; `_player_effect_get_camera_effect_matrix`
and CEILING untouched; 3 card-driven compiles (CI-C1, CI-S x2) + 2 retail diagnostics + stock-equal traces.

Patch `camera_impulse_realcmp_CI1.patch` (sha256 09e51ba4...; `git apply --check` clean):
`fabs(magnitude_squared3d(&flattened_direction) - 1.0f) < _real_epsilon && fabs(magnitude_squared3d(&facing) - 1.0f) <
_real_epsilon` becomes `realcmp(magnitude_squared3d(&flattened_direction), 1.0f) && realcmp(magnitude_squared3d(&facing),
1.0f)`. `realcmp` is the existing real_math.h macro `(fabs((a) - (b)) < _real_epsilon)`; player_effects.c only.

## Step 1 (stock-equal traces; sorttrace2.py, basechk2.py, leafchk.py, RF-L vnjoin.py)
HEAD differs in three x87 hunks (jitter cross product +0x1c4/+0x1e0 leaf order; left.direction dot +0x29c;
facing.direction dot +0x2b5). Every deciding key depends on six VN temp IDs (ID & 3); all relevant products tie on
rank and size:

| Temp | Role | HEAD ID | ID & 3 |
|---|---|---|---|
| t442 | effect->direction j address | 442 | 2 |
| t444 | effect->direction k address | 444 | 0 |
| t530 | direction parameter k address | 530 | 2 |
| t534 | direction parameter j address | 534 | 2 |
| t613 | *global_up3d j address | 613 | 1 |
| t632 | *global_up3d k address | 632 | 0 |

VN temps take IDs 384..687 from one counter (ID = 384 + ordinal); the six sit at ordinals 58/60, 146/150 (first pass),
229/248 (second pass); inserting n entries before ordinal 58 shifts all six by n. Solving the key inequalities for all
six cross-product products and both dots gives one solution, s = 2 (mod 4) (s = 1 or 3 fixes the dots only; a shift
starting after ordinal 60 cannot work). Written on card CI-T1 before any compile. (The x87-noncircular lane's "any
s != 0" holds for the dots alone.)

## Step 2
- realcmp is Bungie's macro: January .rdata has `!realcmp(determinant, 0.0f)`, `realcmp(actor->input.facing_vector.k,
  0.0f)`, `realcmp(plane3d_distance_to_point(&plane, &rotate_to_position), 0.f)` (a call result, as here, in
  biped_limp_noodle, which gates 6/6 EXACT with the lane's realcmp); house rule 12 prefers existing subsystem macros;
  HEAD hand-writes the expansion.
- /Od 0x57dea0 +0x12b..+0x1bb generates the same code for both spellings (neutral) and shows no other code-neutral
  statement before ordinal 58 (its `game_time` local already measured inert, u1).
- Mechanism (trace CI-T3): realcmp parenthesises its first argument (the call result); each use adds one C1 paren
  record (VN opcode 0x267): exactly two inserted entries (ordinals 34 and 44); every later entry identical and shifted
  +2, giving the derived classes.
- Retail (CI-R1): HEAD in retail config differs from Sept-25-2001 at +0x1e2 and is not found in Oct; the candidate
  MATCHES Sept 0x4691d0 and is found verbatim in Oct-12-2001 2276P at 0x7a190 (corroboration, not proof).

## Checks
gate --all 27/2 (HEAD 26/3), `EXACT 752 _player_effect_update_camera_impulse`; keyed diff vs build/base 67 -> 67,
1 changed (EXACT vs January), 0 added, 0 removed; /W3 13 = 13; fake_match_scan 0 before and after. Strip: each realcmp
use alone (S-1, S-2) stays residual (the cross product stays ours, and s = +1 also flips the outer
`dot * angle * intensity` factor order at +0x2ac/+0x2ca): both uses load-bearing. Failed predictions: CI-S (expected
only cross-product hunks left); the CI-T3 card's parenthetical class guess was garbled (observed classes match CI-T1).

## Owner question (worker)
realcmp is first-party but its use at this site is not directly attested (January also hand-writes
`fabs(cos_theta*cos_theta + sin_theta*sin_theta - 1.0f)<_real_epsilon` in one assert). Support: first-party existence
incl. a call-result use in an exact function; rule 12; the pre-compile key prediction; the Sept/Oct retail match HEAD
fails. Disclosure: the byte effect runs through C2's VN ordinal (mod 4). The function is parked
(instruction-scheduling); the lead retires the park after fresh verification (rule 63).

Files: the patch; cards CI-T1, CI-T2, CI-T3, CI-C1, CI-S, CI-R1; logs CI1_gate_all, CI_head_gate_all,
CI1_keyed_vs_build_base, W3_head, W3_ci1, CI1_fake_scan(_head); NOTES.md; scratch pe4_c1.c, pe4_s1.c, pe4_s2.c, tools/.

## Lead verification (2026-09-26)
January split objects contain `realcmp(actor->input.facing_vector.k, 0.0f)` (actors), `!realcmp(determinant, 0.0f)`
(matrix_math), `realcmp(plane3d_distance_to_point(&plane, &rotate_to_position), 0.f)` (biped_limp_noodle) and
`assert_valid_realcmp(%f, %f)` (actor_moving). Patch applied in the lane: gate 27/2, `EXACT 752`; keyed diff vs
build/base 1 changed (EXACT vs January), 0 added, 0 removed; diff --check clean. Park entry retired in
config/parked.json (narrow 19-line removal); its reopen text asks for "a natural same-compiler helper
definition/context that explains the operand schedule without collateral regressions".
