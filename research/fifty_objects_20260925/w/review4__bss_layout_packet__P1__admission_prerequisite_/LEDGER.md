# Review ledger: rasterizer_lights P1 storage/.bss layout packet (wave 4, base 16542e46)

Reviewer slug: scratch/w/review4__bss_layout_packet__P1__admission_prerequisite_/
Worker slug under review: scratch/w/rasterizer_lights/ (LEDGER.md read in full)
Tree: git status clean at start and after every step; nothing outside this slug was written
(gate.py writes its usual scratch/_gate_<pid>.* temporaries; patch trials were done outside the repo).

## Probes / checks (hypothesis -> evidence -> result)

1. Patch hygiene. `git apply --check` on the CURRENT tree: P1_storage_layout.patch OK, P1_symbols_json.patch OK,
   the pair together OK, production.patch OK. Applying both P1 patches outside the repo (scratchpad copy) reproduces
   worker stg.c byte-for-byte (CRLF) and worker config/symbols.json byte-for-byte (CRLF, 2,182,981 B).
   My own in-place 6-line edit of symbols.json (python, CRLF split) == worker copy. Only lines 6213, 23227-23231 differ.
2. csplit baseline: regen with UNMODIFIED config -> 833 objs, byte-identical to build/split (build/split is current).
   csplit with P1 symbols.json -> 833 objs, ONLY source/rasterizer/rasterizer_lights.obj differs; == worker regen obj.
3. gate.py (current split): base.c 10 exact / 3 residual; p1.c 9 exact / 4 residual
   (_rasterizer_lens_flare_submit [reloc-identity]: the latch rename) -> source alone regresses.
4. cmp_r.py (gate logic vs REGEN split): p1.obj 10 exact / 3 residual (submit EXACT again); same residual set
   (submit_for_cluster, draw, reset). p1.obj sections == worker stg.obj (0 diffs). p1 vs base.obj: only the .bss owner
   key, the submit relocation names and the latch symbol differ.
5. object_audit copy (AUDIT_SPLIT_ROOT): p1 vs regen = .bss section ok, 41 January symbols, 0 differ; FAIL(3) = the three
   known code residuals only. base vs current = FAIL(9) (6 storage/offset rows). base vs regen (symbols.json alone)
   = FAIL(7): latch MISSING + both array offsets.
6. PDB storage (pdb_storage.py logic): current split/base 6 disagreements -> regen/P1 0. cachebeta publics: none of the
   6 names is public; no public symbol lies in the .bss range 0x466B80..0x4B82A5; the 4 public functions of the
   object are public; evaluate_corona_rotation absent (static). Data publics use the same address space as
   symbols.json file_offset (760/760 data publics match exactly), so absence is meaningful.
7. objdiff 3.3.1 one-unit (absolute paths): prod reproduces build/report.json row (only metadata/source_path differs).
   prod: code 1996/5244, data 335089/335089, 9 fns. p1+regen: IDENTICAL. base+regen (symbols.json alone) and
   p1+current (source alone): data 1484/335089, .bss 99.9997% -> a half-landing loses 333,605 data bytes.
8. VC7 .bss law re-measured independently in this TU (labA-E):
   labA both uninit, results2 declared first -> results@0 (declaration order inert for uninit);
   labB results2 `= {0}` only -> results@0; labC both init, results first -> results@0;
   labD both init, results2 first -> January; labE results `= {0}` only -> January.
   => with HCEX names, January's order needs results in the explicitly-initialised bucket (bss1) or both
   initialised with results2 declared first (bss2). Identical objects; bytes cannot choose between them.
9. STRIP TEST: results without `= {0}` -> results@0/results2@30464 (FAIL 2 symbols). warned without `= FALSE`
   -> hash bucket: results2@0, warned@262176, results@262184 ... (FAIL 4 + .bss size 333612). Both initialisers
   are load-bearing. `= FALSE` pre-exists on the latch; `= {0}` on results is the one new load-bearing token.
10. /Od (halo_cache_symbols.exe, later compiler with SSE and /GS): count@0xeaa0c0, params@0xeaa0c8,
    results@0xeb60c8, results2@0xeb83c8; latch byte at 0xecea08, NOT adjacent to the file statics. That is
    consistent with a function-static latch (supports the HCEX static-local form). Its .bss order uses another
    compiler's law, so it cannot decide bss1 or bss2.
11. HCEX compiland re-dumped (hcex_rl.txt): addresses results 0x16EE4F8 < results2 0x16F23F8 < params < count <
    warned (Static Local, unsigned char). Under the VC7 law the address order of UNINITIALISED statics is name-hash
    order, not declaration order, so "bss1 keeps HCEX declaration order" is overstated. The VC16 .bss law is not
    measured. I tried to calibrate it: a census found only 2 TUs usable (stack_walk_windows, ai_debug), and neither has
    an HCEX SHIP compiland. So HCEX cannot decide the initialiser either way.
12. Runtime test tools/test_rasterizer_lights_frame_runtime.py: prepare_body's relocation-name resolution simulated on
    the regen split and on p1.obj -> owner/size/sha ok, 0 unresolvable targets (static symbols keep their names).
13. January side of the regen split: all 13 January function normalized shas unchanged (draw park target 05fb3208
    unchanged); only _rasterizer_lens_flare_submit relocation NAMES change (latch). The draw park (base 25834daf) is
    unchanged by P1.
14. /W3 /Zs: 13 diagnostics before and after (identical set; one #line-space line number shifts by 2).
    fake_match_scan: 0 leads on both. provider_link: PASS (full) and "no new surplus" (baseline-restricted);
    surplus_identity: 11 COMDATs, 0 not identical (P1 surplus sections == base).
15. House rules: no header touched, no prototype, no float, no view cast, no new block scope (function-scope static
    local, same form as the landed dynavobgeom `static boolean warned = FALSE;`). The invented name
    local_lens_flare_error_printed is replaced by HCEX's `warned`.
