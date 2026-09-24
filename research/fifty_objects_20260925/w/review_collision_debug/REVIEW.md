# REVIEW collision_debug (adversarial review of OBJECT_COMPLETE_CANDIDATE)

Base worktree HEAD e9e62b78, git status clean. Source unchanged by the proposal (cand.c == production copy).

## Step 1 gate
`gate.py source/physics/collision_debug --source cand.c --all --out cand.obj` -> EXACT 4192 _collision_debug_render; exact 1 residual 0 unwritten 0.

## Step 2 object_audit (build/split as-is)
All 17 January sections ok (incl .text 4192, .bss 28962, .data 16, .rdata cube 96 align 4/4, 13 literals). 29 January symbols, 1 differ: _collision_debug_cube_vertices storage split 2 / ours 3. Surplus: .drectve, 11 literals (1 empty string, 2 global_current_collision_user asserts, 8 __real), 3 D3D SDK tables, 4 code COMDATs (_add_vectors3d 48, _point_from_line3d 48, _scale_vector3d 48, _set_real_vector3d 32). Matches worker.

## Step 3 storage evidence
- pdb_storage: 29 split symbols, 1 disagreement: _collision_debug_cube_vertices split 2 ours 3 PDB-public False.
- Independent grep of scratch/tools/cachebeta_publics.txt: every external collision_debug_* global in this object (flags x5 + length + ignore_object_index in .data; width/height/phantom_bsp/phantom_bsp_found in .bss; _collision_debug_render) IS a PDB public; there is NO public at 0x0028B2A8 (only neighbouring bsp3d / collision_debug string literals). So January's cube array was file-static; source `static const` is right and symbols.json is wrong.
- symbols.json line 19561 = `{ "file_offset": 2667176, "flags": 0, "name": "_collision_debug_cube_vertices" },` (2667176 = 0x28B2A8). Format of the fix matches the existing 2153 `"static": true` rows (e.g. data row 13748 _bitmap_format_bits_per_pixel_table).

