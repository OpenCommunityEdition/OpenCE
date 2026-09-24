# REVIEW source/physics/collision_bsp (whole object: admission) - review of ADMISSION_FIX_PROPOSED storage packet

approve: True

## per_object
null

## checks
All work is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_collision_bsp__whole_object__admission_\ (REVIEW.md holds the full log). HEAD is 16542e46 and the tree was clean before and after.

1. gate.py --all on the worker's st1.c: exact 27, residual 3, unwritten 0. The residuals are _bsp3d_test_pill_recursive (sha), _bsp3d_test_sphere_recursive (sha) and _collision_surface_test_sphere (size 848 vs 880). The HEAD source gives the same 27/3. st1.obj and base.obj have 69/69 sections equal under section_infos_equal. The only symbol-table difference is _collision_bsp_usage_times, storage 2 -> 3.
2. The patch: `git apply --check scratch/w/collision_bsp/production.patch` is OK against the current tree. Its index b4438fa9 equals the HEAD blob. Applying it to HEAD (CRLF) reproduces st1.c byte-for-byte.
3. symbols.json: I made the same edit myself on a copy of HEAD config/symbols.json, in place at line 23117 with CRLF kept (+16 bytes). The result is byte-identical to the worker's copy at scratch/w/collision_bsp/config/symbols.json.
4. Emulated csplit (build/tools/csplit.exe, output in my slug): the control split equals build/split for 833/833 objects. The edited split differs only in source/physics/collision_bsp.obj. There, 35/35 sections are equal and the only change is _collision_bsp_usage_times 2 -> 3.
5. object_audit (slug copy pointed at the emulated split): st1.obj has 35 January symbols with 0 differences. The only DIFF rows are the 3 residual .text sections. Controls show the two edits must land together: base.obj vs the emulated split fails on storage 3/2, and st1.obj vs the control split fails on 2/3.
6. Storage truth:
   - `_collision_bsp_usage_times` is not a cachebeta.pdb public. The collision_bsp compiland is present: its external functions are public and its statics are absent, so the oracle holds in this TU. The sibling `_collision_usage_times` is also non-public and already static in symbols.json and in collisions.c.
   - PDB storage check on the candidate vs the emulated split: 0 disagreements (production has 1).
   - relocs.json: offsets 0x456EB0..0x456EBC are referenced only by _collision_bsp_test_vector (+0,+4) and _collision_bsp_test_sphere (+8,+0xC).
   - No other split object names the symbol. A grep of all of source/ and the headers finds it only in collision_bsp.c, with no extern declaration anywhere.
   - HCEX 2011: the collision_bsp compiland dump (DIA2Dump by full module path) has no file-scope data, so it neither confirms nor contradicts. The atlas lists functions only.
7. objdiff-cli 3.3.1 single-unit projects: the emulated pair (split_emu vs st1) and the control pair (split_ctrl vs base) both give code 7134/10292, data 176/176 (100%) and 27/30 functions. This matches build/report.json.
8. surplus_identity (slug copy run on st1.obj): 21 candidate-only code COMDATs, all IDENTICAL to January's selected providers.
9. provider_link on st1.obj (temp files under my slug): FAIL (2), both pre-existing and disclosed by the worker:
   - _cross_product2d vs actor_combat: the held P1 item.
   - _plane2d_distance_to_point vs geometry: a NODUP hand copy.
   The storage edit touches no external surplus.
10. /W3 /Zs warnings: 22 on base, 22 on the candidate, identical multisets.
11. STRIP TEST on the only remaining construct, the `= { 0 }` on the now-static object:
    - Without the initialiser the object still gates 27/3 with identical function hashes and passes the owner-keyed audit (0 symbol diffs). Only the .bss section index moves.
    - So the initialiser is not load-bearing. It is pre-existing production text, and the minimal one-keyword diff is fine.
12. parked.json: all 3 collision_bsp parks keep identical size, relocation count and normalized_sha256 on both the target side (ctrl, emu, manifest) and the base side (base, st1, manifest), so no park goes stale. There are no collision_bsp entries in object_admission_rejections.json, and no semantic, semantic-data or ownership entry names the symbol.
13. House rules and section 7/8/9 blockers: the diff adds one `static` keyword. It adds no header change, consumer prototype, view cast, hand-expanded helper, float, new name, new aggregate or new COMDAT. There is no new block scope, so the /Od declaration-order check does not apply. It follows the admitted collisions.c idiom.
14. Informational only (not part of this proposal): the worker's geometry_provider_repair.patch passes git apply --check and round-trips to g1.c. The geometry gate rows are identical and all 61 geometry .text hashes are unchanged. Linking st1.obj with g1.obj gives 0 LNK2005 in both orders; linking with production geometry.obj gives 1 LNK2005, so the check discriminates.

## issues
The storage packet itself has no blocking issues. The object stays incomplete, and the integrator should note:

(a) What to apply:
   - production.patch (`static` on collision_bsp_usage_times at source/physics/collision_bsp.c line 305).
   - The config/symbols.json edit at line 23117, in place: append `, "static": true` before ` }`, keep CRLF, do not re-serialise. It is identical to scratch/w/collision_bsp/config/symbols.json.
   - A csplit-only regen. Expect only build/split/source/physics/collision_bsp.obj to change; its emulated sha256 is 35f9f040674643c5...
   - The source patch and the symbols.json edit must land together; either one alone introduces a storage mismatch.
   - There is no header change, so no consumer sweep is needed.

(b) The object remains NOT complete:
   - 3 residual .text sections: _collision_surface_test_sphere (the owner-held SSE __asm class) and the two bsp3d parity-bit residuals (sphere and pill).
   - 2 LNK2005 errors in the selected-provider link: _cross_product2d (held P1) and _plane2d_distance_to_point (geometry NODUP hand copy).

(c) This review does NOT approve the worker's separate geometry_provider_repair.patch. My light check matched the worker's claims, but it is a different unit and needs its own review.

(d) Disclosed pre-existing item, not introduced by this diff: the `struct collision_bsp_usage_times {vector; sphere}` aggregate and its descriptive name are not first-party attested.
   - The bytes cannot distinguish the struct from two loose statics: LARGE_INTEGER forces 8-byte alignment either way, and no assert string names it.
   - It has no pad members (law j), its layout agrees with January's relocations, and it mirrors the admitted collisions.c sibling.
   - A final whole-object admission should disclose this.
