# Decal-clip spill-stage research packet

**No exact close, no production edit, zero credit.** S4 remains one adjacent-load
swap from January: 1776 bytes / 571 instructions / frame `0x6c`; strict relocation
comparison passes. The canonical-context research source is preserved in
`scratch/astra_clip_20260925/C_canonical_context.c` and as
`patches/canonical_to_S4.patch` (git apply --check passed).

New positive result: the counter's symbol is associated with skipped global webs
50 and 99, across all 91 POP / 47 CHOOSE events observed. A live trace reaches
`0x1072f561 -> 0x1072e76b`. At `0x1072e92a` / `0x1072eb4c`, two operands of one
opcode-`0x21` instruction change from register-web kind 1 to memory kind 2, with
their web pointers cleared. The instruction and its next-link are unchanged.
The actual reload order is not yet explained; the final machine-instruction
mapping is still required. Different-symbol clone descendants are not ruled out.

- [Findings and precise next task](FINDINGS.md): evidence, qualifications, 13-cell
  A/B/AB/context results, and exact reproduction commands.
- [Measurements and hashes](measurements.json): per-cell sources/objects, strict
  relocation checks, compact patch hashes, spill records, full command lines,
  and whole-object instrumentation equivalence receipts.
- [Dependency manifest](dependencies/manifest.json): source helpers snapshotted
  with origin hashes; pinned debugger/compiler hashes; no binary bundled.

`instrumentation_check.py` replays the exact debugger command with stock CL.
All three observation probes yield identical whole objects after masking only
the COFF timestamp; the final replay of the operand probe is also raw-byte
identical. `verify_dependencies.py` fails fast on changed/missing dependencies.

The next task is to follow the observed opcode-`0x21` node into integer lowering
and identify when its counter reload is materialized relative to the edge load.
Do not resume already-exhausted source/name-count sweeps or claim impossibility.
