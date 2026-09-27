# K1 CARDS - source/ai/ai_communication :: _ai_communication_event

Base: worktree HEAD (unit source identical to canonical a8854940). Objects in scratch/campaign/workers/K1/.
Baseline HEAD: event [size 8128!=8064, sha] (frame 0x126c, normalised hunks 97); talk_weight [sha]; 46/2.
r3all (r3 patches 02+03+04, recorded by the r3 lane, reproduced): [size 8112!=8064, sha], 92 hunks.

| card | time | change | prediction | outcome |
|---|---|---|---|---|
| K1-01 | 01:59 | r3all + v10h body (continue + HELD flag + HELD uninit look data + /Od delay locals) | 8064 [sha], frame 0x126c | CONFIRMED: 8064 [sha], frame 0x126c, 74 hunks, code ends 0x1f51 (Jan 0x1f59); other rows identical |
| K1-02 | 02:03 | k01 + HELD uninitialised play_type in the speech block | frame 0x1274, 8096, ~40 hunks | CONFIRMED: frame 0x1274, [size 8096!=8064], 40 hunks; 0xcd9 extra store gone |
| K1-03 | 02:03 | k01 + header normalize2d at the alignment site | ~10 hunks fewer, 8064, new COMDATs | PARTIAL: 74 -> 70 hunks (not ~64); 8064 [sha]; one x87 operand-order hunk left (fmul st(1) vs fmul [mem]) |
| K1-04 | 02:03 | K1-02 + K1-03 | additive within +-2, frame 0x1274 | CONFIRMED: 36 hunks (40-4), frame 0x1274, 8096 |
| K1-05 | 02:17 | DIAGNOSTIC: 1..64 filler externs before the event (never landable) | frame unchanged; some reg/x87 hunks may move | FRAME UNCHANGED; STRONGER THAN PREDICTED: event code byte-identical for every K (only $L label numbers move) -> residual is not declaration-count keyed |
| K1-06 | 02:25 | /Od boolean-local animation impulse form (one 2.0 store + dead "animation_type = NONE") | 0x173c region unchanged; animation_weight deeper; hunks <= 36 | CONFIRMED AND MORE: frame 0x1274 -> 0x126c (= January), hunks 36 -> 27, size 8096 -> 8048 (padded 8064 needs +16 code) |
| K1-07 | 02:30 | /Od caller-side guard "if (selected->protagonist_actor_index != NONE)" around look_secondary_at_unit | 0x1db8-0x1df6 hunks vanish, ~24 | CONFIRMED: 27 -> 24 hunks; region identical to January |
| K1-08 | 02:36 | /Od real vector_from_points2d((real_point2d const *)&speaker_head, ..., &alignment) | 0x1d49 hunk vanishes | FAILED: byte-inert (k08 == k07 modulo $L numbers). Kept as the /Od-attested helper spelling (rule 41), inert |

**Correction (lead message, verified 02:45):** the brief's relocation-identity deltas ("missing ??_C@_00CNPNBAHC@?$AA@
x3, __real@3d088889 x3 ...") are a sig.py normalisation ARTIFACT. tools/relocms.py (symbol:X == defined-noncode:.rdata:X
== .rdata+X) gives 336 == 336 relocations with an IDENTICAL external-target multiset for HEAD, r3all, k01 and k08; the
only differences are the two switch jump tables' internal entries (layout offsets). No card assumed the artifact: the
structural diagnosis was made from the aligned instruction streams (block structure, frame, slot census).

