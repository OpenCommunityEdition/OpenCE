# collisions worker report (Lane B, resumed run)

Baseline: `== exact 19  residual 1  unwritten 0  (of 20 listed)`; `_collision_move_point [sha]` 4752, 43 filtered real regions (aln_base_f.txt).
/Od body: 0x7A52B0 (od_collision_move_point.txt). Filtered region count = filt.py over alndiff (diagnostic only).

## try_1 - synthetic block: minimum_k / steepest_clip_index assigned AFTER the output stores
- /Od: stores ..0x7A6547, then `minimum_k=0` 0x7A654B, `steepest=NONE` 0x7A6556, `clip_index=0` 0x7A6560 (uninitialised declarations).
- January: `or edi,-1` (f6b) feeds BOTH the NONE stores (f77,f7a,f85) and steepest (`cmp di,-1` fca/10bd immediate) = const web coalesced into steepest by a copy AFTER the stores. Baseline pinned -1 in eax + separate `or edx,-1`.
- gate: `[size 4768!=4752, sha]`, 42 regions. Const/steepest coalescing now January-shaped, registers rotated. Kept as a step (not a candidate).

## try_2 = try_1 + named element pointer in the steepest loop
- /Od: `[ebp-0xf0] = &collisions[clip_indices[clip_index]]` 0x7A658C..0x7A659E, then two `->plane.n.k` reads through it.
- January: `fcom [ecx+eax+0x18]; add ecx,eax; ... fld [ecx+0x18]` (struct pointer formed, not a field address; baseline had `lea edi,[eax+edi+0x18]; fld [edi]`).
- gate: `[sha]` 4752, 29 regions (from 43). Whole loop + tail registers now match.

## try_3 = try_2 with last_collision declared before collision
- gate `[sha]`, 28 regions; idx load order matches but eax/edx swapped and `add` instead of January `lea eax,[edx+ecx]`. Superseded by try_4.

## try_4 = try_2 with NO last_collision local (direct `collisions[clip_indices[clip_count - 1]].t/.point`)
- /Od: index expression re-evaluated twice, 0x7A64CA..0x7A64DF and 0x7A64E4..0x7A64F3; no pointer slot between -0xe0 (collision) and -0xe4 (minimum_k).
- January: `movsx edx,[ebp+edi*2-0x4a]` f3a ... `lea eax,[edx+ecx]` f51 = separate CSE temp, not an in-place add.
- gate: `[sha]` 4752, 26 regions. Synthetic block f31..fbd byte-identical apart from the inherited 3-byte offset. unitcheck: gains=0 regressions=0. ACCEPTED -> candidate.c.

## try_5 = try_4 + explicit `new_clip_count = N; new_clip_indices[N-1] = ...;` (no `= 0` initialiser, no `new_clip_count++` subscripts)
- /Od: `mov word [ebp-0xcc],1` 0x7A58F6 BEFORE `[ebp-0xd8] = collision_count-1` 0x7A5902; `=2` 0x7A5B0B then constant-index store 0x7A5B27; `=3` 0x7A5CA1; else-if arm `=2` 0x7A5EAA. No `=0` store anywhere.
- January: `mov esi,2` (7a5) precedes `mov [ebp-0x76],di` (7ad); baseline had them the other way round.
- gate `[sha]` 4752, 24 regions (from 26). unitcheck pending with next accepted step.

## try_6 = try_5 with `collision_count += 1` / `-= 1` (/Od movsx;add;mov-word pattern at 0x7A56FA, 0x7A6713, 0x7A67D7)
- byte-identical object to try_5 (inert). NOT adopted (no January evidence either way).
- try_5 unitcheck: gains=0 regressions=0 -> ACCEPTED as candidate.c

## try_7 = try_5 + break test inside the success arm + csmemcpy size from new_clip_count (/Od 0x7A5FFC, 0x7A6019)
- filtered diff identical to try_5 (inert, 24 regions). NOT adopted (no January-side evidence).

