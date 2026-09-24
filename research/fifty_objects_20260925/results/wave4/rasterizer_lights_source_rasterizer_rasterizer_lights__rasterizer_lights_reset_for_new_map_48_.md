# source/rasterizer/rasterizer_lights :: _rasterizer_lights_reset_for_new_map (48)

## verdict
BLOCKED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\fin_owner2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\owner_gated_reset.patch"
]

## production_changes
OWNER-GATED; do not self-land. scratch/w/rasterizer_lights/owner_gated_reset.patch applies on top of production.patch. It changes the first memset length to `(MAXIMUM_LIGHTS_PER_MAP+1)*sizeof(struct lens_flare_occlusion_test_results)` (0x7722) and adds a `BUG (preserved for exact matching)` comment. The candidate file is fin_owner2.c.

## evidence
- fin_owner2 vs the regen split: 11 exact / 2 residual, and reset is EXACT (48). audit_r: 41/41 January symbols. objdiff one-unit: reset 100, matched code 1996 -> 2044.
- New fact: once P1 restores January's .bss layout, the rebuilt object places local_lens_flare_parameters directly after the results array, exactly as January does. The 34 overrun bytes therefore hit params[0] bytes 0..33 in both builds.
- This removes the premise of the 2026-09-22 canonical exclusion, which rejected the overrun because "COFF places a different array immediately after the overwritten one".
- The lane C 5.4 harmlessness proof then applies verbatim: count is reset in the same function, every read is count-bounded, and records become readable only after a full memcpy.
- The alternative spelling from earlier blobs, `sizeof(array)+sizeof(array[0])`, compiles identically.

## blockers
Authentic-bug class (buffer overrun). It needs an explicit owner ruling admitting the BUG-commented overrun. It depends on P1 landing first.

## reopen_criteria
Owner ruling that admits the preserved overrun, now that the neighbour-mismatch objection is gone.

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
