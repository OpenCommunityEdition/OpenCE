# `projectiles.obj` evidence closeout (2026-09-19)

## Scope and strict result

- Unit: `source/items/projectiles.c`; January target:
  `build/split/source/items/projectiles.obj`.
- Final strict gate: **26 exact, 4 residual, 0 unwritten** out of 30 listed
  functions.  The `_point_from_line3d` emitted-owner guard passes.
- No production source or configuration was changed in this pass and no fuzzy
  row receives exact credit.

## Evidence searched

1. January target instructions, relocations, COMDATs, data owners, debug strings,
   and `cachebeta.pdb` publics.
2. Existing IDA/Ghidra exports and the PDB/IDA lane reconstruction packet.
3. HCEA projectile source and the supplied Halo 1 symbol-build executables,
   used as later-build semantic/name evidence only.  Their PE CodeView records
   point at unavailable external PDBs; they do not provide January local/type
   records.
4. Every local `projectiles.c` worktree copy plus all preserved scratch
   candidates under `C:\halo-worktrees`: 287 candidate-sized files, 183 unique
   SHA-256 contents, all recompiled against the current split object.  No
   candidate exceeds the canonical 26 exact functions.

## Cross-build and debug evidence

- Relocation-masked bodies from the 2276 retail object and January are identical
  for every remaining row: `collision_test_line` 512 bytes, `projectile_new`
  432 bytes, `projectile_collision` 2928 bytes, and `projectile_update` 4064
  bytes.  The captured comparison is
  `scratch/oct2276_january_projectiles_body_comparison.json`.  This makes the
  2276 caller/CFG decompilation relevant evidence rather than a merely similar
  later-build donor.
- January's `cachebeta.pdb` module stream 247 contains only `S_OBJNAME` and
  `S_COMPILE2_ST`.  Its line sizes are zero and the module is marked
  `NoDbgInfo`; it has no recoverable projectiles locals, scopes, or source-line
  records.  The parser and inventory are
  `scratch/projectiles_pdb_module_inventory.py` and its captured output.
- The same-revision `/RTC` frame descriptors in `halo_cache_symbols.exe`
  recover authentic local names, sizes, order, and offsets for the three large
  residuals.  The independently supplied `halo_tag_symbols.exe`,
  `sapien_symbols.exe`, `tool_symbols.exe`, and `guerilla_symbols.exe` repeat
  the same names and declaration order (with only expected `/GS` displacement
  changes).  The important records include the two 84-byte `damage_data`
  locals and the vector/effect arrays for collision; `p0_left`, `p1_left`,
  `p0_right`, `p1_right`, `left`, and `forward` for collision-test-line; and
  the complete collision/vector/basis/sound-location sequence for update.
  Reordering declarations to this authenticated order did not change the
  strict count.
- January's PDB link order selects `source/ai/action_charge` as the emitted
  `_point_from_line3d` owner; projectiles is one of 18 undefined referrers.
  Same-compiler controls show that ordinary, `static`, `extern`, and
  `__forceinline` header spellings all emit a raw COMDAT after inlining.
  Therefore projectiles cannot lawfully manufacture or retain that owner.  The
  complete linker-topology proof is in
  `docs/object_matching_logs/point_from_line3d_link_selection_evidence_20260919.md`.

## New mixed-inline ownership experiment

January inlines the `point_from_line3d` arithmetic four times in
`projectile_collision_test_line`, while `projectile_update` has a REL32 undefined
reference to `_point_from_line3d`.  The target object has only an undefined
`_point_from_line3d` symbol; it does **not** own that COMDAT.

The accepted per-TU alias pattern used by other reconstructed units was tested:
include `real_math.h` with `point_from_line3d` renamed to
`projectiles_point_from_line3d_inline`, call the renamed inline at the four
collision sites, and retain the ordinary external declaration for update.  VC7
still emits a 48-byte code owner named
`_projectiles_point_from_line3d_inline`.  This is a candidate-only owner absent
from January and therefore fails the same ownership rule under a different
name.  It was rejected.  No hand-expanded arithmetic or force-inline steering
was admitted.

## Residual disposition

| Function | Target / candidate padded bytes | Result |
|---|---:|---|
| `_projectile_new` | 432 / 432 | Same instructions and relocations; three independent `lea`/`push` operations are scheduled on the opposite sides of two x87 operations.  Direct `SET_FLAG(..., scenario_location_underwater(...))`, block scoping, expression order, and all preserved candidates are byte-inert. |
| `_projectile_collision_test_line` | 512 / 496 | Target proves four genuine inline point computations and an x87-enregistered up-by-direction product.  Natural header inline emission adds a forbidden COMDAT; the named per-TU alias adds an equally invalid renamed owner.  Manual component arithmetic would be fake source and remains rejected. |
| `_projectile_collision` | 2928 / 2928 | Calls/relocations and semantics agree.  A 2276-authenticated natural nested `if` spelling of the overpenetrate/reflect response graph improved the raw differing-byte count from 2366 to 2141 while preserving all 26 exact siblings, size, and 100 relocations, but remained 911 versus January's 914 instructions and differed throughout the frame allocation and later CFG.  It is retained only as `scratch/projectiles_collision_natural_nested_response_edits.json` / `.obj`, with zero credit.  Remaining differences include the x87 lifetime and stack-cell packing of `speed_squared` versus the velocity-noise argument temporary. |
| `_projectile_update` | 4064 / 4064 | Exact relocation count and semantic graph.  Remaining differences are permutations among equal-reference frame cells and ESI/EDI allocation between the definition pointer and velocity CSE.  The newer w14 declaration/flag/switch candidates do not change the strict count. |

The `/QIfist` diagnostic was also measured because it is an allowed per-unit
option when supported by provenance.  It changes none of these four verdicts
and therefore is not proposed for this unit.

## Admission result

No exact function or object was gained.  This pass rules out an important false
escape hatch: renaming the inline helper does not preserve January ownership.
Reopening `collision_test_line` requires authentic evidence for a no-owner
per-call inline facility used by this build, not a macro clone.  The exhaustive
linker proof currently says no such legitimate VC7 spelling exists.  The other
three rows require a measured VC7 scheduling/allocation law or primary source;
the available January PDB is conclusively stripped of the needed local debug
records, while the symbol-build `/RTC` records have already been applied.
