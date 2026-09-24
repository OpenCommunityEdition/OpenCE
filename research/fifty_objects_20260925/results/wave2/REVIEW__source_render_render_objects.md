# review source/render/render_objects
approve=True

## per_object
[
 {
  "unit": "source/render/render_objects",
  "approve": true,
  "issues": "22/22 strict EXACT against a split carrying the symbols.json rename. object_audit PASS 41/41, surplus 9/9 identical, provider_link PASS in both orders (with the packet's action_vehicle), pdb_storage 0 disagreements. HCEX attests the function-static scope, and the January .bss layout supports the uninitialised form. Conditional on the whole packet landing plus a csplit re-run; the status flip is the integrator's step."
 },
 {
  "unit": "source/ai/action_vehicle",
  "approve": true,
  "issues": "Packet component. Removes the NODUP hand copy and its macro rename. The object is section-identical to production except for COMDAT selection (NODUP becomes ANY). Still 17/17, object_audit PASS, provider_link PASS. It is mandatory for render_objects to link."
 },
 {
  "unit": "source/objects/object_lights",
  "approve": true,
  "issues": "Packet component only; not claimed complete. The object is raw-identical to production. The per-unit OBJECTS_H_FILE define is load-bearing for the January-owned c:\\ literal. Its surplus _object_get_bounding_sphere copy is still not identical to January's selected copy, as in January itself, and needs the owner ruling the worker raised."
 }
]

## checks
All checks were my own, independent runs. Full log: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_render_objects\LEDGER.md. Review notes: REVIEW.md in the same directory.
(1) Packaging. All 5 patches pass `git apply --check` against HEAD 931ed8dc, one at a time and as a set. Each file in final/ equals HEAD plus exactly the patch hunks, with CRLF kept. The symbols.json copy differs from HEAD in one line only (23251, edited in place).
(2) Own csplit. I ran build/tools/csplit.exe with the HEAD config plus the edited symbols.json over all 833 objects. Only source/render/render_objects.obj changed. A control csplit with the HEAD config was 833/833 identical to build/split.
(3) Own shadow-tree compile harness (rvcomp.py). It uses build.ninja's cflags and exact source path, with cwd set to the shadow tree. Controls for render_objects, action_vehicle and object_lights were identical to build/base (all sections except .debug$S, raw relocations, and the raw symbol table including aux records).
 - Candidate render_objects: 22/22 EXACT against the new split.
 - Against the production split: 21/22. The one residual, _object_get_cached_render_lighting, differs in relocation target names only.
 - gate.py --source final/render_objects.c with production headers: 21/22, same residual.
(4) Whole-object admission, run with unmodified copies of the scratch/tools scripts in a mirror root (new split plus candidate objects):
 - object_audit: PASS, 41/41 January symbols. .bss is 1150/1150, owned by ?lighting_storage@?1??object_get_cached_render_lighting@@9@9 at offset 0.
 - surplus_identity: 9 COMDATs, 0 not identical. _object_get_bounding_sphere is IDENTICAL to action_vehicle's copy.
 - provider_link: PASS, 26 symbols, both orders.
 - pdb_storage: 0 disagreements (production has 1).
 - Controls in the real tree: base render_objects, and the candidate render_objects against the production action_vehicle, both FAIL with LNK2005 on _object_get_bounding_sphere in both orders.
 - action_vehicle with the packet: audit PASS, provider_link PASS. The only raw change is COMDAT selection NODUP to ANY. The 7 other users pair-link PASS with it.
(5) objdiff 3.3.1 mini project. Production and candidate are both code 5437/5437 and data 4678/4678, with .bss, .data, .rdata and .text at 100%.
(6) Board sweep. I recompiled all 381 source/ cl units from the candidate tree; the 211 libs/ edges are unaffected. rowcmp.py compared per-function section_infos_equal rows and owner-keyed sections, base against candidate:
 - ZERO row changes, apart from render_objects' _object_get_cached_render_lighting going from residual to EXACT.
 - Content changes only in the 7 _object_get_bounding_sphere users (literal c:\ to ..\), plus _physics_update_old and _collision_move_point, both residual before and after.
 - About 55 other units differ only in internal $L label numbering. Control tree0 compiles are identical to base, so this is caused by the packet.
 - objdiff over the 66 raw-changed units: matched code, data and function counts are unchanged.
(7) Strip tests:
 - Production objects.h: the surplus copy is DIFFERENT (Rule 6 fails).
 - No object_lights define: object_audit shows January-owned 33-byte .rdata MISSING.
 - The uninitialised function static is the plainer spelling. The worker's L3 (= { 0 }) is refuted by the .bss layout, and L1 is byte-equal.
(8) Evidence:
 - My DIA2Dump run of HCEX.pdb shows 'Static Local, Type: struct render_lighting, lighting_storage' in object_get_cached_render_lighting. No global of that name exists; render_object_globals is a File Static.
 - cachebeta publics do not list lighting_storage.
 - January relocations to it come only from _object_get_cached_render_lighting. No January or base object references it from outside the unit.
 - The objects.h literal census (litcensus.py) reproduces the worker's F1 exactly.
 - symbols.json already has 6 `?x@?1??fn@@9@9` + "static": true precedents.
(9) Interaction probe. The held collisions 'broad route', rebuilt on the current real_math.h, gives collisions 20/20 both without and with the packet.
(10) Warnings are unchanged. fake_match_scan: 0 leads on the 4 files. No parks or admission rejections for render_objects.

## issues
No blocking issues. Conditions and notes for the integrator:
(a) Apply the packet as one unit. Apply all 5 patches (scratch/w/bounding_sphere/patches/objects_h.patch, action_vehicle.patch, object_lights.patch, render_objects.patch, symbols_json.patch), then re-run csplit.
 - Without action_vehicle.patch, render_objects hits LNK2005.
 - Without objects_h.patch, the surplus copy is not identical to January's selected copy.
 - objects.h without object_lights.patch drops a January-owned object_lights .rdata literal.
(b) The config.json status flip for render_objects (NonMatching to Matching) is not in the packet. It is the integrator's admission step.
(c) The packet moves no objdiff bytes for render_objects: production is already 5437/5437 code and 4678/4678 data. The gain is the strict object admission only.
(d) Side effects to record (not exactness losses):
 - Internal $L label renumbering in about 55 objects.h consumer objects.
 - Codegen changes in two already-residual functions, _physics_update_old and _collision_move_point.
 - objdiff fuzzy drops: collisions 99.54777 to 99.541794, physics 87.95672 to 87.95467. Matched bytes are unchanged.
(e) The objects.h comment explains the mechanism as 'found it beside themselves'. That is a hypothesis: January's .c literals use upper-case SOURCE, while this header literal uses lower-case source. The literal value itself is proven by the census. Optionally reword the comment to cite the census.
(f) object_lights is function- and data-exact but still not admissible. Its _object_get_bounding_sphere copy references the c:\ literal, as January's own object_lights did, which needs an owner ruling; the packet does not claim it.
(g) The optional evaluate_seat patches were not part of this verdict.
