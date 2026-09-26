# Decal clip: counter spill to actual ECX reload

No production changes, source controls, exact close, or credit. The inherited S4
candidate remains 1776 bytes / 571 instructions / frame `0x6c`, one adjacent-load
swap from January. Its normalized body hash remains
`1778c1f9b5a08eb6dd00df321dbd4b6a6192d0207d067312eaa116084913abaf`.

The previously uncertain IR mapping is now established end to end:

1. The spilled counter opcode-`0x21` node is the final `inc ecx` at `+0x4ee`.
2. At `0x10756db4`, opcode-table flags `0x442` admit that node to a later
   memory-operand registerization path. `0x1075703e` returns register ID 2.
   The live call at `0x10756f39` passes `(INC, counter-memory-operand, 2, 1)`.
3. The callee constructs an opcode-1 counter reload immediately before INC,
   using `0x10707c52` to splice the list. The predecessor is already EDGE_LOAD.
   A matching store is constructed immediately after INC.
4. The actual encoder buffer at `0x10751347` identifies the same new reload
   node as bytes `8b 4d ec` at `+0x4e5`. EDGE_LOAD emits `8b 74 96 08` at `+0x4e1`.
   The same-run store-constructor pointer emits `89 4d ec` at `+0x4f5`.
   The later scheduler rebuild preserves EDGE_LOAD -> COUNTER_RELOAD adjacency.

This proves insertion immediately before the increment, not an earlier independent
counter load. It does **not** explain why January has the opposite load order:
January's source and IR are unavailable. See [FINDINGS.md](FINDINGS.md) for exact
boundaries, a normalized chronology, and the next bounded probe.

## Evidence and reproducibility

- [measurements.json](measurements.json): nine complete-object equivalence
  receipts (including the explicitly excluded wrong-hit pilot), source/compiler/
  debugger/config/log hashes, selected records, and normalized final nodes.
- [raw_evidence.txt](raw_evidence.txt): compact verbatim breakpoint records with
  independently reconstructed full ordinals.
- [c2_sites.txt](c2_sites.txt): relevant instructions from the pinned C2 image.
- [parser_tests.txt](parser_tests.txt): four passing regression tests.

All successful instrumented compilations equal stock compilation after masking
only the four-byte COFF timestamp. This is whole-object equality, not target-only.
All compiler disk hashes are unchanged. No executable, DLL, or object is bundled.

Run from the canonical worktree root at baseline `012f7283` (production identical
to `20d0563d`). Each script copies the hash-pinned debugger into its own named
directory under `scratch/astra_clip_r2_20260925` and writes separate configs.

```powershell
python research/astra_breakthroughs_20260925_r2/decal_clip/test_parser.py
python research/astra_breakthroughs_20260925_r2/decal_clip/follow_node.py
python research/astra_breakthroughs_20260925_r2/decal_clip/materialize.py
python research/astra_breakthroughs_20260925_r2/decal_clip/materialize_operands.py
python research/astra_breakthroughs_20260925_r2/decal_clip/predecessor.py
python research/astra_breakthroughs_20260925_r2/decal_clip/watch_reload.py
python research/astra_breakthroughs_20260925_r2/decal_clip/decision.py
python research/astra_breakthroughs_20260925_r2/decal_clip/emission.py
```

`common.py` regenerates the candidate from the tracked R1 patch and canonical
source, checks its full SHA-256, and fails closed on dependency-manifest, source-
helper, compiler, debugger, and target-body drift. It reuses the **tracked R1
source snapshots**, not untracked old-lane Python helpers. The only old-lane path
still used is the pinned debugger executable; `HALO_C2_DEBUGGER` may locate an
identical executable elsewhere. The debugger is copied before every run.

`archive.py` refreshes this packet from existing scratch results. The deliberately
excluded historical `watch_node` wrong-hit pilot is not rerun by the commands
above; its archived receipt and error explanation are retained as a negative.
