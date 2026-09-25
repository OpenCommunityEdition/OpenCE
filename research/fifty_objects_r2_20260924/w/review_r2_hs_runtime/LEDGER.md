# LEDGER review_r2_hs_runtime (adversarial review, verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)
Reviewer: round-2 adversarial reviewer. Tree HEAD b7c1b7d9 (f6d00a8c + baseline commit), git status clean at start.
Target under review: worker scratch/w/hs_runtime__render_debug_trigger_volumes (production.patch, production_crlf.patch, park_retire.patch),
candidate compose_mine_first/source/hs/hs_runtime.c (worker patch composed with hs_runtime_storage converter storage patch).

## Step 0: read brief (sections 0-9), worker MANIFEST + LEDGER, patches, hints.json entry.
- hints: exact 64/66; missing begin_random (T, park private-register-convention) and render_debug_trigger_volumes (T, park unclassified).
- Park text says prior lane deliberately avoided a point_from_line3d COMDAT; candidate now emits one (ruling-5 class) -> must verify ruling text.

## Step 1: patch application at current tree (HEAD b7c1b7d9, clean)
- git apply --check: production.patch, production_crlf.patch, park_retire.patch, park_retire_crlf.patch, storage patch -> all OK.
- GNU patch on LF HEAD blob: mine-only == worker cand.c (CR-stripped); A(prod)+B(storage) == B+A == worker compose_mine_first (LF).
- git apply --directory on CRLF working-tree copies: mine, AB, BA all apply; AB==BA; CRLF results == LF results after CR strip; no mixed EOLs.

## Step 2: independent gates + object_audit (my compiles from patched HEAD copies)
- gate base (HEAD file): 64/2/0 (begin_random sha, trigger_volumes sha). mine-only: 65/1/0. compose (AB): 65/1/0. Only begin_random residual.
- object_audit base FAIL(14) = 2 sections + 12 converter storage rows. mine FAIL(13) = begin_random + 12 storage rows. compose FAIL(1) = begin_random; 129 January symbols, 0 differ.
- New surplus in mine/compose vs base: exactly `+.text sym:_point_from_line3d size 48`. No new literals (no __real@3f800000 / bf800000: t=+-1 folded). 

## Step 3: per-section sweep, surplus identity, provider link
- sweep.py (owner-key + object_audit fallback) over 127 January-owned sections: base->mine gain 1 (_render_debug_trigger_volumes), regression 0, changed 1.
  base->compose identical result. build/base production vs my base.obj: changed 0 (my baseline compile == production).
- surplus_cand.py: mine and compose: 5 candidate-only code COMDATs, 0 not identical. _point_from_line3d 48/0/9b763841f8519177 == January action_charge copy.
- provider_link mine --baseline=base: _point_from_line3d vs action_charge PASS (both orders). compose (all surplus): PASS.
  mine (all surplus): FAIL(12) = only the 12 converter storage rows (NO JANUARY PROVIDER), pre-existing and fixed by storage patch.

## Step 4: /Od re-read (od_linear 0x606c80..0x607470 -> od_tv.txt) + RTC descriptor 0x607464 (parsed with odbuild.rtc)
- RTC: matrix -64/52, local_extent -96/12, world_extent -116/12, corner -176/48, sides -208/24, color -240/16, center -260/12, ray -280/12, result -368/80.
- Scalars: volume_index -0x48 (short), scenario -0x4c, volume -0x50 (`mov [ebp-0x50],0` after scenario store, before loop), edge_index -0x7c, side -0xd8, axis -0xdc.
- scenario is STORED first (0x606cce) yet sits BELOW volume_index -> frame = declaration order, not first-use (same law as light_volumes precedent).
  matrix first used at 0x606d4f yet is the TOPMOST slot -> declared before volume_index.
- Floats 0x93dd5c=1.0, 0x9401b4=-1.0, 0x93dd50=0.5, 0x941b60=0.15, 0x9411fc=0.95. 8 calls to 0x42e0d0 (point_from_line3d): 4 per side arm (arm 1 incl. corner[0]) ... side arm: (pos, world_extent, 1.0, corner0), then 3 tail calls; else arm: corner[0]=position copy then same 3 tail calls; + center (0.5).
  Argument pushes match candidate exactly; both render_debug_string_at_point calls under if/else (0xa1c244 / 0xa1c22c).
