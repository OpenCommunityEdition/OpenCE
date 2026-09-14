# `stack_walk_windows.obj` — loader/comparator packet (opus5 PDB/IDA lane, 2026-09-14)

## Result

| owner | January | candidate | disposition |
| --- | --- | --- | --- |
| `_symbol_sort_proc` | 48 padded / 47 meaningful, 0 relocs | identical | **strict EXACT** (emitted by its genuine loader caller through `qsort`) |
| `_load_symbol_table` | 1568 padded / 1556 meaningful, 93 relocs | 1568 / 94 relocs | fuzzy, exact credit 0 |

The focused gate is `== exact 9 residual 7 unwritten 0 (of 16 listed)`, up from 8 / 6 / 2,
with the `_point_from_line3d` guard passing. The full build and rename-stable
census against the lane base show `_symbol_sort_proc` as the only change for this
object, with 0 regressions tree-wide.

## Evidence

- **January PDB** (cachebeta.pdb, pdb200 extraction): `_load_symbol_table` and
  `_symbol_name_from_address` are public labels at the expected RVAs. The PDB has
  no type or local records for Halo compilands, so it does **not** supply the
  signature; see the interface ledger from this lane for the type census.
- **Signature:** `int load_symbol_table(char *filename, struct debug_symbol_table
  *symbol_table, char *timestamp_str)`, taken from the HCEX Release PDB records
  quoted in `stack_walk_windows_obj_loader_cluster_withheld_20260904.md`
  (cross-build naming and type evidence) together with January's use of the result.
- **Body:** the reviewed 2026-09-04 withheld natural reconstruction. It was
  extracted verbatim from that ledger and applied by `scratch/stack_walk/assemble.py`,
  with no new tuning.

## Ownership decision (declaration)

The 2026-09-04 packet was withheld because a public declaration in a shared
header regressed unrelated exact owners. This lane retested the now-genuine owner
`cseries_windows.h`, adding `struct debug_symbol_table;` plus the `int` prototype.
The full build regressed `game_engine::_populate_statistic_buffer`
(declaration-count sensitivity, since `game_engine.c` includes the header), so
that header change was **reverted and is not part of this lane**.

The admitted source needs no declaration. `load_symbol_table` is called only by
`stack_walk_initialize`, which follows its definition in the same translation
unit, and no other object calls it. The file's old, **wrong** TU-local
`void load_symbol_table(char const *, ..., char const *)` prototype was removed
instead of corrected. The function keeps January's external linkage. No
consumer-local or foreign prototype remains. The private comparator has a normal
static forward prototype.

## Retained shape notes

- `#include <ctype.h>` with `#undef isspace`: January calls the CRT `isspace`
  function (relocations), not the multibyte macro.
- The TU-local constants `MAXIMUM_DEBUG_SYMBOL_NAME_LENGTH`,
  `MAXIMUM_LIBRARY_OBJECT_FILE_NAME_LENGTH`, `DEBUG_SYMBOL_ALLOCATION_COUNT` and
  `DEBUG_SYMBOL_STRING_STORAGE_ALLOCATION_SIZE` come from the withheld source.
- The target-proven legacy edges carry `BUG (preserved for exact matching)`
  comments exactly as reviewed on 2026-09-04: the unchecked timestamp search, the
  continuation reads, the end_str advance, comparator antisymmetry and
  all-zero-RVA trimming.
- The fake scan shows only the two inherited `walk_stack` EBP/ESP captures.

## Residual and reopen

`_load_symbol_table`'s residual is a duplicated `_fgets` call site (one relocation
surplus from loop rotation), a three-register cycle and the two 256-byte arrays
occupying opposite slots. This is unchanged from 2026-09-04 and no tuning was
attempted. Reopen only for original source or compiler-context evidence. A
header declaration can be added only once `cseries_windows.h` consumers tolerate
the extra declaration: rerun the full build and check
`_populate_statistic_buffer`.
