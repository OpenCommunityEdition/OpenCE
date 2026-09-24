# REVIEW source/rasterizer/xbox/rasterizer_xbox_dynavobgeom

approve: True

## per_object
[
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_dynavobgeom",
  "approve": true,
  "issues": "17/17 EXACT against the regenerated split. OBJECT AUDIT: PASS. All 19 surplus definitions are IDENTICAL, and the provider link passes in both orders. csplit changes only this object. Approved with the byte-inert, /Od-attested MAX amendment (admission_packet_amended.patch); the worker's packet is also admissible. Disclosed advisory debt: the consumer-local window extern of the genuine type, the pre-existing load-bearing quad-draw block that the escape law justifies, and the header parameter names."
 }
]

## checks
I re-ran every check myself in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_rasterizer_xbox_dynavobgeom\. Every probe is logged in LEDGER.md there, and REVIEW.md holds the notes.

1. **Patches.** All four worker patches and admission_packet.patch pass `git apply --check` at 954eebd2. Applied to a copy tree, the .c post-image is cmp-equal to cand_final.c and symbols.json is cmp-equal to the worker's copy (10 lines changed in place, CRLF kept).
2. **Gate against the current split.** gate.py gives 16/17. The only residual is `__rasterizer_dynamic_unlit_geometry_draw` [reloc-identity], which is the latch rename. The base .c is 17/17.
3. **Split regeneration.** I regenerated the split with csplit using the patched symbols.json and config.json. It produces 833 objects, and a byte-compare with build/split shows only rasterizer_xbox_dynavobgeom.obj differs. Control: csplit with the unmodified config reproduces build/split exactly.
4. **Gate against the regenerated split.** gate_r (gate.py reading GATE_SPLIT_ROOT) gives 17/17 EXACT.
5. **Object audit.** audit_r against the regenerated split: all 31 January sections ok, 31 January symbols with 0 differences, OBJECT AUDIT: PASS.
6. **Surplus.** My own script compares every candidate-only external definition with every January split definer (section_infos_equal, raw bytes and section flags). All 19 surplus definitions are IDENTICAL; 0 problems.
7. **Provider link.** 19/19 PASS in both link orders; SELECTED-PROVIDER LINK: PASS.
8. **NODUP census.** No NODUP definer for `_dot_product3d` (72 definers), `_vector_from_points3d` (77), `_real_alpha_to_pixel32` (6), `_real_argb_color_to_pixel32` (3) or the three SDK tables (133 each).
9. **Storage against cachebeta publics, checked by address.** pdb_storage.py matches by name only, so I checked addresses directly. There is no public anywhere in 0x465980..0x465B00, so the latch at 0x465A16 was file- or function-static. None of the nine wrapper offsets has a public. Control: the unlit draw at 0x14EE80 does have one.
10. **HCEX.** The unlit draw has `zero_plane` and `Static Local unsigned char warned`.
11. **/Od build, 0x7d82d0.** It calls `vector_from_points3d(0xdfb058 = camera.position, centroid, &[ebp-0x1c])` before new_group, then `-dot_product3d(0xdfb064 = camera.forward, &forward)`. The RTC records name `forward` (12 bytes) and `zero_plane` (16 bytes). The frame's declaration order is group, forward, zero_plane, which the candidate follows. The diff adds no new block scope.
12. **real_math.h semantics.** `vector_from_points3d` computes b - a, so the helper call is equivalent to the old hand-expanded subtraction.
13. **Warnings and fake-match scan.** /W3 /Zs gives 0 warnings in the unit; the 12 shared-header warnings are the same as base. fake_match_scan finds 0 leads.
14. **objdiff 3.3.1, one unit.** Data 525/525 and 16/17 functions. The quad draw scores 97.74, the same under-credit production already has.
15. **Admission audit.** tools/audit_object_admission.py already gives function_gap 0 for this unit through the semantic pipeline. I simulated it with complete=True, the patched rejections and the patched symbols.json: 0 contradicted, 0 revoked.
16. **Test and config references.** Only symbols.json and the .c mention the old latch name. tools/test_rasterizer_frame_statistics_owner.py is unaffected.
17. **Strip test and /Od check of the old `if`.** I tested `largest = MAX(triangle_count, largest)` against the /Od `?:` temporary at 0x7d8604. It is 17/17, and objeq shows the whole object IDENTICAL (51 sections). That makes it byte-inert, so the /Od shape and house rule 18 decide the spelling.
18. **Pre-existing quad-draw bare block.** Hoisting its five locals to function scope gives size 2976 instead of 2960, so the block is load-bearing. The escape law justifies it.
19. **Layer-2 contingency.** I compiled with a shadow real_math.h that drops the lock/unlock_global_random_seed prototype move, placed first on the include path (/showIncludes confirms it is used). The result is still 17/17, so this admission does not depend on that owner ruling.
20. **Amended packet.** The amended packet applies cleanly, and its post-image is cmp-equal to amended_final.c. Against the regenerated split it gives 17/17, audit PASS, surplus 19 IDENTICAL, provider link PASS, and objeq IDENTICAL to the worker's object.