- Assert line /Od 0x214 vs January 0x213 (1-line drift above this point; kept 0x213).
- HCEX.pdb (SHIP) has no record for this function; HCEX_Release.pdb records: matrix, local_extent, world_extent, sides[2], result, ray (names attested).

FINDING F1 (confirmed independently): candidate declaration order volume_index, scenario, matrix, volume, local_extent, world_extent, edge_index, corner, center, ray, result, sides, side, axis, color
contradicts /Od order matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result
in two places: matrix (declared in loop block after if-block scalars) and center/ray/result (before edge-loop locals). Brief s9 + light_volumes wave-2 precedent: reject when an equally exact /Od-consistent grouping exists.

## Step 5: reviewer variants (mkvar.py -> var/)
- full_od (matrix to top of if-block + `{ center; ray; result; ... }` block after the edge loop): mine 65/1/0, comp 65/1/0.
- matrix_only: 65/1/0 (both). block_only: 65/1/0.
- objeq.py: full_od_mine.obj == mine.obj and full_od_comp.obj == compose.obj, matrix_only likewise: 154/154 non-debug sections identical, 488 symbols identical (incl. $L numbering).
- STRIP lab_ternary (string pair back to production ternary): trigger_volumes RESIDUAL [sha] -> if/else load-bearing and /Od-attested.
- STRIP lab_single_tail (3 tail point_from_line3d calls hoisted below if/else): RESIDUAL [sha] -> duplicated arm tail load-bearing and /Od-attested.

## Step 6: warnings, storage, data, parks, begin_random
- /Zs /W3 (warn.py; sanity lab with an unused local proves C4101 is caught): head, worker compose, full_od_comp -> identical 138-line warning sets (all pre-existing), no new warning.
  (gate.py --cflag /W3 /WX is NOT a valid warning test here: the unused-local sanity lab still compiled.)
- pdb_storage_cand.py: base/mine 12 disagreements (converter rows), compose and full_od_comp 0. _point_from_line3d is cachebeta public (line 6312) -> external COMDAT agrees.
- COMDAT selection: mine/compose _point_from_line3d SELECT_ANY(2); production action_charge ANY; nodup_census 24 definers, 0 NODUP.
- tinfo --data base vs amended composed: identical except .debug$S path. object_audit: 61 data sections (.bss 1, .data 1, .rdata 59) ok.
- park_retire.patch applied to HEAD parked.json: 86 -> 85, removes only {hs_runtime, _render_debug_trigger_volumes}; begin_random park kept; version key unchanged; JSON valid.
- begin_random: park private-register-convention; w1 + w3c SKIPPED-EXHAUSTED (4 ModRM bytes). Retired per brief s8. Object stays NonMatching.
- Worker variants re-read: od_order, od_order_vfirst, od_inner_only, od_outer_only, bareblock, color_else/edge, axis_first: NONE puts matrix before volume_index
  (bareblock keeps matrix in the loop block) -> MANIFEST/LEDGER claim that the full /Od order was measured is inaccurate.
- January relocs: _collision_test_vector and _render_debug_string_at_point are REL32 out-of-line calls -> &center, &ray, &result escape -> escape law permits their own block.

## Step 7: amended patch (claimed source/hs/hs_runtime as review_r2_hs_runtime)
- amended/production_amended.patch (LF, git apply vs index) + amended/production_amended_crlf.patch (CRLF working tree); fallback amended/production_matrix_only(.crlf).patch.
- git apply --check OK for all 4 at HEAD b7c1b7d9.
- git apply --directory on CRLF copies: amended alone, amended+storage, storage+amended -> AB==BA; CR-stripped == var/full_od_comp.c; GNU patch LF both orders identical.
- Applied CRLF files gate 65/1/0 (only begin_random). objeq: applied amended-only == worker mine.obj; applied amended+storage == worker-compose object (154 sections, 488 symbols identical).
- object_audit amended+storage FAIL(1) begin_random only, 129/129 symbols; provider_link --baseline PASS both orders; full provider_link on composed PASS; surplus 5/5 IDENTICAL; sweep gain 1 regression 0.
- My amended source is textually identical to the parallel function-reviewer's amended file (independent convergence).

## Final
- Claim released. git status clean (no tracked modification). REVIEW.md written. Verdict approve=false as submitted; amended_patch = amended/production_amended.patch (+_crlf); fallback production_matrix_only.patch.