| K1-09 | 02:44 | /Od "continue;" after each of the six "any_requirements_failed = TRUE;" | 7 jcc diffs vanish, padded 8064, <=22 hunks | CONFIRMED (size): [sha] only, padded 8064 == January, code ends 0x1f4d (Jan 0x1f59); 23 hunks; 2 jcc length diffs remain (address consequences) |
| K1-10 | 02:51 | /Od else-body declaration order + /Od initialisers (protagonist_look_priority = 0; near_player uninitialised, = FALSE in the reply branch; delay_time/ai_delay_time declared at else-body level) | frame unchanged; >= 1 x87/register hunk changes | CONFIRMED: frame/slots unchanged; the weight-product fmul [esi+0x10] x87 hunk (0x17b5) is GONE; 23 -> 21 hunks (ablation pending) |
| K1-10 ablation | 02:55 | A: order+initialisers only / B: near_player move only / C: delay hoist only | (post-hoc check) | A == K1-10 bytes (load-bearing); B == k09, C == k09 (byte-inert /Od fidelity) |
| K1-11 | 02:59 | /Od FUNCTION-level declaration order (scopes unchanged) | frame unchanged; 0x8b7/0x91c may change | FAILED (inert): k11 == k10 modulo $L numbers. Not carried forward (large inert reorder) |
| K1-12 | 03:02 | /Od "if (!suppress_output)" guard around the DISABLED error() | 0x8b7 hunk may resolve | FAILED (negative): DISABLED block unchanged; the weight fmul [esi+0x10] x87 order flipped back (21 -> 23). Not carried forward |
| K1-13 | 03:10 | /Od timer-loop store order (speech_disabled, speech_delay, speech_disabled_reason) | 0x838 hunk resolves only | CONFIRMED AND MORE: 21 -> 13 hunks; 0x838 store schedule, 0x8b7 DISABLED registers, 0x91c bit-test registers and the 0x1a1d pad ALL match January. Left: 0xfc add-esp schedule, normalize2d x87 (0x1d49, 0x1d7d) |
| K1-14 | 03:17 | /Od possibility block: no speaker_unit_index/speech_priority/dialogue_type_index/vocalization_type locals (field reads at each use) + /Od speech statement order | normalize x87 may resolve; CSE risk | CONFIRMED: 13 -> 11 hunks; the normalize scale hunk (0x1d7d/0x1d85) is gone; no new hunk. Left: 0xfc add-esp, 0x1d49 magnitude j-first |
| K1-15 | 03:21 | /Od aggregate view copy "alignment = *(real_vector2d const *)&speaker_unit->object.forward" (x2) | 0x1d49 resolves | FAILED (inert): k15 == k14. Not carried forward (cast class, no byte role) |
| K1-16 | 03:28 | /Od subject-groups store order ([0] then [1], as the cause block) | 0x144 region stays or breaks; 0xfc weak | INERT (k16 == k14): the natural /Od order reproduces January's tail-merged stores too -> carried as fidelity (replaces the [1]-first spelling) |
| K1-17 | 03:31 | DIAGNOSTIC: one unused short local at function / possibility-block / non-reply-block scope | may flip 0xfc/0x1d49 | INERT at all three positions (unused locals never enter the symbol pool) |
| K1-18 | 03:40 | code-neutral /Od statements (a) DISABLED suppress guard, (b) broken ? "broken" : "still holds", (c) original_count copy, (d) apc before total +=, (e) packet store order; each alone + all five | may flip 0x1d49/0xfc | NONE fixes 0xfc or 0x1d49. (a) and (b) are HARMFUL (flip the weight / normalize-scale x87 orders away from January: 13 and 15 hunks); (c)(d)(e) byte-inert; full set 15 hunks. Not carried |
| K1-19 | 03:45 | /Od loop-counter scopes (own clearing-loop counter; team/priority/distance in a block around the timer loop) | weak; if inert STOP tie work | INERT (k19 == k16). STOP: residual tie work ends here (rule "Bound blind residual work") |

**Held-lab best (k16):** 8064 [sha] (padded size == January), frame 0x126c and every frame slot == January, relocation
multiset == January; 2 real residual hunks: 0xfc "add esp,0xc" schedule (subject block) and 0x1d49
magnitude_squared2d term order (ours j*j first via fst forwarding; January i*i first) + their byte consequences
(code ends 0x1f55 vs 0x1f59; jump-table offsets). Reopen criterion: a decoded rule for the deferred-pop flush point
after a call, or for the x87 term order of two indirect products through an inlined pointer temp.
| K1-20 | 03:48 | ablate the dead "else animation_type = NONE" arm of K1-06 | byte-inert | CONFIRMED inert (k20 == k16): dropped from all candidates (no dead store) |
| K1-21 | 03:55 | held-form ablation from k20. Leave-one-out: revert f 17, l 15, p 50, n 16, v 11 (inert) hunks. Admissible base adm (f,l,p,n,v reverted) = [size 8096!=8064], frame 0x1270, 72 hunks. Add-one-in: +f 67 (8096), +l 60 (8064), +p 25 (8064), +n 67 (8080), +l+p 22, +f+p 20, all 11 | p carries most; none alone reaches 8064; f+l+p restore 0x126c | PARTLY WRONG: +l alone and +p alone EACH already reach 8064 and frame 0x126c; p carries most of the gain (72 -> 25) as predicted |