## Step 4 independent split regeneration (ONLY the one line)
- scratch/w/review_collision_debug/config = copy of config/*.json with a byte-level in-place replacement of line 19561 only (diff shows exactly 1 line; no re-serialisation; line endings preserved).
- build/tools/csplit.exe -i cachebeta.exe -p <scratch config> -o scratch/w/review_collision_debug/split (rc 0, 833 objects).
- Byte cmp of all 833 objects vs build/split: ONLY source/physics/collision_debug.obj differs; objeq: its sole difference is the storage class of _collision_debug_cube_vertices (2 -> 3). No other object (incl. no other rdata neighbour) is affected.
- audit (object_audit copy with AUDIT_SPLIT_ROOT) vs regenerated split: 29 January symbols, 0 differ, OBJECT AUDIT: PASS.
- gate (gate.py copy with GATE_SPLIT_ROOT) vs regenerated split: EXACT 4192 _collision_debug_render, exact 1/1; candidate object IDENTICAL (objeq, 36 sections) to the as-is compile.

## Step 5 surplus
- surplus_identity.py: 4 candidate-only code COMDATs, 0 not identical (_set_real_vector3d/actions, _point_from_line3d/action_charge, _scale_vector3d/action_charge, _add_vectors3d/actor_combat).
- own surplus_all.py (every candidate-only external def vs EVERY January split definer; section_infos_equal + raw bytes + section flags/alignment): 18 rows (4 code, 3 SDK tables, 11 literals), each has exactly one January definer, all IDENTICAL, raw bytes equal, flags equal. problems 0. (The worker's surplus_data.py only compared provs[0]; mine covers all definers - same answer.)
- provider_link.py (both input orders, VC7 Link.Exe): 18/18 PASS, SELECTED-PROVIDER LINK: PASS.
- nodup_census: _add_vectors3d 17 definers / _point_from_line3d 22 / _scale_vector3d 64 / _set_real_vector3d 22 / 3 SDK tables 133 each: NODUP [] for all (no hand-copy conflict anywhere on the board).
- .drectve surplus = `/DEFAULTLIB:"LIBC" /DEFAULTLIB:"OLDNAMES"`, identical to every compiled object (e.g. action_charge); not January-owned content.

## Step 6 data / scorer status
- build/report.json (objdiff 3.3.1, production): code 4182/4182, data 29278/29278, sections .bss/.data/.rdata/.text all 100.0. No semantic_data_matches entry needed.
- One-unit objdiff 3.3.1 report (scratch od_proj, `report generate`, no build) of cand.obj vs the REGENERATED split: identical measures (code 4182/4182, data 29278/29278, all 4 sections 100.0). The storage fix cannot cost scorer credit.

## Step 7 parks / rejections / metadata
- config/parked.json, object_admission_rejections.json, semantic_matches.json, semantic_data_matches.json, semantic_credit_rejections.json, symbol_ownership.json: no collision_debug entry. `tools.parked_functions`: none.
- config.json line 515: collision_debug.c index 140 status "NonMatching" (the integrator flips it).

## Step 8 source provenance (source unchanged by this proposal)
- Function landed by the owner in 5671843f ("Match collision_debug_render", Jonas, 2026-09-23), full ninja + stable sweep 0 regressions, commit text: "Function credit only. The object is not marked Matching. _collision_debug_cube_vertices is static in source and in January (absent from cachebeta.pdb publics) but still external in symbols.json. That is a separate, pre-existing ownership follow-up." -> the storage row is the ONLY object-level blocker the owner named; this proposal is exactly that follow-up.
- /Od re-attestation of the add_vectors3d cast (od_linear.py 0x7af800..0x7b1ee4 -> od_linear.txt): 0x7b0c42..0x7b0c5c pushes [ebp-0x18], [ebp-0x2c], [ebp-0x18] and calls 0x406339 -> 0x44e8e0; od_44e8e0.txt = three-component a[i]+b[i] -> out, returns out = add_vectors3d body. So `add_vectors3d(point, vector, point)` is first-party attested (point local as args 1 and 3). real_math.h has no point+vector helper other than add_vectors3d.

## Step 9 STRIP TESTS (whole-object objeq vs cand.obj)
- strip_add.c  `add_vectors3d(&point, &vector, &point)` (casts removed): EXACT 1/1, object IDENTICAL.
- strip_cube.c `&collision_debug_cube_vertices[point_index]` (cast removed): EXACT 1/1, object IDENTICAL.
- strip_all.c  both casts removed: EXACT, IDENTICAL.
- strip_twopi.c `fmod(..., _pi*2.f)` (real_math.h's own spelling, lines 789/794) instead of the raw double `6.2831854820251465`: EXACT, IDENTICAL (same __real@401921fb60000000). The raw double literal is a folded-constant readout (= (double)(float)2pi; no human types it) - implausible spelling, but NOT load-bearing.
- fake_match_scan.py on collision_debug.c/.h: 0 leads.

## Step 10 FINDING: hand-expanded magnitude3d in the features block (rule 6 / rule 13)
- Production line ~550: `radius = (real)sqrt(collision_debug_vector.i * collision_debug_vector.i + ...j*j + ...k*k) * 0.5f + collision_debug_height * 0.5f + collision_debug_width;`
- /Od first-party build, features block (od_linear.txt 0x7b1d6b..0x7b1e00): point_from_line3d call (0x42e0d0) for test_center, then `push 0xdef080 (&collision_debug_vector); call 0x4052d6 -> 0x432950`; 0x432950 = push v; call 0x42df70 (magnitude_squared3d: v.i*v.i+v.j*v.j+v.k*v.k, od_42df70.txt); fstp; call 0x42e320 (square_root -> 0x42e2f0) = real_math.h `magnitude3d` (`return square_root(magnitude_squared3d(v));`, real_math.h:1140). 0x432950 has many callers board-wide (odbuild callers).
- => the first-party source calls the header helper `magnitude3d(&collision_debug_vector)`; production hand-expands it. House rule brief s.2 rule 6: "never hand-expand or hand-copy point_from_line3d or any header helper"; campaign rule 13: "retain ordinary helpers". Needs measuring: does the attested helper spelling also give EXACT (then the hand expansion is a pure source-quality defect to fix before Matching), or not (then the hand expansion is what buys the match = fake)?
- MEASURED mag3d.c (`radius = magnitude3d(&collision_debug_vector) * 0.5f + collision_debug_height * 0.5f + collision_debug_width;`): EXACT 1/1 (4192); object differs from production ONLY by 3 extra COMDATs _magnitude3d(48)/_magnitude_squared3d(48)/_square_root(16); surplus_all: all 3 IDENTICAL to January's selected copies (action_charge / action_alert / action_charge), raw+flags equal; nodup_census NODUP [] for all three (67/92/83 definers); provider_link on mag3d.obj: 21/21 PASS.
- => the attested helper spelling is byte-exact and fully admissible; the production hand expansion is not needed for bytes. It is a leftover of the pre-exception "no helper COMDAT" era (100K ledger item 6 hand-expanded point_from_line3d for the same reason; 5671843f converted those to helpers but missed this one). Under brief rule 6 ("never hand-expand ... any header helper") and campaign rule 13 the object is NOT admissible as Matching with the hand-expanded form.
- Owner standard confirmed in docs/object_matching_logs/claude_object_closure_canonical_reconciliation_20260924.md: "No forced-inline, emission-suppression, hand expansion, or COMDAT metadata steering was admitted"; action_vehicle's hand-written helper copy was replaced by the genuine inline; fourth_five_objects_admission_20260923.md: infection's "direct component arithmetic was replaced by existing vector/point helpers" before Matching.
- Full /Od call census of the render function (od_linear.txt, 43 distinct callees) mapped against production: every other helper/call matches (point_from_line3d x4, set_real_vector3d x3, matrix4x3_transform_vector x4, scale_vector3d, add_vectors3d, csmemmove + csmemset x2 via cseries.h #define, render_debug_* counts 14/7/6/2/1/1, tag_block_get_element x3, game_time_get x3, ...). The /Od fabs/acos/fmod two-level float wrappers are C++ <cmath> overload artifacts of the later build (float->double CRT), not source helpers; the /Od csnzprintf-style wrapper vs January `_snprintf` is decided by January's exact relocation. magnitude3d (0x432950) is the ONLY hand-expanded helper.
- mag3d.c end-to-end vs the REGENERATED split: gate EXACT 1/1; audit PASS 29/29 symbols 0 differ; one-unit objdiff 3.3.1 code 4182/4182 data 29278/29278; fake_match_scan 0 leads.
- Board census (helper_census.py): every build/base copy of _add_vectors3d/_point_from_line3d/_scale_vector3d/_set_real_vector3d/_magnitude3d/_magnitude_squared3d/_square_root has the SAME raw hash as January's single selected copy (17/22/64/22/67/92/83 definers; 2/5/15/3/17/36/21 of them already in Matching units). No novel emission class; no conflicting copy anywhere.
- Optional (non-blocking) mag3d_twopi.c = mag3d.c + `_pi*2.f` for the three raw double literals: EXACT, object IDENTICAL to mag3d.obj (39 sections).

## Patches written (verified to apply with `patch --binary`, post-images == tested files)
- scratch/w/review_collision_debug/symbols.json.collision_debug.patch : ONLY the line-19561 static row (the worker's patches/symbols.json.patch also carries BLOCKED breakable_surfaces / rasterizer_xbox / hardware_geometry rows and must NOT be applied wholesale for this unit).
- scratch/w/review_collision_debug/collision_debug.c.patch : radius -> magnitude3d(&collision_debug_vector) (post-image mag3d.c; CRLF preserved).

## VERDICT: approve = false (as proposed)
Everything the worker claimed about storage, split, surplus and link reproduces independently and is correct: the symbols.json static row is right (no PDB public at 0x28B2A8, no UNDEF anywhere in build/split or build/base, csplit regen changes only collision_debug.obj and only that storage class, audit PASS 29/29, objdiff 100/100). BUT the proposal says "No source change is needed" and asks to certify the object Matching while production still hand-expands the header helper magnitude3d (first-party /Od call 0x7b1dc6 -> 0x432950), which brief rule 6 / campaign rule 13 and the owner's 20260924 reconciliation ("no ... hand expansion ... admitted") forbid. The fix is measured and fully admissible: apply BOTH patches, then regenerate split + full ninja + stable sweep (expect +0 functions / 0 regressions; collision_debug stays EXACT 1/1 and gains 3 identical SELECT_ANY helpers), then flip config/config.json line 515 source/physics/collision_debug.c "NonMatching" -> "Matching". Admission record should list 7 surplus code COMDATs (the 4 existing + _magnitude3d/_magnitude_squared3d/_square_root), 3 SDK tables, 11 literals, all identical to January's selected copies and pair-link PASS.
Non-blocking: the three `6.2831854820251465` double literals are a folded-constant readout (implausible spelling); `_pi*2.f` is byte-identical (mag3d_twopi.c) if the owner wants it cleaned.
