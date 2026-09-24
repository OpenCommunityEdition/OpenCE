# source/cache/xbox_sound_cache

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\od3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\s1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\storage.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\fuzzy_refresh.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\combined_gated.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\od_51caa0.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_sound_cache\\pdb_storage.txt"
]

## production_changes
There are two independent patches, and both pass git apply --check against 931ed8dc.

(A) Landable storage-only ownership correction, zero credit and byte-inert.
- scratch/w/xbox_sound_cache/storage.patch adds `static` to four symbols in source/cache/xbox_sound_cache.c: the prototypes and definitions of sound_cache_locked_block_proc and sound_cache_delete_block_proc, the definition of sound_cache_debug_block_name, and the xbox_sound_cache_globals definition.
- Pair it with scratch/w/xbox_sound_cache/config/symbols.json. Six lines are edited in place and are the only lines that differ: 7222, 7223, 7224, 7225, 7230 and 23289 each gain `, "static": true` (_sound_cache_locked_block_proc, _sound_cache_delete_block_proc, _sound_cache_debug_block_name, _sound_cache_start_loading_sound, _render_inverse_transform_screen_point at file_offset 1761696, and _xbox_sound_cache_globals).
- Csplit must be regenerated after the symbols.json edit.

(B) Owner-gated fuzzy park-body refresh; do not land without a ruling.
- scratch/w/xbox_sound_cache/fuzzy_refresh.patch holds the od3 body. scratch/w/xbox_sound_cache/combined_gated.patch is A+B source; the full candidate TU is scratch/w/xbox_sound_cache/final.c.
- It replaces the production body, which hand-expands point_from_line3d (a rule-6 violation), with the /Od-attested shape. It also adds a TU enum XBOX_SOUND_CACHE_PAGE_COUNT = 1024, used for page_usage, rows and the lruv_new page count.
- If an owner rules it in: update the config/parked.json _sound_cache_debug_render base measurement to size 368, relocs 14, normalized_sha256 6b7a4944931a17255b7a7ffaa1f740c07eeab4344bd478ef2cb460a2aa5e2d88, and re-measure objdiff_percent after the full ninja. The park stays open.

## evidence
All results are gate.py --source strict rows.

Baseline:
- 17 EXACT; `_sound_cache_debug_render` is residual [sha].
- Sibling `_texture_cache_debug_render` is EXACT.

/Od readout (PC halo_cache_symbols.exe, cache\pc_sound_cache.c, fn 0x51caa0; od_51caa0.txt):
- RTC names: colors[16], extent[24], line_pt[16], vec[12].
- Locals: short rows=page_count/640; long x; short page_index=rows*x; long bit; long i.
- The scale update is a REAL `point_from_line3d(&extent[i], &vec, (render.camera.z_near+0.001f)/dot_product3d(&vec,&render.camera.forward), &extent[i])` call. There are no distance/scale/pointer-walk locals.

Best candidate od3/final:
- 119/119 instructions, same 0x454 frame, same slots and registers, 14/14 relocations.
- ONE differing relocation row (render+36 at 0xe2 vs 0xe5): the dot k-term x87 operand order. January emits `fld [ebp-0x10](vec.k); fmul render+0x24`; ours is the reverse.
- The production body differs in about 20 blocks.

Calibration on the texture TU (tex/t_od_graph.c, tex/t_od_all.c):
- The texture graph and list branches in the /Od inventory stay 25/25 EXACT, so the /Od inventory is January-compatible.
- The texture dot is count-insensitive (D=1..12 all exact).

Decoded key (lab dead-local oracle, never landable):
- On od1, only D=4 closes. The terms flip monotonically from render-first to vec-first: k at D=4, j at D=5, i at D=10. There is no periodicity up to D=48; this is a threshold, not mod 8.
- Insensitive to where the locals are placed (top, predicate block, x-loop block, before or after vec, first use after the loop).
- Insensitive to type: 4 real, short or pointer locals all close.
- Unreferenced declarations count 0. Inline-helper temps count 0 (set_real_point2d probes).
- Each accessed field of a non-address-taken aggregate counts 2: one vector with 2 fields closes; 2 vectors with 1 field each close; 3 or 4 overshoot.
- TU-level externs and prototypes (1/2/4/8) are inert.
- Moving the helper definition after the caller is inert.
- Swapping the dot_product3d arguments is inert, which confirms w2 sd2.
- Conclusion: January's function has exactly 4 more units of referenced user IL symbols than the /Od-attested inventory.