## try_8 (DIAGNOSTIC, loop form) = try_5 with do { if (small) break; ... } while (cc<max)
- gate `[sha]` 4752 but 62 regions (entry copies rescheduled). Worse; /Od shows a top-tested while. Rejected.

## try_9 (DIAGNOSTIC, loop form) = try_5 with while (TRUE) { if (small) break; ... }
- same 62-region result as try_8; no preheader loads, even clip_count's `mov ebx,[ebp+8]` preheader reload vanishes. Loop-form family is dead (agrees with 20260914 ledger). Rejected.
## diag_1 (DIAGNOSTIC ONLY) duplicate top assert: no residency change (second compare jump-threaded away).
## scan_preload.py over all 833 January objects: the 2-slot 'preheader load + back-edge reload' pattern occurs ONLY in _collision_move_point (no exact twin to learn from).

## try_10 = try_5 with site-D condition split `if (clip_count > 1 && dot < -eps) if (point_from_planes3d(...))`
- motive: J keeps `collisions` (edx) across the dot-compare block into the point_from_planes3d argument block, and cc/max across the fabs blocks; both splits in ours fall on short-circuit operand boundaries. /Od cannot distinguish `&&` from nested if (all three false-jumps target 0x7A5D0B).
- gate `[sha]` 4752, 14 regions (from 24): preheader loads `mov cx,[ebp+0x1c]`/`mov edi,[ebp-0x1c]`, back-edge reloads, `cmp di,cx`, and ALL tail struct-copy registers (bc2..c74) now match. Left: preheader load ORDER, 0x595 fmul/store order, site D registers.

## try_11 = try_5 with site D as `if (clip_count > 1) { if (dot < -eps && point_from_planes3d(...)) {...} }`
- same evidence as try_10; the clip_count guard as its own `if` mirrors the enclosing `if (clip_count > 0)`.
- gate `[sha]` 4752, 4 regions (2 real differences): site D now byte-identical, A and E identical. unitcheck gains=0 regressions=0. ACCEPTED -> candidate.c.
- Left: (1) preheader order J `mov cx,[ebp+0x1c]; mov edi,[ebp-0x1c]` vs ours edi-then-cx (17b/17f); (2) 595..59b J `fmul [ebp-0x30]; mov [ebp-0x50],eax` vs ours store-then-fmul.

## try_12 = try_11 + else-if arm as `else if (clip_count > 1) { if (dot && line_from_planes3d) }`: filtered diff identical to try_11 (inert). NOT adopted.
## h3probe (DIAGNOSTIC, filler prototypes before the function, K=0..8 on try_11): preheader order 17b/17f matches at K=1,2 only -> H3 numbering key, off by one unit. 595 swap unaffected by K.
## try_13 = try_11 + function-scope `position` with the /Od initialiser chain
- /Od: `position = *old_position` 0x7A52DC (slot -0x18, second local), `clipped_position = position` 0x7A5304, `clipped_velocity = velocity` 0x7A5316; HCEA lists `position` with the function-level aggregates.
- January: preheader now `mov cx,[ebp+0x1c]; mov edi,[ebp-0x1c]` (17b/17f) in January's order; entry copies unchanged (dead `position` copy folded, as in January's 3-copy prologue).
- gate `[sha]` 4752, 2 regions = ONE transposition left (595/598). unitcheck gains=0 regressions=0. ACCEPTED -> candidate.c.

## try_14 = try_13 + /Od declaration order `new_clip_count` before `new_clip_indices` + removal of two stray blank lines my edit script introduced: bytes identical to try_13 (2 regions). Kept as working base (layout only).
## try_15 = try_14 + `set_real_vector3d(..., 0,0,0)` at both zeroing sites (/Od calls 0x42E2B0 at 0x7A5CE8, 0x7A6364): inert. NOT adopted.
## try_16 / try_17 (DIAGNOSTIC) `new_clip_count = 1` after the index store / after the clip_plane copy: inert on 595/598.

