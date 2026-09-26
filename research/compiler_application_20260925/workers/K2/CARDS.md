# K2 cards and outcomes: `_ai_debug_render_actor` (unit source/ai/ai_debug)

Worker K2, solo, patch-only. Base: worktree HEAD fe283cc5; ai_debug.c is byte-identical to canonical a8854940 (no
commit after a8854940 touches it); `build/base/source/ai/ai_debug.obj` keyed-identical to a fresh gate of HEAD
(`scratch/campaign/workers/K2/head.obj`, keyed_diff: 0 changed / 0 added / 0 removed).

## Baseline (2026-09-26 01:47 -0700)
- gate --all: `residual 24976 _ai_debug_render_actor [size 25008!=24976, sha]`; `== exact 59 residual 1 unwritten 0`.
- Frame 0x810 both sides. Code end J 0x5ffd / O 0x601d (+32). Width-normalised code length J 0x5ff2 / O 0x601d (+43).
- Structural-lane metric (branch/jmptable/slot-agnostic exact-text): J 6662 real insns / O 6679, equal 6472
  (diffJ 190 / diffO 207). sdiff level 2 (reg+slot normalised): 134 regions incl. jump-table pseudo-regions.

## F0 (fact, measured 2026-09-26 01:5x): the brief's relocation-identity lever does not exist
The `sig.txt` row prints `miss {"symbol:__real@3dcccccd": 30, ...}` and `extra {"defined-noncode:.rdata+__real@3dcccccd": 30, ...}`,
truncated to 80 characters. The two sides are the SAME 47 literal destinations with the SAME counts, spelled
`symbol:X` in the csplit target and `defined-noncode:.rdata:X` in our compiled object. Normalising the tuple
(`['defined-noncode','.rdata',X,a]` == `['symbol',X,a]`) gives missing {} / extra {}: 1905 = 1905 relocations,
441 distinct identities, relocation types equal. AGENT_BRIEF.md already states "symbol:X and .rdata+X are the same
destination"; ai_debug_obj.md:2529 and the n2 ledger record the same artifact. So there are NO missing 0.1f
constants and NO missing "" literals; the rule-66 leverage reason given in the K2 prompt is refuted.

## Metric used below
`sdiff` regions = reg- and slot-normalised differing regions of J real code (< 0x6000, jump-table pseudo-regions
included as a constant 12-ish floor), `equal` = structural-lane exact-text metric (of 6662 J real instructions),
width-normalised length delta. Head: 84 regions / equal 6472 / +43.

## C1 crouch-block inline conditionals (card cards/C1_crouch_block_inline_conditionals.txt) — OUTCOME
Compiled 2026-09-26 ~02:00. PREDICTION CONFIRMED: 0x46b and 0x630 flip to January order; block 0x1da..0x2ad unchanged.
Also 0xf62 fixed, but the same cross-product group changed shape: new regions 0xf6d (operand swap), 0xf7d/0xf83/0xf89
(term order of the following magnitude). equal 6472 -> 6473; regions 87 -> 85; size/width unchanged (25,008 / +43).
59 exact rows identical. Interpretation: the site set responds to the named-local/temporary state, as the M8 sweep said.

## C2 blockage MAX (card cards/C2_blockage_MAX.txt) — OUTCOME
Compiled 2026-09-26 02:04 on top of C1. Aiming block unchanged. Downstream: the vision-cone site 0x55e0/0x55e4
(`(sign*sin_h)*X` term order, recorded by the structural lane as "not a spelling effect") becomes January. Nothing
else moved. equal 6474; regions 83; size/width unchanged. 59 exact rows identical.

