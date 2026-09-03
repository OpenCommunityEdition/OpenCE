# `flags.obj`: `flag_update` credible fuzzy park (2026-09-03)

## Scope and baseline

This pass was limited to the previously unwritten 1,184-byte
`_flag_update` in `source/objects/widgets/flags.c`. The documented
2,112-byte `_flag_render_proper` fixed point was not retried.

The isolated branch started at `9cf956ff0450a4f1c4bb01fc5552703c2ceae65a`.
The strict focused baseline was:

```text
EXACT       192  _flag_datum_get_cell
EXACT       176  _flag_datum_get_vertex
EXACT        32  _flag_delete
EXACT       384  _flag_new
EXACT       128  _flag_render
UNWRITTEN  2112  _flag_render_proper
EXACT       208  _flag_set_attachment_shape
EXACT       240  _flag_set_trailing_shape
EXACT       288  _flag_tesselate_region
UNWRITTEN  1184  _flag_update
EXACT       672  _flag_update_attachment
EXACT        32  _flags_dispose
EXACT        16  _flags_dispose_from_old_map
EXACT        48  _flags_initialize
EXACT        16  _flags_initialize_for_new_map
EXACT       160  _flags_update
== exact 14 residual 0 unwritten 2
```

## Evidence used

The reconstruction was checked against all of the following before source was
retained:

- January's target instructions, constants, calls, branches, local-array
  layout, and ordered relocations;
- the full Fable scratch corpus under
  `C:/halo-worktrees/fable-small-families-20260901/scratch/workers`, including
  the S1/S2, E, F, H, and V candidate families and `flags.log`;
- the independent later HaloCEA reconstruction in
  `_research_cache/halocea/src/blam/objects/widgets/flag_update.c`;
- the HCEX PDB symbol dump at
  `C:/halo-worktrees/fable-small-families-20260901/scratch/workers/player_control_hcex_symdump.txt`;
- the earlier flags ledgers for the Fable four-function wave and the separate
  `flag_render_proper`, `flag_new`, trailing-shape, and typed-accessor work;
- the local `punpckhdq/halo` checkout, whose flags translation unit remains a
  stub and contributes no competing implementation.

The HCEX symbols independently authenticate these later-source local names and
types: `weather_palette_index`, `attachment_points[5]`, `attachment_y[5]`,
`attachment_location`, `attachment_force_points[40]`, `y_attachments[40]`,
`parent_distances[3]`, `parent_deltas[3]`, `new_position`, `new_location`,
`turbulence`, and `estimated_positions[3]`. January remains authoritative
where the later Xbox 360 build differs.

## Reconstructed behavior

The retained ordinary C:

1. verifies the owner object and rebuilds the marker attachment arrays;
2. obtains the attachment location's underwater/weather state;
3. constructs the three signed-short parent deltas `(-1, 0)`, `(0, 1)`, and
   `(0, -1)` and their scaled rest distances;
4. alternates the row sweep with `flag->update_state`, preserving January's
   backward-pass `row > 0` boundary;
5. applies air or water friction, semantic point-physics flags, and a random
   turbulence vector through `point_physics_update`;
6. snaps first-column attachment rows to marker points and otherwise estimates
   positions from in-range parent vertices;
7. uses January's 4:1 previous-column weighting and four-unit first-column
   attachment-force weighting; and
8. computes velocity from the final position delta divided by elapsed time.

The code uses `real`, `point2d`, `real_point3d`, `real_vector3d`, the typed
point-physics tag accessor, `FLAG`/`SET_FLAG`, the existing typed flag vertex
accessor, vertical parameter formatting, and an explicit terminal `return;`.
The private prototype stays in its owning translation unit. No unrelated
header or exact sibling was changed.

## Strict result

```text
residual   1184  _flag_update  [sha]
== exact 0  residual 1  unwritten 0
```

Target and candidate both have a 1,184-byte padded section and 26 ordered
relocations. Objdiff reports `96.43883%` for the function.

```text
target sha  a8c30b7df9882ad4e154767a3ce63294ebae7d9b8bc5aa1671acc8e034efca6d
base sha    776ba88304402bf2ae995d27de02026f8a9a6655fe565c6385712bbf55c8d2ee
```

Meaningful code is byte-identical through offset `0xA6`. The remaining
differences are dominated by one scalar-home permutation and its downstream
register choices:

| Scalar/lifetime | January | Candidate |
| --- | ---: | ---: |
| row | `ebp-0x04` | `ebp-0x08` |
| conversion temp / neighbor count | `ebp-0x08` | `ebp-0x10` / `ebp-0x0C` |
| wind scale / neighbor-loop count | `ebp-0x0C` | `ebp-0x04` |
| column | `ebp-0x10` | `ebp-0x14` |
| weight sum / reciprocal | `ebp-0x14` | `ebp-0x10` / `ebp-0x0C` |

January reloads row from its home when required. The candidate temporarily
carries it in `EDX`, which accounts for the three additional normalized
instructions even though both functions occupy the same padded envelope.
The fail-closed classifier reports `UNKNOWN`, class `unclassified`, confidence
`NONE`; no narrower compiler recipe is claimed.

## Bounded source-shape audit

The inherited Fable pass had already tested declaration ordering and scopes,
air/water arm order, weight initialization and reciprocal forms, operand
ordering, loop-variable renaming, explicit vector math, helper-based math,
neighbor guard topology, and loop-local versus outer temporary lifetimes.
This pass additionally checked the PDB declaration sequence, PDB aggregate
types, direct versus short-circuit attachment-index lifetime, explicit row
initialization, equivalent `for`/`while` spelling, scalar outer scopes,
parent-distance operand ordering, and per-arm physics-flag construction.

All natural equal-envelope families converged on the same scalar allocation.
The per-arm physics-flag form shrank to 1,168 bytes; the row-start family
changed broader control flow and stack layout. Neither was retained. The
forbidden `point_from_line3d` helper family was not adopted and the final
object emits no such COMDAT.

## Park decision

`_flag_update` is recorded in `config/parked.json` as an honest
`unclassified` credible residual. The retained body is complete and supported
by independent behavior and symbol evidence. It contains no raw address,
volatile or register forcing, compiler barrier, pragma, assembly, fake
dependency, forced inline, representation pun, undefined behavior, or
nonsensical coincidental-match branch.

Reopen only for authoritative January source/local records or a natural
same-compiler donor that explains the scalar-home permutation. The separate
`_flag_render_proper` evidence ruling remains unchanged.