## m8probe (DIAGNOSTIC) K=1..8 unused locals between clipped_velocity and clip_count: 595/598 never flips (K>=5 re-breaks the preheader order = H3 half-units). 595/598 is not a local-ordinal key.
## try_18 = try_14 with clip_velocity_to_plane written as the /Od shows it (0x7A40A0: dot_product3d call, negate, then the same 4-arg helper 0x42E0D0 used for point_from_line3d elsewhere)
- gate --all: `_clip_velocity_to_plane [size 48!=64, relocs 1!=0, sha]` (dot_product3d goes out of line) and move_point `[size 4736!=4752, relocs 228!=226, sha]`. REGRESSION of a baseline-exact helper -> rejected.
## try_19 = try_14 with clip_velocity_to_plane through the TU-local COLLISION_POINT_FROM_LINE3D macro: helper stays EXACT but move_point worsens to 9 regions (preheader order re-breaks, 595/598 unchanged). Rejected.
## try_20 = try_14 with `real distance = -dot(...)` + real `point_from_line3d(...)` call: `_clip_velocity_to_plane [size 48!=64, relocs 1!=0, sha]` -> regression, rejected. (point_from_line3d is not inlinable in this TU's header state; header-frozen.)
## try_21 (DIAGNOSTIC) `double distance` in clip_velocity_to_plane: helper `[sha]`, move_point `[size 4720!=4752, relocs 227!=226, sha]`. Rejected.
## m8probe proto K=1..31 on try_14 (DIAGNOSTIC): the 595 transposition NEVER flips (not numbering-keyed); the preheader order matches for roughly half of all K (hash coin flip) -> January-side evidence for the `position` scope move is weak (first-party /Od + HCEA attest it; bytes only show a 50/50 key landing right).
## try_22 = STRIP TEST of try_13/14: function-scope `real_point3d position;` WITHOUT the /Od initialiser chain -> identical bytes (2 regions). The initialiser chain is inert, so it is NOT adopted; only the scope move is kept.

## FINAL candidate.c = try_22 + layout cleanup (cleanup.py removes 4 stray blank lines from my edit script)
- gate --all + guard: `== emitted-symbol guard passed`, `residual   4752  _collision_move_point  [sha]`, `== exact 19  residual 1  unwritten 0  (of 20 listed)`
- unitcheck gains=0 regressions=0; newsyms OWNERSHIP PASS (0 new); fake_match_scan 0 leads.
- BYTE LEDGER (reloc-zeroed section compare, 4752 vs 4752): exactly 6 differing bytes, 0x595-0x597 and 0x59b-0x59d = ONE transposition inside the 16-byte `clip_plane = collisions[new_clip_indices[0]].plane` copy:
    J  592 fld [ebp-0x54] | 595 mov edx,[edx+0xc] | 598 fmul [ebp-0x30] | 59b mov [ebp-0x50],eax | 59e mov [ebp-0x4c],edx
    O  592 fld [ebp-0x54] | 595 mov [ebp-0x50],eax | 598 fmul [ebp-0x30] | 59b mov edx,[edx+0xc] | 59e mov [ebp-0x4c],edx
  x87 issue slots are IDENTICAL; only the integer copy order differs: January L0 S0 L1 S1 L2 L3 S2 S3 (last load reuses the working source pointer BEFORE the third store), ours L0 S0 L1 S1 L2 S2 L3 S3.
- Measured inert on it: new_clip_count statement position (3 places), declaration order, set_real_vector3d, += 1, break placement, csmemcpy size operand, else-if nesting, prototype count 0..31, unused-local count 1..8, macro/point_from_line3d/double forms of clip_velocity_to_plane (last three regress).
- REOPEN EVIDENCE: a same-compiler exact function containing a 4-dword struct copy from a CSE-shared (non-clobberable) address with one free temp that emits `L2 L3 S2 S3`, to read off what makes the expander pair the last two loads.