## C3..C12 cumulative (cards C3..C12) — OUTCOMES (compiled 2026-09-26 02:05..02:25)
All variants: 25,008 B, width +43, SAME 59 EXACT rows. Region lists vs the previous variant:
- C3 (encounter colour ?:): object code identical to C2 except jump-table label numbers: fully inert.
- C4 (+unreachable colour inline): 0x4d51/0x4d7a push-order pair and 0x55ce/0x56de move (not to January).
- C5 (+unr #1 inline): 0xf6d reverts to 0xf62 (swap only).   - C6 (+unr #2): inert.
- C7 (+panic/hide inline): 0xf62 fixed (f52 group improves).
- C8 (+busy inline): 0x46b and 0x630 flip BACK to our old order.
- C9 (+trying inline, /Od polarity): 0x46b/0x630 January again; 0xf6d and 0x2772/0x277a break.
- C10 (+NONE x2 inline): 0x2772/0x277a, 0x4993, 0x55ce fixed; best point: 82 regions, equal 6476.
- C11 (+danger colour ?:): new 0x2c1a/0x2c29/0x2c32 regions (gun-block neighbourhood).  - C12 (+paths colour): inert.
READING: the responsive sites (0x46b, 0x630, 0xf52..0xf89, 0x2772, 0x2c1a.., 0x4993, 0x4d32/0x4d51, 0x55ce..0x56de)
oscillate with edits made FAR downstream of them (C8/C9 are ~1,600 source lines after 0x46b's statement), so their keys
are whole-function state (pool history), not a running count at the site. Selecting a subset by region yield would be
rule-21/24 steering; none is proposed as a landing on yield grounds.
STABLE across all 13 variants (never moved): fst/fstp 0x1046/0x11eb/0x11ff/0x12c8; the 0x1ee7..0x1f33 interpolation
group; t-copy 0x2ae2..0x2b6e and 0x4917..0x49a3; gun cross-jump 0x2cde..0x2d37; fisub 0x39a6; push order
0x4cb7..0x4d7a; vehicle-avoidance t-copy + push order 0x4f3a..0x5034; 0x59e5; alignment filler 0x1ddd/0x557a/0x55ce.

## C13 hygiene (card cards/C13_unreferenced_string.txt) — OUTCOME
HEAD+E13: object keyed-identical to build/base (0 changed / 0 added / 0 removed); only C1 $L label numbers shift by one.
/W3: C4101 'string' unreferenced local 1 -> 0 (other warnings unchanged: C4013 x2 are the pre-existing
console_printf/error, outside render_actor). On top of C12: code identical to C12. PREDICTION CONFIRMED (inert).

## C14 vehicle-avoidance second cross (card cards/C14_vehicle_avoidance_second_cross.txt) — OUTCOME
HEAD+E14: the vehicle-avoidance block 0x4f3a..0x5034 is byte-identical to HEAD (prediction about the p0.x/p1.x copy
order FAILED: the copy order and t-copy are unchanged). Globally, 0x46b, 0x630, 0xf62 and 0x55e0 become January and
nothing breaks: regions 84 -> 79, exact-text 6472 -> 6479, size 25,008 / width +43 unchanged. This is again the
whole-function ID state, not the block.
Full set E1..E14 (c14): 25,008 B, width +43 -> +32, but regions 84 -> 103 and exact-text 6472 -> 6409: the combined
state re-allocates the [i][j] avoidance loop (0x6eb..0x85d) and the avoidance-objects loop (0xafe..0xb19); famsize shows
the -11 width comes from offsetting allocation noise (+9/+8/+5/+4 vs -15/-11/-5/-4), not from any family closing.

## P1 / P2 VN-count instruments (cards P1_*, P2_*) — OUTCOMES (never landings)
P1 (k folded `blockage_type &= c` after `= 0`): k=1,2,3 produce IDENTICAL objects (bad instrument: only the first one
perturbs the aiming block's zero register). P2 (k known-false `if (gun_offset)` tests in the gun block): k=1,2 code
identical to HEAD, k=3 moves only 0x55e0. The gun cross-jump never moved. H-VN for the gun family: NOT SUPPORTED by
these instruments (negative for these instrument forms only; not an impossibility proof).

## Stop (rule "Bound blind residual work", house rules G64)
14 evidence-backed shapes (C1..C14) + 2 instruments. The byte families are untouched by every one of them; the only
movers are the ID-sensitive operand-order sites, which respond chaotically to edits anywhere in the function.

## Residual map at HEAD (reg/slot-normalised; tools/famsize.py; width-normalised total +43, code end +32)
| family | J offsets | bytes (ours-J) | class | responds to C1..C14? |
|---|---|---:|---|---|
| gun cross-jump | 0x2cdb..0x2d56 | +22 | def-stand magnitude term order (k*k+i*i)+j*j vs J i,j,k blocks J's def-crouch/def-stand tail merge | no (16/16 stable) |
| marker fst/fstp | 0x1046, 0x11eb, 0x11ff, 0x12c8 | +15 | consume-vs-memory store (encoder reg bit, ledger 4660) | no |
| vehicle-avoidance t | 0x4f3a..0x5034 | +7 | t copied by mov vs J fld/fst + two push-order swaps | no |
| align filler / vision | 0x1ddd, 0x557a..0x56de | +4 / -7 | layout filler (downstream of size), vision site 0x55e0 is ID-sensitive | partly |
| prop interpolation | 0x1ee7..0x1f33 | +2 | x87 term/operand order of set_real_point3d args | no |
| t-copy flying #1/#2 | 0x2ae2..0x2b6e, 0x4917..0x49a3 | 0 | throttle.j/.k parameter copy mov vs J fld/fstp | no |
| push order | 0x4cb7..0x4d7a, 0x59e5 | 0 | scheduler ties | no (0x4d32/0x4d51 variant only) |
| fisub | 0x39a6 | 0 | scheduling | no |
| operand order | 0x46b, 0x630, 0xf52..0xf89, 0x2772, 0x4993 | 0 | x87 commutative sort keys (slot IDs) | YES, chaotically |
/Od shape checks done this wave (all equal to ours, so not source defects): gun block (0x49fc71..0x49fe95), flying
throttle block (0x49faa4..0x49fc17), prop interpolation (0x49ecb7..0x49ed78), vehicle-avoidance intersect block.
/Od is NOT January at the aiming-vector p0: /Od calls point_from_line3d(head, up, -0.04f) (0x49f9b1..0x49f9d7),
January subtracts up*0.04f with fsubr (J 0x29e6..0x2a1b), exactly as our hand-written source. So /Od code-neutral shapes
(C1..C14) are D evidence only and cannot be corroborated by January below full exactness.

## /Od-vs-ours local census: differences NOT tested (reopen inputs, D evidence only)
- Action-switch per-case pointer locals in /Od, absent in ours: [ebp-0xad8] and [ebp-0xadc] (two cases, both
  `actor+0x80`), [ebp-0xae0] guard data (wait/look/cower), [ebp-0xaec] search data; the search delay is
  `delay = MIN(120 - search->search_failure_timer, search->search_remaining_time)` (cseries MIN; ours if/else).
  Names/types of the pointers are not attested (no RTC for scalars) -> would need descriptive names (rule 15).
- Obey/command block: /Od scalars -0xef8, -0xefc (=0), -0xf08, -0xf00, -0xf04 (pointers) and a named real
  [ebp-0xf10] = `destination_radius_valid ? destination_radius : 0.5f` (ours passes the conditional inline).
  Ours also carries placeholder names v427/v426/v425/v424 for the four byte flags (-0xf09..-0xf0c): rule-15 debt.
- Ours has `real_point3d *head_position` in the aiming-vectors block; /Od has none, but /Od's p0 there is a different
  (later) spelling, so this is not evidence against ours.
- /Od has a third 12-byte pair (-0x1228/-0x123c) in the late vehicle/projectile region vs ours' two pairs: not mapped.

## Files
- ai_debug_K2_E13_hygiene_unreferenced_local.patch — landable hygiene: object keyed-identical, C4101 1 -> 0.
- ai_debug_K2_E14_RESEARCH_vehicle_cross.patch — research only (D-attested; block bytes unchanged; global ID effect).
- ai_debug_K2_E1-E14_RESEARCH_od_shapes.patch — research only (full D-attested set; owner question Q-K2-1).
- lead_proposals/sig_py_rdata_normalisation.patch — lead-owned tool fix (sig.py ident): render_actor delta -> {}.
- tools/: eval.sh, apply.py, edits.py (E1..E14), gen_edits_e4_e12.py, od_slots.py, famsize.py, rdiff.sh and the
  structural lane's sdiff/slotmap/relax/widthcensus/metric/dumpfn (repointed). Objects: scratch/campaign/workers/K2/.

## Whole-TU checks (2026-09-26 02:23)
review_patch.py (lead reviewer): E13 -> 0 gains / 0 losses / 0 other changes; E14 and E1..E14 -> 0 gains / 0 losses /
1 other change (_ai_debug_render_actor 25008 -> 25008). keyed_diff vs build/base: E13 0 changed; E1..E14 1 changed
(render_actor only), 0 added, 0 removed (no new symbols, no COMDAT/provider change). /W3: E1..E13 warning census
identical to base except C4101 1 -> 0 (no new C4013). fake_match_scan: 0 leads before and after. object_audit:
FAIL(2) unchanged (render_actor residual + jmptable label), identical to base.
