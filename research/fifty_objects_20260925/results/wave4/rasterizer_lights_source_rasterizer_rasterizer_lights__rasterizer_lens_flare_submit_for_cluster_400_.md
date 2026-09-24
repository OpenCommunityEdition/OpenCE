# source/rasterizer/rasterizer_lights :: _rasterizer_lens_flare_submit_for_cluster (400)

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\od_submit_for_cluster.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\LEDGER.md"
]

## production_changes
none

## evidence
- New characterisation:
  - January decodes the marker direction per component in i,j,k order: fild/fmul(1/127)/fstp for each. All three char loads (j->ecx, k->edx, i->eax) are hoisted above the first store.
  - Our set_real_vector3d evaluates the arguments right-to-left (k,j,i) on the x87 stack and then stores i,j,k.
  - Direct component statements (prior c1-c5 and my x3 C4204 initializer) reproduce January's x87 order but not the load hoisting, because direction's address is taken.
  - January's own hs_runtime set_real_vector3d(&extents, x1-x0, ...) is also right-to-left.
  - The later /Od 0x82f710 calls set_real_vector3d with the three products and keeps an otherwise-unused const 1/127 local [ebp-0x58] between parameters and direction.
- Measured inert:
  - const, non-const and outer-block scale locals (kc_const, kc_var, kc_outer, s1).
  - set_real_vector3d(raw chars) followed by scale_vector3d (x4): loads hoisted, but filds still k,j,i.
  - Dead-local oracle D=0..17: inert.
  - TU-name oracles: inert.
- The loop-tail load placement (movzx after `add esp; inc`) also still differs.

## blockers
An instruction-order tie in the inline-argument evaluation of the direction decode. No first-party or law-backed spelling reproduces 'loads hoisted + per-component compute/store'.

## reopen_criteria
Reopen with either:
- first-party evidence of a different January decode form, for example a byte-direction helper whose parameters are integers; or
- a decoded VC7 inline-argument ordering key that produces i-first evaluation with sunk x87 work.
Do not replay the c1-c5, s1, x3, x4 or kc_* shapes.

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
