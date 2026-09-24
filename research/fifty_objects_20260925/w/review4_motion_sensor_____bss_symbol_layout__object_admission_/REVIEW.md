# REVIEW - source/interface/motion_sensor :: .bss symbol layout (object admission)

Reviewer slug: scratch/w/review4_motion_sensor_____bss_symbol_layout__object_admission_/ (base 16542e46, tree clean
before and after). Scope: ONLY the first hunk of scratch/w/motion_sensor/production.patch
(`motion_sensor_globals = NULL`, `sweep_theta = 0.f`). The render_motion_sensor hunks of that file are a separate item.

## Verdict: APPROVE (zero credit; removes the 2-symbol .bss offset blocker)

Landable patch: `bss_only.patch` in this slug (the worker's first hunk, verbatim, CRLF body). It passes
`git apply --check` at 16542e46, and applying it reproduces `cand_bss.c` byte-for-byte.

## Independent re-runs
| check | base (working tree) | cand_bss (hunk 1 only) |
|---|---|---|
| gate --all | exact 16 / residual 3 | identical output, exact 16 / residual 3 |
| coff section_infos_equal, all 75 sections vs base | - | 0 diffs (.debug$S path only) |
| object_audit January symbols | 44 / 2 differ (nvi 0/8, sweep_theta 8/0) | 44 / 0 differ |
| object_audit FAIL count | 5 | 3 (only the three pre-existing residual functions) |
| pdb_storage (production) | 0 disagreements | storage classes unchanged (3/3) |
| provider_link --baseline=base | - | no new surplus; the full run gives 14/14 PASS |
| surplus_identity | 9 COMDATs, 0 not identical | unchanged (sections identical) |
| report.json data | 501/501 (100%) | unchanged, so zero credit |

## Evidence that January wrote the initialisers
- The VC7 .bss law (vc7-data-layout-laws, data-closure playbook, landed board-wide): uninitialised statics come first, in
  name-hash order; explicitly zeroed ones follow in declaration order.
- contribs.json shows the 13-byte .bss contribution at 0x453AC8 belongs to this object. The public, initialised
  `next_vertex_index` sits at offset 0, so no uninitialised static comes before it.
- Lab tests in this TU (lab/): globals-only initialised gives 3 differ; theta-only gives 2 differ; both `= {0}` gives
  0 differ (the zero spelling is inert); both initialised but declared in swapped order gives 2 differ. Both
  initialisers are necessary and sufficient, and the existing declaration order is January's.
- January relocations: initialize stores the malloc result to .bss+4 and tick uses .bss+8 as a float. The symbols.json
  attribution is consistent, so this is not an attribution defect. With the candidate, the resolved relocation targets
  (section owner + offset) of every .bss reference now equal January's. Base differed in 6 exact functions and the
  candidate in 4; the remaining 4 are the pre-existing folded-literal class.
- HCEX.pdb: `motion_sensor_globals` is a File Static of type `struct motion_sensor_globals_definition *`, and
  `sweep_theta` is a File Static float (real). Names, types and storage all match.
- The /Od 2020 build's data layout was inspected and is inconclusive: it comes from a modern compiler and interleaves
  other statics. It was not relied on.

## House rules / admission blockers
- STRIP TEST: removing the initialisers restores the 2-symbol difference, so they are load-bearing. They are attested by
  January's own .bss layout under a measured compiler law. This is the established data-closure lever, not a filler, a
  redundant code store or a pragma. Semantics are unchanged, because statics are zero-initialised anyway.
- Spelling: `= NULL` follows the sibling HUD files (hud_unit.c `unit_hud_globals = NULL`, hud_weapon.c), game.c,
  objects.c and game_allegiance.c. `0.f` follows players.c and the file's own literal style. `real` is kept, not float.
- No header, symbols.json, config, block-scope, name-count, prototype, view-cast, COMDAT or owner-gated class change.
  The hunk does not overlap the held hud_unit stack-sentinel item (lines 278-281 only).

## Observations (non-blocking, outside this hunk)
- HCEX types `next_vertex_index` as `short`; ours is `long`. It is unreferenced in the TU and byte-inert under the 4-byte
  static padding law. It is not touched here; it could be revisited as naming/type hygiene with January evidence.
- The object still has three residual functions (motion_sensor_update, render_motion_sensor, update_motion_sensor), so
  this is not an object completion.
