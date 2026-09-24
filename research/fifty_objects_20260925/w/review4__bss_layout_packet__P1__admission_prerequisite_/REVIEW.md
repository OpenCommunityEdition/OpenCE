# REVIEW: source/rasterizer/rasterizer_lights, P1 storage/.bss layout packet (admission prerequisite)

Verdict: **APPROVE P1 with conditions**, zero credit. P1 fixes admission hygiene: object_audit goes from FAIL(9) to
FAIL(3), and the 3 remaining failures are the known code residuals. PDB storage disagreements go from 6 to 0. Every
metric is unchanged: 10 exact, code 1996/5244, data 335089/335089, and the draw park hash is the same.
Full evidence is in LEDGER.md (15 checks) in this slug.

## What was verified independently

- The patches apply cleanly to 16542e46 (`git apply --check`): P1_storage_layout.patch, P1_symbols_json.patch, the
  two together, and production.patch. Applying the two P1 patches gives worker stg.c and worker config/symbols.json
  byte for byte (CRLF). I re-derived the 6 in-place line edits myself and got the same file.
- csplit: a baseline regen reproduces build/split exactly, 833/833 objects. The P1 regen changes only
  rasterizer_lights.obj.
- Candidate vs the regen split: 10 exact / 3 residual, the same set as production. object_audit shows the .bss
  section ok and 41/41 January symbols matching. PDB storage has 0 disagreements. objdiff 3.3.1 gives metrics
  identical to production. provider_link PASSES, there is no new surplus, and surplus_identity finds 0 differing.
  fake_match_scan finds 0 leads. /W3 shows the same 13 diagnostics.
- January side: all 13 function normalized hashes are unchanged in the regen split. The draw park target is still
  05fb3208 and the park base is still 25834daf. Only `_rasterizer_lens_flare_submit`'s relocation names change,
  because of the latch.
- tools/test_rasterizer_lights_frame_runtime.py resolves its relocations by symbol name. Simulated on the regen
  split and on p1.obj, it still resolves (the static symbols keep their names).
- Storage: none of the 6 names is a cachebeta public. No public lies in the object's .bss range. Data publics share
  symbols.json's address space (760/760 match), so absence does mean static. The names come from HCEX: 4 File
  Statics, the Static Local `warned` (unsigned char) of rasterizer_lens_flare_submit, and the static
  lens_flare_evaluate_corona_rotation_function. The atlas has no data rows here.

## The load-bearing constructs (STRIP TEST)

- `local_lens_flare_occlusion_test_results[...] = {0};` is load-bearing. Without it, results goes back to @0.
  I re-measured the VC7 .bss law in this TU:
  - Uninitialised statics come first in name-hash order. Declaration order has no effect (labA).
  - Initialised statics follow in declaration order (labB-E).
  - With HCEX's names, January's order results2@0 / results@262176 therefore requires results to be explicitly
    initialised (bss1), or both arrays initialised with results2 declared first (bss2). The two give identical
    objects, so bytes cannot choose between them.
  - This is the same evidence class as the landed .bss-law initialisers: canonical 69faf435, the render_media /
    environment_fog / shadows owners, and this lane's rasterizer_xbox_shadows statics.
  - Unlike the held models .bss item, it contradicts no first-party structural form. HCEX has these arrays as File
    Statics at file scope, and P1 keeps them there.
  - I accept it as attested reconstruction, not decoration. The neighbouring declarations already carry
    `= {0}` / `= 0`.
- `static boolean warned = FALSE;`: the `= FALSE` is also load-bearing (without it, warned moves to @262176). But it
  already exists on the latch today, and it is the landed dynavobgeom idiom. The static-local form itself has
  first-party support: HCEX Static Local, and the /Od latch at 0xecea08 is not adjacent to the file statics. It
  replaces the invented name `local_lens_flare_error_printed`, a house-rule 8 improvement.

## Conditions and issues for the integrator

1. **Land atomically.** Source without symbols.json, or symbols.json without source, turns `_rasterizer_lens_flare_submit`
   into [reloc-identity], and objdiff data drops from 335089 to 1484/335089 (.bss 99.9997%): -333,605 data bytes.
   Apply P1_storage_layout.patch and the 6 symbols.json line edits in one commit. Use the full copy at
   scratch/w/rasterizer_lights/config/symbols.json, lines 6213 and 23227-23231, and never re-serialise.
2. Scope: this review covers P1 only. production.patch also bundles P2 (the draw respelling), the parked.json
   re-baseline and the owner-gated reset. Those are not reviewed here, so do not take production.patch on this
   verdict. Use the two split P1 patches.
3. Rationale correction (disclosure, not a blocker). The worker says "bss1 keeps HCEX declaration order", but HCEX's
   address order results<results2 is not proof of declaration order. Under the VC7 law, uninitialised statics are
   ordered by name hash, and VC16's law is unmeasured; the census found no calibration TU with an HCEX SHIP
   compiland. bss2 (both `= {0}`, results2 declared first) is equally consistent with the bytes. bss1 is chosen as
   the minimal edit that keeps the tree's existing declaration order. If VC16 used the VC7 law, the 2011 source lacked
   the initialiser, but every January-exact spelling deviates from that inferred source in some way. Record this in
   the object ledger.
4. P1 moves no metric. It is not an object admission: submit_for_cluster, draw and reset stay residual, and the reset
   overrun is owner-gated.