## issues
No blocking issues. Every worker claim checked out except two, neither of which blocks admission.

**(1) The worker's disclosure (b) is wrong, and I amended it.**
- The worker said January's conditional store is reproduced only by the hand-written `if (triangle_count > largest) largest = triangle_count;`.
- `largest_dynamic_unlit_triangle_count = MAX(triangle_count, largest_dynamic_unlit_triangle_count);` also reproduces it. The whole object is IDENTICAL.
- The later /Od build (0x7d8604..0x7d8623) computes this through a `?:` temporary, which is the MAX expansion.
- I applied it: house rule 18 prefers cseries.h macros, and it is byte-inert.

**(2) One ledger statement in the worker's LEDGER.md is overstated.**
- The worker's LEDGER.md says January's object has UNDEF references to "every surplus literal".
- In fact only 2 of the 7 literals are referenced (global_d3d_device and parameters); all 5 `__real` constants are.
- The other 5 literals belong to the folded real_*_to_pixel32 COMDAT bodies, so the conclusion stands.

**Advisory debt, disclosed and not blocking:**
- **Window global.** The unit keeps a consumer-local `extern struct rasterizer_window_begin_parameters global_window_parameters;`. It uses the genuine complete type and is the same declaration the owner rasterizer_xbox.c uses. Seven Matching objects declare it the same way, and no header declares it.
- **Quad-draw bare block.** This block predates the diff and is load-bearing. The escape law justifies it, and the later /Od build is a dx9 body that cannot confirm or refute it.
- **Header parameter names.** rasterizer_xbox_dynavobgeom.h calls the unlit draw's parameters `bitmap`/`vertex_count`, while the definition uses HCEX's `primary_map`/`triangle_count`. This is cosmetic.

**Production changes.** Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_rasterizer_xbox_dynavobgeom\admission_packet_amended.patch. It concatenates four patches, each passing `git apply --check` at 954eebd2:
- **rasterizer_xbox_dynavobgeom.c.patch:** the worker's .c patch plus the MAX line. Its post-image is amended_final.c.
- **symbols.json.patch:** unchanged from the worker. It edits line 23161 to `?warned@?1??_rasterizer_dynamic_unlit_geometry_draw@@9@9` with "static": true, and adds "static": true to lines 5705, 5706, 5711 and 5713-5718. The full copy is scratch\w\review3_rasterizer_xbox_dynavobgeom\config\symbols.json.
- **config.json.patch:** sets the unit to Matching.
- **object_admission_rejections.json.patch:** retires the entry.

**Integration notes:**
- The .c patch and symbols.json must land together. Ninja reruns csplit, and only this object changes.
- No header changes, so there is no consumer sweep list.
- The worker's admission_packet.patch is an equally valid fallback: it produces an identical object.
