# REVIEW source/rasterizer/rasterizer_lights :: P1 storage/.bss layout packet (admission prerequisite)

approve: True

## per_object
[
 {
  "unit": "source/rasterizer/rasterizer_lights (P1: P1_storage_layout.patch + P1_symbols_json.patch)",
  "approve": true,
  "issues": "Zero credit; must land both halves in one commit (a half-landing costs 333,605 objdiff data bytes and one reloc-identity row). The 'HCEX declaration order' rationale is overstated: bss2 is an equivalent alternative, so disclose it in the ledger. production.patch's P2 / parked.json / reset parts are out of scope and not approved here."
 }
]

## checks
All work is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4__bss_layout_packet__P1__admission_prerequisite_\ (LEDGER.md has 15 checks, REVIEW.md has the verdict). git status was clean before and after.
(1) Patch hygiene:
- `git apply --check` passes on 16542e46 for P1_storage_layout.patch, P1_symbols_json.patch, the two together, and production.patch.
- Applying both P1 patches outside the repo reproduces worker stg.c and worker config/symbols.json byte for byte (CRLF).
- I re-derived the 6 in-place symbols.json line edits (6213, 23227-23231) myself and got the same file. Nothing is re-serialised.
(2) csplit:
- A regen with the unmodified config equals build/split (833/833).
- A regen with the P1 symbols.json changes only rasterizer_lights.obj, and matches the worker's regen.
(3) gate.py against the current split: base.c gives 10 exact / 3 residual. p1.c gives 9/4, because `_rasterizer_lens_flare_submit` becomes [reloc-identity].
(4) The same gate logic against the regen split: p1 gives 10 exact / 3 residual, with the same residual set (submit_for_cluster, draw, reset). p1.obj sections equal worker stg.obj.
(5) object_audit copy with AUDIT_SPLIT_ROOT:
- p1 vs regen: .bss section ok, 41/41 January symbols match, FAIL(3) = the known code residuals only.
- base vs current: FAIL(9).
- base vs regen (symbols.json alone): FAIL(7).
(6) PDB storage: 6 disagreements on the current tree, 0 with P1+regen.
- None of the 6 names is a cachebeta public. No public lies in the .bss range 0x466B80-0x4B82A5.
- Data publics share the symbols.json address space (760/760 match), so absence means static.
(7) objdiff 3.3.1, one-unit reports:
- prod reproduces the build/report.json row (only metadata source_path differs).
- p1+regen is identical to prod: code 1996/5244, data 335089/335089.
- A half-landing (either half alone) gives data 1484/335089 (.bss 99.9997%).
(8) VC7 .bss law re-measured in this TU (labA-E): declaration order has no effect on uninitialised statics; only results `= {0}` (bss1) or both `= {0}` with results2 first (bss2) give January's layout.
(9) STRIP TEST: without results `= {0}`, results goes back to @0. Without `= FALSE`, warned moves to @262176. Both are load-bearing.
(10) /Od (halo_cache_symbols.exe) layout read: the latch byte at 0xecea08 is not adjacent to the file statics, which is consistent with a function-static latch. Its .bss order comes from a different compiler, so it does not decide bss1 vs bss2.
(11) HCEX compiland re-dumped with DIA2Dump -compiland: 4 File Statics plus the Static Local `unsigned char warned` of rasterizer_lens_flare_submit. The atlas has no data rows for this range.
(12) Runtime test tools/test_rasterizer_lights_frame_runtime.py: its relocation-name resolution, simulated on the regen split and p1.obj, still resolves.
(13) January side of the regen split: all 13 function normalized hashes are unchanged, including the draw park target 05fb3208. The park base 25834daf is unchanged by P1.
(14) /W3 /Zs gives the same 13 diagnostics.
- fake_match_scan finds 0 leads.
- provider_link PASSES, both full and restricted to new surplus.
- surplus_identity finds 11 COMDATs, 0 differing, and P1's surplus equals base.
(15) House-rule review:
- No header, prototype, float, view cast or new block scope.
- The function-scope static local has the same form as the landed dynavobgeom `warned`.
- The invented name local_lens_flare_error_printed is replaced by HCEX's `warned`.

## issues
APPROVED for P1 only, with conditions. P1 earns zero credit: every metric is unchanged.

1. MUST LAND ATOMICALLY. Apply source/rasterizer/rasterizer_lights.c from scratch/w/rasterizer_lights/P1_storage_layout.patch and the 6 symbols.json line edits (P1_symbols_json.patch, or line ops from the full copy scratch/w/rasterizer_lights/config/symbols.json, lines 6213 and 23227-23231) in the same commit. Either half alone does two things:
   - `_rasterizer_lens_flare_submit` becomes [reloc-identity].
   - objdiff 3.3.1 data drops from 335089 to 1484/335089 (.bss 99.9997%), a loss of 333,605 data bytes.
   The worker reported only the code half of this.
2. SCOPE. production.patch also bundles P2 (the draw respelling), the parked.json re-baseline of `_rasterizer_lens_flares_draw`, and the owner-gated reset. None of those are reviewed here, so do not take production.patch on this verdict; use the two split P1 patches.
3. RATIONALE CORRECTION (disclosure, not a blocker). The worker says "bss1 keeps HCEX declaration order", but HCEX's address order results<results2 does not prove declaration order:
   - Under the VC7 law, uninitialised statics are ordered by name hash. I re-measured this: declaration order has no effect.
   - VC16's .bss law is unmeasured. My census found no TU with an HCEX SHIP compiland to calibrate it.
   - bss2 (both arrays `= {0}`, results2 declared first) is byte-identical and equally consistent with the evidence. bss1 is simply the minimal edit that keeps the tree's existing declaration order.
   - If VC16 followed the VC7 law, the 2011 source lacked the initialiser. But every January-exact spelling departs from that inferred source, so it cannot be used against bss1.
   Record this ambiguity in the object ledger.
4. POLICY BASIS for the load-bearing `= {0}`. The strip test fails without it. I accept it anyway, as a reconstruction attested by January's own .bss layout plus the VC7 law I re-measured:
   - With the HCEX names, results must be in the initialised group.
   - It follows the landed precedent: 69faf435, the render_media / fog / shadows zero-initialised owners, and this lane's rasterizer_xbox_shadows statics.
   - Unlike the held models .bss item, it contradicts no first-party structure: HCEX has these as File Statics.
   - The `= FALSE` on warned is also load-bearing, but it already exists on the latch and matches the landed dynavobgeom idiom.
5. P1 is not an object admission. submit_for_cluster (x87 order), draw (+0x31c / +0x738) and the owner-gated reset stay residual. After P1, object_audit fails only on those 3 code sections.

No amended patch is needed; the worker's P1 patches are verified as-is.
