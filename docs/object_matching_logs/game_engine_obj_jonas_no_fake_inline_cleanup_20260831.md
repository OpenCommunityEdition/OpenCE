# `game_engine.obj` no-fake-inline cleanup (2026-08-31)

## Outcome

Two definition-local `__declspec(noinline)` attributes were removed from
`find_netgame_flags` and `find_netgame_flag`. Both scan helpers themselves
remain strict exact. One caller loses exactness and is honestly fuzzy-parked
instead of preserving a manufactured exact result.

The affected dispatcher is now named `game_engine_verify_current_map`.
January's PDB authenticates both `find_netgame_*` names but contains no public
symbol at dispatcher offset 642288. The dispatcher and five validator names
come from the corresponding verbose-symbol HCEA build, whose
`netgame_verify_spawn_points` -> `netgame_verify_equipment` ->
`game_engine_verify_current_map` cluster matches January's relative order and
signatures. They are strong cross-build authentic-name evidence, not claimed
as January PDB labels. All six routines are private to `game_engine.c` and are
marked static in source and `config/symbols.json`.

The same HCEA symbol/type corpus and the independent demon reconstruction
corroborate `scenario_netgame_flag.team_index`, the `netgame_flag_type` enum,
and the `game_engine_type` enum. Source now uses those semantic names and enum
constants in place of raw integers. These source-only corrections do not
alter the target-proven behavior.

## Why the old exact result is rejected

With the two attributes present, VC7 preserves the January nested call
boundary and the dispatcher compares exact. There is no original-source or
PDB evidence for those attributes. Their only demonstrated purpose in this
tree is to influence the inliner, so exactness alone cannot justify them.

With natural C:

| Function | Target | Base | Relocations T/B | objdiff |
|---|---:|---:|---:|---:|
| `_game_engine_verify_current_map` | 640 | 1104 | 52/60 | 10.130045% |

- target normalized SHA-256:
  `b3bce6413d9406f83ea59885922cd47be96d95e3fad738820f5802d54dbde7f8`
- base normalized SHA-256:
  `de767fa67df3d2a492eea148ae2b672336b474dd808ca04347637c5a2a0309ab`

January's inlined required-flag validator still calls
`_find_netgame_flags`. The current candidate inlines that exact scan more
deeply and consequently references `_global_scenario_get` and
`_tag_block_get_element_with_size` directly. Six target functions in this
translation unit remain unwritten, which proves the TU is incomplete and
makes different whole-TU optimizer context plausible; it does not prove the
cause. The fail-closed classifier reports UNKNOWN; the park records the
directly observed TU-context inlining difference without claiming a source
recipe.

The earlier GE05/GE10 ledger statements that treated `noinline` as an
"unavoidable recovered boundary" are explicitly superseded. Reopen only with
first-party source evidence or recovered natural call-graph context, never a
compiler directive added solely for bytes.

## Target-proven vestigial parameter

The two CTF spawn-count checks look suspicious: both pass team index zero, and
the validation helper never reads that formal parameter. January disassembly
confirms both `4, 0, game_engine_ctf` call packets and confirms that the callee never reads
the second argument. This is therefore target-proven vestigial/original
behavior rather than a coincidental match. Source names the formal
`unused_team_index` and carries the required `BUG (preserved for exact
matching)` comment. A corrected build needs authoritative starting-location
team-field evidence; changing only the second call argument would do nothing.
