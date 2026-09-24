# source/rasterizer/rasterizer_lights :: storage/.bss layout packet (P1, admission prerequisite)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\stg.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\P1_storage_layout.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\P1_symbols_json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\split\\source\\rasterizer\\rasterizer_lights.obj"
]

## production_changes
Part of C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_lights\production.patch (git apply --check clean). The pieces are also split out: P1_storage_layout.patch and P1_symbols_json.patch. The edited full copy is scratch/w/rasterizer_lights/config/symbols.json, with 6 lines edited in place (CRLF kept, no re-serialise).
(1) rasterizer_lights.c: `local_lens_flare_occlusion_test_results[MAXIMUM_LIGHTS_PER_MAP] = {0};` (results2 stays uninitialised).
(2) Remove the file-scope `local_lens_flare_error_printed`. Use HCEX's function-static `static boolean warned = FALSE;` as the first line of rasterizer_lens_flare_submit (and use it in the error branch).
(3) symbols.json: add "static": true on the rows for _lens_flare_evaluate_corona_rotation_function (line 6213) and _local_lens_flare_occlusion_test_results2, _local_lens_flare_occlusion_test_results, _local_lens_flare_parameters and _local_lens_flare_count (lines 23227-23230).
(4) Line 23231: rename _local_lens_flare_error_printed to `?warned@?1??rasterizer_lens_flare_submit@@9@9` with "static": true.
The edit is .c-local; no header is touched. Its csplit regen changes only rasterizer_lights.obj.

## evidence
- Layout: January's .bss is results2@0, results@262176, params@292640, count@333600, latch@333604. Ours is results@0, results2@30464.
- The VC7 .bss law was measured in a lab. With both arrays uninitialised under their HCEX-attested names (HCEX compiland: local_lens_flare_occlusion_test_results[0x380], ..._results2, local_lens_flare_parameters[0x400], local_lens_flare_count, static-local `warned`), the order is always results first (base, bss3).
- `= {0}` on results alone (bss1) gives January's layout, and so does the bss2 form (both initialised, results2 declared first).
- bss1 is chosen because it keeps HCEX's declaration order (results, results2).
- The static-local latch lands at the same offset 333604. VC7 names it ?warned@?1??rasterizer_lens_flare_submit@@9@9, following the dynavobgeom precedent.
- The storage truth comes from cachebeta publics: none of the 6 symbols is public.
- csplit regen (build/tools/csplit.exe -i cachebeta.exe -p slug/config -o slug/split): rc 0, 833 objects, and only rasterizer_lights.obj differs from build/split.
- Against the regen split:
  - gate_r: 10 exact, the same as the base, with no loss.
  - audit_r: 41 January symbols, 0 differ.
  - PDB storage: 0 disagreements.
  - objdiff 3.3.1 one-unit data: 335089/335089.
- Against the CURRENT split, rasterizer_lens_flare_submit reads [reloc-identity] until the symbols.json line lands, so the source and symbols.json changes must land together.

## blockers
Zero code credit. The object stays blocked by the draw and submit_for_cluster residuals and by the owner-gated reset. The integrator must run the csplit regen and the full sweep. tools/test_rasterizer_lights_frame_runtime.py relies on relocation names _local_lens_flare_count and _local_lens_flare_parameters; this packet leaves both names unchanged (it only adds static storage).

## reopen_criteria
n/a (ready to land).

## task notes
No function became strict-exact without an owner ruling, and the object stays blocked by the draw function, submit_for_cluster, and the owner-gated reset.

**Ready to land** (zero credit, park re-baseline): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_lights\production.patch. It passes git apply --check and touches three files:
- source/rasterizer/rasterizer_lights.c: the P1 storage/layout packet plus the P2 draw changes.
- config/symbols.json: 6 in-place line edits; the full edited copy is scratch/w/rasterizer_lights/config/symbols.json.
- config/parked.json: the draw park re-baseline.

Integrator checklist:
- Run the csplit regen after applying: only rasterizer_lights.obj changes, verified in the slug with 833 objects.
- No header is touched and only this TU changes, so no consumer sweep is needed beyond the normal full ninja.
- Verified at the regen split: symbols match 41/41, PDB storage has 0 disagreements, and the unit keeps 10 exact rows.
- Every surplus COMDAT is identical to production, so the provider-link status does not change.

**Owner-gated:** scratch/w/rasterizer_lights/owner_gated_reset.patch makes reset exact (48). It needs an owner ruling on the preserved overrun; the new evidence is that P1 makes the overrun's neighbour identical to January's.

Laws re-confirmed this wave:
- A35 leaf aging is driven by the first definition: an /Od-attested declaration initializer flipped an x87 frame-scalar operand order.
- A44: dot_product3d argument order is canonicalised and inert.
- The VC7 .bss law: uninitialised statics come first, and the HCEX names force `= {0}` on results.

One process fact for later workers: Windows file names are case-insensitive, so lab files S1..S5 overwrote s1..s3. The ledger records this, and the affected labs were redone as kc_*.

A one-unit objdiff 3.3.1 report reproduces build/report.json exactly and was used for all fuzzy numbers (slug od_proj/).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_lights\LEDGER.md
