# Memory LRUV two-stage link exact admission (2026-09-13)

## Scope and evidence

This bounded wave revalidated `source/memory/data_encoding.c`,
`source/memory/byte_swapping.c`, and `source/memory/lruv_cache.c` with the
PID-safe `tools/campaign/gate.py` harness and the XDK 3911 flags already owned
by the generated build graph. `data_encoding.obj` remains 16/16 exact and
`byte_swapping.obj` remains 5/5 exact; neither source file changed.

The only admitted change is the previously reconstructed two-stage list-link
topology in `lruv_block_new`. The historical 55-experiment LRUV ledger at
commit `345a83afa2d92ed49ccb61f284eaa284898931bb` established this topology in
E8/E9 from the older source reconstruction: the first decision chooses the
new block's predecessor, and a second `best_hole.block_index` decision performs
the common next-link update. The canonical source had folded that second
decision into only the non-head branch. Restoring the natural two-stage shape
on the newer, otherwise target-exact body closes the function without changing
its behavior.

No header, compiler flag, assertion, type, name, inline schedule, or unrelated
translation unit changed. The source contains no barrier, `volatile`, register
hint, forced inline/noinline annotation, assembly, undefined access, raw
address name, or fake dependency.

## Strict result

- `_lruv_block_new`: **strict exact**, 1,302 meaningful / 1,312 padded bytes,
  48/48 semantic relocations, 448/448 aligned instructions, and no normalized
  instruction difference.
- LRUV whole-TU gate: **17 exact / 1 residual / 0 unwritten**, retaining all
  16 prior exact owners.
- `_lruv_debug_to_file`: unchanged honest residual at 464 padded bytes and
  20/20 relocations; the existing typed fuzzy reconstruction remains the best
  admissible source and receives no exact credit.

The wave therefore contributes **+1 exact function, +1,302 meaningful bytes,
+1,312 padded bytes**, with no regression in its assigned units.