Natural shapes tried, all leaving the same k pair:
- sd1: the texture-reconstruction shape, re-measured on the exact helper.
- od1, od2 (TEST_FLAG), od3.
- v1: named distance/scale.
- v2: plus pointer walk, +4 locals, but the dot moves into its own statement.
- od1s: dot arguments swapped.
- od1_helper_last: helper defined after the caller.
- lab_counters: dead PC-style page counters in a switch; refuted at 384 B.

Surplus for od3:
- The new _point_from_line3d (48 B) is section_infos_equal to January's selected copy (action_charge).
- _dot_product3d and _add_vectors3d are also equal.
- provider_link.py: SELECTED-PROVIDER LINK PASS in both orders.

Object:
- build/report.json: .bss 272 100%, .rdata 952 100%.
- The storage patch s1 keeps 17 EXACT with an unchanged debug_render sha (8ae719b8).
- pdb_storage shows 6 cachebeta non-publics. xref_check: 0 UNDEF references from January or from our objects.
- s2 (HCEX boolean return for locked_block_proc) breaks that function, so it is rejected.

## blockers
1. `_sound_cache_debug_render`: one x87 commutative operand pair, set by a threshold key on the count of referenced in-function user IL symbols. January needs exactly 4 more units than the /Od-attested inventory. No genuine source for those 4 units is attested:
   - The /Od build is the PC 2020 revision.
   - HCEX SHIP and Release have the graph compiled out.
   - January's bytes cannot distinguish them.
   Filler or staging locals are the oracle and fail the strip test, so they are owner-gated.
2. The od3 refresh newly emits `_point_from_line3d` from a caller that is not strict-exact. Ruling 2 requires a strict-exact caller, so it cannot land as the park body without an owner ruling.
3. Object completion additionally needs the storage correction (patch A plus the symbols.json regen). Candidate-only XDK D3D tables and __real literals remain the reviewed duplicate-COMDAT surplus; provider link passes.

## reopen_criteria
Reopen on either of these:
- An authoritative 2002 Xbox source or local record for sound_cache_debug_render that names the 4 extra referenced symbol units: 4 scalars, 2 accessed fields of non-address-taken aggregates, or 2 scalars plus 1 field. Validate the lab oracle with lab_od2_R4 / lab_od2_V1ij (exact).
- An owner ruling that admits a specific attested inventory.

For landing the od3 body as the park refresh, an owner ruling is needed on emitting `_point_from_line3d` (byte-identical, provider link PASS) from a caller that is not strict-exact. Do NOT retry: operand swaps, dot argument order, declaration placement or scope, helper definition order, TU declaration counts, TEST_FLAG vs &FLAG, the texture-style distance/scale/pointer walk, or set_real_point2d routing. All are measured inert or non-closing.

## task notes
Side finding (other unit, zero credit): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\xbox_sound_cache\tex\texture_od_inventory.patch (git apply --check OK) rewrites texture_cache_debug_render to the /Od-attested inventory (PC /Od fn 0x51f1d0, od_51f1d0_texture.txt).
- It calls point_from_line3d instead of hand-expanding it, which fixes a rule-6 violation in the current exact texture source.
- It uses line_pt/extent/vec and width/x_offset/y_offset shorts, and drops the bitmap/touched_string locals.
- Gate: xbox_texture_cache stays 25/25 EXACT.
- The new _point_from_line3d COMDAT is equal to January's selected copy (action_charge), and provider_link PASS with --baseline.
- It meets ruling 2 (strict-exact caller) but needs the integrator's full sweep.

Also recorded: the existing exact texture reconstruction's inventory is NOT authenticated by its exactness, because the texture dot is count-insensitive.

The M8 refinement found here is worth a law note. For this function the key is a monotone threshold (not mod 8) on referenced user IL symbols:
- scalars count 1;
- accessed fields of non-address-taken aggregates count 2;
- inline temps and unreferenced declarations count 0;
- insensitive to TU declarations, placement and type.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\xbox_sound_cache\LEDGER.md