## Summary (banked 2026-09-26 02:46:50 -0700)

Candidates (copies/ = full TU sources; objects in scratch/campaign/workers/K1/):
| candidate | file | event | frame | real hunks | other sections |
|---|---|---|---|---|---|
| HEAD | copies/base.c | [size 8128!=8064, sha] | 0x126c | 97 | - |
| ADMISSIBLE (zero credit) | ai_communication_event_admissible.patch (4774a689710205d9), copies/adm2.c | [size 8096!=8064, sha] | 0x1270 | 72 | none changed (keyed_diff, review_patch 0 gains / 0 losses) |
| HELD LAB (owner info only) | ai_communication_event_OWNER_LAB_held.patch (6fe6f855b2eadba0), copies/lab2.c | [sha], padded 8064 == January | 0x126c, every slot == January | 2 (0xfc add-esp schedule; 0x1d49 magnitude term order) + 3-byte shift after 0x1d49 | +4 surplus COMDATs (_normalize2d, _magnitude2d, _magnitude_squared2d, _scale_vector2d) identical to January's action_charge copies; provider link PASS both orders |

Admissible = /Od-attested and January-byte-proven source facts only: r3 02 (incident type ternary), r3 03 (actor-arm
enemy_status[5] polarity, January 0x534-0x5f2 proof), r3 04 (/Od team scope + HCEX [2][8][2] arrays), /Od
continue structure for every rejection (K1-01 part), /Od delay accumulation, K1-06 impulse boolean (one 2.0 store),
K1-07 caller guard, K1-09 requirement continues, K1-10 /Od else-body declaration order/initialisers, K1-13 /Od
timer store order, K1-14 /Od possibility block, K1-16 /Od subject-groups order; the protagonist block keeps
production's TRUE-initialised "valid" (block-scoped).
Held-lab delta (held_forms_delta_adm_to_lab.diff): FALSE-initialised protagonist_invalid flag; uninitialised
recipient_look_data aggregate (+ possibility field look_data); uninitialised play_type; header normalize2d.
Held-form ablation (K1-21): leave-one-out from the lab: flag +6 hunks, look data +4, play_type +39, normalize2d +5;
add-one-in on ADMISSIBLE: flag 67 (8096), look data 60 (8064), play_type 25 (8064), normalize2d 67 (8080).
Battery (battery/): /W3 identical to HEAD (C4146 x1, C4244 x11, no C4013); fake_match_scan 0 leads; object_audit
FAIL(4) exactly as HEAD (talk_weight, event, 2 csplit storage rows); pdb_storage 3 pre-existing disagreements;
relocation multiset 336 == January (only jump-table entries differ by position).

## Owner ruling 2026-09-26 packets (P1/P2/P3), verified at HEAD a7e14ea4 (see packets/VERIFY_a7e14ea4.txt, re-verified packets/VERIFY_4f57f304.txt)
| card | time | change | prediction | outcome |
|---|---|---|---|---|
| K1-22 | 10:05 | P1: actor-arm enemy_status[5] positive target_really_alive (K1-E, APPROVED) | [size 8112], ~94 hunks | BETTER THAN PREDICTED: [size 8096!=8064], frame 0x126c, 96 hunks (prediction of 8112 wrong: the r3 lane's 8112 included 02) |
| K1-23 | 10:12 | P2 on P1: admissible structural repairs minus the dead "protagonist_look_priority = 0" and minus the dead reply-branch "near_player = FALSE" (production's inherited "boolean near_player = FALSE;" kept verbatim) | 8096 / frame 0x1270 unchanged; 72..74 hunks | CONFIRMED: removal of the dead initializer is BYTE-INERT (P2a == adm2 bytes: 8096, 72 hunks); near_player store removal inert (P2 == adm2 bytes). Info: also removing the inherited near_player initializer (P2nb) = 74 hunks, 8096 - not applied (inherited, owner decision) |
| K1-24 | 10:20 | P3 on P2: /Od FALSE-initialised protagonist_invalid set in the six places (K1-A, narrowly admitted) | 8096, frame 0x1270, ~67 hunks, xor bl,bl / mov bl,1 / test bl,bl | CONFIRMED: [size 8096!=8064], frame 0x1270, 67 hunks; flag region 0xf18 xor bl,bl / 0xf63 mov bl,1 / 0xf84 test bl,bl as January |
